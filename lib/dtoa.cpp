#include <cstring>
#include <cstdio>
#include "itoa.h"

inline uint64_t align_trailing_zeroes(uint64_t n) {
    // Simple pure C++ implementation was faster than __builtin_clz version in this situation.
    if (n < 1) return 10;
    if (n < 10) return 100;
    if (n < 100) return 1000;
    if (n < 1000) return 10000;
    if (n < 10000) return 100000;
    if (n < 100000) return 1000000;
    if (n < 1000000) return 10000000;
    if (n < 10000000) return 100000000;
    if (n < 100000000) return 1000000000;
    if (n < 1000000000) return 10000000000;
    if (n < 10000000000) return 100000000000;
    if (n < 100000000000) return 1000000000000;
    if (n < 1000000000000) return 10000000000000;
    if (n < 10000000000000) return 100000000000000;
    return 100000000000000;
}

// TODO implement fastest algorithm http://wm.ite.pl/notesen/2015-12-29-float-to-string.html

char * dtoa_fast(double value, char * buffer) {
    static const double pow10 = 1000000000000000;  // 15 zeroes

    // TODO do it faster
    if (value > 0xFFFFFFFFFFFFFFFFUL || value < 0.0000000000000001) {
        int len = snprintf(buffer, 33, "%.15e", value);
        if (len > 0) buffer += len;
        return buffer;
    }

    if (value < 0) {
        *(buffer++) = '-';
        value = -value;
    }

    uint64_t whole = (uint64_t) value;

    buffer = u64toa_sse2(whole, buffer);

    uint64_t frac = (value - whole) * pow10;
    *(buffer++) = '.';

    if (frac > 0) {
        uint64_t align = align_trailing_zeroes(whole);
        uint64_t delta = frac % align;
        if (uint64_t(delta * 10 / align) >= 5) {
            frac += align;
        }
        frac -= frac % align;

        // set all 16 leading zeroes with two assignments
        *((uint64_t*) buffer) = 0x3030303030303030UL;
        *(((uint64_t*) buffer) + 1) = 0x3030303030303030UL;
        if (frac > 99999999999999) {
        } else if (frac > 9999999999999) {
            buffer += 1;
        } else if (frac > 999999999999) {
            buffer += 2;
        } else if (frac > 99999999999) {
            buffer += 3;
        } else if (frac > 9999999999) {
            buffer += 4;
        } else if (frac > 999999999) {
            buffer += 5;
        } else if (frac > 99999999) {
            buffer += 6;
        } else if (frac > 9999999) {
            buffer += 7;
        } else if (frac > 999999) {
            buffer += 8;
        } else if (frac > 99999) {
            buffer += 9;
        } else if (frac > 9999) {
            buffer += 10;
        } else if (frac > 999) {
            buffer += 11;
        } else if (frac > 99) {
            buffer += 12;
        } else if (frac > 9) {
            buffer += 13;
        } else {
            buffer += 14;
        }
        // NOTE remove trailing zeroes
        while (!(frac % 10)) frac /= 10;
    }
    return u64toa_sse2(frac, buffer);
}
