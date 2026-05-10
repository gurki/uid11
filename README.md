# uid11 ⋅ xid Ξ
ergonomic 64-bit uid in 11 characters

```
encode(u64) -> base58 (11 chars, bitcoin alphabet)
decode(s)   -> u64

# example xid
1918655116723066557  ⇄  5TK7MT834rx  ⇄  2026-05-10T22:41:37.826Z
```

`uid11` is a compact identifier format: a 64-bit unsigned integer encoded as a fixed-width 11-character base58 string. It comes with one bundled timestamped profile, `xid`, that splits the 64 bits into a 42-bit millisecond timestamp and 22 bits of randomness — useful when you want short, time-ordered, URL-safe IDs without committing to 128 bits.

Implementations: **C++23** · **Python 3** · **TypeScript** · **Nim**. All four agree on the format and are checked against the same [`test-vectors.json`](./test-vectors.json). The format is described in [`SPECIFICATION.md`](./SPECIFICATION.md).


## Table of contents

- [Quick start](#quick-start)
- [Install](#install)
- [API at a glance](#api-at-a-glance)
- [When *not* to use this](#when-not-to-use-this)
- [Security](#security)
- [Database storage](#database-storage)
- [Format & rationale](#format--rationale)
- [Examples](#examples)
- [Versioning & stability](#versioning--stability)
- [Acknowledgements](#acknowledgements)


## Quick start

### C++ (single-header, C++23)

```cpp
#include <uid11/uid11.h>

// codec
auto s = uid11::random_string();     // "Up5nFzfoe9K"
auto v = uid11::decode(s);           // std::optional<uint64_t>

// xid profile (42 bit ms | 22 bit random)
auto id  = uid11::xid::generate();
auto txt = uid11::xid::timestamp(id);  // "2026-05-10T23:03:15.383Z"
```

### Python 3

```python
import uid11

uid11.random_string()             # "Up5nFzfoe9K"
uid11.decode("Up5nFzfoe9K")       # int | None

uid11.xid.generate()              # int (42 bit ms | 22 bit random)
uid11.xid.timestamp(some_id)      # "2026-05-10T23:03:15.383Z"
```

### TypeScript

```ts
import { encode, decode, randomString, xid } from "uid11";

randomString();                   // "Up5nFzfoe9K"
decode("Up5nFzfoe9K");            // bigint | null

xid.generate();                   // bigint
xid.timestamp(someId);            // "2026-05-10T23:03:15.383Z"
```

### Nim

```nim
import uid11
import uid11/xid

discard randomString()            # "Up5nFzfoe9K"
discard decode("Up5nFzfoe9K")     # Option[uint64]

discard xid.generate()
discard xid.timestamp(someId)
```


## Install

### C++ — CMake `FetchContent`

```cmake
include(FetchContent)
FetchContent_Declare(uid11
  GIT_REPOSITORY https://github.com/gurki/uid11.git
  GIT_TAG        v0.2.0
)
FetchContent_MakeAvailable(uid11)

target_link_libraries(my_app PRIVATE uid11::uid11)
```

Or after a system install (`cmake --install <build-dir>`):

```cmake
find_package(uid11 0.2 REQUIRED)
target_link_libraries(my_app PRIVATE uid11::uid11)
```

The library transitively requires C++23 (uses `<format>`, `<print>`, `<ranges>`, `std::optional::transform`).

### Python, TypeScript, Nim

Currently the Python, TypeScript and Nim sources live alongside the C++ in this repo and are best consumed by vendoring the relevant file (`python/uid11.py`, `ts/uid11.ts`, `nim/uid11.nim` + `nim/uid11/xid.nim`). PyPI / npm / nimble packaging is on the roadmap.


## API at a glance

Names are slightly adapted to each language's idioms (`camelCase` in TS, `snake_case` in Python, etc.); the shapes are identical.

### Pure codec (top-level)

| Operation       | Returns                       | Notes                                                  |
| --------------- | ----------------------------- | ------------------------------------------------------ |
| `encode(u)`     | 11-char `string`              | always 11 chars, left-padded with `'1'`                |
| `decode(s)`     | `u64` or null                 | strict: only 11-char alphabet input                    |
| `decode_partial(s)` | `{lower, upper}` or null  | closed range for 0..11 char prefix; see spec §4.1      |
| `is_valid(s)`   | `bool`                        | exactly 11 alphabet chars                              |
| `is_valid_partial(s)` | `bool`                  | 0..11 alphabet chars                                   |

### Random 64-bit (top-level)

| Operation        | Returns          | Notes                                                  |
| ---------------- | ---------------- | ------------------------------------------------------ |
| `random()`       | `u64`            | C++: thread-local `xoshiro256++` (fast, not crypto). Python/TS/Nim: OS CSPRNG. |
| `random_string()`| 11-char `string` | shorthand for `encode(random())`                       |

### `xid` profile namespace

```
[ 42 bit ms since xid epoch | 22 bit random ]
```

| Operation                | Returns               | Notes                                                  |
| ------------------------ | --------------------- | ------------------------------------------------------ |
| `xid.pack(ms, rand)`     | `u64`                 | pure; pack a wall-clock ms and a 22-bit random         |
| `xid.unpack(payload)`    | `{unix_ms, random}`   | pure; full inverse of `pack` — see [Unpacked](#unpacked) |
| `xid.random_field(p)`    | `u64`                 | pure; the 22-bit random tie-breaker                    |
| `xid.timepoint(p)`       | datetime / Date       | pure; extracts the timestamp from a xid payload        |
| `xid.timestamp(p)`       | ISO-8601 string       | pure; ms-precision UTC string ending in `Z`            |
| `xid.generate()`         | `u64`                 | uses the wall clock + the language's random source     |
| `xid.generate_string()`  | 11-char string        | shorthand for `encode(xid.generate())`                 |

##### Unpacked

`xid.unpack(payload)` returns a small record with two fields:

| Field      | Type | Meaning                                                 |
| ---------- | ---- | ------------------------------------------------------- |
| `unix_ms`  | u64  | ms since the **Unix** epoch — symmetric with `pack`'s first parameter |
| `random`   | u64  | the 22-bit random tie-breaker                           |

Naming is adjusted per language: `unpacked` / `Unpacked` (C++ / Python NamedTuple / TS `interface` / Nim `object`); fields are `unix_ms` / `unixMs` per local convention. Round-trip property: `pack(u.unix_ms, u.random) == payload`.

Constants live in the namespace too: `xid.time_bits == 42`, `xid.random_bits == 22`, `xid.epoch_ms == 1321009871111`.


## When *not* to use this

`uid11` deliberately picks a different point on the design space than UUIDs. Consider a 128-bit format (UUIDv4/v7, ULID) instead if **any** of the following hold:

- You generate IDs across many uncoordinated nodes at high volume — 22 bits of random tie-breaker gives ~4.2M values per millisecond per node, after which birthday collisions start to matter (see [§8.5 of the spec](./SPECIFICATION.md) for the table).
- You need broad ecosystem tooling (Postgres `uuid` type, language-level UUID parsers, etc.).
- The IDs are security-sensitive (session tokens, password reset tokens, capability URLs). See [security](#security).
- You can't tolerate 1-ms timestamp leakage in public IDs.

Use `uid11` when:

- Compactness matters (URLs, logs, copy-paste).
- 64-bit native storage matters (`BIGINT`, `int64`).
- You control the generation topology (single-node, sharded, or you can mitigate the collision envelope yourself).
- You want strings that sort lexicographically the same way the underlying integers sort — encoded `uid11` strings preserve numeric order.


## Security

```
uid11 values are identifiers, not secrets.
```

- The 22-bit random field in `xid` is a **tie-breaker**, not a cryptographic nonce. Do not treat the field as unguessable.
- Timestamped `uid11` values reveal creation time at ~1 ms precision. That's frequently fine for log lines, perfectly bad for capability URLs.
- The C++ implementation uses **xoshiro256++**, which is fast but not a CSPRNG. The Python and TypeScript implementations use the OS CSPRNG (`secrets`, `crypto.getRandomValues`). The Nim implementation uses the stdlib RNG.
- **Do not use `uid11` as an API key, session token, password reset token, or any other bearer credential.** Use at least 128 bits of cryptographic randomness for that.


## Database storage

`uid11` payloads are unsigned 64-bit integers, but most databases (and Java, JavaScript-as-number) expose only **signed** 64-bit integers. Three pragmatic options:

| Option              | Pros                              | Cons                                                 |
| ------------------- | --------------------------------- | ---------------------------------------------------- |
| `CHAR(11)`          | portable; URL-ready; sorts right  | 11 bytes vs 8; collation-sensitive                   |
| `BIGINT` (signed)   | compact native int                | values above `INT64_MAX` need re-interpretation      |
| `BINARY(8)` / `BYTEA` | full u64 range; compact         | needs encode/decode at the application boundary      |

JavaScript: `Number` can't safely represent every 64-bit integer. Use `bigint` at the API boundary (the TS implementation already does).


## Format & rationale

### Why 64 bit?
Assume we encode millisecond resolution timestamps with each uid.
Due to the birthday paradox, generating just a couple thousand items per millisecond would already result in a 50% chance of collisions.
Twitter had an average of [5700 TPS](https://blog.x.com/engineering/en_us/a/2013/new-tweets-per-second-record-and-how) back in 2013.
UUIDs with 128 bit were introduced to allow basically collision free _universally_ unique ids across distributed systems.
However, the extra space is not needed when care is taken in the design and usage.
E.g. Twitter introduced their [Snowflake](https://blog.x.com/engineering/en_us/a/2010/announcing-snowflake) id that splits load among multiple data centers (10 bit), with each appending a running sequence (12 bit).

As for performance, 64-bit ids are hard to beat, as they're the bloodline of today's computer architectures.
Similarly, basically any database has a native `int64` type.
In terms of compactness, 128-bit numbers and string representations are a mouthful to read and cumbersome to select to copy.
The improvements in efficiency of cutting information in half is significant (from transmitting bytes over fiber, to copy-pasting string rep to a friend).

> Using `64 bit` offers a great tradeoff between efficiency (performance, space, ergonomics) and information density, for both centralized and distributed systems.

### Why Base58?
Let's take a look at how many characters we need to encode 64 bits in different bases.

```
log_16(2^64) = 16.000 -> 16
...
log_32(2^64) = 12.800 -> 13
...
log_41(2^64) ≈ 11.945 -> 12
...
log_56(2^64) ≈ 11.020 -> 12
log_57(2^64) ≈ 10.972 -> 11
log_58(2^64) ≈ 10.925 -> 11
log_59(2^64) ≈ 10.879 -> 11
...
log_64(2^64) ≈ 10.666 -> 11
...
log_85(2^64) ≈  9.985 -> 10
```

- Using `hex` encoding, we need 16 characters to encode the `64 / 4` nibbles.
- Moving up, `base32` requires 13 characters.
- We get the next improvement at `base41` with 12 characters. As there's no standard alphabet for this, and there are better alternatives coming, let's move on.
- With `base57` we're getting the first 11 character encoding. This is very close to the well standardized `base58`.
- Now `base58` which has a confusion-free alphanumeric alphabet, popularized by bitcoin and wallet addresses.
- Anything larger than this up to `base64` turns out also doesn't add any value.
- Only at `base85` could we save another character, however there's no good (i.e. url-safe, readable, standardized) set of symbols.

> `Base58` is a great choice, with an almost perfect information density for the given alphabet and number of characters.
> It offers a great tradeoff between ergonomics and compactness.

### Why Millisecond Resolution?
We want to encode time. What resolution should we aim for?
Time since unix epoch is usually counted in either seconds or milliseconds.
UUIDv7 and co. suggest milliseconds or even finer.
Snowflake and many derivatives use milliseconds.
On the other hand, [Sonyflake](https://github.com/sony/sonyflake) uses 10 ms discretization.

Ultimately, while a coarser discretization like 10 ms allows for significantly longer lifetimes (e.g. 69.7 vs. 697 years at 41 bits), we feel like this doesn't outweigh the lack of resolution for use cases that benefit from it.

> Real-time applications benefit from full millisecond-resolution timestamps, e.g. for logging or tracing.

### Why 42 bit Timestamp?
Let's look at the end of the range that we can represent with different numbers of bits.

| **Bits** | **Max range (ms)**         | **Human range**                   | **Last timestamp (UTC, from 1970‑01‑01)** |
| -------- | -------------------------- | --------------------------------- | ----------------------------------------- |
| 16       | 65,535                     | 0 years, 0 days, 00:01:05.535     | 1970-01-01T00:01:05.535Z                  |
| 24       | 16,777,215                 | 0 years, 0 days, 04:39:37.215     | 1970-01-01T04:39:37.215Z                  |
| 32       | 4,294,967,295              | 0 years, 49 days, 17:02:47.295    | 1970-02-19T17:02:47.295Z                  |
| 35       | 34,359,738,367             | 1 years, 32 days, 16:22:18.367    | 1971-02-02T16:22:18.367Z                  |
| 41       | 2,199,023,255,551          | 69 years, 249 days, 15:47:35.551  | 2039-09-07T15:47:35.551Z                  |
| **42**   | **4,398,046,511,103**      | **139 years, 134 days, 07:35:11.103** | **2109-05-15T07:35:11.103Z** (from Unix epoch) |
| 44       | 17,592,186,044,415         | 557 years, 173 days, 06:20:44.415 | 2527-06-23T06:20:44.415Z                  |
| 48       | 281,474,976,710,655        | ≈8,920 years                      | —                                         |
| 64       | 18,446,744,073,709,551,615 | ≈584,554,049 years                | —                                         |

The design goal is for entries to last at least 100 years.
While 41 bit is close, adding the extra bit gets us comfortably beyond 100 years into the future.
This also leaves the 22 bits for randomness or machine / sequence fields, similar to Snowflake and others.

From the custom xid epoch (`2011-11-11T11:11:11.111Z`), 42 bits rolls over at **2151-05-18T09:31:07.215Z**.

> `42 bit` millisecond resolution timestamp lasts us easily over 100 years, forming a practical sweetspot.

### Why `xid`?

> The name is a combination of the roman numeral `XI` for the number eleven, and `ID` for unique identifier.

### Why the Custom Epoch?
We don't expect the timestamp to be used much in retrospect, especially for large-scale modern-tech systems.
So choosing a custom epoch beyond unix epoch adds multiple decades of lifetime basically for free.
However, we do want to accommodate the use case, and some time around the introduction of Snowflake feels like a reasonable cutoff.

> To lean into the 11 and `XI` theme, we chose the `11th of November '11, at 11:11:11 o'clock and 111 ms` as epoch for `xid`.


## Examples

Output is illustrative — actual values are time- and randomness-dependent.

### Random 🎲
```
Up5nFzfoe9K
imx4vkQVSeh
2rLiqqyvBQL
eQG8K7zBXWF
FDcKxG5ZXt2
DLSzidpSjJM
itVD9QS15Jv
BMmYctLM7KZ
```

### Time & random ⌚🎲 — over sequential time points
```
5TK9pQskNyW -> 2026-05-10T23:03:15.383Z
5TK9pQxKcBd -> 2026-05-10T23:03:15.395Z
5TK9pR2skCE -> 2026-05-10T23:03:15.407Z
5TK9pR7XF3V -> 2026-05-10T23:03:15.420Z
5TK9pRC4fwj -> 2026-05-10T23:03:15.432Z
5TK9pRH1ZqD -> 2026-05-10T23:03:15.445Z
```

### Time & random ⌚🎲 — at a single time point
Note the shared 7-character prefix, the 22 bits of randomness diverge after it:
```
5TK9pRMAfKW -> 2026-05-10T23:03:15.457Z
5TK9pRMRR1W -> 2026-05-10T23:03:15.457Z
5TK9pRMKSd3 -> 2026-05-10T23:03:15.457Z
5TK9pRMQrd4 -> 2026-05-10T23:03:15.457Z
5TK9pRMTY46 -> 2026-05-10T23:03:15.457Z
```


## Versioning & stability

Currently **pre-1.0**. The encoded format itself (alphabet, length, bit layout, epoch) is locked and is verified across implementations by `test-vectors.json`. The API surface is still allowed to break between minor versions:

- **0.2.0** — namespace split (`uid11::xid::*`, `uid11::detail::*`); `decode_partial` now returns a `{lower, upper}` range per spec §4.1.
- **0.1.0** — initial release.

After 1.0, the following are intended to be stable:

- The base58 alphabet (`123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz`)
- The 11-character fixed length
- The xid layout (42 bit time | 22 bit random) and epoch (`1321009871111` ms)
- The encoding rules (MSB-first, `'1'`-padded)


## Acknowledgements
- David Blackman and Sebastiano Vigna for their work on [fast PRNGs](https://prng.di.unimi.it/)
- Ex-Twitter team and their fantastic engineering [blog posts](https://blog.x.com/engineering/en_us/a/2010/announcing-snowflake)
