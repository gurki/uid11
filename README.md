# xid Ξ
flexible, url-safe, single-header 64bit UID in 11 characters


## Format

### Why 64 bit?
Assume we encode millisecond resolution timestamps with each UID.
Due to the birthday paradox, generating just a couple thousand items per millisecond would already result in a 50% chance of collisions.
Twitter had an average of [5700 TPS](https://blog.x.com/engineering/en_us/a/2013/new-tweets-per-second-record-and-how) back in 2013.
UUIDs with 128 bit were introduced to allow basically collision free _universally_ unique ids across distributed systems.
However, the extra space is not needed when care is taken in the design and usage.
E.g. Twitter introduced their [Snowflake](https://blog.x.com/engineering/en_us/a/2010/announcing-snowflake) id that splits load among multiple data centers (10 bit), with each appending a running sequence (12 bit).

As for performance, 64-bit ids are hard to beat, as they're the bloodline of todays computer architectures.
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

- Using `hex` encoding, we need of 16 characters to encode the `64 / 4` nibbles.
- Moving up, `base32` requires 13 characters.
- We get the next improvement at `base41` with 12 characters.
As there's no standard alphabet for this, and there's better alternatives coming, let's move on.
- With `base57` we're getting the first 11 character encoding.
This is very close to the well standardized `base58`.
- Now `base58` which has a confusion-free alphanumeric alphabet, popularized by bitcoin and wallet addresses.
- Anything larger than this up to `base64` turns out also doesn't add any value.
Only at `base85` we could safe another character, however there's no good (i.e. url-safe, readable, standardized) set of symbols.

> `Base58` is a great choice, with an almost perfect information density for the given alphabet and number of characters.
It offers a great tradeoff between ergonomics and compactness.

### Why Millisecond Resolution?
We want to encode time. What resolution should we aim for?
Time since unix epoch is usually counted in either seconds or milliseconds. 
UUIDv7 and co. suggest milliseconds or even finer.
Snowflake and many derivatives use milliseconds.
On the other hand, [Sonyflake](https://github.com/sony/sonyflake) uses 10 ms discretization.

Ultimately, while a coarser discretization like 10 ms allows for significantly longer lifetimes (e.g. 69.7 vs. 697 years at 41 bits), we feel like this doesn't outweigh the lack of resolution for use cases that benefit from it.

> Real-time applications benefit from full millisecond-resolution timestamps, e.g. for logging or tracing.

### Why 42 bit Timestamp?
Let's look at the end of the range that we can represent with different number of bits.

| **Bits** | **Max range (ms)**         | **Human range**                   | **Last timestamp (UTC, from 1970‑01‑01)** |
| -------- | -------------------------- | --------------------------------- | ----------------------------------------- |
| 16       | 65,535                     | 0 years, 0 days, 00:01:05.535     | 1970-01-01T00:01:05.535Z                  |
| 24       | 16,777,215                 | 0 years, 0 days, 04:39:37.215     | 1970-01-01T04:39:37.215Z                  |
| 32       | 4,294,967,295              | 0 years, 49 days, 17:02:47.295    | 1970-02-19T17:02:47.295Z                  |
| 35       | 34,359,738,367             | 1 years, 32 days, 16:22:18.367    | 1971-02-02T16:22:18.367Z                  |
| 41       | 2,199,023,255,551          | 69 years, 249 days, 15:47:35.551  | 2039-09-07T15:47:35.551Z                  |
| 42       | 4,398,046,511,103          | 139 years, 134 days, 07:35:11.103 | 2109-05-15T07:35:11.103Z                  |
| 44       | 17,592,186,044,415         | 557 years, 173 days, 06:20:44.415 | 2527-06-23T06:20:44.415Z                  |
| 48       | 281,474,976,710,655        | ≈8,920 years                      | —                                         |
| 64       | 18,446,744,073,709,551,615 | ≈584,554,049 years                | —                                         |


The design goal is for entries to last at least 100 years.
While 41 bit is close, adding the extra bit gets us comfortably beyond 100 years into the future.
This also leaves the 22 bits for randomness or machine / sequence fields, similar to Snowflake and others.

> `42 bit` millisecond resolution timestamp lasts us easily over 100 years, forming a practical sweetspot.

### Why `xid`?
The name is 

### Why the Custom Epoch?
We don't expect the timestamp to be used much in retrospect, especially for large-scale modern-tech systems.
So choosing a custom epoch beyond unix epoch adds multiple decades of lifetime basically for free.
However, we do want to accomodate the use case, and some time around the introduction of Snowflake feels like a reasonable cutoff.

> To lean into the 11 and `XI` theme, we chose the `11th of November '11, at 11:11:11 o'clock and 111 ms` as epoch for `xid`.


## Examples

### Random 🎲
```
75mc2sQdSji
AiD2n4c2d9p
Uve9KXZjpY5
Ca3Z7BAVbp9
Wjg3reUoieQ
fCN4sYuF64d
deGzSwfGZSo
cfUS2bVCP31
GWmu4gA7UqE
iYsvKPYXFo7
1Ag98BdKNx7
```

### Time & Random ⌚🎲
over sequential time points
```
5Gg7kK8XiNh -> 2025-08-05T18:27:15.704
5Gg7kKAiLEN -> 2025-08-05T18:27:15.728
5Gg7kKCEzHz -> 2025-08-05T18:27:15.744
5Gg7kKDeKyY -> 2025-08-05T18:27:15.760
5Gg7kKF2aNq -> 2025-08-05T18:27:15.775
5Gg7kKGZjLp -> 2025-08-05T18:27:15.791
5Gg7kKHvHCV -> 2025-08-05T18:27:15.806
5Gg7kKKTBZU -> 2025-08-05T18:27:15.822
5Gg7kKLrqbW -> 2025-08-05T18:27:15.837
5Gg7kKNSBXc -> 2025-08-05T18:27:15.854
5Gg7kKPegwg -> 2025-08-05T18:27:15.868
```

at single time point
```
5Gg7ttomy2c -> 2025-08-05T18:32:27.225
5Gg7ttojr4N -> 2025-08-05T18:32:27.225
5Gg7ttohTDi -> 2025-08-05T18:32:27.225
5Gg7ttojASV -> 2025-08-05T18:32:27.225
5Gg7ttoktTR -> 2025-08-05T18:32:27.225
5Gg7ttoibpF -> 2025-08-05T18:32:27.225
5Gg7ttohZFU -> 2025-08-05T18:32:27.225
5Gg7ttoieuz -> 2025-08-05T18:32:27.225
5Gg7ttokCg7 -> 2025-08-05T18:32:27.225
5Gg7ttojjRu -> 2025-08-05T18:32:27.225
5Gg7ttok3LG -> 2025-08-05T18:32:27.225
```

## Acknowledgements
- David Blackman and Sebastiano Vigna for their work on [fast PRNGs](https://prng.di.unimi.it/)