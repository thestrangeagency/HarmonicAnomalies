#include "Hex.hpp"

#define MAX_GRAIN_SIZE 4410
#define AVERAGE_SIZE 128

struct Grain
{
    float buffer[MAX_GRAIN_SIZE];
    int size = MAX_GRAIN_SIZE;
    int writeIndex = 0;
    int readIndex = 0;

    float averageBuffer[AVERAGE_SIZE];
    int averageIndex = 0;

    void setVoltage(float v, float blend)
    {
        blend = clamp(blend, 0.0, 1.0);
        float blended = v * blend + buffer[writeIndex] * (1.0 - blend);

        buffer[writeIndex] = blended;
        ++writeIndex %= size;

        averageBuffer[averageIndex] = abs(blended);
        ++averageIndex %= AVERAGE_SIZE;
    }

    float getVoltage()
    {
        float voltage = buffer[readIndex];
        ++readIndex %= size;
        return voltage;
    }

    float getAverageVoltage()
    {
        float average = 0;
        for (int i = 0; i < AVERAGE_SIZE; i++)
        {
            average += averageBuffer[i];
        }
        return average / AVERAGE_SIZE;
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

    GrainHex(int r) : Hex(r)
    {
        grains.resize(length);
    }

    void setVoltage(float v, float blend) override
    {
        grains[writeCursor].setVoltage(v, blend);
    }

    // float getVoltage() override
    // {
    //     // ignoring read ring for now
    //     return grains[readCursor].getVoltage();
    // }

    float getTileVoltage(int i) override
    {
        tiles[i].read = 1;
        return grains[i].getVoltage();
    }

    void advanceWriteCursor(float x, float y, float z) override
    {
        // do nothing unless at start of a grain
        if (grains[writeCursor].atWriteStart())
        {
            tiles[writeCursor].writ = 1;
            tiles[writeCursor].v = grains[writeCursor].getAverageVoltage();

            Hex::advanceWriteCursor(x, y, z);
        }
    }

    void advanceReadCursor(float x, float y, float z) override
    {
        // do nothing unless at start of a grain
        if (grains[readCursor].atReadStart())
        {
            Hex::advanceReadCursor(x, y, z);
        }
    }
};