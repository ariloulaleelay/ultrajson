/*
Developed by ESN, an Electronic Arts Inc. studio.
Copyright (c) 2014, Electronic Arts Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ESN, Electronic Arts Inc. nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS INC. BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Portions of code from MODP_ASCII - Ascii transformations (upper/lower, etc)
http://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

Numeric decoder derived from from TCL library
http://www.opensource.apple.com/source/tcl/tcl-14/tcl/license.terms
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
*/

#include "ultrajson.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <float.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if ( (defined(_WIN32) || defined(WIN32) ) && ( defined(_MSC_VER) ) )
#define snprintf sprintf_s
#endif

/*
Worst cases being:

Control characters (ASCII < 32)
0x00 (1 byte) input => \u0000 output (6 bytes)
1 * 6 => 6 (6 bytes required)

or UTF-16 surrogate pairs
4 bytes input in UTF-8 => \uXXXX\uYYYY (12 bytes).

4 * 6 => 24 bytes (12 bytes required)

The extra 2 bytes are for the quotes around the string

*/
#define RESERVE_STRING(_len) (2 + ((_len) * 6))

static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 10000000000, 100000000000, 1000000000000, 10000000000000, 100000000000000, 1000000000000000};
static const char g_hexChars[] = "0123456789abcdef";
static const char g_escapeChars[] = "0123456789\\b\\t\\n\\f\\r\\\"\\\\\\/";

static void SetError (JSOBJ obj, JSONObjectEncoder *enc, const char *message) {
  enc->errorMsg = message;
  enc->errorObj = obj;
}

/*
FIXME: Keep track of how big these get across several encoder calls and try to make an estimate
That way we won't run our head into the wall each call */
void Buffer_Realloc (JSONObjectEncoder *enc, size_t cbNeeded)
{
  size_t curSize = enc->end - enc->start;
  size_t newSize = curSize * 2;
  size_t offset = enc->offset - enc->start;

  while (newSize < curSize + cbNeeded)
  {
    newSize *= 2;
  }

  if (enc->heap)
  {
    enc->start = (char *) enc->realloc (enc->start, newSize);
    if (!enc->start)
    {
      SetError (NULL, enc, "Could not reserve memory block");
      return;
    }
  }
  else
  {
    char *oldStart = enc->start;
    enc->heap = 1;
    enc->start = (char *) enc->malloc (newSize);
    if (!enc->start)
    {
      SetError (NULL, enc, "Could not reserve memory block");
      return;
    }
    memcpy (enc->start, oldStart, offset);
  }
  enc->offset = enc->start + offset;
  enc->end = enc->start + newSize;
}

void Buffer_EscapeString (JSONObjectEncoder *enc, const char *io, const char *end) {
  char *of = (char *) enc->offset;

  for (;io < end; io++) {
    switch (*io) {
      case '\0': (*of++) = '\\'; (*of++) = '\"'; break;
      case '\"': (*of++) = '\\'; (*of++) = '\"'; break;
      case '\\': (*of++) = '\\'; (*of++) = '\\'; break;
      case '\b': (*of++) = '\\'; (*of++) = 'b'; break;
      case '\f': (*of++) = '\\'; (*of++) = 'f'; break;
      case '\n': (*of++) = '\\'; (*of++) = 'n'; break;
      case '\r': (*of++) = '\\'; (*of++) = 'r'; break;
      case '\t': (*of++) = '\\'; (*of++) = 't'; break;

      default:
        (*of++) = (*io);
        break;
    }
  }
  enc->offset = of;
  return;
}

#define Buffer_Reserve(__enc, __len) \
    if ( (size_t) ((__enc)->end - (__enc)->offset) < (size_t) (__len))  \
    {   \
      Buffer_Realloc((__enc), (__len));\
    }   \


#define Buffer_AppendCharUnchecked(__enc, __chr) \
                *((__enc)->offset++) = __chr; \

FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC strreverse(char* begin, char* end)
{
  char aux;
  while (end > begin)
  aux = *end, *end-- = *begin, *begin++ = aux;
}

void Buffer_AppendIndentUnchecked(JSONObjectEncoder *enc, JSINT32 value)
{
  int i;
  if (enc->indent > 0)
    while (value-- > 0)
      for (i = 0; i < enc->indent; i++)
        Buffer_AppendCharUnchecked(enc, ' ');
}

void Buffer_AppendIntUnchecked(JSONObjectEncoder *enc, JSINT32 value)
{
  char* wstr;
  JSUINT32 uvalue = (value < 0) ? -value : value;

  wstr = enc->offset;
  // Conversion. Number is reversed.

  do *wstr++ = (char)(48 + (uvalue % 10)); while(uvalue /= 10);
  if (value < 0) *wstr++ = '-';

  // Reverse string
  strreverse(enc->offset,wstr - 1);
  enc->offset += (wstr - (enc->offset));
}

void Buffer_AppendLongUnchecked(JSONObjectEncoder *enc, JSINT64 value)
{
  char* wstr;
  JSUINT64 uvalue = (value < 0) ? -value : value;

  wstr = enc->offset;
  // Conversion. Number is reversed.

  do *wstr++ = (char)(48 + (uvalue % 10ULL)); while(uvalue /= 10ULL);
  if (value < 0) *wstr++ = '-';

  // Reverse string
  strreverse(enc->offset,wstr - 1);
  enc->offset += (wstr - (enc->offset));
}

void Buffer_AppendUnsignedLongUnchecked(JSONObjectEncoder *enc, JSUINT64 value)
{
  char* wstr;
  JSUINT64 uvalue = value;

  wstr = enc->offset;
  // Conversion. Number is reversed.

  do *wstr++ = (char)(48 + (uvalue % 10ULL)); while(uvalue /= 10ULL);

  // Reverse string
  strreverse(enc->offset,wstr - 1);
  enc->offset += (wstr - (enc->offset));
}

int Buffer_AppendDoubleUnchecked(JSOBJ obj, JSONObjectEncoder *enc, double value)
{
  /* if input is larger than thres_max, revert to exponential */
  const double thres_max = (double) 1e16 - 1;
  int count;
  double diff = 0.0;
  char* str = enc->offset;
  char* wstr = str;
  unsigned long long whole;
  double tmp;
  unsigned long long frac;
  int neg;
  double pow10;

  if (value == HUGE_VAL || value == -HUGE_VAL)
  {
    SetError (obj, enc, "Invalid Inf value when encoding double");
    return FALSE;
  }

  if (!(value == value))
  {
    SetError (obj, enc, "Invalid Nan value when encoding double");
    return FALSE;
  }

  /* we'll work in positive values and deal with the
  negative sign issue later */
  neg = 0;
  if (value < 0)
  {
    neg = 1;
    value = -value;
  }

  pow10 = g_pow10[enc->doublePrecision];

  whole = (unsigned long long) value;
  tmp = (value - whole) * pow10;
  frac = (unsigned long long)(tmp);
  diff = tmp - frac;

  if (diff > 0.5)
  {
    ++frac;
    /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
    if (frac >= pow10)
    {
      frac = 0;
      ++whole;
    }
  }
  else
  if (diff == 0.5 && ((frac == 0) || (frac & 1)))
  {
    /* if halfway, round up if odd, OR
    if last digit is 0.  That last part is strange */
    ++frac;
  }

  /* for very large numbers switch back to native sprintf for exponentials.
  anyone want to write code to replace this? */
  /*
  normal printf behavior is to print EVERY whole number digit
  which can be 100s of characters overflowing your buffers == bad
  */
  if (value > thres_max)
  {
     enc->offset += snprintf(str, enc->end - enc->offset, "%.15e", neg ? -value : value);
     return TRUE;
   }

  if (enc->doublePrecision == 0)
  {
    diff = value - whole;

    if (diff > 0.5)
    {
      /* greater than 0.5, round up, e.g. 1.6 -> 2 */
      ++whole;
    }
    else
    if (diff == 0.5 && (whole & 1))
    {
      /* exactly 0.5 and ODD, then round up */
      /* 1.5 -> 2, but 2.5 -> 2 */
      ++whole;
    }

  //vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
  }
    else
    if (frac)
    {
      count = enc->doublePrecision;
      // now do fractional part, as an unsigned number
      // we know it is not 0 but we can have leading zeros, these
      // should be removed
      while (!(frac % 10))
      {
        --count;
        frac /= 10;
      }
      //^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

      // now do fractional part, as an unsigned number
      do
      {
        --count;
        *wstr++ = (char)(48 + (frac % 10));
      } while (frac /= 10);
      // add extra 0s
      while (count-- > 0)
      {
        *wstr++ = '0';
      }
      // add decimal
      *wstr++ = '.';
    }
    else
    {
      *wstr++ = '0';
      *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);

    if (neg)
    {
      *wstr++ = '-';
    }
    strreverse(str, wstr-1);
    enc->offset += (wstr - (enc->offset));

    return TRUE;
}

