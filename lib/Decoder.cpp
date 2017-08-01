#include "ultrajson.h"
#include "py_defines.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <errno.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

struct DecoderState {
  char *start;
  char *end;
  wchar_t *escStart;
  wchar_t *escEnd;
  int escHeap;
  int lastType;
  JSUINT32 objDepth;
  void *prv;
  JSONObjectDecoder *dec;
};

class Decoder {
};

JSOBJ FASTCALL_MSVC decode_any( struct DecoderState *ds) FASTCALL_ATTR;
typedef JSOBJ (*PFN_DECODER)( struct DecoderState *ds);

static PyObject * SetError(int offset, const char *message) {
  // TODO set error message
  //ds->dec->errorOffset = ds->start + offset;
  //ds->dec->errorStr = (char *) message;
}

double createDouble(double intNeg, double intValue, double frcValue, int frcDecimalCount)
{
  static const double g_pow10[] = {1.0, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001,0.0000001, 0.00000001, 0.000000001, 0.0000000001, 0.00000000001, 0.000000000001, 0.0000000000001, 0.00000000000001, 0.000000000000001};
  return (intValue + (frcValue * g_pow10[frcDecimalCount])) * intNeg;
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decodePreciseFloat(struct DecoderState *ds) {
  char *end;
  double value;
  errno = 0;

  value = strtod(ds->start, &end);

  if (errno == ERANGE) {
    return SetError(ds, -1, "Range error when decoding numeric as double");
  }

  ds->start = end;
  return ds->dec->newDouble(ds->prv, value);
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decode_numeric (struct DecoderState *ds) {
  int intNeg = 1;
  int mantSize = 0;
  JSUINT64 intValue;
  JSUINT64 prevIntValue;
  int chr;
  int decimalCount = 0;
  double frcValue = 0.0;
  double expNeg;
  double expValue;
  char *offset = ds->start;

  JSUINT64 overflowLimit = LLONG_MAX;

  if (*(offset) == '-')
  {
    offset ++;
    intNeg = -1;
    overflowLimit = LLONG_MIN;
  }

  // Scan integer part
  intValue = 0;

  while (1)
  {
    chr = (int) (unsigned char) *(offset);

    switch (chr)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        //PERF: Don't do 64-bit arithmetic here unless we know we have to
        prevIntValue = intValue;
        intValue = intValue * 10ULL + (JSLONG) (chr - 48);

        if (intNeg == 1 && prevIntValue > intValue)
        {
          return SetError(ds, -1, "Value is too big!");
        }
        else if (intNeg == -1 && intValue > overflowLimit)
        {
          return SetError(ds, -1, overflowLimit == LLONG_MAX ? "Value is too big!" : "Value is too small");
        }

        offset ++;
        mantSize ++;
        break;
      }
      case '.':
      {
        offset ++;
        goto DECODE_FRACTION;
        break;
      }
      case 'e':
      case 'E':
      {
        offset ++;
        goto DECODE_EXPONENT;
        break;
      }

      default:
      {
        goto BREAK_INT_LOOP;
        break;
      }
    }
  }

BREAK_INT_LOOP:

  ds->lastType = JT_INT;
  ds->start = offset;

  if (intNeg == 1 && (intValue & 0x8000000000000000ULL) != 0)
  {
    return ds->dec->newUnsignedLong(ds->prv, intValue);
  }
  else if ((intValue >> 31))
  {
    return ds->dec->newLong(ds->prv, (JSINT64) (intValue * (JSINT64) intNeg));
  }
  else
  {
    return ds->dec->newInt(ds->prv, (JSINT32) (intValue * intNeg));
  }

DECODE_FRACTION:

  if (ds->dec->preciseFloat)
  {
    return decodePreciseFloat(ds);
  }

  // Scan fraction part
  frcValue = 0.0;
  for (;;)
  {
    chr = (int) (unsigned char) *(offset);

    switch (chr)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        if (decimalCount < JSON_DOUBLE_MAX_DECIMALS)
        {
          frcValue = frcValue * 10.0 + (double) (chr - 48);
          decimalCount ++;
        }
        offset ++;
        break;
      }
      case 'e':
      case 'E':
      {
        offset ++;
        goto DECODE_EXPONENT;
        break;
      }
      default:
      {
        goto BREAK_FRC_LOOP;
      }
    }
  }

