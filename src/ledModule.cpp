#include "ledModule.h"
#include "esp_log.h"
#include "CustomType.hpp"
#include "freertos/freertos.h"
#include "freertos/task.h"
#include "Simplex.h"

static char* newStrCpy(const char* str)
{
    if (!str)
        return nullptr;
    
    int i = 0;
    while (str[i])
        i++;

    char *dst = nullptr;

    dst = new char[i+1];

    dst[i] = '\0';

    for (auto j = 0; j < i; j++)
        dst[j] = str[j];

    return dst;
}

static char* newStrCat(const char* a, const char* b)
{
    if (!a || !b)
        return nullptr;

    int i = 0;
    for (; a[i]; i++);

    int j = 0;
    for (; b[j]; j++);

    char* dst = nullptr;

    dst = new char[i+j+1];

    dst[i+j] = '\0';

    for (auto k = 0; k < i; k++)
        dst[k] = a[k];

    for (auto k = 0; k < j; k++)
        dst[k + i] = b[k];

    return dst;
}

LedModule::LedModule(gpio_num_t ledPin, uint16_t numLed)
{
    fNumLed = numLed;
    fLedPin = ledPin;
    fLedData = new RGB[numLed];

    FLed.addLeds(fLedPin, fLedData, numLed);
}

void    LedModule::_initSTR(LedZone* ledZone)
{
    if (ledZone)
    {
        char* name = newStrCat(ledZone->fName, "_NBLED");
        STR_UInt16Ref(ledZone->fNbLed, name);
        delete[] name;

        name = newStrCat(ledZone->fName, "_STARTINDEX");
        STR_UInt16Ref(ledZone->fStartIndex, name);
        delete[] name;
    }
}

LedZone* LedModule::AddForeColorZone(uint16_t startIndex, uint16_t numLed, RGB color, const char* name, transition_type_t transition)
{
    LedZone* newZone = new LedZone{
        .fNbLed = numLed,
        .fStartIndex = startIndex,
        .fForeColor = color,
        .fBicolor = false,
        .fMonoFore = true,
        .fMonoBack = false,
        .fName = newStrCpy(name)
    };

    _initSTR(newZone);

    fLedZones.push_back(newZone);

    return fLedZones.back();
}

LedZone* LedModule::AddBackColorZone(uint16_t startIndex, uint16_t numLed, RGB color, const char* name, transition_type_t transition)
{
    LedZone* newZone = new LedZone{
        .fNbLed = numLed,
        .fStartIndex = startIndex,
        .fBackColor = color,
        .fBicolor = false,
        .fMonoFore = false,
        .fMonoBack = true,
        .fName = newStrCpy(name)
    };

    fLedZones.push_back(newZone);
    
    return fLedZones.back();
}

LedZone* LedModule::AddBiColorZone(uint16_t startIndex, uint16_t numLed, RGB foreColor, RGB backColor, const char* name, transition_type_t transition)
{
    LedZone* newZone = new LedZone{
        .fNbLed = numLed,
        .fStartIndex = startIndex,
        .fForeColor = foreColor,
        .fBackColor = backColor,
        .fBicolor = true,
        .fName = newStrCpy(name)
    };

    _initSTR(newZone);

    fLedZones.push_back(newZone);

    return fLedZones.back();
}

void    LedModule::SetLedColor(size_t i, RGB color)
{
    if (i < fNumLed)
        fLedData[i] = color;
}

RGB     LedModule::GetLedColor(size_t i)
{
    if (i > fNumLed)
        i = 0;
    return fLedData[i];
}

RGB&    LedModule::operator[](size_t i)
{
    if (i > fNumLed)
        i = 0;
    return fLedData[i];
}

void    LedModule::Update()
{
    for (auto i = fLedZones.rbegin(); i != fLedZones.rend(); i++)
    {
        if ((*i)->fTransition)
        {
            (*i)->fTransition->Apply(this, *i);
        }
        else if ((*i)->fMonoBack || (*i)->fBicolor)
        {
            for (auto ledIndex = 0; ledIndex < (*i)->fNbLed; ledIndex++)
            {
                fLedData[ledIndex + (*i)->fStartIndex] = (*i)->fBackColor;
            }
        }
    }

    FLed.show();
}

Transition::Transition(const char* name)
{
    fName = newStrCpy(name);
}

LoadingTransition::LoadingTransition(const char* name) : Transition(name)
{
    fRate.fName = newStrCat(name, "_RATE");
    fRate.fRate = 0;

    fRate.InitSTR();

    char* settingName = newStrCat(name, "_DIRECTION");
    STR_UInt8Ref(fDirection, settingName);
    delete[] settingName;
}

