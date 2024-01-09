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

struct FlatBinary : app::SvgSwitch
{
    FlatBinary()
    {
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Binary_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Binary_1.svg")));
    }
};

struct FlatTrinary : app::SvgSwitch
{
    FlatTrinary()
    {
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Trinary_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Trinary_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/Trinary_2.svg")));
    }
};