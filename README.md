# Harmonic Anomalies

## HexNut

Three dimensional looper.

<img src='./docs/hexnut.png' width=240>

### Donut Reborn

Inspired by our [Donut™ app](https://www.youtube.com/watch?v=mITLz1rrSN0) from a decade ago, HexNut is a looping buffer, that can be accessed along three different axes. By reading and writing along one axis, you can create loops, echoes, and the like. But venturing off-axis yields the truest grit.

It is not three dimensional in the traditional sense, but it does present three axes across a hexagonal grid, making looping a bit bonkers, just how you like it. See a [video of HexNut in action here](https://vimeo.com/893993026).

### Controls

#### Mode

HexNut has 3 modes, as follows.

1. _Vector_ • Data is accessed by following a vector determined by the Write or Read `X`, `Y`, and `Z` controls.
1. _Ring_ • Data is accessed along a ring determined by the `SIZE` parameter.
1. _Vortex_ • Data is accessed by spiraling within a ring determined by the `SIZE` parameter.

#### Vector

There are two sets of access controls, one for writing, `WRITE`, and one for reading, `READ`. Each set controls, from top to bottom, the vector according to which the read and write heads move through the buffer. Double-clicking a control sets it to zero, which can be quite handy to limit movement to a given axis.

#### Blend

The `BLEND` parameter controls how much writing mixes with content already in the buffer. To fully overwrite, set the control fully clockwise. To have no effect when writing, set it fully anticlockwise.

#### Clip

The `CLIP` parameter truncates the buffer, which has the effect of changing loop timing.

#### Spread

The `SPREAD` parameter uses a ring around the read head to read surrounding data. Its effect is often similar to that of a chorus or doubler effect.

### Expander

The `Hex CV` expander allows you to control the read and write vectors, the ring sizes, and writing blend with control voltage. In each section, the controls are arranged in the following order: `X`, `Y`, `Z`, `SIZE`.

## HexaGrain

Three dimensional granular looper.

<img src='./docs/hexagrain.png' width=240>

### HexNut Reborn

A granular version of HexNut. Instead of a grid of individual samples, we use a grid of audio grains.

### Controls

Controls are all the same as HexNut, with the exception of an additional `SIZE` parameter.

#### Size

There are two parameters in the `SIZE` section. The top one controls the written grain size. Smaller grains are faster to read, so as you tweak grain sizes, you may notice that the speed of the read head starts to vary as well. The second parameter still sets the ring size for _Ring_ and _Vortex_ modes, just as it does in HexNut.

## Acknowledgements

A huge thanks to [Red Blob Games](https://www.redblobgames.com) for helping me to think through [hexagonal grids](https://www.redblobgames.com/grids/hexagons/#coordinates). And of course this would all be nothing without the amazing VCV Rack community!