/*
FIXME:
Handle integration functions returning NULL here */

/*
FIXME:
Perhaps implement recursion detection */

void encode(JSOBJ obj, JSONObjectEncoder *enc, const char *name, size_t cbName) {
  const char *value;
  char *objName;
  int count;
  JSOBJ iterObj;
  size_t szlen;
  JSONTypeContext tc;

  if (enc->level > enc->recursionMax) {
    SetError (obj, enc, "Maximum recursion level reached");
    return;
  }

  /*
  This reservation must hold

  length of _name as encoded worst case +
  maxLength of double to string OR maxLength of JSLONG to string
  */

  Buffer_Reserve(enc, 256 + RESERVE_STRING(cbName));
  if (enc->errorMsg) {
    return;
  }

  if (name) {
    Buffer_AppendCharUnchecked(enc, '\"');
    Buffer_EscapeString(enc, name, name + cbName);
    Buffer_AppendCharUnchecked(enc, '\"');
    Buffer_AppendCharUnchecked (enc, ':');
  }

  tc.encoder_prv = enc->prv;
  enc->beginTypeContext(obj, &tc, enc);

  switch (tc.type) {
    case JT_INVALID:
      return;

    case JT_ARRAY: {
      count = 0;

      Buffer_AppendCharUnchecked (enc, '[');

      while (enc->iterNext(obj, &tc)) {
        if (count > 0)
          Buffer_AppendCharUnchecked (enc, ',');

        iterObj = enc->iterGetValue(obj, &tc);

        enc->level ++;
        encode(iterObj, enc, NULL, 0);
        count ++;
      }

      enc->iterEnd(obj, &tc);
      Buffer_AppendCharUnchecked (enc, ']');
      break;
    }

    case JT_OBJECT: {
      count = 0;
      Buffer_AppendCharUnchecked (enc, '{');

      while (enc->iterNext(obj, &tc)) {
        if (count > 0)
          Buffer_AppendCharUnchecked (enc, ',');

        iterObj = enc->iterGetValue(obj, &tc);
        objName = enc->iterGetName(obj, &tc, &szlen);

        enc->level ++;
        encode (iterObj, enc, objName, szlen);
        count ++;
      }

      enc->iterEnd(obj, &tc);
      Buffer_AppendCharUnchecked (enc, '}');
      break;
    }

    case JT_LONG: {
      Buffer_AppendLongUnchecked (enc, enc->getLongValue(obj, &tc));
      break;
    }

    case JT_ULONG: {
      Buffer_AppendUnsignedLongUnchecked (enc, enc->getUnsignedLongValue(obj, &tc));
      break;
    }

    case JT_INT: {
      Buffer_AppendIntUnchecked (enc, enc->getIntValue(obj, &tc));
      break;
    }

    case JT_TRUE: {
      Buffer_AppendCharUnchecked (enc, 't');
      Buffer_AppendCharUnchecked (enc, 'r');
      Buffer_AppendCharUnchecked (enc, 'u');
      Buffer_AppendCharUnchecked (enc, 'e');
      break;
    }

    case JT_FALSE: {
      Buffer_AppendCharUnchecked (enc, 'f');
      Buffer_AppendCharUnchecked (enc, 'a');
      Buffer_AppendCharUnchecked (enc, 'l');
      Buffer_AppendCharUnchecked (enc, 's');
      Buffer_AppendCharUnchecked (enc, 'e');
      break;
    }


    case JT_NULL: {
      Buffer_AppendCharUnchecked (enc, 'n');
      Buffer_AppendCharUnchecked (enc, 'u');
      Buffer_AppendCharUnchecked (enc, 'l');
      Buffer_AppendCharUnchecked (enc, 'l');
      break;
    }

    case JT_DOUBLE: {
      if (!Buffer_AppendDoubleUnchecked (obj, enc, enc->getDoubleValue(obj, &tc))) {
        enc->endTypeContext(obj, &tc);
        enc->level --;
        return;
      }
      break;
    }

    case JT_UTF8: {
        value = enc->getStringValue(obj, &tc, &szlen);
        if(!value) {
          SetError(obj, enc, "utf-8 encoding error");
          return;
        }

        Buffer_Reserve(enc, RESERVE_STRING(szlen));
        if (enc->errorMsg) {
          enc->endTypeContext(obj, &tc);
          return;
        }

        Buffer_AppendCharUnchecked (enc, '\"');
        Buffer_EscapeString(enc, value, value + szlen);
        Buffer_AppendCharUnchecked (enc, '\"');

        break;
    }

    case JT_RAW: {
        value = enc->getStringValue(obj, &tc, &szlen);
        if(!value) {
          SetError(obj, enc, "utf-8 encoding error");
          return;
        }

        Buffer_Reserve(enc, RESERVE_STRING(szlen));
        if (enc->errorMsg) {
          enc->endTypeContext(obj, &tc);
          return;
        }

        memcpy(enc->offset, value, szlen);
        enc->offset += szlen;

        break;
    }
  }

  enc->endTypeContext(obj, &tc);
  enc->level --;
}

