#include "plugin.hpp"
#include "UI.hpp"

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

    bool is_active = false;

    dsp::PulseGenerator pulseGenerator;

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger pulseTrigger;
    dsp::SchmittTrigger activeTrigger;

    enum ParamId
    {
        PERIOD_PARAM,
        REPEAT_PARAM,
        RESET_PERIOD_PARAM,
        THROUGH_PARAM,
        ACTIVATE_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        CLOCK_INPUT,
        RESET_INPUT,
        PULSE_INPUT,
        ACTIVATE_INPUT,
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
        ACTIVE_LIGHT,
        LIGHTS_LEN
    };

    Repeat()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(PERIOD_PARAM, 0.f, MAX_COUNT, 8.f, "Period");
        configParam(REPEAT_PARAM, 0.f, MAX_COUNT, 1.f, "Repeat");
        configParam(RESET_PERIOD_PARAM, 0.f, MAX_COUNT, 1.f, "Reset Period");

        configSwitch(THROUGH_PARAM, 0.0f, 1.0f, 1.0f, "Through", {"Off", "On"});
        configSwitch(ACTIVATE_PARAM, 0.0f, 1.0f, 1.0f, "Always Active", {"Off", "On"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(PULSE_INPUT, "Pulse");
        configInput(ACTIVATE_INPUT, "Activate");

        configOutput(PULSE_OUTPUT, "Pulse");
    }

    void process(const ProcessArgs &args) override
    {
        float repeat_v = params[REPEAT_PARAM].getValue();
        float period_v = params[PERIOD_PARAM].getValue();
        float reset_period_v = params[RESET_PERIOD_PARAM].getValue();
        float through_v = params[THROUGH_PARAM].getValue();

        bool should_clock = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f);
        bool should_reset = resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f);
        bool should_pulse = pulseTrigger.process(inputs[PULSE_INPUT].getVoltage(), 0.1f, 1.f);

        bool should_activate = activeTrigger.process(inputs[ACTIVATE_INPUT].getVoltage(), 0.1f, 1.f);
        bool always_active = params[ACTIVATE_PARAM].getValue();

        bool through = through_v > .5;
        bool shouldPulse = false;

        if (always_active || should_activate)
        {
            is_active = true;
        }

        if (inputs[RESET_INPUT].isConnected())
        {
            if (should_reset)
            {
                if (++resetCount >= round(reset_period_v))
                {
                    inputCount = 0;
                    pulseTrainCount = 0;
                    resetCount = 0;

                    if (!always_active)
                        is_active = false;
                }
            }
        }

        if (inputs[CLOCK_INPUT].isConnected())
        {
            if (should_clock)
            {
                if (pulseTrainCount > 0)
                {
                    shouldPulse = true;
                    pulseTrainCount--;

                    if (!always_active)
                        is_active = false;
                }
            }
        }

        if (inputs[PULSE_INPUT].isConnected())
        {
            if (is_active && should_pulse)
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
        lights[ACTIVE_LIGHT].setBrightness(is_active ? 1.f : 0.f);
    }
};

struct RepeatWidget : ModuleWidget
{
    RepeatWidget(Repeat *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Repeat.svg")));

        addParam(createParam<FlatSnapKnob>(Vec(6, 66), module, Repeat::PERIOD_PARAM));
        addChild(createLight<FlatLight<YellowishLight>>(Vec(39, 72), module, Repeat::INPUT_COUNT_LIGHT));

        addParam(createParam<FlatSnapKnob>(Vec(6, 122), module, Repeat::REPEAT_PARAM));
        addChild(createLight<FlatLight<YellowishLight>>(Vec(39, 128), module, Repeat::TRAIN_COUNT_LIGHT));
        addParam(createParam<FlatSnapKnob>(Vec(60, 122), module, Repeat::RESET_PERIOD_PARAM));

        addParam(createParam<FlatBinary>(Vec(6, 178), module, Repeat::ACTIVATE_PARAM));
        addInput(createInput<FlatPort>(Vec(33, 178), module, Repeat::ACTIVATE_INPUT));
        addChild(createLight<FlatLight<YellowishLight>>(Vec(66, 184), module, Repeat::ACTIVE_LIGHT));

        addInput(createInput<FlatPort>(Vec(6, 234), module, Repeat::CLOCK_INPUT));
        addInput(createInput<FlatPort>(Vec(60, 234), module, Repeat::RESET_INPUT));

        addInput(createInput<FlatPort>(Vec(6, 290), module, Repeat::PULSE_INPUT));
        addParam(createParam<FlatBinary>(Vec(33, 290), module, Repeat::THROUGH_PARAM));
        addOutput(createOutput<FlatPort>(Vec(60, 290), module, Repeat::PULSE_OUTPUT));
    }
};

Model *modelRepeat = createModel<Repeat, RepeatWidget>("Repeat");