BREAK_FRC_LOOP:
  //FIXME: Check for arithemtic overflow here
  ds->lastType = JT_DOUBLE;
  ds->start = offset;
  return ds->dec->newDouble (ds->prv, createDouble( (double) intNeg, (double) intValue, frcValue, decimalCount));

DECODE_EXPONENT:
  if (ds->dec->preciseFloat)
  {
    return decodePreciseFloat(ds);
  }

  expNeg = 1.0;

  if (*(offset) == '-')
  {
    expNeg = -1.0;
    offset ++;
  }
  else
  if (*(offset) == '+')
  {
    expNeg = +1.0;
    offset ++;
  }

  expValue = 0.0;

  for (;;)
  {
    chr = (int) (unsigned char) *(offset);

    switch (chr)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        expValue = expValue * 10.0 + (double) (chr - 48);
        offset ++;
        break;
      }
      default:
      {
        goto BREAK_EXP_LOOP;
      }
    }
  }

BREAK_EXP_LOOP:
  //FIXME: Check for arithemtic overflow here
  ds->lastType = JT_DOUBLE;
  ds->start = offset;
  return ds->dec->newDouble (ds->prv, createDouble( (double) intNeg, (double) intValue , frcValue, decimalCount) * pow(10.0, expValue * expNeg));
}

