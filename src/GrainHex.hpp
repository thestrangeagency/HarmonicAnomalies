#include "Hex.hpp"

#define MAX_GRAIN_SIZE 4410

struct GrainTile : Tile
{
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
    };

    Grain grain;

    void setVoltage(float v, float blend)
    {
        grain.setVoltage(v, blend);
        writ = 1;
    }

    float getVoltage()
    {
        return grain.getVoltage();
    }
};

struct GrainHex : Hex
{
    const int radius = 16;

    GrainTile *writeTile; // TODO queue up next tile for xFade
    GrainTile *readTile;

    GrainHex() : Hex()
    {
        writeTile = &tiles[writeCursor];
        readTile = &tiles[readCursor];
    }

    void setVoltage(float v, float blend)
    {
        writeTile->setVoltage(v, blend);
    }
};