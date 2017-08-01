try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
import os.path
import glob
import re

CLASSIFIERS = filter(None, map(str.strip, """
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


c_module = Extension(
    'helljson',
    sources=glob.glob('./lib/*.c*'),
    include_dirs=['./include/'],
    extra_compile_args=['-D_GNU_SOURCE', '-O3', '-msse3']
)


def get_version():
    filename = os.path.join(os.path.dirname(__file__), './include/version.h')
    try:
        version_file = open(filename)
        header = version_file.read()
    finally:
        if version_file:
            version_file.close()
    m = re.search(r'#define\s+HELLJSON_VERSION\s+"(\d+\.\d+(?:\.\d+)?)"', header)
    assert m, "version.h must contain HELLJSON_VERSION macro"
    return m.group(1)


def get_readme():
    try:
        f = open('README.rst')
        return f.read()
    finally:
        f.close()

setup(
    name='helljson',
    version=get_version(),
    description="Yet another fast JSON encoder/decoder for Python",
    long_description=get_readme(),
    ext_modules=[c_module],
    author="Andrey Proskurnev",
    author_email="andrey@proskurnev.ru",
    download_url="http://github.com/ariloulaleelay/helljson",
    license="BSD License",
    platforms=['any'],
    classifiers=CLASSIFIERS,
)
