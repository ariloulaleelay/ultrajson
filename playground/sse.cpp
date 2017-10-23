#include <stdio.h>
#include <string.h>
#include <iostream>

wchar_t * decode_utf(const char * str) {
    size_t len = strlen(str);

    wchar_t * result = new wchar_t[len * 5 + 1];
    wchar_t * out = result;

    const char * p = str;
    unsigned int p1, p2, p3, p4;

    for (;;) {
        switch (*p) {

            case 0:
                *out = 0;
                return result;

            default:
                *(out++) = *(p++);
                continue;

            case '\\':
                p++;
                if (*p != 'u' || (p - str + 4) > len) { // TODO check lengths corner cases
                    *out = 0;
                    return result;
                }
                p++;
                p1 = *(p++) - '0';
                if (p1 > 9) p1 = p1 + '0' + 10 - 'A';
                if (p1 > 15) p1 -= p1 + 'A' - 'a';

                p2 = *(p++) - '0';
                if (p2 > 9) p2 = p2 + '0' + 10 - 'A';
                if (p2 > 15) p2 -= p2 + 'A' - 'a';

                p3 = *(p++) - '0';
                if (p3 > 9) p3 = p3 + '0' + 10 - 'A';
                if (p3 > 15) p3 -= p3 + 'A' - 'a';

                p4 = *(p++) - '0';
                if (p4 > 9) p4 = p4 + '0' + 10 - 'A';
                if (p4 > 15) p4 -= p4 + 'A' - 'a';

                *(out++) = (p1 << 12) + (p2 << 8) + (p3 << 4) + p4;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {

    printf("Decoded: `%ls`\n", decode_utf("XXX\\u0064Z hello"));
    return 0;
}