PyObject * decodeTrue() {
  if (cursor - end != 4) {
    setError("Unexpected length for 'true'");
    return NULL;
  }
  if (*((uint32_t *) cursor) != 0x65757274) { // ascii true in hex
    setError("Bad symbol, expecting 'true'");
    return NULL;
  }
  cursor += 4;
  return Py_RETURN_TRUE;
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decode_false ( struct DecoderState *ds)
{
  char *offset = ds->start;
  offset ++;

  if (*(offset++) != 'a')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;
  if (*(offset++) != 's')
    goto SETERROR;
  if (*(offset++) != 'e')
    goto SETERROR;

  ds->lastType = JT_FALSE;
  ds->start = offset;
  return ds->dec->newFalse(ds->prv);

SETERROR:
  return SetError(ds, -1, "Unexpected character found when decoding 'false'");
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decode_null ( struct DecoderState *ds)
{
  char *offset = ds->start;
  offset ++;

  if (*(offset++) != 'u')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;

  ds->lastType = JT_NULL;
  ds->start = offset;
  return ds->dec->newNull(ds->prv);

SETERROR:
  return SetError(ds, -1, "Unexpected character found when decoding 'null'");
}

FASTCALL_ATTR void FASTCALL_MSVC SkipWhitespace(struct DecoderState *ds)
{
  char *offset = ds->start;

  for (;;)
  {
    switch (*offset)
    {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        offset ++;
        break;

      default:
        ds->start = offset;
        return;
    }
  }
}

enum DECODESTRINGSTATE
{
  DS_ISNULL = 0x32,
  DS_ISQUOTE,
  DS_ISESCAPE,
  DS_UTFLENERROR,

};

PyObject * decodeString() {
  Py_UNICODE * rp = stringBuffer;
  Py_UNICODE * re = stringBuffer + stringBufferLength;

  for (*cursor < end; cursor++) {

    if (rp == re) {
      rp = reserveStringBuffer(stringBufferLength * 1.5);
      if (rp == NULL) {
        setError("Can't allocate memory");
        return NULL;
      }
      re = stringBuffer + stringBufferLength;
    }

    if (*cursor <= 0x7F) { // most common case for ascii
      if (*cursor == '\\') {
        // TODO analyze char
        cursor++;
        if (cursor >= end) {
          setError("Unexpected end of line");
          return NULL;
        }
        switch (*cursor) {
          default:
            *(rp++) = *cursor;
        }
      } else if (*cursor == '\"') {
        // TODO return result
        return NULL;
      } else {
        // most frequent case for ascii strings
        // works faster than table matching
        *(rp++) = *p;
      }
    } else {
      // TODO unicode escaping here
      *(rp++) = '?';
    }
  }

  for (;;) {
    switch (g_decoderLookup[(JSUINT8)(*inputOffset)])
    {
      case DS_ISNULL: {
        return SetError(ds, -1, "Unmatched ''\"' when when decoding 'string'");
      }
      case DS_ISQUOTE:
      {
        ds->lastType = JT_UTF8;
        inputOffset ++;
        ds->start += ( (char *) inputOffset - (ds->start));
        return ds->dec->newString(ds->prv, ds->escStart, escOffset);
      }
      case DS_UTFLENERROR:
      {
        return SetError (ds, -1, "Invalid UTF-8 sequence length when decoding 'string'");
      }
      case DS_ISESCAPE:
        inputOffset ++;
        switch (*inputOffset)
        {
          case '\\': *(escOffset++) = L'\\'; inputOffset++; continue;
          case '\"': *(escOffset++) = L'\"'; inputOffset++; continue;
          case '/':  *(escOffset++) = L'/';  inputOffset++; continue;
          case 'b':  *(escOffset++) = L'\b'; inputOffset++; continue;
          case 'f':  *(escOffset++) = L'\f'; inputOffset++; continue;
          case 'n':  *(escOffset++) = L'\n'; inputOffset++; continue;
          case 'r':  *(escOffset++) = L'\r'; inputOffset++; continue;
          case 't':  *(escOffset++) = L'\t'; inputOffset++; continue;

          case 'u':
          {
            int index;
            inputOffset ++;

            for (index = 0; index < 4; index ++)
            {
              switch (*inputOffset)
              {
                case '\0': return SetError (ds, -1, "Unterminated unicode escape sequence when decoding 'string'");
                default: return SetError (ds, -1, "Unexpected character in unicode escape sequence when decoding 'string'");

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                  sur[iSur] = (sur[iSur] << 4) + (JSUTF16) (*inputOffset - '0');
                  break;

                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                  sur[iSur] = (sur[iSur] << 4) + 10 + (JSUTF16) (*inputOffset - 'a');
                  break;

                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                  sur[iSur] = (sur[iSur] << 4) + 10 + (JSUTF16) (*inputOffset - 'A');
                  break;
              }

              inputOffset ++;
            }

            if (iSur == 0)
            {
              if((sur[iSur] & 0xfc00) == 0xd800)
              {
                // First of a surrogate pair, continue parsing
                iSur ++;
                break;
              }
              (*escOffset++) = (wchar_t) sur[iSur];
              iSur = 0;
            }
            else
            {
              // Decode pair
              if ((sur[1] & 0xfc00) != 0xdc00)
              {
                return SetError (ds, -1, "Unpaired high surrogate when decoding 'string'");
              }
#if WCHAR_MAX == 0xffff
              (*escOffset++) = (wchar_t) sur[0];
              (*escOffset++) = (wchar_t) sur[1];
#else
              (*escOffset++) = (wchar_t) 0x10000 + (((sur[0] - 0xd800) << 10) | (sur[1] - 0xdc00));
#endif
              iSur = 0;
            }
          break;
        }

        case '\0': return SetError(ds, -1, "Unterminated escape sequence when decoding 'string'");
        default: return SetError(ds, -1, "Unrecognized escape sequence when decoding 'string'");
      }
      break;

      case 1:
      {
        *(escOffset++) = (wchar_t) (*inputOffset++);
        break;
      }

      case 2:
      {
        ucs = (*inputOffset++) & 0x1f;
        ucs <<= 6;
        if (((*inputOffset) & 0x80) != 0x80)
        {
          return SetError(ds, -1, "Invalid octet in UTF-8 sequence when decoding 'string'");
        }
        ucs |= (*inputOffset++) & 0x3f;
        *(escOffset++) = (wchar_t) ucs;
        break;
      }

      case 3:
      {
        JSUTF32 ucs = 0;
        ucs |= (*inputOffset++) & 0x0f;

        for (index = 0; index < 2; index ++)
        {
          ucs <<= 6;
          oct = (*inputOffset++);

          if ((oct & 0x80) != 0x80)
          {
            return SetError(ds, -1, "Invalid octet in UTF-8 sequence when decoding 'string'");
          }

          ucs |= oct & 0x3f;
        }

        if (ucs < 0x800) return SetError (ds, -1, "Overlong 3 byte UTF-8 sequence detected when encoding string");
        *(escOffset++) = (wchar_t) ucs;
        break;
      }

      case 4:
      {
        JSUTF32 ucs = 0;
        ucs |= (*inputOffset++) & 0x07;

        for (index = 0; index < 3; index ++)
        {
          ucs <<= 6;
          oct = (*inputOffset++);

          if ((oct & 0x80) != 0x80)
          {
            return SetError(ds, -1, "Invalid octet in UTF-8 sequence when decoding 'string'");
          }

          ucs |= oct & 0x3f;
        }

        if (ucs < 0x10000) return SetError (ds, -1, "Overlong 4 byte UTF-8 sequence detected when decoding 'string'");

#if WCHAR_MAX == 0xffff
        if (ucs >= 0x10000)
        {
          ucs -= 0x10000;
          *(escOffset++) = (wchar_t) (ucs >> 10) + 0xd800;
          *(escOffset++) = (wchar_t) (ucs & 0x3ff) + 0xdc00;
        }
        else
        {
          *(escOffset++) = (wchar_t) ucs;
        }
#else
        *(escOffset++) = (wchar_t) ucs;
#endif
        break;
      }
    }
  }
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decode_array(struct DecoderState *ds) {
  JSOBJ itemValue;
  JSOBJ newObj;
  int len;
  ds->objDepth++;
  if (ds->objDepth > JSON_MAX_OBJECT_DEPTH) {
    return SetError(ds, -1, "Reached object decoding depth limit");
  }

  newObj = ds->dec->newArray(ds->prv);
  len = 0;

  ds->lastType = JT_INVALID;
  ds->start ++;

  for (;;)
  {
    SkipWhitespace(ds);

    if ((*ds->start) == ']')
    {
      ds->objDepth--;
      if (len == 0)
      {
        ds->start ++;
        return newObj;
      }

      ds->dec->releaseObject(ds->prv, newObj);
      return SetError(ds, -1, "Unexpected character found when decoding array value (1)");
    }

    itemValue = decode_any(ds);

    if (itemValue == NULL)
    {
      ds->dec->releaseObject(ds->prv, newObj);
      return NULL;
    }

    ds->dec->arrayAddItem (ds->prv, newObj, itemValue);

    SkipWhitespace(ds);

    switch (*(ds->start++))
    {
    case ']':
    {
      ds->objDepth--;
      return newObj;
    }
    case ',':
      break;

    default:
      ds->dec->releaseObject(ds->prv, newObj);
      return SetError(ds, -1, "Unexpected character found when decoding array value (2)");
    }

    len ++;
  }
}

FASTCALL_ATTR JSOBJ FASTCALL_MSVC decode_object( struct DecoderState *ds)
{
  JSOBJ itemName;
  JSOBJ itemValue;
  JSOBJ newObj;

  ds->objDepth++;
  if (ds->objDepth > JSON_MAX_OBJECT_DEPTH) {
    return SetError(ds, -1, "Reached object decoding depth limit");
  }

  newObj = ds->dec->newObject(ds->prv);

  ds->start ++;

  for (;;)
  {
    SkipWhitespace(ds);

    if ((*ds->start) == '}')
    {
      ds->objDepth--;
      ds->start ++;
      return newObj;
    }

    ds->lastType = JT_INVALID;
    itemName = decode_any(ds);

    if (itemName == NULL)
    {
      ds->dec->releaseObject(ds->prv, newObj);
      return NULL;
    }

    if (ds->lastType != JT_UTF8)
    {
      ds->dec->releaseObject(ds->prv, newObj);
      ds->dec->releaseObject(ds->prv, itemName);
      return SetError(ds, -1, "Key name of object must be 'string' when decoding 'object'");
    }

    SkipWhitespace(ds);

    if (*(ds->start++) != ':')
    {
      ds->dec->releaseObject(ds->prv, newObj);
      ds->dec->releaseObject(ds->prv, itemName);
      return SetError(ds, -1, "No ':' found when decoding object value");
    }

    SkipWhitespace(ds);

    itemValue = decode_any(ds);

    if (itemValue == NULL)
    {
      ds->dec->releaseObject(ds->prv, newObj);
      ds->dec->releaseObject(ds->prv, itemName);
      return NULL;
    }

    ds->dec->objectAddKey (ds->prv, newObj, itemName, itemValue);

    SkipWhitespace(ds);

    switch (*(ds->start++))
    {
      case '}':
      {
        ds->objDepth--;
        return newObj;
      }
      case ',':
        break;

      default:
        ds->dec->releaseObject(ds->prv, newObj);
        return SetError(ds, -1, "Unexpected character in found when decoding object value");
    }
  }
}

PyObject * Decoder::decodeAny() {
  for (;cursor < end; cursor++) {
    switch (*cursor) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-': return decodeNumeric();

      case '"': return decodeString();

      case '{': return decodeObject();
      case '[': return decodeArray();

      case 't': return decodeTrue();
      case 'f': return decodeFalse();
      case 'n': return decodeNone();

      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;

      default:
        setError("Expected object or value");
        return NULL;
    }
  }
}


PyObject * JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer) {
  /*
  FIXME: Base the size of escBuffer of that of cbBuffer so that the unicode escaping doesn't run into the wall each time */
  struct DecoderState ds;
  wchar_t escBuffer[(JSON_MAX_STACK_BUFFER_SIZE / sizeof(wchar_t))];
  JSOBJ ret;

  ds.start = (char *) buffer;
  ds.end = ds.start + cbBuffer;

  ds.escStart = escBuffer;
  ds.escEnd = ds.escStart + (JSON_MAX_STACK_BUFFER_SIZE / sizeof(wchar_t));
  ds.escHeap = 0;
  ds.prv = dec->prv;
  ds.dec = dec;
  ds.dec->errorStr = NULL;
  ds.dec->errorOffset = NULL;
  ds.objDepth = 0;

  ds.dec = dec;

  ret = decode_any (&ds);

  if (ds.escHeap) {
    dec->free(ds.escStart);
  }

  if (!(dec->errorStr)) {
    if ((ds.end - ds.start) > 0)
    {
      SkipWhitespace(&ds);
    }

    if (ds.start != ds.end && ret)
    {
      dec->releaseObject(ds.prv, ret);
      return SetError(&ds, -1, "Trailing data");
    }
  }

  return ret;
}


PyObject * decode_any_object(const char *buffer, const char *tail) {

  objects<vector>
  PyObject[] * objects = NULL;
  char * error_string = NULL;

  ret = decode_any(buffer, tail, objects, &error_string);

  if (!(dec->errorStr)) {
    if ((ds.end - ds.start) > 0)
    {
      SkipWhitespace(&ds);
    }

    if (ds.start != ds.end && ret)
    {
      dec->releaseObject(ds.prv, ret);
      return SetError(&ds, -1, "Trailing data");
    }
  }

  return ret;
}
