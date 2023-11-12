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
    int ringRadius = radius - 2;

    const float width = diameter * dx;
    const float height = diameter * dy;

    const int yAxis = 3 * radius - 2;
    const int length = pow(radius, 3) - pow(radius - 1, 3); // 21931 if radius = 86

    std::vector<Tile> tiles;

    int writeCursor = 0;
    int readCursor = 0;

    Hex()
    {
        tiles.resize(length);
        initTiles();
    }

    void setVoltage(float v)
    {
        tiles[writeCursor].v = v;
    }

    void advanceWriteCurser(int x, int y, int z)
    {
        float y_step = yAxis;
        float z_step = y + 1;

        writeCursor += x + y * y_step + z * z_step;
        writeCursor = clamp(writeCursor);
    }

private:
    int clamp(int x)
    {
        if (x < 0)
            x += length;
        else
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
        VX_PARAM,
        VY_PARAM,
        VZ_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        RESET_INPUT,
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
    float writeX;
    float writeY;
    float writeZ;

    HexNut()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(VX_PARAM, 0.f, 1.f, 1.f, "write x");
        configParam(VY_PARAM, 0.f, 1.f, 0.f, "write y");
        configParam(VZ_PARAM, 0.f, 1.f, 0.f, "write z");
        configInput(RESET_INPUT, "reset");
        configInput(INPUT_INPUT, "input");
        configOutput(OUTPUT_OUTPUT, "output");
    }

    void process(const ProcessArgs &args) override
    {
        float in_v = inputs[INPUT_INPUT].getVoltage();
        hex.setVoltage(in_v);

        writeX += params[VX_PARAM].getValue();
        if (writeX >= 1)
        {
            writeX -= 1;
            hex.advanceWriteCurser(1, 0, 0);
        }
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

    void drawLayer(const DrawArgs &args, int layer) override
    {
        if (module && layer == 1)
        {
            drawTiles(args);
            drawWriteCursor(args);
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

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 80)), module, HexNut::VX_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 90)), module, HexNut::VY_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 100)), module, HexNut::VZ_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20, 80)), module, HexNut::RESET_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20, 90)), module, HexNut::INPUT_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20, 100)), module, HexNut::OUTPUT_OUTPUT));

        HexDisplay *display = createWidget<HexDisplay>(mm2px(Vec(0.0, 13.039)));
        display->box.size = mm2px(Vec(66.04, 55.88));
        display->module = module;
        display->hex = &module->hex;
        display->moduleWidget = this;
        addChild(display);
    }
};

Model *modelHexNut = createModel<HexNut, HexNutWidget>("HexNut");