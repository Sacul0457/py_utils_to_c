from setuptools import Extension, setup

setup(
    name="py_utils_to_c",
    version="0.0.0",
    ext_modules=[
        Extension("py_utils_to_c._core", ["py_utils_to_c/_core.c"]),
    ],
)