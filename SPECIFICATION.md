# uid11: Fixed-length (11) Base58 identifier · Profile: **xid**

**Spec version:** 1.1 (draft) · **Status:** Proposed Standard

> **Changes from 1.0:** prefix length range widened from `1..11` to `0..11`; added §4.1 normative u64 projection rules (clamp / no‑match for ranges crossing or exceeding `2^64 − 1`); added §4.2 informative test vectors; clarified §3.6, §7, and §8.4 wording accordingly.


## 1. Purpose & scope

**uid11** defines a compact, human-readable, URL-safe identifier format with:

* **Fixed length:** 11 Base58 symbols (Bitcoin alphabet)
* **Payload:** a 64-bit unsigned integer
* **Ordering:** lexicographic order of the 11-symbol string equals numeric order of the payload
* **Prefixes:** leading symbols are MSB-first and can be used as range prefixes

**xid** is a concrete uid11 profile with timestamp semantics (see §8).


## 2. Terminology

* **uid11** — the canonical 11-symbol text form over a 64‑bit payload.
* **Profile** — a defined interpretation of the 64‑bit payload bits.
* **xid** — the timestamped uid11 profile defined in this document (42‑bit ms | 22‑bit random).
* **Prefix** — the first *N* symbols of a uid11 (0 ≤ *N* ≤ 11), representing a numeric range. `N = 0` (the empty prefix) denotes the entire u64 space; `N = 11` denotes a single value.
* **MSB-first** — leftmost text symbol encodes the highest‑order bits of the payload.


## 3. uid11 encoding (normative)

### 3.1 Alphabet & base

* **Alphabet (Bitcoin Base58):**

  ```
  123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz
  ```

  (no `0`, `O`, `I`, `l`)
* **Base:** 58

### 3.2 Text length

* The canonical uid11 text form **MUST** be exactly **11 symbols**.

### 3.3 Canonical encoding

Given a 64‑bit unsigned payload `value`, encode in Base58 **MSB‑first** (most‑significant digit first), then **left‑pad with `'1'`** (the alphabet’s zero) to **exactly 11 symbols**.

**Notes:** Left‑padding with `'1'` does **not** change the numeric value; it only enforces fixed width. Implementations **MUST NOT** right‑pad (appending symbols), which would change the numeric value and break ordering.

### 3.4 Canonical decoding (strict)

Accept exactly 11 symbols from the alphabet and map back to the 64‑bit payload. Inputs with any other length or non‑alphabet characters **MUST** be rejected in canonical mode.

### 3.5 Validation

* **Regex (strict canonical):**

  ```
  ^[1-9A-HJ-NP-Za-km-z]{11}$
  ```
* **ABNF:**

  ```
  ALPHA58 = %x31-39 / %x41-48 / %x4A-4E / %x50-5A / %x61-6B / %x6D-7A
  UID11   = 11ALPHA58
  ```

### 3.6 Partial decoding (prefix‑aware mode)

Decoders operating in prefix‑aware mode **MAY** accept **0..11** symbols. For inputs of length N < 11, they **MUST** interpret the text as a numeric range per §4. For N = 11 the range degenerates to a single value and behaves identically to canonical strict decoding (§3.4).

Implementations **MUST** return either a closed range `[lowerBound, upperBound]` or signal "no match" (`null` / `None` / `nullopt` / equivalent) per the u64 projection rules of §4.1.


## 4. Prefix semantics (normative for prefix-aware consumers)

For a prefix `P` of length *N* (0 ≤ *N* ≤ 11), define:

```
scale      = 58^(11 - N)
lowerBound = value(P) * scale
upperBound = lowerBound + (scale - 1)
```

where `value(P)` is the base-58 numeric value of `P`, with `value("") = 0`.

A prefix **MUST** be interpreted as the **closed** numeric range `[lowerBound, upperBound]`, subject to the u64 projection in §4.1.

> Keeping the first *N* symbols preserves roughly `N·log2(58)` ≈ `5.858·N` most‑significant bits of the payload. For timestamped profiles with 42 time bits, approximate timestamp bucket sizes are: **N=6 → \~114 ms**, **N=7 → \~2 ms**, **N≥8 → 1 ms (full precision)**.

### 4.1 Projection to the u64 payload space (normative)

