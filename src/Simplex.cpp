#include "Simplex.h"
#include "esp_random.h"
#include "esp_log.h"

void Simplex::begin()
{
    for (auto i = 0; i < 256; i++)
        fPermList[i] = i;

    for (auto i = 255; i > 0; i--)
    {
        uint8_t j = esp_random() % (i + 1);
        uint8_t tmp = fPermList[i];
        fPermList[i] = fPermList[j];
        fPermList[j] = tmp;
    }


}

uint8_t Simplex::Noise(_iq16 x, _iq16 y)
{
    //skew
    _iq16 s = _IQ16mpy(x + y, F);

    _iq16 xs = x + s;
    _iq16 ys = y + s;

    _iq16 i = _IQ16floor(xs); //
    _iq16 j = _IQ16floor(ys); // This is the square in the simplices grid

    //unskew
    _iq16 t = _IQ16mpy(i + j, G);

    _iq16 xf = i - t; //
    _iq16 yf = j - t; // Top Left coordinate of the cell

    _iq16 xp0 = x - xf; //
    _iq16 yp0 = y - yf; // Coordinate of the point in the cell

    _iq16 iOffset = 0;
    _iq16 jOffset = 0;

    if (xp0 > yp0)
        iOffset = _IQ16(1);
    else
        jOffset = _IQ16(1);

    _iq16 xp1 = xp0 - iOffset + G;
    _iq16 yp1 = yp0 - jOffset + G;

    _iq16 xp2 = xp0 - _IQ16(1) + 2 * G;
    _iq16 yp2 = yp0 - _IQ16(1) + 2 * G;

    uint8_t h = fPermList[(fPermList[_IQ16int(j) & 255] + _IQ16int(i)) & 255];

    int8_t* grad0 = grad[h & 7];

    h = fPermList[(fPermList[_IQ16int(j + jOffset) & 255] + _IQ16int(i + iOffset)) & 255];

    int8_t* grad1 = grad[h & 7];

    h = fPermList[(fPermList[(_IQ16int(j) + 1) & 255] + _IQ16int(i) + 1) & 255];

    int8_t* grad2 = grad[h & 7];

    _iq16 scal0 = xp0 * grad0[0] + yp0 * grad0[1];
    _iq16 scal1 = xp1 * grad1[0] + yp1 * grad1[1];
    _iq16 scal2 = xp2 * grad2[0] + yp2 * grad2[1];

    _iq16 t0 = _IQ16(0.5f) - _IQ16mpy(xp0, xp0) - _IQ16mpy(yp0, yp0);
    _iq16 t1 = _IQ16(0.5f) - _IQ16mpy(xp1, xp1) - _IQ16mpy(yp1, yp1);
    _iq16 t2 = _IQ16(0.5f) - _IQ16mpy(xp2, xp2) - _IQ16mpy(yp2, yp2);

    _iq16 contrib0 = _IQ16(0);
    _iq16 contrib1 = _IQ16(0);
    _iq16 contrib2 = _IQ16(0);

    if (t0 >= 0)
        contrib0 = _IQ16mpy(_IQ16mpy(_IQ16mpy(t0, t0), _IQ16mpy(t0, t0)), scal0); // t0^4 * scal0

    if (t1 >= 0)
        contrib1 = _IQ16mpy(_IQ16mpy(_IQ16mpy(t1, t1), _IQ16mpy(t1, t1)), scal1); // t1^4 * scal1

    if (t2 >= 0)
        contrib2 = _IQ16mpy(_IQ16mpy(_IQ16mpy(t2, t2), _IQ16mpy(t2, t2)), scal2); // t2^4 * scal2

    uint8_t noise = _IQ16int((70 * (contrib0 + contrib1 + contrib2) + _IQ16(1)) * 127);

    /*ESP_LOGI("CONTRIB0", "%f", _IQ16toF(t0));
    ESP_LOGI("CONTRIB1", "%f", _IQ16toF(t1));
    ESP_LOGI("CONTRIB2", "%f", _IQ16toF(t2));
    ESP_LOGI("NOISE", "%d", noise);*/

    return noise;
}