# uid11.py — tiny Base58 uid helpers (Bitcoin alphabet) + time-random XID

from __future__ import annotations
from datetime import datetime, timezone, timedelta
import secrets
import time
from typing import Optional

# Constants (match the original)
ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
BASE = len(ALPHABET)  # 58
LENGTH = 11
MIN_U64_B58 = "11111111111"            # encode(0)
MAX_U64_B58 = "jpXCZedGfVQ"            # encode(2**64 - 1)

# Time+random layout (mirrors header: 44 time bits, 20 random bits)
TIME_BITS = 44
RAND_BITS = 64 - TIME_BITS             # 20
EPOCH_MS = 1321009871111               # 2011-11-11T11:11:11.111Z
_EPOCH = datetime.fromtimestamp(EPOCH_MS / 1000, tz=timezone.utc)
_RAND_MASK = (1 << RAND_BITS) - 1

# Precompute decode map
_INDEX = {c: i for i, c in enumerate(ALPHABET)}

# ---------- Base58 encode/decode for 64-bit payloads ----------

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
    acc = 0
    for ch in s:
        val = _INDEX.get(ch)
        if val is None:
            return None
        acc = acc * BASE + val
    return acc

def is_valid_partial(s: str) -> bool:
    """True if s <= 11 chars and all chars are in the Base58 alphabet."""
    return len(s) <= LENGTH and all(ch in _INDEX for ch in s)

def is_valid(s: str) -> bool:
    """True if s is exactly 11 Base58 chars."""
    return len(s) == LENGTH and is_valid_partial(s)

def decode(s: str) -> Optional[int]:
    """Decode an 11-char Base58 string into int, or None if invalid."""
    if not is_valid(s):
        return None
    return _unpack(s)

def decode_partial(s: str) -> Optional[int]:
    """
    Decode a prefix (<= 11 chars). Returns the value left-shifted in base58
    so it occupies the high-order positions of the 11-char space.
    """
    if not is_valid_partial(s):
        return None
    acc = _unpack(s)
    return None if acc is None else acc * (BASE ** (LENGTH - len(s)))

# ---------- Randomness ----------

def random() -> int:
    """Cryptographically strong 64-bit random integer."""
    return secrets.randbits(64)

def random_string() -> str:
    """11-char Base58 string encoding a random 64-bit number."""
    return encode(random())

# ---------- Time + random XID (44 time bits ms since EPOCH, 20 random bits) ----------

def _now_ms() -> int:
    return int(time.time() * 1000)

def xid() -> int:
    """64-bit ID: high 44 bits = ms since EPOCH_MS, low 20 bits = randomness."""
    time_bits = (_now_ms() - EPOCH_MS) << RAND_BITS
    rand_bits = secrets.randbits(RAND_BITS) & _RAND_MASK
    return (time_bits | rand_bits) & ((1 << 64) - 1)

def xid_string() -> str:
    """Base58-encoded xid()."""
    return encode(xid())

def timepoint(payload: int) -> datetime:
    """UTC datetime (ms precision) extracted from a packed payload/xid."""
    ms_since_epoch = (payload >> RAND_BITS)
    return _EPOCH + timedelta(milliseconds=ms_since_epoch)

def timestamp(payload: int) -> str:
    """ISO 8601 'Z' timestamp (milliseconds) from payload/xid."""
    return timepoint(payload).isoformat(timespec="milliseconds").replace("+00:00", "Z")

def pack(time_since_unix_epoch_ms: int, rnd: int) -> int:
    """Compose a payload from absolute unix time (ms) and a random integer."""
    time_bits = (time_since_unix_epoch_ms - EPOCH_MS) << RAND_BITS
    rand_bits = rnd & _RAND_MASK
    return (time_bits | rand_bits) & ((1 << 64) - 1)

__all__ = [
    "ALPHABET", "BASE", "LENGTH",
    "MIN_U64_B58", "MAX_U64_B58",
    "encode", "decode", "decode_partial",
    "is_valid", "is_valid_partial",
    "random", "random_string",
    "xid", "xid_string", "timepoint", "timestamp", "pack",
]
