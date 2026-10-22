from __future__ import annotations

from typing import Iterable, TypeVar

T = TypeVar('T')

def minmax(iterable: Iterable[T]) -> T:
    """
    Similar to the builtins min and max, but merges the operation for speedups.
    Raises :exec:`ValueError` if the iterable is empty/exhausted.
    """
    pass