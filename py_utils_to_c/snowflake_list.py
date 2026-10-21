from __future__ import annotations

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    try:
        from discord.utils import SnowflakeList
    except ImportError:
        pass

def dpy_utils_snowflakelist_has(snowflake_list: SnowflakeList, target: int):
    """
    Similar to discord.py's discord.SnowflakeList.has
    """
    pass


def dpy_utils_snowflakelist_get(snowflake_list: SnowflakeList, target: int):
    """
    Similar to discord.py's discord.SnowflakeList.get
    """
    pass


def dpy_utils_snowflakelist_add(snowflake_list: SnowflakeList, snowflake: int):
    """
    Similar to discord.py's discord.SnowflakeList.add
    """
    pass

try:
    from _core import * # import the C version
except ImportError:
    pass