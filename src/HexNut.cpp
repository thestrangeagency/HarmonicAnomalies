#include "plugin.hpp"
#include "Hex.hpp"
#include "UI.hpp"
#include "HexExCV.hpp"

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
        // modes

        float w_mode_v = params[WRITE_MODE_PARAM].getValue();
        float r_mode_v = params[READ_MODE_PARAM].getValue();
        hex.writeMode = hex.floatToMode(w_mode_v);
        hex.readMode = hex.floatToMode(r_mode_v);

        // crop

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

        // rings

        float w_r_v = params[WRITE_RADIUS_PARAM].getValue() + cv_write_size_v;
        float r_r_v = params[READ_RADIUS_PARAM].getValue() + cv_read_size_v;
        hex.setWriteMaxRadius(w_r_v);
        hex.setReadMaxRadius(r_r_v);

        float read_ring_radius_v = params[READ_RING_PARAM].getValue();
        if (read_ring_radius_v != lastReadRingRadius)
        {
            hex.ringRadius = round(read_ring_radius_v);
            hex.updateReadRingOffsets();
            lastReadRingRadius = read_ring_radius_v;
        }

        // i/o

        float in_v = inputs[INPUT_INPUT].getVoltage();
        float blend_v = params[BLEND_PARAM].getValue() + cv_blend_v;
        hex.setVoltage(in_v, blend_v);

        outputs[OUTPUT_OUTPUT].setVoltage(hex.getVoltage());

        // cursors

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

    HexDisplay()
    {
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