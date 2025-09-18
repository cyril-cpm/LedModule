#pragma once

#include "Led.h"
#include "CustomType.hpp"
#include "transitionValue.h"
#include <vector>
#include "IQmathLib.h"

typedef enum {
    TRANSITION_TYPE_LOADING         = 1,
    TRANSITION_TYPE_FADING          = 2,
    TRANSITION_TYPE_BLINKING        = 3,
    TRANSITION_TYPE_FLICKERING      = 4,
    TRANSITION_TYPE_SIMPLEX_SLICE   = 5,
    TRANSITION_TYPE_SIMPLEX_FADE    = 6
} transition_type_t;

class LedModule;
class LedZone;

class Transition
{
    public:
    Transition(const char* name);
    virtual void Apply(LedModule* module, LedZone* zone) = 0;

    private:
    Transition();
    char*   fName;
};

class LoadingTransition : public Transition
{
    public:
    LoadingTransition(const char* name);
    virtual void Apply(LedModule* module, LedZone* zone);

    typedef enum {
        BEGIN_END = 0,
        END_BEGIN = 1,
        MID_EXT = 2,
        EXT_MID = 3
    } LoadDirection;

    private:
    LoadingTransition();
    TransitionValue fRate;
    uint8_t   fDirection = BEGIN_END;
};

class FadingTransition : public Transition
{
    public:
    FadingTransition(const char* name);
    virtual void Apply(LedModule* module, LedZone* zone);

    private:
    FadingTransition();
    TransitionValue fRate;
};

struct SimplexOctave
{
    _iq16   freqX;
    _iq16   freqY;
    _iq16   amplitude;
    String  name;
};

class SimplexTransition : public Transition
{
    public:
    SimplexTransition(const char* name);
    virtual void    Apply(LedModule* module, LedZone* zone);
    void            AddOctave(float x, float y, float amp, const char* name);

    enum {
        SLICE = 0,
        FADE = 1
    };

    private:
    SimplexTransition();
    TransitionValue             fRate;
    std::vector<SimplexOctave>  fOctaves;
    uint8_t                     fType = SLICE;
};

class BlinkingTransition : public Transition
{
    public:
    BlinkingTransition(const char* name);
    virtual void    Apply(LedModule* module, LedZone* zone);

    private:
    BlinkingTransition();
    TransitionValue             fRate;
    uint16_t                    fMidBlinkSpeed = 50;
    uint16_t                    fEdgeBlinkSpeed = 1250;
    uint32_t                    fTickStamp = 0;
    bool                        fPolarity = false;
    int8_t                      fExp = 0;
};

struct LedZone
{
    public:
    uint16_t            fNbLed = 0;
    uint16_t            fStartIndex;
    RGB                 fForeColor = RGB(0, 0, 0);
    RGB                 fBackColor = RGB(0, 0, 0);
    bool                fBicolor = false;
    bool                fMonoFore = false;
    bool                fMonoBack = false;
    
    char*               fName;

    Transition*         fTransition = nullptr;
    
};

class LedModule
{

    private:
    LedModule();
    void    _initSTR(LedZone* ledZone);

    RGB*        fLedData;
    uint16_t      fNumLed = 0;
    gpio_num_t  fLedPin = GPIO_NUM_NC;

    std::vector<LedZone*>    fLedZones;

    public:
    LedModule(gpio_num_t ledPin, uint16_t numLed);

    LedZone* AddForeColorZone(uint16_t startIndex, uint16_t numLed, RGB color, const char* name, transition_type_t transition = TRANSITION_TYPE_LOADING);
    LedZone* AddBackColorZone(uint16_t startIndex, uint16_t numLed, RGB color, const char* name, transition_type_t transition = TRANSITION_TYPE_LOADING);
    LedZone* AddBiColorZone(uint16_t startIndex, uint16_t numLed, RGB foreColor, RGB backColor, const char* name, transition_type_t transition = TRANSITION_TYPE_LOADING);

    void    SetLedColor(size_t i, RGB color);
    RGB     GetLedColor(size_t i);

    RGB&    operator[](size_t i);

    void Update();
};