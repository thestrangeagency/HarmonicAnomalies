#include "plugin.hpp"

#include <cmath>
#include <algorithm>
#include <array>

struct Tile
{
    float x;
    float y;
    float v;
};

struct Hex
{
    const float size = .5;
    const float dx = size * 3 / 2;
    const float dy = size * sqrt(3);

    const int radius = 86;
    const int diameter = radius * 2;

    const float width = diameter * dx;
    const float height = diameter * dy;

    const int yAxis = 3 * radius - 2;
    const int length = pow(radius, 3) - pow(radius - 1, 3); // 21931 if radius = 86

    std::vector<Tile> tiles;

    int writeCursor = 0;
    int readCursor = 0;

    float writeX = 0;
    float writeY = 0;
    float writeZ = 0;

    float readX = 0;
    float readY = 0;
    float readZ = 0;

    int y_step = yAxis;
    int z_step = y_step + 1;

    int ringRadius = 0;
    int maxRingRadius = 10;
    std::vector<int> ringDirs = {-1, -z_step, -y_step, 1, z_step, y_step}; // directions around a ring
    std::vector<int> ringOffsets;

    Hex()
    {
        tiles.resize(length);
        ringOffsets.resize(maxRingRadius * 6);
        initTiles();
    }

    void setVoltage(float v, float blend)
    {
        tiles[writeCursor].v = v * blend + tiles[writeCursor].v * (1.0 - blend);
    }

    float getVoltage()
    {
        return ringRadius < 1 ? tiles[readCursor].v : getRingVoltage();
    }

    float getRingVoltage()
    {
        float voltage = 0;

        for (const auto &offset : ringOffsets)
        {
            Tile tile = getReadTileAtOffset(offset);
            voltage += tile.v;
        }

        return voltage / ringOffsets.size();
    }

    Tile getTile(int i)
    {
        return tiles[clamp(i)];
    }

    Tile getReadTile()
    {
        return tiles[readCursor];
    }

    Tile getReadTileAtOffset(int offset)
    {
        return getTile(readCursor + offset);
    }

    void updateReadRingOffsets()
    {
        ringOffsets.clear();
        int t = z_step * ringRadius;

        for (const auto &dir : ringDirs)
        {
            for (int i = 0; i < ringRadius; i++)
            {
                t = t + dir;
                ringOffsets.push_back(t);
            }
        }
    }

    void advanceWriteCursor(float x, float y, float z)
    {
        writeX += x;
        writeY += y;
        writeZ += z;

        writeX = fmod(writeX, length);
        writeY = fmod(writeY, length);
        writeZ = fmod(writeZ, length);

        writeCursor = round(writeX) + round(writeY) * y_step + round(writeZ) * z_step;
        writeCursor = clamp(writeCursor);
    }

    void advanceReadCursor(float x, float y, float z)
    {
        readX += x;
        readY += y;
        readZ += z;

        readX = fmod(readX, length);
        readY = fmod(readY, length);
        readZ = fmod(readZ, length);

        readCursor = round(readX) + round(readY) * y_step + round(readZ) * z_step;
        readCursor = clamp(readCursor);
    }

private:
    int clamp(int x)
    {
        while (x < 0)
            x += length;
        x %= length;
        return x;
    }

    void initTiles()
    {
        for (int i = 0; i < length; ++i)
        {
            std::array<float, 2> c = coordAt(i);
            tiles[i].x = c[0];
            tiles[i].y = c[1];
            tiles[i].v = 0;
        }
    }

    std::array<float, 2> coordAt(int i)
    {
        std::array<int, 3> coords = toCoords(i);
        return {width / 2 + (coords[0] - coords[1]) * dx, height / 2 + (coords[2] - (coords[0] + coords[1]) / 2) * dy};
    }

    std::array<int, 3> toCoords(int i)
    {
        std::array<int, 3> coords = getCoords(i, 0, 0);
        return *std::max_element(coords.begin(), coords.end()) < radius ? coords : getCoords(i - length, 0, 0);
    }

