#include "Hex.hpp"

#define MAX_GRAIN_SIZE 4410

struct Grain
{
    float buffer[MAX_GRAIN_SIZE];
    int size = MAX_GRAIN_SIZE;
    int writeIndex = 0;
    int readIndex = 0;

    void setVoltage(float v, float blend)
    {
        buffer[writeIndex] = v * blend + buffer[writeIndex] * (1.0 - blend);
        ++writeIndex %= size;
    }

    float getVoltage()
    {
        float voltage = buffer[readIndex];
        ++readIndex %= size;
        return voltage;
    }

    bool atWriteStart()
    {
        return writeIndex == 0;
    }

    bool atReadStart()
    {
        return readIndex == 0;
    }
};

struct GrainHex : Hex
{
    std::vector<Grain> grains;

    Grain *writeGrain;
    Grain *readGrain;

    GrainHex(int r) : Hex(r)
    {
        grains.resize(length);

        writeGrain = &grains[writeCursor];
        readGrain = &grains[readCursor];
    }

    void setVoltage(float v, float blend)
    {
        writeGrain->setVoltage(v, blend);
    }

    float getVoltage()
    {
        // ignoring read ring for now
        return readGrain->getVoltage();
    }

    void advanceWriteCursor(float x, float y, float z)
    {
        // do nothing unless at start of a grain
        if (writeGrain->atWriteStart())
        {
            Hex::advanceWriteCursor(x, y, z);
            writeGrain = &grains[writeCursor];
        }
    }

    void advanceReadCursor(float x, float y, float z)
    {
        // do nothing unless at start of a grain
        if (readGrain->atReadStart())
        {
            Hex::advanceReadCursor(x, y, z);
            readGrain = &grains[readCursor];
        }
    }
};