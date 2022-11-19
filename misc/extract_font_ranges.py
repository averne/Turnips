#!/usr/bin/env python3

import os, sys, json
from pathlib import Path
from glob import glob


# Keep that in sync with gui.cpp
LANG_NAMES = [
    "English",
    "Nederlands"
    "Français",
    "Italiano",
    "Deutsch",
    "Español",
    "Português",
    "简体中文",
    "繁體中文",
    "日本語",
    "琉球諸語",
    "한국어",
]


def walk_values(d):
    for v in d.values():
        if type(v) is dict:
            for _v in walk_values(v):
                yield _v
        else:
            yield v


def main(argc, argv):
    if argc < 2:
        print(f"Usage: {argv[0]} dir")
        return 1

    d = Path(argv[1])

    s = set()

    for j in glob(str(d / "*.json")):
        print(f"Reading {j}")
        with open(j, "r") as fp:
            dat = json.load(fp)

        for v in walk_values(dat):
            if type(v) is str:
                s.update(v)

    for l in LANG_NAMES:
        s.update(l)

    s = [ord(c) for c in sorted(s)]

    ranges = []
    for i, c in enumerate(s):
        prevc = s[i-1] if i > 0 else c
        nextc = s[i+1] if i < len(s)-1 else c

        if c != prevc + 1:
            ranges.append(c)
        if c != nextc - 1:
            ranges.append(c)

    for i in range(0, len(ranges), 16):
        print(", ".join(f"{c:#06x}" for c in ranges[i:i+16]) + ",")


if __name__ == "__main__":
    sys.exit(main(len(sys.argv), sys.argv))