    std::array<int, 3> getCoords(int x, int y, int z)
    {
        return x < 0 ? getCoords(x + yAxis + 1, y, z + 1) : x < radius ? std::array<int, 3>{x, y, z}
                                                                       : getCoords(x - yAxis, y + 1, z);
    }
};

struct HexNut : Module
{
    enum ParamId
    {
        VWX_PARAM,
        VWY_PARAM,
        VWZ_PARAM,
        VRX_PARAM,
        VRY_PARAM,
        VRZ_PARAM,
        BLEND_PARAM,
        READ_RING_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        CV_VWX_INPUT,
        CV_VWY_INPUT,
        CV_VWZ_INPUT,
        CV_VRX_INPUT,
        CV_VRY_INPUT,
        CV_VRZ_INPUT,
        INPUT_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUTPUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId
    {
        LIGHTS_LEN
    };

    Hex hex;
    float lastReadRingRadius = 0;

    HexNut()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(VWX_PARAM, -1.f, 1.f, 0.f, "write x");
        configParam(VWY_PARAM, -1.f, 1.f, 0.f, "write y");
        configParam(VWZ_PARAM, -1.f, 1.f, 0.f, "write z");

        configParam(VRX_PARAM, -1.f, 1.f, 0.f, "read x");
        configParam(VRY_PARAM, -1.f, 1.f, 0.f, "read y");
        configParam(VRZ_PARAM, -1.f, 1.f, 0.f, "read z");

        configParam(BLEND_PARAM, 0.f, 1.f, 1.f, "blend");
        configParam(READ_RING_PARAM, 0.f, hex.maxRingRadius, 0.f, "read ring radius");

        configInput(CV_VWX_INPUT, "CV wx");
        configInput(CV_VWY_INPUT, "CV wy");
        configInput(CV_VWZ_INPUT, "CV wz");

        configInput(CV_VRX_INPUT, "CV rx");
        configInput(CV_VRY_INPUT, "CV ry");
        configInput(CV_VRZ_INPUT, "CV rx");

        configInput(INPUT_INPUT, "signal");
        configOutput(OUTPUT_OUTPUT, "signal");
    }

    /* ==================================================================== */
    /* ==================================================================== */
    /* ==================================================================== */

    void process(const ProcessArgs &args) override
    {
        float in_v = inputs[INPUT_INPUT].getVoltage();
        float blend_v = params[BLEND_PARAM].getValue();
        hex.setVoltage(in_v, blend_v);

        float read_ring_radius_v = params[READ_RING_PARAM].getValue();
        if (read_ring_radius_v != lastReadRingRadius)
        {
            hex.ringRadius = round(read_ring_radius_v);
            hex.updateReadRingOffsets();
            lastReadRingRadius = read_ring_radius_v;
        }

        outputs[OUTPUT_OUTPUT].setVoltage(hex.getVoltage());

        float scale = 0.1;

        float wx = params[VWX_PARAM].getValue() + inputs[CV_VWX_INPUT].getVoltage() * scale;
        float wy = params[VWY_PARAM].getValue() + inputs[CV_VWY_INPUT].getVoltage() * scale;
        float wz = params[VWZ_PARAM].getValue() + inputs[CV_VWZ_INPUT].getVoltage() * scale;

        hex.advanceWriteCursor(wx, wy, wz);

        float rx = params[VRX_PARAM].getValue() + inputs[CV_VRX_INPUT].getVoltage() * scale;
        float ry = params[VRY_PARAM].getValue() + inputs[CV_VRY_INPUT].getVoltage() * scale;
        float rz = params[VRZ_PARAM].getValue() + inputs[CV_VRZ_INPUT].getVoltage() * scale;

        hex.advanceReadCursor(rx, ry, rz);
    }
};