The 11‑character base‑58 string space contains `58^11 ≈ 2.50·10^19` values, larger than the `2^64 ≈ 1.84·10^19` u64 payload space. For a prefix range `[lowerBound, upperBound]`, three outcomes are possible:

1. **Fully inside u64** (`upperBound ≤ 2^64 − 1`) — the range is returned unchanged.
2. **Straddles the boundary** (`lowerBound ≤ 2^64 − 1 < upperBound`) — implementations **MUST** clamp the upper bound to `2^64 − 1`. The returned range is `[lowerBound, 2^64 − 1]`.
3. **Fully outside u64** (`lowerBound > 2^64 − 1`) — the prefix matches no valid u64 payload. Implementations **MUST** signal "no match".

Two corner cases are direct consequences of these rules:

* For the empty prefix (`N = 0`), `scale = 58^11 > 2^64`, so the result is the closed range `[0, 2^64 − 1]` — the whole u64 space.
* For a full 11‑character prefix (`N = 11`), `scale = 1`, so `lowerBound = upperBound = value(P)` and the rules reduce to canonical strict decoding (§3.4).

### 4.2 Test vectors (informative)

The repository's `test-vectors.json` is the canonical cross‑implementation reference; the table below mirrors a representative subset. All values are decimal.

| Prefix         | Length | lowerBound                  | upperBound                  | Notes                                            |
| -------------- | -----: | --------------------------: | --------------------------: | ------------------------------------------------ |
| `""`           |      0 | `0`                         | `18446744073709551615`      | empty prefix → whole u64 space (§4.1 case 2)     |
| `"1"`          |      1 | `0`                         | `430804206899405823`        | scale = 58¹⁰, fully inside u64                   |
| `"i"`          |      1 | `17662972482875638784`      | `18093776689775044607`      | fully inside u64                                 |
| `"j"`          |      1 | `18093776689775044608`      | `18446744073709551615`      | upper clamped from `42·58¹⁰ + 58¹⁰ − 1` (case 2) |
| `"k"`          |      1 | — (no match)                | — (no match)                | lower = `43·58¹⁰ > 2^64 − 1` (case 3)            |
| `"11111111111"`|     11 | `0`                         | `0`                         | degenerate range, scale = 1                      |
| `"jpXCZedGfVQ"`|     11 | `18446744073709551615`      | `18446744073709551615`      | u64 maximum                                      |


## 5. Profiles & extensibility

A **profile** defines the meaning of the 64‑bit payload. Examples:

* **xid** (this document): `[ 42‑bit timestamp (ms) | 22‑bit random ]`
* **uid11-fullrand:** `[ 64‑bit random ]`
* **uid11-snowish:** `[ 1‑bit reserved | 41‑bit timestamp | 10‑bit machine | 12‑bit sequence ]`
* **uid11-ts-seq-rand:** `[ 42‑bit timestamp | 12‑bit sequence | 10‑bit random ]`

Profiles SHOULD specify epochs (if any), field order, generation rules, and security considerations. New profiles **MUST NOT** conflict with **xid** semantics when the same 11‑symbol space is used without external disambiguation.


## 6. Sorting, storage, and transport

* **Sorting:** bytewise/lexicographic sort of the 11‑symbol text is equivalent to numeric payload order.
* **Storage:** fixed‑width 11‑char keys simplify indexing and UI alignment.
* **Transport:** alphabet is URL‑ and filename‑safe without escaping.


## 7. Error handling (normative)

Decoders **MUST** reject:

* lengths ≠ 11;
* out‑of‑alphabet characters.

**Exception (prefix‑aware mode):** In partial decoding (§3.6), decoders **MUST NOT** reject lengths in 0..11 on length grounds alone; for N < 11 they **MUST** interpret the input as a numeric range per §4 and apply the u64 projection of §4.1. A prefix whose entire range falls outside `[0, 2^64 − 1]` (§4.1 case 3) **MUST** be reported as "no match" rather than rejected as malformed.

Encoders/decoders **MUST** apply profile‑specific validity rules (e.g., timestamp range checks) when a profile is in use.


## 8. Profile **xid** (normative)

### 8.1 Layout

```
MSB                                      LSB
+--------------------------+--------------------------+
| 42-bit timestamp (ms)    | 22-bit random            |
+--------------------------+--------------------------+
```

