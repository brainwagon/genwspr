#!/usr/bin/env python
#
# genwspr.py
#
# A program which generates the tone sequence needed for a particular
# beacon message.  It could definitely use some additional commenting.

import sys
import os
import re
import textwrap
import argparse
import string

syncv = [
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0,
    1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1,
    1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0,
    0,
]


class MaidenheadError(Exception):
    def __init__(self, label):
        self.label = label

    def __str__(self):
        return repr(self.label)

    def __str__(self):
        return self.label


def normalizecallsign(callsign):
    callsign = list(callsign)
    idx = None
    for i, ch in enumerate(callsign):
        if ch in string.digits:
            idx = i
    newcallsign = 6 * [" "]
    newcallsign[2 - idx : 2 - idx + len(callsign)] = callsign
    return "".join(newcallsign)


def encodecallsign(callsign):
    callsign = normalizecallsign(callsign)
    lds = string.digits + string.ascii_uppercase + " "
    ld = string.digits + string.ascii_uppercase
    d = string.digits
    ls = string.ascii_uppercase + " "
    acc = lds.find(callsign[0])
    acc *= len(ld)
    acc += ld.find(callsign[1])
    acc *= len(d)
    acc += d.find(callsign[2])
    acc *= len(ls)
    acc += ls.find(callsign[3])
    acc *= len(ls)
    acc += ls.find(callsign[4])
    acc *= len(ls)
    acc += ls.find(callsign[5])
    return tobin(acc, 28)


def tobin(v, l):
    x = []
    while v != 0:
        x.append(str(v % 2))
        v = v // 2
    while len(x) < l:
        x.append("0")
    x.reverse()
    return "".join(x)[0:l]


gridsquarepat = re.compile("[A-R][A-R][0-9][0-9]([a-x][a-x])?$")

def grid2ll(grid):
    if gridsquarepat.match(grid):
        # go ahead and decode it.
        p = ord(grid[0]) - ord("A")
        p *= 10
        p += ord(grid[2]) - ord("0")
        p *= 24
        if len(grid) == 4:
            p += 12
        else:
            p += (ord(grid[4]) - ord("a")) + 0.5
        lng = (p / 12) - 180.0
        p = ord(grid[1]) - ord("A")
        p *= 10
        p += ord(grid[3]) - ord("0")
        p *= 24
        if len(grid) == 4:
            p += 12
        else:
            p += (ord(grid[5]) - ord("a")) + 0.5
        lat = (p / 24) - 90.0
        return (lat, lng)
    else:
        # malformed gridsquare
        raise MaidenheadError('Malformed grid reference "%s"' % grid)


def encodegrid(grid):
    lat, lng = grid2ll(grid)
    lng = int((180 - lng) / 2.0)
    lat = int(lat + 90.0)
    return tobin(lng * 180 + lat, 15)


def encodepower(power):
    power = int(power)
    power = power + 64
    return tobin(power, 7)


class convolver:
    def __init__(self):
        self.acc = 0

    def encode(self, bit):
        self.acc = ((self.acc << 1) & 0xFFFFFFFF) | bit
        return parity(self.acc & 0xF2D05351), parity(self.acc & 0xE4613C47)


def encode(l):
    e = convolver()
    f = []
    l = [int(x) for x in list(l)]
    for x in l:
        b0, b1 = e.encode(x)
        f.append(b0)
        f.append(b1)
    return f


def parity(x):
    sx = x
    even = 0
    while x:
        even = 1 - even
        x = x & (x - 1)
    return even


idx = list(range(0, 256))


def bitstring(x):
    return "".join([str((x >> i) & 1) for i in (7, 6, 5, 4, 3, 2, 1, 0)])


def bitreverse(x):
    bs = bitstring(x)
    return int(bs[::-1], 2)


ridx = [x for x in [bitreverse(x) for x in idx] if x < 162]

usage = "genwspr [options] callsign grid power"

p = argparse.ArgumentParser(usage=usage)
p.add_argument("callsign", help="amateur radio callsign")
p.add_argument("grid", help="Maidenhead gridsquare of transmitter")
p.add_argument("power", help="power in dBm (30 == 1W)")
args = p.parse_args()

callsign = encodecallsign(args.callsign)
grid = encodegrid(args.grid)
power = encodepower(args.power)
message = callsign + grid + power + 31 * "0"
message = encode(message)

# interleave...
imessage = 162 * [0]

for x in range(162):
    imessage[ridx[x]] = message[x]

for i in range(0, 162, 18):
    for j in range(18):
        print(f"{2*imessage[i+j]+syncv[i+j]}, ", end="")
    print()

# $Log$
