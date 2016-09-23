# coding=UTF-8
from __future__ import print_function, unicode_literals
import six
from six.moves import range, zip

import calendar
import datetime
import functools
import decimal
import json
import math
import time
import pytz
import hjson
if six.PY2:
    import unittest2 as unittest
else:
    import unittest


try:
    from blist import blist
except ImportError:
    blist = None

json_unicode = json.dumps if six.PY3 else functools.partial(json.dumps, encoding="utf-8")


class HellJSONTests(unittest.TestCase):
    def test_encodeDecimal(self):
        sut = decimal.Decimal("1337.1337")
        encoded = hjson.dumps(sut, double_precision=100)
        decoded = hjson.decode(encoded)
        self.assertEqual(decoded, 1337.1337)

    def test_encodeStringConversion(self):
        input = "A string \\ / \b \f \n \r \t </script> &"
        not_html_encoded = '"A string \\\\ \\/ \\b \\f \\n \\r \\t <\\/script> &"'
        html_encoded = '"A string \\\\ \\/ \\b \\f \\n \\r \\t \\u003c\\/script\\u003e \\u0026"'
        not_slashes_escaped = '"A string \\\\ / \\b \\f \\n \\r \\t </script> &"'

        def helper(expected_output, **encode_kwargs):
            output = hjson.dumps(input, **encode_kwargs)
            self.assertEqual(output, expected_output)
            if encode_kwargs.get('escape_forward_slashes', True):
                self.assertEqual(input, json.loads(output))
                self.assertEqual(input, hjson.decode(output))

        # Default behavior assumes encode_html_chars=False.
        helper(not_html_encoded, ensure_ascii=True)
        helper(not_html_encoded, ensure_ascii=False)

        # Make sure explicit encode_html_chars=False works.
        helper(not_html_encoded, ensure_ascii=True, encode_html_chars=False)
        helper(not_html_encoded, ensure_ascii=False, encode_html_chars=False)

        # Make sure explicit encode_html_chars=True does the encoding.
        helper(html_encoded, ensure_ascii=True, encode_html_chars=True)
        helper(html_encoded, ensure_ascii=False, encode_html_chars=True)

        # Do escape forward slashes if disabled.
        helper(not_slashes_escaped, escape_forward_slashes=False)

    def testWriteEscapedString(self):
        self.assertEqual('"\\u003cimg src=\'\\u0026amp;\'\\/\\u003e"', hjson.dumps("<img src='&amp;'/>", encode_html_chars=True))

    def test_doubleLongIssue(self):
        sut = {'a': -4342969734183514}
        encoded = json.dumps(sut)
        decoded = json.loads(encoded)
        self.assertEqual(sut, decoded)
        encoded = hjson.dumps(sut, double_precision=100)
        decoded = hjson.decode(encoded)
        self.assertEqual(sut, decoded)

    def test_doubleLongDecimalIssue(self):
        sut = {'a': -12345678901234.56789012}
        encoded = json.dumps(sut)
        decoded = json.loads(encoded)
        self.assertEqual(sut, decoded)
        encoded = hjson.dumps(sut, double_precision=100)
        decoded = hjson.decode(encoded)
        self.assertEqual(sut, decoded)

    def test_encodeDecodeLongDecimal(self):
        sut = {'a': -528656961.4399388}
        encoded = hjson.dumps(sut, double_precision=15)
        hjson.decode(encoded)

    def test_decimalDecodeTest(self):
        sut = {'a': 4.56}
        encoded = hjson.dumps(sut)
        decoded = hjson.decode(encoded)
        self.assertAlmostEqual(sut[u'a'], decoded[u'a'])

    def test_decimalDecodeTestPrecise(self):
        sut = {'a': 4.56}
        encoded = hjson.dumps(sut)
        decoded = hjson.decode(encoded, precise_float=True)
        self.assertEqual(sut, decoded)

    def test_encodeDictWithUnicodeKeys(self):
        input = {"key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1", "key1": "value1"}
        hjson.dumps(input)

        input = {"بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1", "بن": "value1"}
        hjson.dumps(input)

    def test_encodeDoubleConversion(self):
        input = math.pi
        output = hjson.dumps(input)
        self.assertEqual(round(input, 5), round(json.loads(output), 5))
        self.assertEqual(round(input, 5), round(hjson.decode(output), 5))

    def test_encodeWithDecimal(self):
        input = 1.0
        output = hjson.dumps(input)
        self.assertEqual(output, "1.0")

    def test_encodeDoubleNegConversion(self):
        input = -math.pi
        output = hjson.dumps(input)

        self.assertEqual(round(input, 5), round(json.loads(output), 5))
        self.assertEqual(round(input, 5), round(hjson.decode(output), 5))

    def test_encodeArrayOfNestedArrays(self):
        input = [[[[]]]] * 20
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        #self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeArrayOfDoubles(self):
        input = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        #self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_doublePrecisionTest(self):
        input = 30.012345678901234
        output = hjson.dumps(input, double_precision=15)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, hjson.decode(output))

        output = hjson.dumps(input, double_precision=9)
        self.assertEqual(round(input, 9), json.loads(output))
        self.assertEqual(round(input, 9), hjson.decode(output))

        output = hjson.dumps(input, double_precision=3)
        self.assertEqual(round(input, 3), json.loads(output))
        self.assertEqual(round(input, 3), hjson.decode(output))

    def test_invalidDoublePrecision(self):
        input = 30.12345678901234567890
        output = hjson.dumps(input, double_precision=20)
        # should snap to the max, which is 15
        self.assertEqual(round(input, 15), json.loads(output))
        self.assertEqual(round(input, 15), hjson.decode(output))

        output = hjson.dumps(input, double_precision=-1)
        # also should snap to the max, which is 15
        self.assertEqual(round(input, 15), json.loads(output))
        self.assertEqual(round(input, 15), hjson.decode(output))

        # will throw typeError
        self.assertRaises(TypeError, hjson.dumps, input, double_precision='9')
        # will throw typeError
        self.assertRaises(TypeError, hjson.dumps, input, double_precision=None)

    def test_encodeStringConversion2(self):
        input = "A string \\ / \b \f \n \r \t"
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, '"A string \\\\ \\/ \\b \\f \\n \\r \\t"')
        self.assertEqual(input, hjson.decode(output))

    def test_decodeUnicodeConversion(self):
        pass

    def test_encodeUnicodeConversion1(self):
        input = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)
        self.assertEqual(enc, json_unicode(input))
        self.assertEqual(dec, json.loads(enc))

    def test_encodeControlEscaping(self):
        input = "\x19"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)
        self.assertEqual(input, dec)
        self.assertEqual(enc, json_unicode(input))

    def test_encodeUnicodeConversion2(self):
        input = "\xe6\x97\xa5\xd1\x88"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)
        self.assertEqual(enc, json_unicode(input))
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicodeSurrogatePair(self):
        input = "\xf0\x90\x8d\x86"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)

        self.assertEqual(enc, json_unicode(input))
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicode4BytesUTF8(self):
        input = "\xf0\x91\x80\xb0TRAILINGNORMAL"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)

        self.assertEqual(enc, json_unicode(input))
        self.assertEqual(dec, json.loads(enc))

    def test_encodeUnicode4BytesUTF8Highest(self):
        input = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
        enc = hjson.dumps(input)
        dec = hjson.decode(enc)

        self.assertEqual(enc, json_unicode(input))
        self.assertEqual(dec, json.loads(enc))

    # Characters outside of Basic Multilingual Plane(larger than
    # 16 bits) are represented as \UXXXXXXXX in python but should be encoded
    # as \uXXXX\uXXXX in json.
    def testEncodeUnicodeBMP(self):
        s = '\U0001f42e\U0001f42e\U0001F42D\U0001F42D'  # 🐮🐮🐭🐭
        encoded = hjson.dumps(s)
        encoded_json = json.dumps(s)

        if len(s) == 4:
            self.assertEqual(len(encoded), len(s) * 4 + 2)

        decoded = hjson.loads(encoded)
        self.assertEqual(s, decoded)

        # hjson outputs an UTF-8 encoded str object
        if six.PY3:
            encoded = hjson.dumps(s, ensure_ascii=False)
        else:
            encoded = hjson.dumps(s, ensure_ascii=False).decode("utf-8")

        # json outputs an unicode object
        encoded_json = json.dumps(s, ensure_ascii=False)
        self.assertEqual(len(encoded), len(s) + 2)  # original length + quotes
        self.assertEqual(encoded, encoded_json)
        decoded = hjson.loads(encoded)
        self.assertEqual(s, decoded)

    def testEncodeSymbols(self):
        s = '\u273f\u2661\u273f'  # ✿♡✿
        encoded = hjson.dumps(s)
        encoded_json = json.dumps(s)
        self.assertEqual(len(encoded), len(s) * 3 + 2)  # 6 characters + quotes
        decoded = hjson.loads(encoded)
        self.assertEqual(s, decoded)

        # hjson outputs an UTF-8 encoded str object
        if six.PY3:
            encoded = hjson.dumps(s, ensure_ascii=False)
        else:
            encoded = hjson.dumps(s, ensure_ascii=False).decode("utf-8")

        # json outputs an unicode object
        encoded_json = json.dumps(s, ensure_ascii=False)
        self.assertEqual(len(encoded), len(s) + 2)  # original length + quotes
        self.assertEqual(encoded, encoded_json)
        decoded = hjson.loads(encoded)
        self.assertEqual(s, decoded)

    def test_encodeArrayInArray(self):
        input = [[[[]]]]
        output = hjson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeIntConversion(self):
        input = 31337
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeIntNegConversion(self):
        input = -31337
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeLongNegConversion(self):
        input = -9223372036854775808
        output = hjson.dumps(input)

        json.loads(output)
        hjson.decode(output)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeListConversion(self):
        input = [1, 2, 3, 4]
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeDictConversion(self):
        input = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, hjson.decode(output))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeNoneConversion(self):
        input = None
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeTrueConversion(self):
        input = True
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeFalseConversion(self):
        input = False
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeDatetimeConversion(self):
        ts = time.time()
        input = datetime.datetime.fromtimestamp(ts)
        output = hjson.dumps(input)
        expected = calendar.timegm(input.utctimetuple())
        self.assertEqual(int(expected), json.loads(output))
        self.assertEqual(int(expected), hjson.decode(output))

    def test_encodeDateConversion(self):
        ts = time.time()
        input = datetime.date.fromtimestamp(ts)

        output = hjson.dumps(input)
        tup = (input.year, input.month, input.day, 0, 0, 0)

        expected = calendar.timegm(tup)
        self.assertEqual(int(expected), json.loads(output))
        self.assertEqual(int(expected), hjson.decode(output))

    def test_encodeWithTimezone(self):
        start_timestamp = 1383647400
        a = datetime.datetime.utcfromtimestamp(start_timestamp)

        a = a.replace(tzinfo=pytz.utc)
        b = a.astimezone(pytz.timezone('America/New_York'))
        c = b.astimezone(pytz.utc)

        d = {'a':a, 'b':b, 'c':c}
        json_string = hjson.dumps(d)
        json_dict = hjson.decode(json_string)

        self.assertEqual(json_dict.get('a'),start_timestamp)
        self.assertEqual(json_dict.get('b'),start_timestamp)
        self.assertEqual(json_dict.get('c'),start_timestamp)

    def test_encodeToUTF8(self):
        input = b"\xe6\x97\xa5\xd1\x88"
        if six.PY3:
            input = input.decode('utf-8')
        enc = hjson.dumps(input, ensure_ascii=False)
        dec = hjson.decode(enc)
        self.assertEqual(enc, json.dumps(input, ensure_ascii=False))
        self.assertEqual(dec, json.loads(enc))

    def test_decodeFromUnicode(self):
        input = "{\"obj\": 31337}"
        dec1 = hjson.decode(input)
        dec2 = hjson.decode(str(input))
        self.assertEqual(dec1, dec2)

    def test_encodeRecursionMax(self):
        # 8 is the max recursion depth
        class O2:
            member = 0
            pass

        class O1:
            member = 0
            pass

        input = O1()
        input.member = O2()
        input.member.member = input
        self.assertRaises(OverflowError, hjson.dumps, input)

    def test_encodeDoubleNan(self):
        input = float('nan')
        self.assertRaises(OverflowError, hjson.dumps, input)

    def test_encodeDoubleInf(self):
        input = float('inf')
        self.assertRaises(OverflowError, hjson.dumps, input)

    def test_encodeDoubleNegInf(self):
        input = -float('inf')
        self.assertRaises(OverflowError, hjson.dumps, input)

    def test_decodeJibberish(self):
        input = "fdsa sda v9sa fdsa"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenArrayStart(self):
        input = "["
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenObjectStart(self):
        input = "{"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenArrayEnd(self):
        input = "]"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayDepthTooBig(self):
        input = '[' * (1024 * 1024)
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenObjectEnd(self):
        input = "}"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeObjectDepthTooBig(self):
        input = '{' * (1024 * 1024)
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeStringUnterminated(self):
        input = "\"TESTING"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeStringUntermEscapeSequence(self):
        input = "\"TESTING\\\""
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeStringBadEscape(self):
        input = "\"TESTING\\\""
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeTrueBroken(self):
        input = "tru"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeFalseBroken(self):
        input = "fa"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeNullBroken(self):
        input = "n"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenDictKeyTypeLeakTest(self):
        input = '{{1337:""}}'
        for x in range(1000):
            self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenDictLeakTest(self):
        input = '{{"key":"}'
        for x in range(1000):
            self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeBrokenListLeakTest(self):
        input = '[[[true'
        for x in range(1000):
            self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeDictWithNoKey(self):
        input = "{{{{31337}}}}"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeDictWithNoColonOrValue(self):
        input = "{{{{\"key\"}}}}"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeDictWithNoValue(self):
        input = "{{{{\"key\":}}}}"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeNumericIntPos(self):
        input = "31337"
        self.assertEqual(31337, hjson.decode(input))

    def test_decodeNumericIntNeg(self):
        input = "-31337"
        self.assertEqual(-31337, hjson.decode(input))

    def test_encodeNullCharacter(self):
        input = "31337 \x00 1337"
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

        input = "\x00"
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

        self.assertEqual('"  \\u0000\\r\\n "', hjson.dumps("  \u0000\r\n "))

    def test_decodeNullCharacter(self):
        input = "\"31337 \\u0000 31337\""
        self.assertEqual(hjson.decode(input), json.loads(input))

    def test_encodeListLongConversion(self):
        input = [9223372036854775807, 9223372036854775807, 9223372036854775807,
                 9223372036854775807, 9223372036854775807, 9223372036854775807]
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeListLongUnsignedConversion(self):
        input = [18446744073709551615, 18446744073709551615, 18446744073709551615]
        output = hjson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeLongConversion(self):
        input = 9223372036854775807
        output = hjson.dumps(input)
        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_encodeLongUnsignedConversion(self):
        input = 18446744073709551615
        output = hjson.dumps(input)

        self.assertEqual(input, json.loads(output))
        self.assertEqual(output, json.dumps(input))
        self.assertEqual(input, hjson.decode(output))

    def test_numericIntExp(self):
        input = "1337E40"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_numericIntFrcExp(self):
        input = "1.337E40"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpEPLUS(self):
        input = "1337E+9"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpePLUS(self):
        input = "1.337e+40"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpE(self):
        input = "1337E40"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpe(self):
        input = "1337e40"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpEMinus(self):
        input = "1.337E-4"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_decodeNumericIntExpeMinus(self):
        input = "1.337e-4"
        output = hjson.decode(input)
        self.assertEqual(output, json.loads(input))

    def test_dumpToFile(self):
        f = six.StringIO()
        hjson.dump([1, 2, 3], f)
        self.assertEqual("[1,2,3]", f.getvalue())

    def test_dumpToFileLikeObject(self):
        class filelike:
            def __init__(self):
                self.bytes = ''

            def write(self, bytes):
                self.bytes += bytes

        f = filelike()
        hjson.dump([1, 2, 3], f)
        self.assertEqual("[1,2,3]", f.bytes)

    def test_dumpFileArgsError(self):
        self.assertRaises(TypeError, hjson.dump, [], '')

    def test_loadFile(self):
        f = six.StringIO("[1,2,3,4]")
        self.assertEqual([1, 2, 3, 4], hjson.load(f))

    def test_loadFileLikeObject(self):
        class filelike:
            def read(self):
                try:
                    self.end
                except AttributeError:
                    self.end = True
                    return "[1,2,3,4]"

        f = filelike()
        self.assertEqual([1, 2, 3, 4], hjson.load(f))

    def test_loadFileArgsError(self):
        self.assertRaises(TypeError, hjson.load, "[]")

    def test_version(self):
        if six.PY2:
            self.assertRegexpMatches(hjson.__version__, r'^\d+\.\d+(\.\d+)?$', "hjson.__version__ must be a string like '1.4.0'")
        else:
            self.assertRegex(hjson.__version__, r'^\d+\.\d+(\.\d+)?$', "hjson.__version__ must be a string like '1.4.0'")

    def test_encodeNumericOverflow(self):
        self.assertRaises(OverflowError, hjson.dumps, 12839128391289382193812939)

    def test_encodeNumericOverflowNested(self):
        for n in range(0, 100):
            class Nested:
                x = 12839128391289382193812939

            nested = Nested()
            self.assertRaises(OverflowError, hjson.dumps, nested)

    def test_decodeNumberWith32bitSignBit(self):
        #Test that numbers that fit within 32 bits but would have the
        # sign bit set (2**31 <= x < 2**32) are decoded properly.
        docs = (
            '{"id": 3590016419}',
            '{"id": %s}' % 2**31,
            '{"id": %s}' % 2**32,
            '{"id": %s}' % ((2**32)-1),
        )
        results = (3590016419, 2**31, 2**32, 2**32-1)
        for doc, result in zip(docs, results):
            self.assertEqual(hjson.decode(doc)['id'], result)

    def test_encodeBigEscape(self):
        for x in range(10):
            if six.PY3:
                base = '\u00e5'.encode('utf-8')
            else:
                base = "\xc3\xa5"
            input = base * 1024 * 1024 * 2
            hjson.dumps(input)

    def test_decodeBigEscape(self):
        for x in range(10):
            if six.PY3:
                base = '\u00e5'.encode('utf-8')
                quote = "\"".encode()
            else:
                base = "\xc3\xa5"
                quote = "\""
            input = quote + (base * 1024 * 1024 * 2) + quote
            hjson.decode(input)

    def test_object_default(self):
        # An object without toDict or __json__ defined should be serialized
        # as an empty dict.
        class ObjectTest:
            pass

        output = hjson.dumps(ObjectTest())
        dec = hjson.decode(output)
        self.assertEquals(dec, {})

    def test_object_with_complex_json(self):
        # If __json__ returns a string, then that string
        # will be used as a raw JSON snippet in the object.
        obj = {u'foo': [u'bar', u'baz']}
        class JSONTest:
            def __json__(self):
                return hjson.dumps(obj)

        d = {u'key': JSONTest()}
        output = hjson.dumps(d)
        dec = hjson.decode(output)
        self.assertEquals(dec, {u'key': obj})

    def test_object_with_json_type_error(self):
        # __json__ must return a string, otherwise it should raise an error.
        for return_value in (None, 1234, 12.34, True, {}):
            class JSONTest:
                def __json__(self):
                    return return_value

            d = {u'key': JSONTest()}
            self.assertRaises(TypeError, hjson.dumps, d)

    def test_object_with_json_attribute_error(self):
        # If __json__ raises an error, make sure python actually raises it.
        class JSONTest:
            def __json__(self):
                raise AttributeError

        d = {u'key': JSONTest()}
        self.assertRaises(AttributeError, hjson.dumps, d)

    def test_decodeArrayTrailingCommaFail(self):
        input = "[31337,]"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayLeadingCommaFail(self):
        input = "[,31337]"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayOnlyCommaFail(self):
        input = "[,]"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayUnmatchedBracketFail(self):
        input = "[]]"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayEmpty(self):
        input = "[]"
        obj = hjson.decode(input)
        self.assertEqual([], obj)

    def test_decodeArrayDict(self):
        input = "{}"
        obj = hjson.decode(input)
        self.assertEqual({}, obj)

    def test_decodeArrayOneItem(self):
        input = "[31337]"
        hjson.decode(input)

    def test_decodeLongUnsignedValue(self):
        input = "18446744073709551615"
        hjson.decode(input)

    def test_decodeBigValue(self):
        input = "9223372036854775807"
        hjson.decode(input)

    def test_decodeSmallValue(self):
        input = "-9223372036854775808"
        hjson.decode(input)

    def test_decodeTooBigValue(self):
        input = "18446744073709551616"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeTooSmallValue(self):
        input = "-90223372036854775809"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeVeryTooBigValue(self):
        input = "18446744073709551616"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeVeryTooSmallValue(self):
        input = "-90223372036854775809"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeWithTrailingWhitespaces(self):
        input = "{}\n\t "
        hjson.decode(input)

    def test_decodeWithTrailingNonWhitespaces(self):
        input = "{}\n\t a"
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeArrayWithBigInt(self):
        input = '[18446744073709551616]'
        self.assertRaises(ValueError, hjson.decode, input)

    def test_decodeFloatingPointAdditionalTests(self):
        self.assertEqual(-1.1234567893, hjson.loads("-1.1234567893"))
        self.assertEqual(-1.234567893, hjson.loads("-1.234567893"))
        self.assertEqual(-1.34567893, hjson.loads("-1.34567893"))
        self.assertEqual(-1.4567893, hjson.loads("-1.4567893"))
        self.assertEqual(-1.567893, hjson.loads("-1.567893"))
        self.assertEqual(-1.67893, hjson.loads("-1.67893"))
        self.assertEqual(-1.7893, hjson.loads("-1.7893", precise_float=True))
        self.assertEqual(-1.893, hjson.loads("-1.893"))
        self.assertEqual(-1.3, hjson.loads("-1.3"))

        self.assertEqual(1.1234567893, hjson.loads("1.1234567893"))
        self.assertEqual(1.234567893, hjson.loads("1.234567893"))
        self.assertEqual(1.34567893, hjson.loads("1.34567893"))
        self.assertEqual(1.4567893, hjson.loads("1.4567893"))
        self.assertEqual(1.567893, hjson.loads("1.567893"))
        self.assertEqual(1.67893, hjson.loads("1.67893"))
        self.assertEqual(1.7893, hjson.loads("1.7893", precise_float=True))
        self.assertEqual(1.893, hjson.loads("1.893"))
        self.assertEqual(1.3, hjson.loads("1.3"))

    def test_encodeBigSet(self):
        s = set()
        for x in range(0, 100000):
            s.add(x)
        hjson.dumps(s)

    @unittest.skipIf(blist is None, "This test tests functionality with the blist library")
    def test_encodeBlist(self):
        b = blist(list(range(10)))
        c = hjson.dumps(b)
        d = hjson.loads(c)

        self.assertEqual(10, len(d))

        for x in range(10):
            self.assertEqual(x, d[x])

    def test_encodeEmptySet(self):
        s = set()
        self.assertEqual("[]", hjson.dumps(s))

    def test_encodeSet(self):
        s = set([1, 2, 3, 4, 5, 6, 7, 8, 9])
        enc = hjson.dumps(s)
        dec = hjson.decode(enc)

        for v in dec:
            self.assertTrue(v in s)

    def test_ReadBadObjectSyntax(self):
        input = '{"age", 44}'
        self.assertRaises(ValueError, hjson.decode, input)

    def test_ReadTrue(self):
        self.assertEqual(True, hjson.loads("true"))

    def test_ReadFalse(self):
        self.assertEqual(False, hjson.loads("false"))

    def test_ReadNull(self):
        self.assertEqual(None, hjson.loads("null"))

    def test_WriteTrue(self):
        self.assertEqual("true", hjson.dumps(True))

    def test_WriteFalse(self):
        self.assertEqual("false", hjson.dumps(False))

    def test_WriteNull(self):
        self.assertEqual("null", hjson.dumps(None))

    def test_ReadArrayOfSymbols(self):
        self.assertEqual([True, False, None], hjson.loads(" [ true, false,null] "))

    def test_WriteArrayOfSymbolsFromList(self):
        self.assertEqual("[true,false,null]", hjson.dumps([True, False, None]))

    def test_WriteArrayOfSymbolsFromTuple(self):
        self.assertEqual("[true,false,null]", hjson.dumps((True, False, None)))

    @unittest.skipIf(not six.PY3, "Only raises on Python 3")
    def test_encodingInvalidUnicodeCharacter(self):
        s = "\udc7f"
        self.assertRaises(UnicodeEncodeError, hjson.dumps, s)

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