struct HexDisplay : LedDisplay
{
    HexNut *module;
    Hex *hex;
    ModuleWidget *moduleWidget;
    std::string fontPath;

    HexDisplay()
    {
        fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");

        // demoPointBufferInit();
    }

    void hexagon(NVGcontext *vg, float x, float y, float size, NVGcolor color)
    {
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + size, y);

        for (int i = 1; i < 6; i++)
        {
            float angle = i * M_PI / 3;
            nvgLineTo(vg, x + cos(angle) * size, y + sin(angle) * size);
        }

        nvgClosePath(vg);
        nvgFillColor(vg, color);
        nvgFill(vg);
    }

    NVGcolor colorFromVoltage(float v)
    {
        float vNorm = abs(v) / 5.0;
        int l = 255 * vNorm;
        return nvgRGBA(l, l, l, 255);
    }

    void center(const DrawArgs &args)
    {
        float offX = box.size.x / 2 - hex->tiles[0].x;
        float offY = box.size.y / 2 - hex->tiles[0].y;

        nvgTranslate(args.vg, offX, offY);
    }

    void drawTile(const DrawArgs &args, Tile tile)
    {
        hexagon(args.vg, tile.x, tile.y, hex->size, colorFromVoltage(tile.v));
    }

    void drawTiles(const DrawArgs &args)
    {
        for (int i = 0; i < hex->length; i++)
        {
            Tile tile = hex->tiles[i];
            drawTile(args, tile);
        }
    }

    void drawWriteCursor(const DrawArgs &args)
    {
        int writeCursor = hex->writeCursor;
        Tile tile = hex->tiles[writeCursor];
        hexagon(args.vg, tile.x, tile.y, hex->size * 2, nvgRGBA(255, 0, 0, 255));
        drawTile(args, tile);
    }

    void drawReadRing(const DrawArgs &args)
    {
        for (const auto &offset : hex->ringOffsets)
        {
            Tile tile = hex->getReadTileAtOffset(offset);
            hexagon(args.vg, tile.x, tile.y, hex->size, nvgRGBA(255, 255, 0, 255));
        }
    }

    void drawReadCursor(const DrawArgs &args)
    {
        int readCursor = hex->readCursor;
        Tile tile = hex->tiles[readCursor];
        hexagon(args.vg, tile.x, tile.y, hex->size * 2, nvgRGBA(0, 255, 0, 255));
        drawTile(args, tile);
    }

    void drawLayer(const DrawArgs &args, int layer) override
    {
        if (module && layer == 1)
        {
            center(args);
            drawTiles(args);
            drawWriteCursor(args);
            drawReadRing(args);
            drawReadCursor(args);
        }

        Widget::drawLayer(args, layer);
    }
};

struct HexNutWidget : ModuleWidget
{
    HexNutWidget(HexNut *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/HexNut.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 80)), module, HexNut::CV_VWX_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 90)), module, HexNut::CV_VWY_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 100)), module, HexNut::CV_VWZ_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 80)), module, HexNut::VWX_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 90)), module, HexNut::VWY_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 100)), module, HexNut::VWZ_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 80)), module, HexNut::CV_VRX_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 90)), module, HexNut::CV_VRY_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 100)), module, HexNut::CV_VRZ_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 80)), module, HexNut::VRX_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 90)), module, HexNut::VRY_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 100)), module, HexNut::VRZ_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 110)), module, HexNut::BLEND_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 110)), module, HexNut::READ_RING_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 110)), module, HexNut::INPUT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30, 110)), module, HexNut::OUTPUT_OUTPUT));

        HexDisplay *display = createWidget<HexDisplay>(mm2px(Vec(0.0, 13.039)));
        display->box.size = mm2px(Vec(50, 55.88));
        display->module = module;
        display->hex = &module->hex;
        display->moduleWidget = this;
        addChild(display);
    }
};

Model *modelHexNut = createModel<HexNut, HexNutWidget>("HexNut");