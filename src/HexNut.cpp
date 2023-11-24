#include "plugin.hpp"
#include "UI.hpp"
#include "HexExCV.hpp"

#include <cmath>
#include <algorithm>
#include <array>

struct Tile
{
    float x;
    float y;
    float v;
    float writ;
    float read;
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

    int readLength = length;
    int writeLength = length;

    std::vector<Tile> tiles;

    int writeCursor = 0;
    int writeRingCursor = 0;

    int readCursor = 0;
    int readRingCursor = 0;

    float writeX = 0;
    float writeY = 0;
    float writeZ = 0;

    float readX = 0;
    float readY = 0;
    float readZ = 0;

    int y_step = yAxis;
    int z_step = y_step + 1;

    int ringRadius = 0; // radius of ring around cursor
    int maxRingRadius = 64;
    std::vector<int> ringDirs = {-1, -z_step, -y_step, 1, z_step, y_step}; // directions around a ring
    std::vector<int> ringOffsets;                                          // given a radius, offsets from cursor to ring around cursor

    enum Mode
    {
        VECTOR,
        RING,
        VORTEX
    };

    Mode floatToMode(float value)
    {
        int roundedValue = static_cast<int>(std::round(value));
        return static_cast<Mode>(roundedValue - 1);
    }

    Mode writeMode = Mode::VECTOR;
    Mode readMode = Mode::VECTOR;

    int writePosRingRadius = radius / 2;
    int writePosRingDir = 0;
    int writePosRingStep = 0;
    int writeMaxRadius = radius;

    int readPosRingRadius = radius / 2;
    int readPosRingDir = 0;
    int readPosRingStep = 0;
    int readMaxRadius = radius;

    Hex()
    {
        tiles.resize(length);
        ringOffsets.resize(maxRingRadius * 6);
        initTiles();
    }

    int voltageToRadius(float v)
    {
        int r = round(radius * v);
        r = clamp(r, 1, radius);
        return r;
    }

    void setWriteMaxRadius(float v)
    {
        int r = voltageToRadius(v);
        writeMaxRadius = r;
    }

    void setReadMaxRadius(float v)
    {
        int r = voltageToRadius(v);
        readMaxRadius = r;
    }

    void setCrop(float v)
    {
        int r = voltageToRadius(v);
        readLength = writeLength = pow(r, 3) - pow(r - 1, 3);
    }

    void setVoltage(float v, float blend)
    {
        blend = clamp(blend, 0.0, 1.0);
        tiles[writeCursor].v = v * blend + tiles[writeCursor].v * (1.0 - blend);
        tiles[writeCursor].writ = 1;
    }

    float getVoltage()
    {
        return ringRadius < 1 ? getTileVoltage(readCursor) : getRingVoltage();
    }

    float getTileVoltage(int i)
    {
        tiles[i].read = 1;
        return tiles[i].v;
    }

    float getRingVoltage()
    {
        float voltage = 0;

        for (const auto &offset : ringOffsets)
        {
            int i = getReadIndexAtOffset(offset);
            voltage += getTileVoltage(i);
        }

        return voltage / sqrt(ringOffsets.size());
    }

    void decayTile(int i)
    {
        tiles[i].writ *= .75;
        tiles[i].read *= .75;
    }

    Tile getTile(int i)
    {
        return tiles[wrap(i, length)];
    }

    Tile getReadTile()
    {
        return tiles[readCursor];
    }

    Tile getReadTileAtOffset(int offset)
    {
        return getTile(readCursor + offset);
    }

    int getReadIndexAtOffset(int offset)
    {
        return wrap(readCursor, readLength);
    }

    void updateReadRingOffsets()
    {
        ringOffsets.clear();
        int top = z_step * ringRadius;

        for (const auto &dir : ringDirs)
        {
            for (int i = 0; i < ringRadius; i++)
            {
                top = top + dir;
                ringOffsets.push_back(top);
            }
        }
    }

    void advanceWriteCursor(float x, float y, float z)
    {
        if (writeMode == VECTOR)
        {
            writeX += x;
            writeY += y;
            writeZ += z;

            writeX = fmod(writeX, length);
            writeY = fmod(writeY, length);
            writeZ = fmod(writeZ, length);

            writeCursor = round(writeX) + round(writeY) * y_step + round(writeZ) * z_step;
        }
        else if (writeMode == RING || writeMode == VORTEX)
        {
            writeCursor += ringDirs[writePosRingDir];
            bool isRingEdgeComplete = ++writePosRingStep >= (writeMode == RING ? writeMaxRadius : writePosRingRadius);
            if (isRingEdgeComplete)
            {
                writePosRingStep = 0;
                writePosRingDir++; // change to next edge direction

                bool isRingComplete = writePosRingDir == static_cast<int>(ringDirs.size());
                if (isRingComplete && writeMode == VORTEX)
                {
                    // increase radius in vortex mode
                    writePosRingRadius++;
                    writePosRingRadius %= writeMaxRadius;
                }
                writePosRingDir %= ringDirs.size();
            }
        }

        writeCursor = wrap(writeCursor, writeLength);
    }

