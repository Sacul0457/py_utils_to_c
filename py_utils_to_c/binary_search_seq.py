from __future__ import annotations

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from collections.abc import Sequence

def binary_search_seq_has(seq: Sequence[int], target: int) -> bool:
    """
    Implements a binary search on an integer sequence.
    Returns `True` if the `target` is in the sequence, else `False`.
    """
    pass


def binary_search_seq_get(seq: Sequence[int], target: int) -> int | None:
    """
    Returns the position of the target, or `None` if the target is not in the sequence.
    """
    pass