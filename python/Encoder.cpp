#include <cstring>
#include <cstdio>

#include "Encoder.hpp"

Encoder::Encoder():
    depth(0),
    bufferMemoryAllocated(0),
    buffer(heap),
    out(buffer),
    isError(false)
{
}

Encoder::~Encoder() {
}

inline bool Encoder::pushCharUnsafe(char c) {
    *(out++) = c;
    return true;
}

bool Encoder::pushString(const char * str, size_t len) {
    if (!reserve(len * 5 / 4 + 2)) // 2 for quotes
        return false;

    pushCharUnsafe('\"');
    memcpy(out, str, len);
    out += len;
    pushCharUnsafe('\"');

    return true;
}

bool Encoder::pushUcs(Py_UNICODE * str, size_t lengthInBytes) {
    if (!reserve(lengthInBytes * 5 / 4 + 2)) // 2 for quotes
        return false;

    pushCharUnsafe('\"');

    Py_UNICODE * to = str + lengthInBytes / sizeof(Py_UNICODE);

    for (; str < to; ) {
        if (*str > 127) {
            *(out++) = (char) (0xC0 | ( *(str) >> 6 ));
            *(out++) = (char) (0x80 | ( *(str++) & 0x3F));
        } else {
            *(out++) = *(str++);
        }
    }

    pushCharUnsafe('\"');

    return true;
}

void Encoder::pushConstUnsafe(const char * val, size_t length) {
    memcpy(out, val, length);
    out += length;
}

bool Encoder::pushBool(bool val) {
    if (!reserve(5)) // 5 for false
        return false;
    if (val)
        pushConstUnsafe("true", 4);
    else
        pushConstUnsafe("false", 5);
    return true;
}

bool Encoder::reserve(size_t len) {
    len += depth + 2; // always reserve depth extra characters, plus comma and colon
    if (bufferMemoryAllocated == 0) { // we using heap
        if (out + len - buffer <= sizeof(heap)) // use -1 to check size after single character addition
            return true;
        // NOTE allocate 1 bytes more
        return false;
        // TODO memory reallocation
    }
    return false;
    // TODO memory allocation
}

const char * Encoder::result() {
    return buffer;
}

size_t Encoder::resultSize() {
    return out - buffer;
}

bool Encoder::enterMap() {
    depth += 1;
    if (!reserve(1))
        return false;
    return pushCharUnsafe('{');
}

bool Encoder::exitMap() {
    depth -= 1;
    return pushCharUnsafe('}');
}

bool Encoder::enterSeq() {
    depth += 1;
    if (!reserve(1))
        return false;
    return pushCharUnsafe('[');
}

bool Encoder::exitSeq() {
    depth -= 1;
    return pushCharUnsafe(']');
}

size_t Encoder::getDepth() {
    return depth;
}

bool Encoder::pushColon() {
    return pushCharUnsafe(':');
}

bool Encoder::pushComma() {
    return pushCharUnsafe(',');
}

bool Encoder::pushInteger(long long value) {
    if (!reserve(20))
        return false;
    char buf[20];
    char *p = buf;
    while (value > 0) {
        *(p++) = value % 10 + '0';
        value = value / 10;
    }
    while (p > buf) {
        *(out++) = *(--p);
    }
    return true;
}

bool Encoder::pushInteger(unsigned long long value) {
    if (!reserve(20))
        return false;
    char buf[20];
    char *p = buf;
    while (value > 0) {
        *(p++) = value % 10 + '0';
        value = value / 10;
    }
    while (p > buf) {
        *(out++) = *(--p);
    }
    return true;
}