    void advanceReadCursor(float x, float y, float z)
    {
        readX += x;
        readY += y;
        readZ += z;

        readX = fmod(readX, length);
        readY = fmod(readY, length);
        readZ = fmod(readZ, length);

        int readVectorCursor = round(readX) + round(readY) * y_step + round(readZ) * z_step;

        if (readMode == RING || readMode == VORTEX)
        {
            readRingCursor += ringDirs[readPosRingDir];
            bool isRingEdgeComplete = ++readPosRingStep >= (readMode == RING ? readMaxRadius : readPosRingRadius);
            if (isRingEdgeComplete)
            {
                readPosRingStep = 0;
                readPosRingDir++; // change to next edge direction

                bool isRingComplete = readPosRingDir == static_cast<int>(ringDirs.size());
                if (isRingComplete && readMode == VORTEX)
                {
                    // increase radius in vortex mode
                    readPosRingRadius++;
                    readPosRingRadius %= readMaxRadius;
                }
                readPosRingDir %= ringDirs.size();
            }
            readRingCursor = wrap(readRingCursor, readLength);
        }

        readCursor = wrap(readVectorCursor + readRingCursor, readLength);
    }

private:
    int wrap(int x, int wrapLength)
    {
        while (x < 0)
            x += wrapLength;
        x %= wrapLength;
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
            tiles[i].writ = 0;
            tiles[i].read = 0;
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
        WRITE_MODE_PARAM,
        READ_MODE_PARAM,
        WRITE_RADIUS_PARAM,
        READ_RADIUS_PARAM,
        VWX_PARAM,
        VWY_PARAM,
        VWZ_PARAM,
        VRX_PARAM,
        VRY_PARAM,
        VRZ_PARAM,
        BLEND_PARAM,
        READ_RING_PARAM,
        CROP_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
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
    float lastCrop = 1;

    HexNut()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(WRITE_MODE_PARAM, 1.f, 3.f, 1.f, "Write Mode");
        configParam(READ_MODE_PARAM, 1.f, 3.f, 1.f, "Read Mode");

        configParam(WRITE_RADIUS_PARAM, 0.f, 1.f, 1.f, "Vortex Write Radius");
        configParam(READ_RADIUS_PARAM, 0.f, 1.f, 1.f, "Vortex Read Radius");

        configParam(VWX_PARAM, -1.f, 1.f, 0.f, "Write x");
        configParam(VWY_PARAM, -1.f, 1.f, 0.f, "Write y");
        configParam(VWZ_PARAM, -1.f, 1.f, 0.f, "Write z");

        configParam(VRX_PARAM, -1.f, 1.f, 0.f, "Read x");
        configParam(VRY_PARAM, -1.f, 1.f, 0.f, "Read y");
        configParam(VRZ_PARAM, -1.f, 1.f, 0.f, "Read z");

        configParam(BLEND_PARAM, 0.f, 1.f, 1.f, "Blend");
        configParam(READ_RING_PARAM, 0.f, hex.maxRingRadius, 0.f, "Read Ring Radius");
        configParam(CROP_PARAM, 0.f, 1.f, 1.f, "Crop");

        configInput(INPUT_INPUT, "Signal");
        configOutput(OUTPUT_OUTPUT, "Signal");
    }

    /* ==================================================================== */
    /* ==================================================================== */
    /* ==================================================================== */

