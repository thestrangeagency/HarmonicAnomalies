#pragma once
#include "plugin.hpp"

struct FlatKnob : RoundKnob
{
    FlatKnob()
    {
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobFg.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobBg.svg")));
    }
};

struct FlatPort : app::SvgPort
{
    FlatPort()
    {
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/JackIn.svg")));
    }
};

struct FlatPortOut : app::SvgPort
{
    FlatPortOut()
    {
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/JackOut.svg")));
    }
};

struct FlatSwitch : app::SvgSwitch
{
    FlatSwitch()
    {
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Switch_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Switch_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Switch_2.svg")));
    }
};