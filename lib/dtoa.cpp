#include <cstring>
#include <cstdio>
#include "itoa.h"

// TODO implement fastest algorithm http://wm.ite.pl/notesen/2015-12-29-float-to-string.html

char * dtoa_fast(double value, char * buffer) {
    static const double pow10 = 1000000000000;

    // TODO do it faster
    if (value > 0xFFFFFFFFFFFFFFFFUL) {
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
    buffer = u64toa_sse2(whole, buffer);

    double fracDouble = (value - whole) * pow10;
    uint64_t frac = (uint64_t) fracDouble;

    if (fracDouble - frac >= 0.5)
        frac ++;

    *(buffer++) = '.';
    if (frac > 0) {
        *((uint64_t*) buffer) = 0x3030303030303030UL; // set all 8 leading zeroes at once
        *(((uint32_t*) buffer) + 1) = 0x30303030L; // set all 4 leading zeroes at once
        if (frac > 99999999999) {
        } else if (frac > 9999999999) {
            buffer += 1;
        } else if (frac > 999999999) {
            buffer += 2;
        } else if (frac > 99999999) {
            buffer += 3;
        } else if (frac > 9999999) {
            buffer += 4;
        } else if (frac > 999999) {
            buffer += 5;
        } else if (frac > 99999) {
            buffer += 6;
        } else if (frac > 9999) {
            buffer += 7;
        } else if (frac > 999) {
            buffer += 8;
        } else if (frac > 99) {
            buffer += 9;
        } else if (frac > 9) {
            buffer += 10;
        } else {
            buffer += 11;
        }
        // NOTE remove trailing zeroes
        while (!(frac % 10)) frac /= 10;
    }
    return u64toa_sse2(frac, buffer);
}
