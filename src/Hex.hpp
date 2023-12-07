#pragma once
#include <cmath>
#include <algorithm>
#include <array>

struct Tile
{
    float x;
    float y;
    float v;
    float writ;
    float read;
};

struct Hex
{
    const float size = .5;
    const float dx = size * 3 / 2;
    const float dy = size * sqrt(3);

    int radius;
    int diameter;

    float width;
    float height;

    int yAxis;
    int length;

    int readLength;
    int writeLength;

    int y_step;
    int z_step;

    std::vector<Tile> tiles;

    int writeCursor = 0;
    int writeRingCursor = 0;

    int readCursor = 0;
    int readRingCursor = 0;

    float writeX = 0;
    float writeY = 0;
    float writeZ = 0;

    float readX = 0;
    float readY = 0;
    float readZ = 0;

    int ringRadius = 0; // radius of ring around cursor
    int maxRingRadius = 64;
    std::vector<int> ringDirs;    // directions around a ring
    std::vector<int> ringOffsets; // given a radius, offsets from cursor to ring around cursor

    enum Mode
    {
        VECTOR,
        RING,
        VORTEX
    };

    Mode floatToMode(float value)
    {
        int roundedValue = static_cast<int>(std::round(value));
        return static_cast<Mode>(roundedValue - 1);
    }

    Mode writeMode = Mode::VECTOR;
    Mode readMode = Mode::VECTOR;

    int writePosRingRadius;
    int writePosRingDir = 0;
    int writePosRingStep = 0;
    int writeMaxRadius;

    int readPosRingRadius;
    int readPosRingDir = 0;
    int readPosRingStep = 0;
    int readMaxRadius;

    Hex(int r) : radius(r)
    {
        initGeometry();
        initTiles();
    }

    void initGeometry()
    {
        diameter = radius * 2;

        width = diameter * dx;
        height = diameter * dy;

        yAxis = 3 * radius - 2;
        length = pow(radius, 3) - pow(radius - 1, 3);
        // 721 if radius = 16
        // 21931 if radius = 86

        readLength = length;
        writeLength = length;

        y_step = yAxis;
        z_step = y_step + 1;

        ringDirs = {-1, -z_step, -y_step, 1, z_step, y_step};
        ringOffsets.resize(maxRingRadius * 6);

        writePosRingRadius = radius / 2;
        writeMaxRadius = radius;

        readPosRingRadius = radius / 2;
        readMaxRadius = radius;
    }

    int voltageToRadius(float v)
    {
        int r = round(radius * v);
        r = clamp(r, 1, radius);
        return r;
    }

    void setWriteMaxRadius(float v)
    {
        int r = voltageToRadius(v);
        writeMaxRadius = r;
    }

    void setReadMaxRadius(float v)
    {
        int r = voltageToRadius(v);
        readMaxRadius = r;
    }

    void setCrop(float v)
    {
        int r = voltageToRadius(v);
        readLength = writeLength = pow(r, 3) - pow(r - 1, 3);
    }

    void setVoltage(float v, float blend)
    {
        blend = clamp(blend, 0.0, 1.0);
        tiles[writeCursor].v = v * blend + tiles[writeCursor].v * (1.0 - blend);
        tiles[writeCursor].writ = 1;
    }

    float getVoltage()
    {
        return ringRadius < 1 ? getTileVoltage(readCursor) : getRingVoltage();
    }

    float getTileVoltage(int i)
    {
        tiles[i].read = 1;
        return tiles[i].v;
    }

    float getRingVoltage()
    {
        float voltage = 0;

        for (const auto &offset : ringOffsets)
        {
            int i = getReadIndexAtOffset(offset);
            voltage += getTileVoltage(i);
        }

        return voltage / sqrt(ringOffsets.size());
    }

    void decayTile(int i)
    {
        tiles[i].writ *= .75;
        tiles[i].read *= .75;
    }

    Tile getTile(int i)
    {
        return tiles[wrap(i, length)];
    }

    Tile getReadTile()
    {
        return tiles[readCursor];
    }

    Tile getReadTileAtOffset(int offset)
    {
        return getTile(readCursor + offset);
    }

    int getReadIndexAtOffset(int offset)
    {
        return wrap(readCursor, readLength);
    }

    void updateReadRingOffsets()
    {
        ringOffsets.clear();
        int top = z_step * ringRadius;

        for (const auto &dir : ringDirs)
        {
            for (int i = 0; i < ringRadius; i++)
            {
                top = top + dir;
                ringOffsets.push_back(top);
            }
        }
    }

    void advanceWriteCursor(float x, float y, float z)
    {
        writeX += x;
        writeY += y;
        writeZ += z;

        writeX = fmod(writeX, length);
        writeY = fmod(writeY, length);
        writeZ = fmod(writeZ, length);

        int writeVectorCursor = round(writeX) + round(writeY) * y_step + round(writeZ) * z_step;

        if (writeMode == RING || writeMode == VORTEX)
        {
            writeRingCursor += ringDirs[writePosRingDir];
            bool isRingEdgeComplete = ++writePosRingStep >= (writeMode == RING ? writeMaxRadius : writePosRingRadius);
            if (isRingEdgeComplete)
            {
                writePosRingStep = 0;
                writePosRingDir++; // change to next edge direction

                bool isRingComplete = writePosRingDir == static_cast<int>(ringDirs.size());
                if (isRingComplete && writeMode == VORTEX)
                {
                    // increase radius in vortex mode
                    writePosRingRadius++;
                    writePosRingRadius %= writeMaxRadius;
                }
                writePosRingDir %= ringDirs.size();
            }
            writeRingCursor = wrap(writeRingCursor, writeLength);
        }

        writeCursor = wrap(writeVectorCursor + writeRingCursor, writeLength);
    }

    void advanceReadCursor(float x, float y, float z)
    {
        readX += x;
        readY += y;
        readZ += z;

        readX = fmod(readX, length);
        readY = fmod(readY, length);
        readZ = fmod(readZ, length);

        int readVectorCursor = round(readX) + round(readY) * y_step + round(readZ) * z_step;

        if (readMode == RING || readMode == VORTEX)
        {
            readRingCursor += ringDirs[readPosRingDir];
            bool isRingEdgeComplete = ++readPosRingStep >= (readMode == RING ? readMaxRadius : readPosRingRadius);
            if (isRingEdgeComplete)
            {
                readPosRingStep = 0;
                readPosRingDir++; // change to next edge direction

                bool isRingComplete = readPosRingDir == static_cast<int>(ringDirs.size());
                if (isRingComplete && readMode == VORTEX)
                {
                    // increase radius in vortex mode
                    readPosRingRadius++;
                    readPosRingRadius %= readMaxRadius;
                }
                readPosRingDir %= ringDirs.size();
            }
            readRingCursor = wrap(readRingCursor, readLength);
        }

        readCursor = wrap(readVectorCursor + readRingCursor, readLength);
    }

    int wrap(int x, int wrapLength)
    {
        while (x < 0)
            x += wrapLength;
        x %= wrapLength;
        return x;
    }

    void initTiles()
    {
        tiles.resize(length);

        for (int i = 0; i < length; ++i)
        {
            std::array<float, 2> c = coordAt(i);
            tiles[i].x = c[0];
            tiles[i].y = c[1];
            tiles[i].v = 0;
            tiles[i].writ = 0;
            tiles[i].read = 0;
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