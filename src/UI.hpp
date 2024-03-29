#pragma once
#include "plugin.hpp"

static const NVGcolor SCHEME_YELLOWISH = nvgRGB(0xE4, 0xEC, 0x8F);

template <typename TBase = GrayModuleLightWidget>
struct TYellowishLight : TBase
{
    TYellowishLight()
    {
        this->addBaseColor(SCHEME_YELLOWISH);
    }
};
using YellowishLight = TYellowishLight<>;

template <typename TBase>
struct FlatLight : TSvgLight<TBase>
{
    FlatLight()
    {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Light.svg")));
    }
};

struct FlatKnob : RoundKnob
{
    FlatKnob()
    {
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobFg.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobBg.svg")));
    }
};

struct FlatSnapKnob : FlatKnob
{
    FlatSnapKnob()
    {
        snap = true;
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