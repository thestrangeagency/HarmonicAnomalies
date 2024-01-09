#include "plugin.hpp"

#define MAX_COUNT 64.f

/*
    Counts triggers coming in via PULSE_INPUT. When trigger count meets PERIOD_PARAM, begins emitting REPEAT_PARAM number of pulses, one per clock tick. If REPEAT_PARAM is zero acts as a mute.

    The RESET_PERIOD_PARAM can be used to reset every N reset inputs. This works well with an EOC signal, when we want repeats every N input cycles.
*/

struct Repeat : Module
{
    int inputCount = 0;
    int pulseTrainCount = 0;
    int resetCount = 0;

    dsp::PulseGenerator pulseGenerator;

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger pulseTrigger;

    enum ParamId
    {
        PERIOD_PARAM,
        REPEAT_PARAM,
        RESET_PERIOD_PARAM,
        THROUGH_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        CLOCK_INPUT,
        RESET_INPUT,
        PULSE_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        PULSE_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId
    {
        INPUT_COUNT_LIGHT,
        TRAIN_COUNT_LIGHT,
        LIGHTS_LEN
    };

    Repeat()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PERIOD_PARAM, 0.f, MAX_COUNT, 8.f, "Period");
        configParam(REPEAT_PARAM, 0.f, MAX_COUNT, 1.f, "Repeat");
        configParam(RESET_PERIOD_PARAM, 0.f, MAX_COUNT, 1.f, "Reset Period");
        configSwitch(THROUGH_PARAM, 0.0f, 1.0f, 1.0f, "Through", {"Off", "On"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(PULSE_INPUT, "Pulse");
        configOutput(PULSE_OUTPUT, "Pulse");
    }

    void process(const ProcessArgs &args) override
    {
        float repeat_v = params[REPEAT_PARAM].getValue();
        float period_v = params[PERIOD_PARAM].getValue();
        float reset_period_v = params[RESET_PERIOD_PARAM].getValue();
        float through_v = params[THROUGH_PARAM].getValue();

        bool through = through_v > .5;
        bool shouldPulse = false;

        if (inputs[RESET_INPUT].isConnected())
        {
            if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f))
            {
                if (++resetCount >= round(reset_period_v))
                {
                    inputCount = 0;
                    pulseTrainCount = 0;
                    resetCount = 0;
                }
            }
        }

        if (inputs[CLOCK_INPUT].isConnected())
        {
            if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f))
            {
                if (pulseTrainCount > 0)
                {
                    shouldPulse = true;
                    pulseTrainCount--;
                }
            }
        }

        if (inputs[PULSE_INPUT].isConnected())
        {
            if (pulseTrigger.process(inputs[PULSE_INPUT].getVoltage(), 0.1f, 1.f))
            {
                inputCount++;
                if (through)
                    shouldPulse = true;
            }
        }

        // period threshold reached
        if (inputCount >= period_v)
        {
            pulseTrainCount = round(repeat_v);
            inputCount = 0;

            // acts as a mute when REPEAT_PARAM is 0
            if (round(repeat_v) == 0)
            {
                shouldPulse = false;
            }
        }

        if (shouldPulse)
        {
            pulseGenerator.trigger(1e-3f);
        }

        bool pulse = pulseGenerator.process(args.sampleTime);
        outputs[PULSE_OUTPUT].setVoltage(pulse ? 10.0f : 0.0f);

        lights[INPUT_COUNT_LIGHT].setBrightness(clamp(inputCount / period_v, 0.f, 1.f));
        lights[TRAIN_COUNT_LIGHT].setBrightness(clamp(pulseTrainCount / repeat_v, 0.f, 1.f));
    }
};

struct RepeatWidget : ModuleWidget
{
    RepeatWidget(Repeat *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Repeat.svg")));

        addParam(createParamCentered<CKSS>(mm2px(Vec(15.24, 20)), module, Repeat::THROUGH_PARAM));

        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(15.24, 35)), module, Repeat::PERIOD_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(15.24, 50)), module, Repeat::REPEAT_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(15.24, 65)), module, Repeat::RESET_PERIOD_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 105.32)), module, Repeat::CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 77.544)), module, Repeat::RESET_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 91.432)), module, Repeat::PULSE_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 119.208)), module, Repeat::PULSE_OUTPUT));

        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(9.948, 21.992)), module, Repeat::INPUT_COUNT_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(20.532, 21.992)), module, Repeat::TRAIN_COUNT_LIGHT));
    }
};

Model *modelRepeat = createModel<Repeat, RepeatWidget>("Repeat");