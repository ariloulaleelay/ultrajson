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

#include "py_defines.h"
#include <stdio.h>
#include <datetime.h>

#include "Encoder.h"

static PyObject* type_decimal = NULL;

#if (PY_VERSION_HEX < 0x02050000)
typedef ssize_t Py_ssize_t;
#endif

Encoder encoderSingleton;

void init_dumps(void) {
  PyObject* mod_decimal = PyImport_ImportModule("decimal");
  if (mod_decimal) {
    type_decimal = PyObject_GetAttrString(mod_decimal, "Decimal");
    Py_INCREF(type_decimal);
    Py_DECREF(mod_decimal);
  } else {
    PyErr_Clear();
  }

  PyDateTime_IMPORT;
}

bool TraverseObject(PyObject *obj, Encoder & encoder);

bool TraverseDictKey(PyObject *obj, Encoder & encoder) {
  if (PyUnicode_Check(obj)) {
    return encoder.pushUcs(PyUnicode_AS_UNICODE(obj), (size_t) PyUnicode_GET_DATA_SIZE(obj));
  } else if (PyString_Check(obj)) {
    return encoder.pushString(PyString_AS_STRING(obj), (size_t) PyString_GET_SIZE(obj));
  }
  encoder.error = "unknown dict key type";
  return false;
}

bool TraverseDict(PyObject *obj, Encoder & encoder) {

  PyObject *key, *value;
  Py_ssize_t pos = 0;
  bool notFirst = false;

  if (!encoder.enterMap())
    return false;

  while (PyDict_Next(obj, &pos, &key, &value)) {
    if (notFirst)
      encoder.pushComma();

    notFirst = true;

    if (!TraverseDictKey(key, encoder))
      return false;

    encoder.pushColon();

    if (!TraverseObject(value, encoder))
      return false;
  }
  encoder.exitMap();
  return true;
}

bool TraverseList(PyObject *obj, Encoder & encoder) {
  if (!encoder.enterSeq())
    return false;

  size_t size = PyList_GET_SIZE(obj);
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      encoder.pushComma();
    if (!TraverseObject(PyList_GET_ITEM(obj, i), encoder))
      return false;
  }
  encoder.exitSeq();
  return true;
}

bool TraverseTuple(PyObject *obj, Encoder & encoder) {
  if (!encoder.enterSeq())
    return false;

  size_t size = PyTuple_GET_SIZE(obj);
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      encoder.pushComma();
    if (!TraverseObject(PyTuple_GET_ITEM(obj, i), encoder))
      return false;
  }
  encoder.exitSeq();
  return true;
}

bool TraverseSet(PyObject *obj, Encoder & encoder) {
  if (!encoder.enterSeq())
    return false;

  PyObject * iter = PyObject_GetIter(obj);
  PyObject * item;
  bool isFirst = true;

  if (iter == NULL)
    return false;

  while((item = PyIter_Next(iter))) {
    if (isFirst)
      isFirst = false;
    else
      encoder.pushComma();

    if (!TraverseObject(item, encoder)) {
      Py_DECREF(item);
      Py_DECREF(iter);
      return false;
    }
    Py_DECREF(item);
  }
  Py_DECREF(iter);
  encoder.exitSeq();
  return true;
}