* **Random field:** 22 unbiased bits (tie‑breaker; not cryptographic).

### 8.2 Epoch & lifetime

* **EPOCH:** `2011-11-11T11:11:11.111Z`
  **EPOCH\_MS:** `1321009871111`
* **Valid timestamp range:** `0 … 2^42−1` ms → **\~139.43 years**.
* **Rollover (from EPOCH):** `2151-05-18T09:31:07.215Z`.

### 8.3 Generation (normative)

```
now_ms    := system_time_ms_utc()
delta_ms  := now_ms - EPOCH_MS
assert 0 ≤ delta_ms ≤ (2^42 - 1)
rand_22   := UniformRandomInteger(0, 2^22 - 1)
payload   := (delta_ms << 22) | rand_22
text      := Base58EncodeFixed11(payload)
```

### 8.4 Parsing (normative)

```
payload   := Base58DecodeFixed11(text)
delta_ms  := payload >> 22
rand_22   := payload & (2^22 - 1)
timestamp := EPOCH + delta_ms milliseconds
```

**Prefix‑aware parsing (optional):** For an input `text` of length N < 11, compute `lowerBound`/`upperBound` as in §3.6 (applying the u64 projection of §4.1), then derive:

```
ts_lo   = EPOCH + (lowerBound >> 22) milliseconds
ts_hi   = EPOCH + (upperBound >> 22) milliseconds
rand_lo =  lowerBound        & (2^22 - 1)
rand_hi =  upperBound        & (2^22 - 1)
```

### 8.5 Collision guidance (informative)

22‑bit random ⇒ **4,194,304** possibilities per millisecond.
Approximate probability of ≥1 collision among *n* IDs generated **in the same ms on the same node** (birthday bound, `M=4,194,304`):

| n in one ms | Collision probability |
| ----------: | --------------------: |
|         100 |               \~0.12% |
|       1,000 |                 \~11% |
|       5,000 |                 \~95% |
|      10,000 |             \~99.999% |

**Operational recommendations:** for single‑node generators, sustained rates around **≤1,000 IDs/ms** are typically acceptable; for higher bursts, consider sharding across nodes or switching to a sequence‑bearing profile.

### 8.6 Test vectors (normative)

All encodings use the alphabet in §3 and fixed‑11 left‑padding.

| Case                   | (timestamp, random) | 64‑bit payload (hex) | uid11 text    |
| ---------------------- | ------------------: | -------------------: | ------------- |
| **Zero**               |           ts=0, r=0 | `0x0000000000000000` | `11111111111` |
| **+1 ms after epoch**  |           ts=1, r=0 | `0x0000000000400000` | `1111111NVpb` |
| **+1 day after epoch** |  ts=86,400,000, r=0 | `0x0001499700000000` | `113q8KFkAEs` |

*(Where `ts` is milliseconds since `2011-11-11T11:11:11.111Z`.)*


## 9. Reference pseudocode (informative)

```text
// uid11 constants
ALPHABET  = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
BASE      = 58
UID11_LEN = 11

// xid constants
EPOCH_MS  = 1321009871111  // 2011-11-11T11:11:11.111Z

// xid.pack
function xid_new(now_ms):
  delta = now_ms - EPOCH_MS
  assert 0 <= delta <= (1<<42) - 1
  r = UniformRandomInteger(0, (1<<22) - 1)
  payload = (delta << 22) | r
  return base58_encode_fixed11(payload)

// xid.unpack
function xid_parse(text):
  assert text matches ^[1-9A-HJ-NP-Za-km-z]{11}$
  payload = base58_decode(text)
  delta   = payload >> 22
  random  = payload & ((1<<22) - 1)
  ts_ms   = EPOCH_MS + delta
  return (ts_ms, random)
```


## 10. Security considerations (informative)

* **Time leakage:** Profiles with timestamps reveal creation time at 1 ms precision. Protect where sensitive.
* **Guessability:** uid11/xid are not bearer secrets.
* **Enumeration:** Monotonic IDs aid scraping; apply rate limits and ACLs.


## 11. Acknowledgements

Thanks to the community work on Snowflake‑like IDs and URL‑safe alphabets; uid11/xid aim to keep the best parts while improving readability and fixed‑width ergonomics.
