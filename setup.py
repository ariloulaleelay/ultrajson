try:
  from setuptools import setup, Extension
except ImportError:
  from distutils.core import setup, Extension
import distutils.sysconfig
import os.path
import re
import sys

CLASSIFIERS = filter(None, map(str.strip,
"""
Development Status :: 5 - Production/Stable
Intended Audience :: Developers
License :: OSI Approved :: BSD License
Programming Language :: C
Programming Language :: Python :: 2.4
Programming Language :: Python :: 2.5
Programming Language :: Python :: 2.6
Programming Language :: Python :: 2.7
Programming Language :: Python :: 3
Programming Language :: Python :: 3.2
""".splitlines()))

module1 = Extension('hjson',
                    sources = ['./python/hjson.cpp',
                               './python/Encoder.cpp',
                               './python/dumps.cpp',
                               './python/itoa_sse2.cpp',
                               # './python/JSONtoObj.cpp',
                               './lib/ultrajsonenc.c',
                               './lib/ultrajsondec.c'],
                    include_dirs = ['./python', './lib'],
                    extra_compile_args=['-D_GNU_SOURCE'])

def get_version():
    filename = os.path.join(os.path.dirname(__file__), './python/version.h')
    file = None
    try:
        file = open(filename)
        header = file.read()
    finally:
        if file:
            file.close()
    m = re.search(r'#define\s+HJSON_VERSION\s+"(\d+\.\d+(?:\.\d+)?)"', header)
    assert m, "version.h must contain HJSON_VERSION macro"
    return m.group(1)

f = open('README.rst')
try:
    README = f.read()
finally:
    f.close()

setup (name = 'hjson',
       version = get_version(),
       description = "Ultra fast JSON encoder and decoder for Python",
       long_description = README,
       ext_modules = [module1],
       author="Jonas Tarnstrom",
       author_email="jonas.tarnstrom@esn.me",
       download_url="http://github.com/esnme/ultrajson",
       license="BSD License",
       platforms=['any'],
       url="http://www.esn.me",
       classifiers=CLASSIFIERS,
       )
