#include <cstring>
#include <cstdio>

#include "Encoder.h"
#include "itoa.h"
#include "dtoa.h"

typedef uint32_t v4si __attribute__ ((vector_size (16)));
typedef char v16qi __attribute__ ((vector_size (16)));

int __counter = 0;
#define TO_MASK(x) (__builtin_ia32_pmovmskb128(reinterpret_cast<v16qi>(x)))

#define PRINTMARK(x) printf("MARK %s %d\n", (x), __counter++)

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
    if (!reserve(len * 6 / 4 + 2 + 16)) // 2 for quotes 16 for simd vector
        return false;

    pushCharUnsafe('\"');
    const char * to = str + len;
    for(;str < to;) {
        if (*str < 127) {
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
        } else { // do utf-8 escaping
            *(out++) = (char) (0xC0 | ( *(str) >> 6 ));
            *(out++) = (char) (0x80 | ( *(str++) & 0x3F));
        }
    }
    //memcpy(out, str, len);
    //out += len;
    pushCharUnsafe('\"');

    return true;
}

inline Py_UNICODE * Encoder::_writeUcs(Py_UNICODE * str, Py_UNICODE * till) {
    wchar_t c;
    for (; str < till; ) {
        c = *str;
        if (c <= 0x7F) {
            switch(c) {
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

                case '\r':
                    *(out++) = '\r'; *(out++) = 'r'; str++;
                    break;

                case '\0':
                    *(out++) = '\\'; *(out++) = '0'; str++;
                    break;

                default:
                    *(out++) = *(str++);
                    break;
            }
        } else if (c <= 0x7FF) {
            *(out++) = (char) (0xC0 | ( c >> 6 ));
            *(out++) = (char) (0x80 | ( c & 0x3F));
            str++;
        } else if (c <= 0x7FFF) {
            *(out++) = (char) (0xE0 | (c >> 12 ));
            *(out++) = (char) (0x80 | ((c >> 6) & 0x3F ));
            *(out++) = (char) (0x80 | (c & 0x3F));
            str++;
        } else if (c <= 0x7FFFF) {
            *(out++) = (char) (0xF0 | ((c >> 18)));
            *(out++) = (char) (0x80 | ((c >> 12) & 0x3F));
            *(out++) = (char) (0x80 | ((c >> 6) & 0x3F));
            *(out++) = (char) (0x80 | (c & 0x3F));
            str++;
        } else if (c <= 0x7FFFFF) {
            *(out++) = (char) (0xF8 | ((c >> 24)));
            *(out++) = (char) (0x80 | ((c >> 18) & 0x3F));
            *(out++) = (char) (0x80 | ((c >> 12) & 0x3F));
            *(out++) = (char) (0x80 | ((c >> 6) & 0x3F));
            *(out++) = (char) (0x80 | (c & 0x3F));
            str++;
        } else {
            *(out++) = (char) (0xFC | ((c >> 30) & 0x01)); // & 0x01 protects us from bad utf
            *(out++) = (char) (0x80 | ((c >> 24 & 0x3F)));
            *(out++) = (char) (0x80 | ((c >> 18) & 0x3F));
            *(out++) = (char) (0x80 | ((c >> 12) & 0x3F));
            *(out++) = (char) (0x80 | ((c >> 6) & 0x3F));
            *(out++) = (char) (0x80 | (c & 0x3F));
            str++;
        }
    }
    return str;
}