    void process(const ProcessArgs &args) override
    {
        float w_mode_v = params[WRITE_MODE_PARAM].getValue();
        float r_mode_v = params[READ_MODE_PARAM].getValue();
        hex.writeMode = hex.floatToMode(w_mode_v);
        hex.readMode = hex.floatToMode(r_mode_v);

        float crop_v = params[CROP_PARAM].getValue();
        if (crop_v != lastCrop)
        {
            hex.setCrop(crop_v);
            lastCrop = crop_v;
        }

        // expander

        float cv_vwx_v = 0, cv_vwy_v = 0, cv_vwz_v = 0;
        float cv_vrx_v = 0, cv_vry_v = 0, cv_vrz_v = 0;
        float cv_write_size_v = 0, cv_read_size_v = 0;
        float cv_blend_v = 0;
        float cv_scale = 0.1;

        Module *expander = getRightExpander().module;
        if (expander && expander->model == modelHexExCV)
        {
            cv_vwx_v = expander->getInput(HexExCV::CV_VWX_INPUT).getVoltage() * cv_scale;
            cv_vwy_v = expander->getInput(HexExCV::CV_VWY_INPUT).getVoltage() * cv_scale;
            cv_vwz_v = expander->getInput(HexExCV::CV_VWZ_INPUT).getVoltage() * cv_scale;

            cv_write_size_v = expander->getInput(HexExCV::CV_WRITE_SIZE_INPUT).getVoltage() * cv_scale;

            cv_vrx_v = expander->getInput(HexExCV::CV_VRX_INPUT).getVoltage() * cv_scale;
            cv_vry_v = expander->getInput(HexExCV::CV_VRY_INPUT).getVoltage() * cv_scale;
            cv_vrz_v = expander->getInput(HexExCV::CV_VRZ_INPUT).getVoltage() * cv_scale;

            cv_read_size_v = expander->getInput(HexExCV::CV_READ_SIZE_INPUT).getVoltage() * cv_scale;

            cv_blend_v = expander->getInput(HexExCV::CV_BLEND_INPUT).getVoltage() * cv_scale;
        }

        // end expander

        float w_r_v = params[WRITE_RADIUS_PARAM].getValue() + cv_write_size_v;
        float r_r_v = params[READ_RADIUS_PARAM].getValue() + cv_read_size_v;
        hex.setWriteMaxRadius(w_r_v);
        hex.setReadMaxRadius(r_r_v);

        float in_v = inputs[INPUT_INPUT].getVoltage();
        float blend_v = params[BLEND_PARAM].getValue() + cv_blend_v;
        hex.setVoltage(in_v, blend_v);

        float read_ring_radius_v = params[READ_RING_PARAM].getValue();
        if (read_ring_radius_v != lastReadRingRadius)
        {
            hex.ringRadius = round(read_ring_radius_v);
            hex.updateReadRingOffsets();
            lastReadRingRadius = read_ring_radius_v;
        }

        outputs[OUTPUT_OUTPUT].setVoltage(hex.getVoltage());

        float wx = params[VWX_PARAM].getValue() + cv_vwx_v;
        float wy = params[VWY_PARAM].getValue() + cv_vwy_v;
        float wz = params[VWZ_PARAM].getValue() + cv_vwz_v;

        hex.advanceWriteCursor(wx, wy, wz);

        float rx = params[VRX_PARAM].getValue() + cv_vrx_v;
        float ry = params[VRY_PARAM].getValue() + cv_vry_v;
        float rz = params[VRZ_PARAM].getValue() + cv_vrz_v;

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

    NVGcolor colorFromTile(Tile tile)
    {
        float vNorm = abs(tile.v) / 5.0;
        int r = fmin(255, round(128 * vNorm + 255 * tile.writ));
        int g = 128 * vNorm;
        int b = fmin(255, round(128 * vNorm + 255 * tile.read));
        return nvgRGBA(r, g, b, 255);
    }

    void center(const DrawArgs &args)
    {
        nvgTranslate(args.vg, 150, 4);
        nvgRotate(args.vg, 3 * M_PI / 6);
    }

    void drawTile(const DrawArgs &args, Tile tile)
    {
        hexagon(args.vg, tile.x, tile.y, hex->size, colorFromTile(tile));
    }

    void drawTiles(const DrawArgs &args)
    {
        for (int i = 0; i < hex->length; i++)
        {
            Tile tile = hex->tiles[i];
            drawTile(args, tile);
            hex->decayTile(i);
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
            hexagon(args.vg, tile.x, tile.y, hex->size, nvgRGBA(0, 0, 255, 255));
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
        float tR = 12; // template radius

        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/HexNutFlat.svg")));

        addParam(createParam<FlatKnob>(Vec(7, 234), module, HexNut::WRITE_RADIUS_PARAM));
        addParam(createParam<FlatKnob>(Vec(119, 234), module, HexNut::READ_RADIUS_PARAM));

        addParam(createParamCentered<FlatSwitch>(Vec(7 + tR, 262 + tR), module, HexNut::WRITE_MODE_PARAM));
        addParam(createParamCentered<FlatSwitch>(Vec(119 + tR, 262 + tR), module, HexNut::READ_MODE_PARAM));

        addParam(createParam<FlatKnob>(Vec(35, 206), module, HexNut::VWX_PARAM));
        addParam(createParam<FlatKnob>(Vec(35, 234), module, HexNut::VWY_PARAM));
        addParam(createParam<FlatKnob>(Vec(35, 262), module, HexNut::VWZ_PARAM));

        addParam(createParam<FlatKnob>(Vec(91, 206), module, HexNut::VRX_PARAM));
        addParam(createParam<FlatKnob>(Vec(91, 234), module, HexNut::VRY_PARAM));
        addParam(createParam<FlatKnob>(Vec(91, 262), module, HexNut::VRZ_PARAM));

        addParam(createParam<FlatKnob>(Vec(35, 318), module, HexNut::BLEND_PARAM));
        addParam(createParam<FlatKnob>(Vec(63, 318), module, HexNut::CROP_PARAM));
        addParam(createParam<FlatKnob>(Vec(91, 318), module, HexNut::READ_RING_PARAM));

        addInput(createInputCentered<FlatPort>(Vec(7 + tR, 346 + tR), module, HexNut::INPUT_INPUT));
        addOutput(createOutputCentered<FlatPortOut>(Vec(119 + tR, 346 + tR), module, HexNut::OUTPUT_OUTPUT));

        HexDisplay *display = createWidget<HexDisplay>((Vec(0.0, 41 - 4)));
        display->box.size = (Vec(150, 130 + 8));
        display->module = module;
        display->hex = &module->hex;
        display->moduleWidget = this;
        addChild(display);
    }
};

Model *modelHexNut = createModel<HexNut, HexNutWidget>("HexNut");