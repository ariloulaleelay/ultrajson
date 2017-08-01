# coding=UTF-8
from __future__ import print_function, unicode_literals
import six
from six.moves import range, zip

import datetime
import functools
import decimal
import json
import math
import time
import pytz
import helljson
import ujson
if six.PY2:
    import unittest2 as unittest
else:
    import unittest


try:
    from blist import blist
except ImportError:
    blist = None

json_unicode = json.dumps if six.PY3 else functools.partial(json.dumps, encoding="utf-8")
to_fix = False


class HellJSONTests(unittest.TestCase):

    def test_encodeDecimal(self):
        sut = decimal.Decimal("1337.1337")
        encoded = helljson.dumps(sut)
        decoded = ujson.decode(encoded)
        self.assertEqual(decoded, 1337.1337)

    def test_doubleLongIssue(self):
        sut = {'a': -4342969734183514}
        encoded = json.dumps(sut)
        decoded = json.loads(encoded)
        self.assertEqual(sut, decoded)
        encoded = helljson.dumps(sut)
        decoded = ujson.decode(encoded)
        self.assertEqual(sut, decoded)

    def test_doubleLongDecimalIssue(self):
        sut = {'a': -12345678901234.56789012}
        encoded = json.dumps(sut)
        decoded = json.loads(encoded)
        self.assertEqual(sut, decoded)
        encoded = helljson.dumps(sut)
        decoded = ujson.decode(encoded)
        self.assertEqual(sut, decoded)

    def test_encodeDecodeLongDecimal(self):
        sut = {'a': -528656961.4399388}
        encoded = helljson.dumps(sut)
        ujson.decode(encoded)

    def test_decimalDecodeTest(self):
        sut = {'a': 4.56}
        encoded = helljson.dumps(sut)
        decoded = ujson.decode(encoded)
        self.assertAlmostEqual(sut[u'a'], decoded[u'a'])

    def test_decimalDecodeTestPrecise(self):
        sut = {'a': 4.56}
        encoded = helljson.dumps(sut)
        decoded = ujson.decode(encoded, precise_float=True)
        self.assertEqual(sut, decoded)

    def test_encodeDictWithUnicodeKeys(self):
        input = {"key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1"}
        helljson.dumps(input)

        input = {"بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1"}
        helljson.dumps(input)

    def test_encodeDoubleConversion(self):
        input = math.pi
        output = helljson.dumps(input)
        self.assertEqual(round(input, 10), round(json.loads(output), 10))
        self.assertEqual(round(input, 10), round(ujson.decode(output), 10))

    def test_encodeWithDecimal(self):
        input = 1.0
        output = helljson.dumps(input)
        self.assertEqual(output, "1.0")

    def test_encodeDoubleNegConversion(self):
        input = -math.pi
        output = helljson.dumps(input)

        self.assertEqual(round(input, 10), round(json.loads(output), 10))
        self.assertEqual(round(input, 10), round(ujson.decode(output), 10))

    def test_encodeArrayOfNestedArrays(self):
        input = [[[[]]]] * 20
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input).replace(', ', ','))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeArrayOfDoubles(self):
        input = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input).replace(', ', ','))
        self.assertEqual(input, ujson.decode(output))

    def test_doublePrecisionTest(self):
        input = 30.012345678901234
        output = helljson.dumps(input)
        self.assertEqual(round(input, 12), json.loads(output))
        self.assertEqual(round(input, 12), ujson.decode(output))

    def test_invalidDoublePrecision(self):
        input = 30.12345678901234567890
        output = helljson.dumps(input)
        # should snap to the max, which is 12
        self.assertEqual(round(input, 12), json.loads(output))
        self.assertEqual(round(input, 12), ujson.decode(output))

        output = helljson.dumps(input)
        # also should snap to the max, which is 13
        self.assertEqual(round(input, 12), json.loads(output))
        self.assertEqual(round(input, 12), ujson.decode(output))

    def test_encodeStringConversion2(self):
        input = "A string \\ / \b \f \n \r \t"
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, '"A string \\\\ / \\b \\f \\n \\r \\t"')
        self.assertEqual(input, ujson.decode(output))

    def test_decodeUnicodeConversion(self):
        pass

    def test_encodeUnicodeConversion1(self):
        input = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)
        self.assertEqual(dec, input)

    def test_encodeControlEscaping(self):
        input = "\x19"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)
        self.assertEqual(input, dec)

    def test_encodeUnicodeConversion2(self):
        input = "\xe6\x97\xa5\xd1\x88"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicodeSurrogatePair(self):
        input = u"\xf0\x90\x8d\x86"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)

        self.assertEqual(dec, input)
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicode4BytesUTF8(self):
        input = "\xf0\x91\x80\xb0TRAILINGNORMAL"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)

        self.assertEqual(dec, input)
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicode4BytesUTF8Highest(self):
        input = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)

        self.assertEqual(input, dec)

    # Characters outside of Basic Multilingual Plane(larger than
    # 16 bits) are represented as \UXXXXXXXX in python but should be encoded
    # as \uXXXX\uXXXX in json.
    def testEncodeUnicodeBMP(self):
        s = '\U0001f42e\U0001f42e\U0001F42D\U0001F42D'  # 🐮🐮🐭🐭
        encoded = helljson.dumps(s)
        encoded_json = json.dumps(s)

        if len(s) == 4:
            self.assertEqual(len(encoded), len(s) * 4 + 2)

        decoded = ujson.loads(encoded)
        self.assertEqual(s, decoded)

        # helljson outputs an UTF-8 encoded str object
        if six.PY3:
            encoded = helljson.dumps(s)
        else:
            encoded = helljson.dumps(s).decode("utf-8")

        # json outputs an unicode object
        encoded_json = json.dumps(s, ensure_ascii=False)
        self.assertEqual(len(encoded), len(s) + 2)  # original length + quotes
        self.assertEqual(encoded, encoded_json)
        decoded = ujson.loads(encoded)
        self.assertEqual(s, decoded)

    def testEncodeSymbols(self):
        s = '\u273f\u2661\u273f'  # ✿♡✿
        encoded = helljson.dumps(s)
        encoded_json = json.dumps(s)
        self.assertEqual(len(encoded), len(s) * 3 + 2)  # 6 characters + quotes
        decoded = ujson.loads(encoded)
        self.assertEqual(s, decoded)

        # helljson outputs an UTF-8 encoded str object
        if six.PY3:
            encoded = helljson.dumps(s)
        else:
            encoded = helljson.dumps(s).decode("utf-8")

        # json outputs an unicode object
        encoded_json = json.dumps(s, ensure_ascii=False)
        self.assertEqual(len(encoded), len(s) + 2)  # original length + quotes
        self.assertEqual(encoded, encoded_json)
        decoded = ujson.loads(encoded)
        self.assertEqual(s, decoded)

    def test_encodeArrayInArray(self):
        input = [[[[]]]]
        output = helljson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeIntConversion(self):
        input = 31337
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeIntNegConversion(self):
        input = -31337
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeLongNegConversion(self):
        input = -9223372036854775808
        output = helljson.dumps(input)

        json.loads(output)
        ujson.decode(output)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeListConversion(self):
        input = [1, 2, 3, 4]
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeDictConversion(self):
        input = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, ujson.decode(output))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeNoneConversion(self):
        input = None
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeTrueConversion(self):
        input = True
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeFalseConversion(self):
        input = False
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeDatetimeConversion(self):
        ts = time.time()
        input = datetime.datetime.fromtimestamp(ts)
        output = helljson.dumps(input)
        expected = input.isoformat()
        self.assertEqual(expected, json.loads(output))
        self.assertEqual(expected, ujson.decode(output))

    def test_encodeDateConversion(self):
        ts = time.time()
        input = datetime.date.fromtimestamp(ts)

        output = helljson.dumps(input)

        expected = datetime.date.fromtimestamp(ts).isoformat()
        self.assertEqual(expected, json.loads(output))
        self.assertEqual(expected, ujson.decode(output))

    @unittest.skipIf(not to_fix, "Disable until fixed")
    def test_encodeWithTimezone(self):
        start_timestamp = 1383647400
        start_date = '2013-11-05 10:30:00'
        a = datetime.datetime.utcfromtimestamp(start_timestamp)

        a = a.replace(tzinfo=pytz.utc)
        b = a.astimezone(pytz.timezone('America/New_York'))
        c = b.astimezone(pytz.utc)

        d = {'a': a, 'b': b, 'c': c}
        json_string = helljson.dumps(d)
        json_dict = ujson.decode(json_string)

        self.assertEqual(json_dict.get('a'), start_date)
        self.assertEqual(json_dict.get('b'), start_date)
        self.assertEqual(json_dict.get('c'), start_date)

    def test_encodeToUTF8(self):
        input = b"\xe6\x97\xa5\xd1\x88"
        if six.PY3:
            input = input.decode('utf-8')
        enc = helljson.dumps(input)
        dec = ujson.decode(enc)
        self.assertEqual(enc, json.dumps(input, ensure_ascii=False))
        self.assertEqual(dec, json.loads(enc))

    def test_decodeFromUnicode(self):
        input = "{\"obj\": 31337}"
        dec1 = ujson.decode(input)
        dec2 = ujson.decode(str(input))
        self.assertEqual(dec1, dec2)

    def test_encodeRecursionMax(self):
        # 8 is the max recursion depth
        input = {'d': {}}
        input['d']['d'] = input

        self.assertRaises(OverflowError, helljson.dumps, input)

    def test_encodeDoubleNan(self):
        input = float('nan')
        self.assertRaises(OverflowError, helljson.dumps, input)

    def test_encodeDoubleInf(self):
        input = float('inf')
        self.assertRaises(OverflowError, helljson.dumps, input)

    def test_encodeDoubleNegInf(self):
        input = -float('inf')
        self.assertRaises(OverflowError, helljson.dumps, input)

    def test_decodeJibberish(self):
        input = "fdsa sda v9sa fdsa"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenArrayStart(self):
        input = "["
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenObjectStart(self):
        input = "{"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenArrayEnd(self):
        input = "]"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayDepthTooBig(self):
        input = '[' * (1024 * 1024)
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenObjectEnd(self):
        input = "}"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeObjectDepthTooBig(self):
        input = '{' * (1024 * 1024)
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeStringUnterminated(self):
        input = "\"TESTING"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeStringUntermEscapeSequence(self):
        input = "\"TESTING\\\""
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeStringBadEscape(self):
        input = "\"TESTING\\\""
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeTrueBroken(self):
        input = "tru"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeFalseBroken(self):
        input = "fa"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeNullBroken(self):
        input = "n"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenDictKeyTypeLeakTest(self):
        input = '{{1337:""}}'
        for x in range(1000):
            self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenDictLeakTest(self):
        input = '{{"key":"}'
        for x in range(1000):
            self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeBrokenListLeakTest(self):
        input = '[[[true'
        for x in range(1000):
            self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeDictWithNoKey(self):
        input = "{{{{31337}}}}"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeDictWithNoColonOrValue(self):
        input = "{{{{\"key\"}}}}"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeDictWithNoValue(self):
        input = "{{{{\"key\":}}}}"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeNumericIntPos(self):
        input = "31337"
        self.assertEqual(31337, ujson.decode(input))

    def test_decodeNumericIntNeg(self):
        input = "-31337"
        self.assertEqual(-31337, ujson.decode(input))

    def test_encodeNullCharacter(self):
        input = "31337 \x00 1337"
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

        input = "\x00"
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

        self.assertEqual('"  \\u0000\\r\\n "', helljson.dumps("  \u0000\r\n "))

    def test_decodeNullCharacter(self):
        input = "\"31337 \\u0000 31337\""
        self.assertEqual(ujson.decode(input), json.loads(input))

    def test_encodeListLongConversion(self):
        input = [9223372036854775807, 9223372036854775807, 9223372036854775807,
                 9223372036854775807, 9223372036854775807, 9223372036854775807]
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeListLongUnsignedConversion(self):
        input = [18446744073709551615, 18446744073709551615, 18446744073709551615]
        output = helljson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeLongConversion(self):
        input = 9223372036854775807
        output = helljson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_encodeLongUnsignedConversion(self):
        input = 18446744073709551615
        output = helljson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, ujson.decode(output))

    def test_numericIntExp(self):
        input = "1337E40"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_numericIntFrcExp(self):
        input = "1.337E40"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpEPLUS(self):
        input = "1337E+9"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpePLUS(self):
        input = "1.337e+40"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpE(self):
        input = "1337E40"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpe(self):
        input = "1337e40"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpEMinus(self):
        input = "1.337E-4"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpeMinus(self):
        input = "1.337e-4"
        output = ujson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_dumpToFile(self):
        f = six.StringIO()
        helljson.dump([1, 2, 3], f)
        self.assertEqual("[1,2,3]", f.getvalue())

    def test_dumpToFileLikeObject(self):
        class filelike:
            def __init__(self):
                self.bytes = ''

            def write(self, bytes):
                self.bytes += bytes

        f = filelike()
        helljson.dump([1, 2, 3], f)
        self.assertEqual("[1,2,3]", f.bytes)

    def test_dumpFileArgsError(self):
        self.assertRaises(TypeError, helljson.dump, [], '')

    def test_loadFile(self):
        f = six.StringIO("[1,2,3,4]")
        self.assertEqual([1, 2, 3, 4], ujson.load(f))

    def test_loadFileLikeObject(self):
        class filelike:
            def read(self):
                try:
                    self.end
                except AttributeError:
                    self.end = True
                    return "[1,2,3,4]"

        f = filelike()
        self.assertEqual([1, 2, 3, 4], ujson.load(f))

    def test_loadFileArgsError(self):
        self.assertRaises(TypeError, ujson.load, "[]")

    def test_version(self):
        if six.PY2:
            self.assertRegexpMatches(helljson.__version__, r'^\d+\.\d+(\.\d+)?$', "helljson.__version__ must be a string like '1.4.0'")
        else:
            self.assertRegex(helljson.__version__, r'^\d+\.\d+(\.\d+)?$', "helljson.__version__ must be a string like '1.4.0'")

    def test_encodeNumericOverflow(self):
        self.assertRaises(OverflowError, helljson.dumps, 12839128391289382193812939)

    def test_decodeNumberWith32bitSignBit(self):
        # Test that numbers that fit within 32 bits but would have the
        # sign bit set (2**31 <= x < 2**32) are decoded properly.
        docs = (
            '{"id": 3590016419}',
            '{"id": %s}' % 2**31,
            '{"id": %s}' % 2**32,
            '{"id": %s}' % ((2**32)-1),
        )
        results = (3590016419, 2**31, 2**32, 2**32-1)
        for doc, result in zip(docs, results):
            self.assertEqual(ujson.decode(doc)['id'], result)

    def test_encodeBigEscape(self):
        for x in range(10):
            if six.PY3:
                base = '\u00e5'.encode('utf-8')
            else:
                base = "\xc3\xa5"
            input = base * 1024 * 1024 * 2
            helljson.dumps(input)

    def test_decodeBigEscape(self):
        for x in range(10):
            if six.PY3:
                base = '\u00e5'.encode('utf-8')
                quote = "\"".encode()
            else:
                base = "\xc3\xa5"
                quote = "\""
            input = quote + (base * 1024 * 1024 * 2) + quote
            ujson.decode(input)

    def test_decodeArrayTrailingCommaFail(self):
        input = "[31337,]"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayLeadingCommaFail(self):
        input = "[,31337]"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayOnlyCommaFail(self):
        input = "[,]"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayUnmatchedBracketFail(self):
        input = "[]]"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayEmpty(self):
        input = "[]"
        obj = ujson.decode(input)
        self.assertEqual([], obj)

    def test_decodeArrayDict(self):
        input = "{}"
        obj = ujson.decode(input)
        self.assertEqual({}, obj)

    def test_decodeArrayOneItem(self):
        input = "[31337]"
        ujson.decode(input)

    def test_decodeLongUnsignedValue(self):
        input = "18446744073709551615"
        ujson.decode(input)

    def test_decodeBigValue(self):
        input = "9223372036854775807"
        ujson.decode(input)

    def test_decodeSmallValue(self):
        input = "-9223372036854775808"
        ujson.decode(input)

    def test_decodeTooBigValue(self):
        input = "18446744073709551616"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeTooSmallValue(self):
        input = "-90223372036854775809"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeVeryTooBigValue(self):
        input = "18446744073709551616"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeVeryTooSmallValue(self):
        input = "-90223372036854775809"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeWithTrailingWhitespaces(self):
        input = "{}\n\t "
        ujson.decode(input)

    def test_decodeWithTrailingNonWhitespaces(self):
        input = "{}\n\t a"
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeArrayWithBigInt(self):
        input = '[18446744073709551616]'
        self.assertRaises(ValueError, ujson.decode, input)

    def test_decodeFloatingPointAdditionalTests(self):
        self.assertEqual(-1.1234567893, ujson.loads("-1.1234567893"))
        self.assertEqual(-1.234567893, ujson.loads("-1.234567893"))
        self.assertEqual(-1.34567893, ujson.loads("-1.34567893"))
        self.assertEqual(-1.4567893, ujson.loads("-1.4567893"))
        self.assertEqual(-1.567893, ujson.loads("-1.567893"))
        self.assertEqual(-1.67893, ujson.loads("-1.67893"))
        self.assertEqual(-1.7893, ujson.loads("-1.7893", precise_float=True))
        self.assertEqual(-1.893, ujson.loads("-1.893"))
        self.assertEqual(-1.3, ujson.loads("-1.3"))

        self.assertEqual(1.1234567893, ujson.loads("1.1234567893"))
        self.assertEqual(1.234567893, ujson.loads("1.234567893"))
        self.assertEqual(1.34567893, ujson.loads("1.34567893"))
        self.assertEqual(1.4567893, ujson.loads("1.4567893"))
        self.assertEqual(1.567893, ujson.loads("1.567893"))
        self.assertEqual(1.67893, ujson.loads("1.67893"))
        self.assertEqual(1.7893, ujson.loads("1.7893", precise_float=True))
        self.assertEqual(1.893, ujson.loads("1.893"))
        self.assertEqual(1.3, ujson.loads("1.3"))

    def test_encodeBigSet(self):
        s = set()
        for x in range(0, 100000):
            s.add(x)
        helljson.dumps(s)

    @unittest.skipIf(blist is None, "This test tests functionality with the blist library")
    def test_encodeBlist(self):
        b = blist(list(range(10)))
        c = helljson.dumps(b)
        d = ujson.loads(c)

        self.assertEqual(10, len(d))

        for x in range(10):
            self.assertEqual(x, d[x])

    def test_encodeEmptySet(self):
        s = set()
        self.assertEqual("[]", helljson.dumps(s))

    def test_encodeSet(self):
        s = set([1, 2, 3, 4, 5, 6, 7, 8, 9])
        enc = helljson.dumps(s)
        dec = ujson.decode(enc)

        for v in dec:
            self.assertTrue(v in s)

    def test_ReadBadObjectSyntax(self):
        input = '{"age", 44}'
        self.assertRaises(ValueError, ujson.decode, input)

    def test_ReadTrue(self):
        self.assertEqual(True, ujson.loads("true"))

    def test_ReadFalse(self):
        self.assertEqual(False, ujson.loads("false"))

    def test_ReadNull(self):
        self.assertEqual(None, ujson.loads("null"))

    def test_WriteTrue(self):
        self.assertEqual("true", helljson.dumps(True))

    def test_WriteFalse(self):
        self.assertEqual("false", helljson.dumps(False))

    def test_WriteNull(self):
        self.assertEqual("null", helljson.dumps(None))

    def test_ReadArrayOfSymbols(self):
        self.assertEqual([True, False, None], ujson.loads(" [ true, false,null] "))

    def test_WriteArrayOfSymbolsFromList(self):
        self.assertEqual("[true,false,null]", helljson.dumps([True, False, None]))

    def test_WriteArrayOfSymbolsFromTuple(self):
        self.assertEqual("[true,false,null]", helljson.dumps((True, False, None)))

    @unittest.skipIf(not six.PY3, "Only raises on Python 3")
    def test_encodingInvalidUnicodeCharacter(self):
        s = "\udc7f"
        self.assertRaises(UnicodeEncodeError, helljson.dumps, s)

"""
def test_decodeNumericIntFrcOverflow(self):
input = "X.Y"
raise NotImplementedError("Implement this test!")


def test_decodeStringUnicodeEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeBrokenEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeInvalidEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUTF8(self):
input = "someutfcharacters"
raise NotImplementedError("Implement this test!")

"""

if __name__ == "__main__":
    unittest.main()

"""
# Use this to look for memory leaks
if __name__ == '__main__':
    from guppy import hpy
    hp = hpy()
    hp.setrelheap()
    while True:
        try:
            unittest.main()
        except SystemExit:
            pass
        heap = hp.heapu()
        print(heap)
"""
