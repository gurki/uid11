# xid Ξ
flexible, url-safe, single-header 64bit uid in 11 characters


## Format

### Why 64 bit?
Assume we encode millisecond resolution timestamps with each uid.
Due to the birthday paradox, generating just a couple thousand items per millisecond would already result in a 50% chance of collisions.
Twitter had an average of 5700 TPS back in 2013[^1].\
UUIDs with 128 bit were introduced to allow basically collision free 

[^1]: [](https://blog.x.com/engineering/en_us/a/2013/new-tweets-per-second-record-and-how)


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