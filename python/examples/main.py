import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

import uid11

def main():
    s = uid11.random_string()
    print(f"random base58 string: {s}")

    n = uid11.decode(s)
    print(f"decoded integer: {n}")

    x = uid11.xid()
    print(f"xid integer: {x}")

    tp = uid11.timepoint(x)
    print(f"timepoint from xid: {tp}")

    ts = uid11.timestamp(x)
    print(f"timestamp from xid: {ts}")

    xs = uid11.xid_string()
    print(f"xid base58 string: {xs}")

if __name__ == "__main__":
    main()
