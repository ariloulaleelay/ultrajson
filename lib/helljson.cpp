#include "py_defines.h"
#include "version.h"

/* dumps */
PyObject* dumps(PyObject* self, PyObject *args, PyObject *kwargs);

void init_dumps(void);

/* dump */
PyObject* dump(PyObject* self, PyObject *args, PyObject *kwargs);

/* loads */
//PyObject* loads(PyObject* self, PyObject *args, PyObject *kwargs);

/* JSONFileToObj */
//PyObject* JSONFileToObj(PyObject* self, PyObject *args, PyObject *kwargs);


static PyMethodDef helljsonMethods[] = {
  {"dump", (PyCFunction) dump, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file."},
  {"dumps", (PyCFunction) dumps, METH_VARARGS | METH_KEYWORDS,  "Converts arbitrary object recursively into JSON."},
//  {"loads", (PyCFunction) loads, METH_VARARGS | METH_KEYWORDS,  "Converts JSON as string to dict object structure."},
//  {"load", (PyCFunction) JSONFileToObj, METH_VARARGS | METH_KEYWORDS, "Converts JSON as file to dict object structure."},
  {NULL, NULL, 0, NULL}       /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "helljson",
  0,              /* m_doc */
  -1,             /* m_size */
  helljsonMethods,   /* m_methods */
  NULL,           /* m_reload */
  NULL,           /* m_traverse */
  NULL,           /* m_clear */
  NULL            /* m_free */
};

#define PYMODINITFUNC       PyObject *PyInit_helljson(void)
#define PYMODULE_CREATE()   PyModule_Create(&moduledef)
#define MODINITERROR        return NULL

#else

#define PYMODINITFUNC       PyMODINIT_FUNC inithelljson(void)
#define PYMODULE_CREATE()   Py_InitModule("helljson", helljsonMethods)
#define MODINITERROR        return

#endif

PYMODINITFUNC {
  PyObject *module;
  PyObject *version_string;

  init_dumps();
  module = PYMODULE_CREATE();

  if (module == NULL) {
    MODINITERROR;
  }

  version_string = PyString_FromString (HELLJSON_VERSION);
  PyModule_AddObject (module, "__version__", version_string);

#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}
