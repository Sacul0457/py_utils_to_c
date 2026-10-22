# py_utils_to_c
A C extension that implements common python utility (*utils*) functions/classes. Naturally this will speed it up a lot, and faster is gud am I right?
> [!IMPORTANT]
> This project is still under development and is ultimately a hobby project :)
>
> I **do not** mean for this to replace any util functions in any project and speedups are usually negligible in most workloads.

## Features
- Extremely fast implementations (up to 11x faster on some)
- Mimics the python version (almost for now)

### Benchmarks/Speedups
| Function             | Python      | C Version              |
|----------------------|:-----------:|:----------------------:|
| discord.utils.get    | 15.3 ms     | 2.8 ms: 5.46x faster   |
| discord.utils.find   | 2.9 ms      | 2.9 ms: no change      |
| SnowflakeList.get    | 6.7 ms      | 0.6 ms: 11.2x faster   |
| SnowflakeList.has    | 6.6 ms      | 0.6 ms: 11.0x faster   |

*There are initial benchmarks and overall performance I have seen over the course of creating/testing.* Will make a more reliable one and share it in the repo.

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


def main():
    classes = [ExampleClass(i) for i in range(100)]
    get_value: ExampleClass = dpy_utils_get(classes, value=30)
    find_value: ExampleClass = dpy_utils_find(lambda c: c.value == 30, classes) 

    print(get_value, find_value)
    assert(get_value is find_value)

if __name__ == "__main__":
    main()
```
