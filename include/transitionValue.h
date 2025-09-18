#pragma once

#include <sys/_stdint.h>

struct TransitionValue
{
    public:
    uint8_t     fRate = 0;

    uint8_t     fBegin = 0;
    uint8_t     fEnd = 255;
    uint32_t    fTime = 5000;

    bool        fTransitioning = false;
    bool        fForward = false;
    uint32_t    fTickStamp = 0;
    uint32_t    fRatio = 0;
    uint8_t     fDeltaRate = 0;
    int8_t      fExp = 0;

    char*       fName = nullptr;

    void                Update();
    void                InitSTR();

    operator uint8_t& ();
};