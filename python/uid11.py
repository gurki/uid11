# uid11.py — Base58 uid helpers (Bitcoin alphabet) + time-random xid profile.
#
# Public surface mirrors the C++ layered API:
#
#   uid11.*       — format constants + pure codec + profile-agnostic random
#   uid11.xid.*   — the xid profile (42 bit ms timestamp | 22 bit random),
#                   both pure packers and stateful generation
#
# Anything prefixed with `_` is implementation detail and not part of the
# stable API.

from __future__ import annotations
from datetime import datetime, timezone, timedelta
import secrets
import time
from typing import NamedTuple, Optional

__version__ = "0.2.0"

# ---------- format constants ----------

ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
BASE = len(ALPHABET)                    # 58
LENGTH = 11
MIN_U64_B58 = "11111111111"             # encode(0)
MAX_U64_B58 = "jpXCZedGfVQ"             # encode(2**64 - 1)

# precomputed alphabet -> index map for decoding
_INDEX = {c: i for i, c in enumerate(ALPHABET)}
_U64_MAX = (1 << 64) - 1


# ---------- pure codec ----------

def encode(payload: int) -> str:
    """Encode 0 <= payload < 2**64 into an 11-char Base58 string."""
    if payload < 0 or payload >= 1 << 64:
        raise ValueError("payload must be a 64-bit unsigned integer")
    out = ["1"] * LENGTH
    v = payload
    for i in range(LENGTH - 1, -1, -1):
        out[i] = ALPHABET[v % BASE]
        v //= BASE
    return "".join(out)


def _unpack(s: str) -> Optional[int]:
    """Decode an already-validated prefix; returns None only on u64 overflow."""
    acc = 0
    for ch in s:
        val = _INDEX.get(ch)
        if val is None:
            return None
        acc = acc * BASE + val
        if acc > _U64_MAX:
            return None
    return acc


def is_valid_partial(s: str) -> bool:
    """True if s has 0..11 chars, all from the Base58 alphabet."""
    return len(s) <= LENGTH and all(ch in _INDEX for ch in s)


def is_valid(s: str) -> bool:
    """True if s is exactly 11 Base58 chars."""
    return len(s) == LENGTH and is_valid_partial(s)


def decode(s: str) -> Optional[int]:
    """Decode an 11-char Base58 string into int, or None if invalid."""
    if not is_valid(s):
        return None
    return _unpack(s)


class PrefixRange(NamedTuple):
    """Closed numeric range [lower, upper] of u64 values matching a base58
    prefix. For a full 11-char string the range degenerates to a single value.
    """
    lower: int
    upper: int


def decode_partial(s: str) -> Optional[PrefixRange]:
    """Closed range of u64 values matching a 0..11 char base58 prefix.

    Returns None if the prefix contains non-alphabet chars, is too long, or
    maps to a range that lies entirely outside [0, 2^64). See SPECIFICATION.md
    §3.6 / §4.

    Edge cases:
        ""              -> (0, 2^64 - 1)   (the whole u64 space)
        11-char string  -> (v, v)          (same as decode())
        lower > u64_max -> None
        upper > u64_max -> upper clamped to u64_max
    """
    if not is_valid_partial(s):
        return None

    if len(s) == 0:
        return PrefixRange(0, _U64_MAX)

    val = _unpack(s)
    if val is None:
        return None  # full 11-char string overflowed u64

    n = len(s)
    scale = BASE ** (LENGTH - n)
    lower = val * scale
    if lower > _U64_MAX:
        return None
    upper = lower + scale - 1
    if upper > _U64_MAX:
        upper = _U64_MAX
    return PrefixRange(lower, upper)


# ---------- profile-agnostic random 64-bit ----------

def random() -> int:
    """Cryptographically strong 64-bit random integer."""
    return secrets.randbits(64)


def random_string() -> str:
    """11-char Base58 string encoding a random 64-bit number."""
    return encode(random())


# ---------- xid profile (42 bit ms timestamp | 22 bit random) ----------

def _now_ms() -> int:
    return int(time.time() * 1000)


class xid:
    """xid profile namespace.

    Layout: [ 42 bit ms since epoch | 22 bit random ]
    Epoch:  2011-11-11T11:11:11.111Z (epoch_ms = 1321009871111)
    Rollover: 2151-05-18T09:31:07.215Z from xid epoch.

    Used as a namespace; never instantiated. Access members as
    ``uid11.xid.generate()``, ``uid11.xid.pack(...)`` etc.
    """

    time_bits = 42
    random_bits = 64 - time_bits        # 22
    epoch_ms = 1321009871111
    _epoch = datetime.fromtimestamp(epoch_ms / 1000, tz=timezone.utc)
    _rand_mask = (1 << random_bits) - 1

    def __init__(self) -> None:         # pragma: no cover
        raise TypeError("uid11.xid is a namespace and cannot be instantiated")

    # -- pure --

    @staticmethod
    def pack(time_since_unix_epoch_ms: int, rnd: int) -> int:
        """Compose a payload from absolute unix-ms time and a random integer."""
        time_field = (time_since_unix_epoch_ms - xid.epoch_ms) << xid.random_bits
        rand_field = rnd & xid._rand_mask
        return (time_field | rand_field) & _U64_MAX

    @staticmethod
    def timepoint(payload: int) -> datetime:
        """UTC datetime (ms precision) extracted from a xid payload."""
        ms_since_epoch = payload >> xid.random_bits
        return xid._epoch + timedelta(milliseconds=ms_since_epoch)

    @staticmethod
    def timestamp(payload: int) -> str:
        """ISO 8601 'Z' timestamp (milliseconds) from a xid payload."""
        tp = xid.timepoint(payload)
        return tp.strftime("%Y-%m-%dT%H:%M:%S.") + f"{tp.microsecond // 1000:03d}Z"

    # -- stateful --

    @staticmethod
    def generate() -> int:
        """Fresh xid: high 42 bits = ms since epoch, low 22 bits = randomness."""
        return xid.pack(_now_ms(), secrets.randbits(xid.random_bits))

    @staticmethod
    def generate_string() -> str:
        """Base58-encoded fresh xid."""
        return encode(xid.generate())


__all__ = [
    "ALPHABET", "BASE", "LENGTH",
    "MIN_U64_B58", "MAX_U64_B58",
    "encode", "decode", "decode_partial", "PrefixRange",
    "is_valid", "is_valid_partial",
    "random", "random_string",
    "xid",
    "__version__",
]
