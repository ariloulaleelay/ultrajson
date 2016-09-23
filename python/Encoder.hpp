#include <cstddef>
#include "py_defines.h"


class Encoder {
private:
    size_t depth;
    size_t bufferMemoryAllocated;
    char heap[65536];
    char *buffer;
    char *out;

private:

    bool reserve(size_t len);
    bool pushCharUnsafe(char c);
    void pushConstUnsafe(const char * val, size_t length);

public:
    bool isError;

public:
    Encoder();
    ~Encoder();

    bool pushString(const char * str, size_t length);
    bool pushUcs(Py_UNICODE * str, size_t lengthInBytes);

    bool pushBool(bool val);

    // numeric
    bool pushInteger(long long value);
    bool pushInteger(unsigned long long value);
    bool pushDouble(double value);
    bool pushNone();

    // constants
    bool pushColon();
    bool pushComma();

    bool enterMap();
    bool exitMap();

    bool enterSeq();
    bool exitSeq();

    size_t resultSize();
    const char * result();

    size_t getDepth();
};
