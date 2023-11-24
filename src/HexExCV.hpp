#include "plugin.hpp"
#include "UI.hpp"

struct HexExCV : Module
{
    enum ParamId
    {
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
        CV_BLEND_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUTPUTS_LEN
    };
    enum LightId
    {
        LIGHTS_LEN
    };

    HexExCV()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(CV_VWX_INPUT, "CV Write X");
        configInput(CV_VWY_INPUT, "CV Write Y");
        configInput(CV_VWZ_INPUT, "CV Write Z");

        configInput(CV_VRX_INPUT, "CV Read X");
        configInput(CV_VRY_INPUT, "CV Read Y");
        configInput(CV_VRZ_INPUT, "CV Read Z");

        configInput(CV_BLEND_INPUT, "CV Blend");
    }

    void process(const ProcessArgs &args) override
    {
    }
};

struct HexExCVWidget : ModuleWidget
{
    HexExCVWidget(HexExCV *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/HexExCV.svg")));

        addInput(createInput<FlatPort>((Vec(10, 94)), module, HexExCV::CV_VWX_INPUT));
        addInput(createInput<FlatPort>((Vec(10, 122)), module, HexExCV::CV_VWY_INPUT));
        addInput(createInput<FlatPort>((Vec(10, 150)), module, HexExCV::CV_VWZ_INPUT));

        addInput(createInput<FlatPort>((Vec(10, 206)), module, HexExCV::CV_VRX_INPUT));
        addInput(createInput<FlatPort>((Vec(10, 234)), module, HexExCV::CV_VRY_INPUT));
        addInput(createInput<FlatPort>((Vec(10, 262)), module, HexExCV::CV_VRZ_INPUT));

        addInput(createInput<FlatPort>((Vec(10, 318)), module, HexExCV::CV_BLEND_INPUT));
    }
};

Model *modelHexExCV = createModel<HexExCV, HexExCVWidget>("HexExCV");