bool TraverseObject(PyObject *obj, Encoder & encoder) {
  if (encoder.getDepth() > 1000) {
    encoder.error = "Maximum depth reached. Recursion?";
    encoder.exceptionObject = PyExc_OverflowError;
    return false;
  }

  if (PyIter_Check(obj)) {
    goto ITERABLE;
  }

  if (PyUnicode_Check(obj)) {
    return encoder.pushUcs(PyUnicode_AS_UNICODE(obj), (size_t) PyUnicode_GET_DATA_SIZE(obj));
  } else if (PyString_Check(obj)) {
    return encoder.pushString(PyString_AS_STRING(obj), (size_t) PyString_GET_SIZE(obj));
  } else if (PyBool_Check(obj)) {
    return encoder.pushBool(obj == Py_True);
  } else if (PyLong_Check(obj) || PyInt_Check(obj)) {
    int64_t value = PyLong_AsLongLong(obj);

    if (!PyErr_Occurred())
        return encoder.pushInteger(value);

    if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
      PyErr_Clear();
      uint64_t value = PyLong_AsUnsignedLongLong(obj);
      if (PyErr_Occurred()) {
        encoder.error = "number too big";
        encoder.exceptionObject = PyExc_OverflowError;
        return false;
      }
      return encoder.pushInteger(value);
    }
    return false;
  } else if (PyFloat_Check(obj)) {
    double value = PyFloat_AS_DOUBLE(obj);
    return encoder.pushDouble(value);
  } else if (obj == Py_None) {
    return encoder.pushNone();
  } else if (type_decimal && PyObject_IsInstance(obj, type_decimal)) {
    double value = PyFloat_AsDouble(obj);
    return encoder.pushDouble(value);
  } else if (PyDateTime_Check(obj)) {
    return encoder.pushDateTime(
        PyDateTime_GET_YEAR(obj),
        PyDateTime_GET_MONTH(obj),
        PyDateTime_GET_DAY(obj),
        PyDateTime_DATE_GET_HOUR(obj),
        PyDateTime_DATE_GET_MINUTE(obj),
        PyDateTime_DATE_GET_SECOND(obj),
        PyDateTime_DATE_GET_MICROSECOND(obj)
    );
  } else if (PyDate_Check(obj)) {
    return encoder.pushDate(
        PyDateTime_GET_YEAR(obj),
        PyDateTime_GET_MONTH(obj),
        PyDateTime_GET_DAY(obj)
    );
  }

ITERABLE:

  if (PyDict_Check(obj)) {
    return TraverseDict(obj, encoder);
  } else if (PyList_Check(obj)) {
    return TraverseList(obj, encoder);
  } else if (PyTuple_Check(obj)) {
    return TraverseTuple(obj, encoder);
  } else if (PySet_Check(obj)) {
    return TraverseSet(obj, encoder);
  }

  encoder.error = "Can't find serialization method for object";
  return false;
}

PyObject* dumps(PyObject* self, PyObject *args, PyObject *kwargs) {

  static char *kwlist[] = { "obj", NULL };

  PyObject *objectToDump = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &objectToDump))
    return NULL;

  encoderSingleton.reset();
  if (!TraverseObject(objectToDump, encoderSingleton)) {
    PyErr_Format(encoderSingleton.exceptionObject, encoderSingleton.error ? encoderSingleton.error : "unknown hjson error");
    return NULL;
  }

  if (PyErr_Occurred()) {
    return NULL;
  }

  PyObject * result = PyString_FromStringAndSize(encoderSingleton.result(), encoderSingleton.resultSize());

  return result;
}

PyObject* dump(PyObject* self, PyObject *args, PyObject *kwargs) {
  PyObject *data;
  PyObject *file;
  PyObject *string;
  PyObject *write;
  PyObject *argtuple;

  if (!PyArg_ParseTuple (args, "OO", &data, &file)) {
    return NULL;
  }

  if (!PyObject_HasAttrString (file, "write")) {
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  write = PyObject_GetAttrString (file, "write");

  if (!PyCallable_Check (write)) {
    Py_XDECREF(write);
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  argtuple = PyTuple_Pack(1, data);

  string = dumps(self, argtuple, kwargs);

  if (string == NULL) {
    Py_XDECREF(write);
    Py_XDECREF(argtuple);
    return NULL;
  }

  Py_XDECREF(argtuple);

  argtuple = PyTuple_Pack (1, string);
  if (argtuple == NULL) {
    Py_XDECREF(write);
    return NULL;
  }

  if (PyObject_CallObject (write, argtuple) == NULL) {
    Py_XDECREF(write);
    Py_XDECREF(argtuple);
    return NULL;
  }

  Py_XDECREF(write);
  Py_DECREF(argtuple);
  Py_XDECREF(string);

  Py_RETURN_NONE;
}
