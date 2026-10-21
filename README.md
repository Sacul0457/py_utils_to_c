# py_utils_to_c
A C extension that implements common python utility (*utils*) functions/classes. Naturally this will speed it up a lot, and speed is gud.
> [!NOTE]
> This project is still under development and this is ultimately a hobby project :)

## Features
- Extremely fast implementations (up to 10x faster on some)
- Mimics the python version

## Installing

Currently it is not on Pip sadly, but probably will be in the future!
Hence you have to clone the development version.

> [!NOTE]
> Python 3.14 is required (for now)

```sh
git clone https://github.com/Sacul0457/py_utils_to_c.git
```

## Setup
Since this is a development version, you have to build/setup the c-exension.

Don't worry, this is extremely easy to do, just have to enter the command below (in the root directory):
```sh
# Linux/macOS
python3 setup.py build_ext --inplace

# Windows
py -3.14 setup.py build_ext --inplace
```

## Example
> [!IMPORTANT]
> Since this project is cloned, you need to `import py_utils_to_c.py_utils_to_c` for now.

```python
from py_utils_to_c.py_utils_to_c import dpy_utils_get, dpy_utils_find

class ExampleClass:
    def __init__(self, value: int) -> None:
        self.value = value

    def __repr__(self) -> str:
        return str(self.value)

    def __eq__(self, value: ExampleClass) -> bool:
        return self.value == value

def main():
    classes = [ExampleClass(i) for i in range(100)]
    get_value: ExampleClass = dpy_utils_get(classes, value=30)
    find_value: ExampleClass = dpy_utils_find(lambda c: c.value == 30, classes) 

    print(get_value, find_value)
    assert(get_value is find_value)

if __name__ == "__main__":
    main()
```
