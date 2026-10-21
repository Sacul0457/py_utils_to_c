from __future__ import annotations

from typing import Sequence, Callable

def dpy_utils_get(seq: Sequence, **attrs):
    """
    Similar to discord.py's discord.utils.get
    """
    pass


def dpy_utils_find(predicate: Callable, seq: Sequence):
    """
    Similar to discord.py's discord.utils.find
    """
    pass


try:
    from _core import *  # import the C version
except ImportError:
    pass