void LoadingTransition::Apply(LedModule* module, LedZone* zone)
{
    fRate.Update();
    uint32_t rateFactor = (1u << 16) / zone->fNbLed;

    for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
    {
       
        uint8_t ratedLed = (ledIndex * rateFactor) >> 8;
        int resolvedIndex = zone->fStartIndex;
        int invResolvedIndex = zone->fStartIndex;

        switch (fDirection)
        {
        case BEGIN_END:
            resolvedIndex += ledIndex;
            break;
        
        case END_BEGIN:
            resolvedIndex += zone->fNbLed - 1 - ledIndex;
            break;

        case MID_EXT:
            resolvedIndex += (zone->fNbLed - 1 - ledIndex) >> 1;
            invResolvedIndex += (ledIndex >> 1) + (zone->fNbLed >> 1);
            break;

        case EXT_MID:
            resolvedIndex += ledIndex >> 1;
            invResolvedIndex += ((zone->fNbLed - 1 - ledIndex) >> 1) + (zone->fNbLed >> 1);
        default:
            break;
        }
        
        if (!fRate.fRate || ratedLed > fRate.fRate)
        {
            if (zone->fBicolor)
            {
                module->SetLedColor(resolvedIndex, zone->fBackColor);

                if (fDirection == MID_EXT || fDirection == EXT_MID)
                    module->SetLedColor(invResolvedIndex, zone->fBackColor);
            }
            else if (zone->fMonoBack)
            {
                module->SetLedColor(resolvedIndex, zone->fBackColor);

                if (fDirection == MID_EXT || fDirection == EXT_MID)
                    module->SetLedColor(invResolvedIndex, zone->fBackColor);
            }
        }
        else
        {
            if (zone->fBicolor)
            {
                module->SetLedColor(resolvedIndex, zone->fForeColor);

                if (fDirection == MID_EXT || fDirection == EXT_MID)
                    module->SetLedColor(invResolvedIndex, zone->fForeColor);
            }

            else if (zone->fMonoFore)
            {
                module->SetLedColor(resolvedIndex, zone->fForeColor);

                if (fDirection == MID_EXT || fDirection == EXT_MID)
                    module->SetLedColor(invResolvedIndex, zone->fForeColor);
            }
        }
    }
}

FadingTransition::FadingTransition(const char* name) : Transition(name)
{
    fRate.fName = newStrCat(name, "_RATE");
    fRate.fRate = 0;

    fRate.InitSTR();
}

void FadingTransition::Apply(LedModule* module, LedZone* zone)
{
    fRate.Update();
    
    RGB color;
    uint8_t invRate = ~fRate;
    
    if (zone->fBicolor)
    {
        RGB foreColor = zone->fForeColor;
        RGB backColor = zone->fBackColor;

        color.r = ((foreColor.r * fRate) >> 8) + ((backColor.r * ~fRate) >> 8);
        color.g = ((foreColor.g * fRate) >> 8) + ((backColor.g * ~fRate) >> 8);
        color.b = ((foreColor.b * fRate) >> 8) + ((backColor.b * ~fRate) >> 8);

        for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
            module->SetLedColor(ledIndex + zone->fStartIndex, color);
    }
    else if (zone->fMonoFore)
    {
        color.r = ((zone->fForeColor.r * fRate.fRate) >> 8);
        color.g = ((zone->fForeColor.g * fRate.fRate) >> 8);
        color.b = ((zone->fForeColor.b * fRate.fRate) >> 8);

        for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
        {
            auto offsetedLedIndex = ledIndex + zone->fStartIndex;

            color.r += (module->GetLedColor(offsetedLedIndex).r * invRate) >> 8;
            color.g += (module->GetLedColor(offsetedLedIndex).g * invRate) >> 8;
            color.b += (module->GetLedColor(offsetedLedIndex).b * invRate) >> 8;

            module->SetLedColor(ledIndex + zone->fStartIndex, color);
        }
    }
    else if (zone->fMonoBack)
    {
        color.r = ((zone->fBackColor.r * invRate) >> 8);
        color.g = ((zone->fBackColor.g * invRate) >> 8);
        color.b = ((zone->fBackColor.b * invRate) >> 8);

        for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
        {
            auto offsetedLedIndex = ledIndex + zone->fStartIndex;

            color.r += (module->GetLedColor(offsetedLedIndex).r * fRate.fRate) >> 8;
            color.g += (module->GetLedColor(offsetedLedIndex).g * fRate.fRate) >> 8;
            color.b += (module->GetLedColor(offsetedLedIndex).b * fRate.fRate) >> 8;

            module->SetLedColor(ledIndex + zone->fStartIndex, color);
        }
    }
}

