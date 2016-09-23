#include <cstring>
#include <cstdio>
#include "itoa.h"

// TODO implement fastest algorithm http://wm.ite.pl/notesen/2015-12-29-float-to-string.html

char * dtoa_fast(double value, char * buffer) {
    static const double pow10 = 1000000000;

    if (value > 1e16 - 1) {
        char buf[64];
        sprintf(buf, "%.15e", value);
        memcpy(buffer, buf, strlen(buf));
        buffer += strlen(buf);
        return buffer;
    }

    if (value < 0) {
        *(buffer++) = '-';
        value = -value;
    }

    uint64_t whole = (uint64_t) value;
    uint64_t frac = (uint64_t)((value - whole) * pow10);

    buffer = u64toa_sse2(whole, buffer);
    *(buffer++) = '.';
    if (frac > 0) {
        if (frac > 99999999) {
        } else if (frac > 9999999) {
            *(buffer++) = '0';
        } else if (frac > 999999) {
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else if (frac > 99999) {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else if (frac > 9999) {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else if (frac > 999) {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else if (frac > 99) {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else if (frac > 9) {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        } else {
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
            *(buffer++) = '0';
        }
        // NOTE we can remove trailing zeroes, but it will slightly decrease performance
        //while (!(frac % 10)) frac /= 10;
    }
    return u64toa_sse2(frac, buffer);
}
