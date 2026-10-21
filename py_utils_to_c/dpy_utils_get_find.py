from __future__ import annotations

from typing import Sequence, Callable, TypeVar

T = TypeVar('T')

def dpy_utils_get(seq: Sequence[T], **attrs) -> T | None:
    """
    Similar to discord.py's discord.utils.get
    """
    pass


def dpy_utils_find(predicate: Callable, seq: Sequence[T]) -> T | None:
    """
    Similar to discord.py's discord.utils.find
    """
    pass