#include "plugin.hpp"


struct HexNut : Module {
	enum ParamId {
		VX_PARAM,
		VY_PARAM,
		VZ_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RESET_INPUT,
		INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	HexNut() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(VX_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VZ_PARAM, 0.f, 1.f, 0.f, "");
		configInput(RESET_INPUT, "");
		configInput(INPUT_INPUT, "");
		configOutput(OUTPUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct HexNutWidget : ModuleWidget {
	HexNutWidget(HexNut* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/HexNut.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(2.824, 31.504)), module, HexNut::VX_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(2.824, 34.941)), module, HexNut::VY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(2.824, 38.379)), module, HexNut::VZ_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.499, 31.504)), module, HexNut::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.499, 34.941)), module, HexNut::INPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.499, 38.379)), module, HexNut::OUTPUT_OUTPUT));
	}
};


Model* modelHexNut = createModel<HexNut, HexNutWidget>("HexNut");