SimplexTransition::SimplexTransition(const char* name) : Transition(name)
{
    fRate.fName = newStrCat(name, "_RATE");
    fRate.fRate = 0;

    fRate.InitSTR();

    char* settingName = newStrCat(name, "_TYPE");
    STR_UInt8Ref(fType, settingName);
    delete[] settingName;
}

void SimplexTransition::Apply(LedModule* module, LedZone* zone)
{
    fRate.Update();
    
    _iq16 y = _IQ16(xTaskGetTickCount()/10);

    for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
    {
        uint8_t noiseLevel = 0;

        noiseLevel = SIMPLEX.Noise(_IQ16mpy(_IQ16(ledIndex), fOctaves[0].freqX),
                                    _IQ16mpy(y, fOctaves[0].freqY));

        switch (fType)
        {
            case SLICE:
                /*(*module)[ledIndex + zone->fStartIndex] = (noiseLevel > fRate) ?
                                                            zone->fForeColor :
                                                            zone->fBackColor;*/
                module->SetLedColor(ledIndex + zone->fStartIndex, (noiseLevel < fRate) ?
                                                            zone->fForeColor :
                                                            zone->fBackColor);
                break;

            case FADE:
            {
                RGB color;
                color.r = ((zone->fForeColor.r * noiseLevel) >> 8) + ((zone->fBackColor.r * (~noiseLevel)) >> 8);
                color.g = ((zone->fForeColor.g * noiseLevel) >> 8) + ((zone->fBackColor.g * (~noiseLevel)) >> 8);
                color.b = ((zone->fForeColor.b * noiseLevel) >> 8) + ((zone->fBackColor.b * (~noiseLevel)) >> 8);
                (*module)[ledIndex + zone->fStartIndex] = color;
                break;
            }
            default:
                break;
        }
    }
}

void    SimplexTransition::AddOctave(float x, float y, float amp, const char* name)
{
    fOctaves.push_back({
        .freqX = _IQ16(x),
        .freqY = _IQ16(y),
        .amplitude = _IQ16(amp),
        .name = name
    });
}

BlinkingTransition::BlinkingTransition(const char* name) : Transition(name)
{
    fRate.fName = newStrCat(name, "_RATE");
    fRate.fRate = 0;
    fRate.InitSTR();

    char* settingName = newStrCat(name, "_MIDSPEED");
    STR_UInt16Ref(fMidBlinkSpeed, settingName);
    delete[] settingName;

    settingName = newStrCat(name, "_EDGESPEED");
    STR_UInt16Ref(fEdgeBlinkSpeed, settingName);
    delete[] settingName;

    settingName = newStrCat(name, "_EXP");
    STR_Int8Ref(fExp, settingName);
    delete[] settingName;
}

void BlinkingTransition::Apply(LedModule* module, LedZone* zone)
{
    fRate.Update();

    if (fRate == 0)
    {
        fTickStamp = xTaskGetTickCount();
        fPolarity = false;
    }
    else if (fRate == 255)
    {
        fTickStamp = xTaskGetTickCount();
        fPolarity = true;
    }
    else
    {
        uint32_t deltaTime = pdTICKS_TO_MS(xTaskGetTickCount() - fTickStamp);
        uint16_t ratedBlinkSpeed;

        uint8_t rate = (fRate > 127 ? ~fRate : fRate) << 1;

        if (fExp > 0)
        {
            uint16_t t = rate;
            for (auto i = 0; i < fExp; i++)
            {
                t = (t * rate) >> 8;
            }
        }
        else if (fExp < 0)
        {
            auto limit = -fExp;

            uint16_t invNewRate = ~rate;
            uint16_t t = ~rate;
            for (auto i = 0 ; i < limit; i++)
            {
                t = (t * invNewRate) >> 8;
            }
            rate = ~t;
        }

        ratedBlinkSpeed = fEdgeBlinkSpeed - (rate * (fEdgeBlinkSpeed - fMidBlinkSpeed) >> 8);

        if (deltaTime > ratedBlinkSpeed)
        {
            fTickStamp = xTaskGetTickCount();
            fPolarity = !fPolarity;
        }
    }

    RGB color;
    bool applyColor = false;

    if (zone->fMonoFore && fPolarity)
    {
        color = zone->fForeColor;
        applyColor = true;
    }
    else if (zone->fMonoBack && !fPolarity)
    {
        color = zone->fBackColor;
        applyColor = true;
    }
    else if (zone->fBicolor)
    {
        color = fPolarity ? zone->fForeColor : zone->fBackColor; 
        applyColor = true;
    }

    if (applyColor)
    {
        for (auto ledIndex = 0; ledIndex < zone->fNbLed; ledIndex++)
        {
            (*module)[ledIndex + zone->fStartIndex] = color; 
        }
    }
}