bool Encoder::pushUcs(Py_UNICODE * str, size_t lengthInBytes) {
    if (!reserve(lengthInBytes * 6 / 4 + 2 + 16)) // 2 for quotes 16 for vector
        return false;

    pushCharUnsafe('\"');

    static const v4si asciiCodes = { 0x7F, 0x7F, 0x7F, 0x7F };
    size_t len = lengthInBytes / sizeof(Py_UNICODE);

    union {
        v4si vec4si;
        v16qi vec16qi;
        uint64_t u64vec[2];
        uint32_t u32vec[4];
        char u8vec[16];
    };

    if ((uint64_t) str % 16) { // align to 16 bytes
        uint64_t l = ((uint64_t) str % 16) / sizeof(Py_UNICODE);
        if (l > len)
            l = len;
        str = _writeUcs(str, str + l);
        len -= l;
    }

    while (len >= 4) {
        vec4si = *((v4si *) str);
        if (!TO_MASK(vec4si > asciiCodes)) {
            static const v16qi unsafe1 = {92, 10, 34, 9, 92, 10, 34, 9, 92, 10, 34, 9, 92, 10, 34, 9};
            static const v16qi unsafe2 = {13, 13, 0, 0, 13, 13, 0, 0, 13, 13, 0, 0, 13, 13, 0, 0};

            v16qi shuffle = {0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12};
            vec16qi = __builtin_shuffle(vec16qi, shuffle);
            v4si unsafe_markers = (vec16qi == unsafe1) | (vec16qi == unsafe2);

            if (TO_MASK(unsafe_markers)) {
                // there is unsafe characters
                for (int i = 0; i < 4; i++) {
                    switch(unsafe_markers[i]) {
                        default:
                            *(out++) = (char) vec4si[i];
                            break;

                        case 0x000000FF: // 92 '\'
                            *(out++) = '\\';
                            *(out++) = '\\';
                            break;

                        case 0x0000FF00: // 10 '\n'
                            *(out++) = '\\';
                            *(out++) = 'n';
                            break;

                        case 0x00FF0000: // 34 '"'
                            *(out++) = '\\';
                            *(out++) = '\"';
                            break;

                        case 0xFFFF0000: // it's zero
                            *(out++) = '\\';
                            *(out++) = '0';
                            break;

                        case 0x0000FFFF: // 13 '\r'
                            *(out++) = '\\';
                            *(out++) = 'r';
                            break;

                        case 0xFF000000: // 9 '\t'
                            *(out++) = '\\';
                            *(out++) = 't';
                            break;
                    }
                }
            } else {
                // safe ascii it's fastest case
                static const v16qi reposition = {0, 4, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                vec16qi = __builtin_shuffle(vec16qi, reposition);
                *((uint32_t *) out) = u32vec[0];
                out += 4;
            }
        } else {
            _writeUcs(str, str + 4);
        }
        str += 4;
        len -= 4;
    }

    if (len > 0) {
        _writeUcs(str, str + len);
    }

    pushCharUnsafe('\"');

    return true;
}

void Encoder::pushConstUnsafe(const char * val, size_t length) {
    memcpy(out, val, length);
    out += length;
}

bool Encoder::pushBool(bool val) {
    if (!reserve(8)) // for single value
        return false;
    if (val) {
        *((uint32_t *) out) = 0x65757274; // ascii true in hex
        out += 4;
    } else {
        *((uint64_t *) out) = 0x00000065736C6166UL; // ascii false in hex
        out += 5;
    }
    return true;
}

bool Encoder::pushNone() {
    if (!reserve(4)) // 4 for null
        return false;
    *((uint32_t *) out) = 0x6C6C756E; // asci null
    out += 4;
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
    if (!reserve(22))
        return false;
    out = i64toa_sse2(value, out);
    return true;
}

bool Encoder::pushInteger(uint64_t value) {
    if (!reserve(22))
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

bool Encoder::pushDateTime(uint64_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (!reserve(22))
        return false;

    *(out++) = '\"';
    *(out++) = year / 1000 - '0';
    year = year % 1000;
    *(out++) = year / 100 - '0';
    year = year % 100;
    *(out++) = year / 10 - '0';
    *(out++) = year % 10 - '0';

    *(out++) = '-';
    *(out++) = month / 10 - '0';
    *(out++) = month % 10 - '0';

    *(out++) = '-';
    *(out++) = day / 10 - '0';
    *(out++) = day % 10 - '0';

    *(out++) = ' ';
    *(out++) = hour / 10 - '0';
    *(out++) = hour % 10 - '0';

    *(out++) = ':';
    *(out++) = minute / 10 - '0';
    *(out++) = minute % 10 - '0';

    *(out++) = ':';
    *(out++) = second / 10 - '0';
    *(out++) = second % 10 - '0';
    *(out++) = '\"';
    return true;
}

bool Encoder::pushDate(uint64_t year, uint8_t month, uint8_t day) {
    if (!reserve(12))
        return false;

    *(out++) = '\"';
    *(out++) = year / 1000 - '0';
    year = year % 1000;
    *(out++) = year / 100 - '0';
    year = year % 100;
    *(out++) = year / 10 - '0';
    *(out++) = year % 10 - '0';

    *(out++) = '-';
    *(out++) = month / 10 - '0';
    *(out++) = month % 10 - '0';

    *(out++) = '-';
    *(out++) = day / 10 - '0';
    *(out++) = day % 10 - '0';
    *(out++) = '\"';

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