char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *enc, char *_buffer, size_t _cbBuffer)
{
  enc->malloc = enc->malloc ? enc->malloc : malloc;
  enc->free =  enc->free ? enc->free : free;
  enc->realloc = enc->realloc ? enc->realloc : realloc;
  enc->errorMsg = NULL;
  enc->errorObj = NULL;
  enc->level = 0;

  if (enc->recursionMax < 1) {
    enc->recursionMax = JSON_MAX_RECURSION_DEPTH;
  }

  if (enc->doublePrecision < 0 || enc->doublePrecision > JSON_DOUBLE_MAX_DECIMALS) {
    enc->doublePrecision = JSON_DOUBLE_MAX_DECIMALS;
  }

  if (_buffer == NULL) {
    _cbBuffer = 32768;
    enc->start = (char *) enc->malloc (_cbBuffer);
    if (!enc->start) {
      SetError(obj, enc, "Could not reserve memory block");
      return NULL;
    }
    enc->heap = 1;
  } else {
    enc->start = _buffer;
    enc->heap = 0;
  }

  enc->end = enc->start + _cbBuffer;
  enc->offset = enc->start;

  encode (obj, enc, NULL, 0);

  Buffer_Reserve(enc, 1);

  if (enc->errorMsg) {
    return NULL;
  }

  Buffer_AppendCharUnchecked(enc, '\0');

  return enc->start;
}
