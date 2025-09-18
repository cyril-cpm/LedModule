#pragma once

#include "stdint.h"
#include "IQmathLib.h"

#define _IQ16floor(A)  (((A) >> 16) << 16)

static int8_t grad[8][2] = {
    {1, 1},
    {1, -1},
    {-1, 1},
    {-1, -1},
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1}
};

class Simplex {

public:
    uint8_t Noise(_iq16 x, _iq16 y);
    void begin();

private:
    uint8_t fPermList[256];


    static const _iq16  F = _IQ16(0.3660f);
    static const _iq16  G = _IQ16(0.2113f);

    _iq16               scale = _IQ16(0.1f);

};

static Simplex SIMPLEX;