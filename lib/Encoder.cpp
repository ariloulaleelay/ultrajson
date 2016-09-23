#include <cstring>
#include <cstdio>

#include "Encoder.h"
#include "itoa.h"
#include "dtoa.h"

Encoder::Encoder():
    depth(0),
    bufferMemoryAllocated(0),
    buffer(heap),
    out(buffer),
    isError(false)
{
}

Encoder::~Encoder() {
    if (bufferMemoryAllocated) {
        free(buffer);
        bufferMemoryAllocated = 0;
    }
}

inline bool Encoder::pushCharUnsafe(char c) {
    *(out++) = c;
    return true;
}

bool Encoder::pushString(const char * str, size_t len) {
    if (!reserve(len * 5 / 4 + 2)) // 2 for quotes
        return false;

    pushCharUnsafe('\"');
    const char * to = str + len;
    for(;str < to;) {
        switch(*str) {
            case '\\':
                *(out++) = '\\'; *(out++) = '\\'; str++;
                break;

            case '\"':
                *(out++) = '\\'; *(out++) = '\"'; str++;
                break;

            case '\t':
                *(out++) = '\\'; *(out++) = 't'; str++;
                break;

            case '\n':
                *(out++) = '\\'; *(out++) = 'n'; str++;
                break;

            case '\0':
                *(out++) = '\\'; *(out++) = '0'; str++;
                break;

            default:
                *(out++) = *(str++);
                break;
        }
    }
    //memcpy(out, str, len);
    //out += len;
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
            switch(*str) {
                case '\\':
                    *(out++) = '\\'; *(out++) = '\\'; str++;
                    break;

                case '\"':
                    *(out++) = '\\'; *(out++) = '\"'; str++;
                    break;

                case '\t':
                    *(out++) = '\\'; *(out++) = 't'; str++;
                    break;

                case '\n':
                    *(out++) = '\\'; *(out++) = 'n'; str++;
                    break;

                case '\0':
                    *(out++) = '\\'; *(out++) = '0'; str++;
                    break;

                default:
                    *(out++) = *(str++);
                    break;
            }
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

bool Encoder::pushNone() {
    if (!reserve(4)) // 4 for null
        return false;
    pushConstUnsafe("null", 4);
    return true;
}

bool Encoder::reserve(size_t len) {
    len += depth + 2; // always reserve depth extra characters, plus comma and colon
    if (bufferMemoryAllocated == 0) { // we using heap
        if (out - heap + len < sizeof(heap)) // use -1 to check size after single character addition
            return true;

        buffer = (char *) malloc(out - heap + len + 1);
        if (!buffer)
            return false;
        bufferMemoryAllocated = out - heap + len + 1;
        memcpy(buffer, heap, out - heap);
        out = buffer + (out - heap);
    }

    if (out - buffer + len < bufferMemoryAllocated)
        return true;

    char * newBuffer = (char *) realloc(buffer, out - buffer + len + 1);
    if (!newBuffer)
        return false;
    bufferMemoryAllocated = out - buffer + len + 1;
    out = newBuffer + (out - buffer);
    buffer = newBuffer;

    return true;
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

bool Encoder::pushInteger(int64_t value) {
    if (!reserve(32))
        return false;
    out = i64toa_sse2(value, out);
    return true;
}

bool Encoder::pushInteger(uint64_t value) {
    if (!reserve(32))
        return false;
    out = u64toa_sse2(value, out);
    return true;
}

bool Encoder::pushDouble(double value) {
    if (!reserve(64))
        return false;
    out = dtoa_fast(value, out);
    return true;
}

void Encoder::reset() {
    if (bufferMemoryAllocated > sizeof(heap) * 1024) { // it's about 60 mb, don't hold it
        free(buffer);
        buffer = heap;
        bufferMemoryAllocated = 0;
    }
    out = buffer;
    depth = 0;
    isError = false;
}
