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
    char *error;
    PyObject *exceptionObject;

public:
    Encoder();
    ~Encoder();

    void setError(char * error);

    bool pushString(const char * str, size_t length);
    bool pushUcs(Py_UNICODE * str, size_t lengthInBytes);

    bool pushBool(bool val);

    // numeric
    bool pushInteger(int64_t value);
    bool pushInteger(uint64_t value);
    bool pushDouble(double value);
    bool pushNone();


    // custom types
    bool pushDate(uint64_t year, uint8_t month, uint8_t day);
    bool pushDateTime(uint64_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint64_t microsecond);


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

    // reset encoder
    void reset();
};
