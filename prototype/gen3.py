# Copyright 2017 Google Inc.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Experimental waveform generation for high-speed data transfer.

# Accepts binary on stdout, generates 16-bit, 48kHz unsigned raw audio on
# stdout.

# Typical usage: python gen3.py < image.hgr | play -r 48000 -e unsigned -b 16 -c 1 -t raw /dev/stdin

import math
import random
import struct
import sys
import select

# 1/4 decimation filter, designed at http://t-filter.engineerjs.com/
taps = [ 0.0034318407537883944, 0.007721435205385981,
	0.0029926124117617085, 0.0031082900115474888, -0.001436019609353224,
	-0.0043961326917427365, -0.006435012959940026, -0.0051661794431604,
	-0.001140257627257838, 0.004433896134425089, 0.00887405978367211,
	0.009728104001449687, 0.005771685916094728, -0.001965011017639367,
	-0.010331611532103636, -0.015194473569765056, -0.0133203553208322,
	-0.004204708547631679, 0.009124578125868242, 0.020855952962872133,
	0.024636019559872026, 0.016539237131236097, -0.002467215246804982,
	-0.02582273018788425, -0.04296950183720577, -0.042953616484717,
	-0.018776101834434407, 0.02921796632770221, 0.0923373652456262,
	0.15562018971695984, 0.20236083498500945, 0.2195677222741821,
	0.20236083498500945, 0.15562018971695984, 0.0923373652456262,
	0.02921796632770221, -0.018776101834434407, -0.042953616484717,
	-0.04296950183720577, -0.02582273018788425, -0.002467215246804982,
	0.016539237131236097, 0.024636019559872026, 0.020855952962872133,
	0.009124578125868242, -0.004204708547631679, -0.0133203553208322,
	-0.015194473569765056, -0.010331611532103636, -0.001965011017639367,
	0.005771685916094728, 0.009728104001449687, 0.00887405978367211,
	0.004433896134425089, -0.001140257627257838, -0.0051661794431604,
	-0.006435012959940026, -0.0043961326917427365, -0.001436019609353224,
	0.0031082900115474888, 0.0029926124117617085, 0.007721435205385981,
	0.0034318407537883944]

def aget(a, i):
	if i < 0 or i >= len(a): return 0
	return a[i]

def convolve(f1, f2):
	result = []
	for i in range(len(f1) + len(f2) - 1):
		s = 0
		for j in range(0, len(f2)):
			s += aget(f1, i - j) * aget(f2, j)
		result.append(s)
	return result

# width in samples
def mkpulse(width):
    if width == 0: return
    p = [8.0 / width] * width
    return convolve(taps, p)

def bump(buf, pos, pulse, scale):
    for i, y in enumerate(pulse):
        buf[pos + i] += scale * y

obuf = [0] * 500
markers = []

def encode(buf, seq):
    base = 8
    inc = 4
    pulses = []
    for i in range(4):
        pulses.append(mkpulse(base + i * inc))
    x = 0
    pol = 1
    for sym in seq:
        markers.append(x + 31)
        bump(buf, x, pulses[sym], pol)
        pol *= -1
        x += base + sym * inc
    markers.append(x + 31)

if False:
    syms = [random.randrange(4) for i in range(24)]

    encode(obuf, syms)

    for x, y in enumerate(obuf): print(x, y)
    print()
    print()
    for x in markers: print(x, 0)

class PrintOutput:
    def out(self, x):
        print(0.6 * x)

class RawOutput:
    def __init__(self):
        self.i = 0
    def out(self, x):
        if self.i == 0:
            ysc = 32768 + math.floor(32767 * 0.6 * x)
            o = sys.stdout
            if sys.version_info >= (3,0): o = o.buffer
            o.write(struct.pack("<H", ysc))
        self.i = (self.i + 1) % 4

class Encoder:
    def __init__(self, out):
        self.out = out
        self.win = []
        self.x = 0
        self.pol = 1
        self.base = 0
        self.inc = 1
        self.slop = (len(taps) - 1) // 2
        self.pulses = []
        for i in range(65):
            self.pulses.append(mkpulse(self.base + i * self.inc))
    def add_pulse(self, pulse, scale, xinc):
        self.win += [0] * (self.x + len(pulse) - len(self.win))
        bump(self.win, self.x, pulse, scale)
        self.x += xinc
        if self.x > self.slop:
            for i in range(self.x - self.slop):
                self.out.out(self.win[i])
            self.win = self.win[self.x - self.slop:]
            self.x = self.slop
    def encode(self, sym):
        xinc = self.base + sym * self.inc
        self.add_pulse(self.pulses[sym], self.pol, xinc)
        self.pol *= -1
    def drain(self):
        for x in self.win:
            self.out.out(x)

def decidePulseWidthRaw(syms):
    positions = []
    x = 0
    for sym in syms:
        x += 8.5 + 27 * .192 * sym # cycles * sample rate
        positions.append(x)
    for i in range(len(syms) - 1):
        left = syms[i]
        right = syms[i+1]
        if left < right:
            positions[i] -= 1
        if left > right:
            positions[i] += 1
    widths = []
    last_pos = 0
    for position in positions:
        width = int(round(position - last_pos))
        widths.append(width)
        last_pos = position
    for i in range(len(syms) - 2):
        if syms[i: i + 3] == [2, 3, 2]:
            widths[i + 1] -= 1
    #sys.stderr.write('widths: ' + `widths` + '\n')
    return widths

def decidePulseWidth(syms):
    return decidePulseWidthRaw([syms[-1]] + syms + [syms[0]])[1:-1]

def interactiveGen():
    o = RawOutput()
    e = Encoder(o)
    if False:
        for i in range(32):
            e.encode(random.randrange(4))
        e.drain()
    pulses = [8]
    while True:
        for x in pulses:
            e.encode(x)
        if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
            s = sys.stdin.readline().strip().split()
            if s == ['r']:
                syms = [random.randrange(4) for i in range(42)]
                pulses = decidePulseWidth(syms)
            elif len(s) > 0:
                pulses = decidePulseWidth(map(int, s))

def encodeBytes(bytes):
    syms = [3] * 128 + [0, 3, 3, 3] * 4 # leader
    if True:
        for b in bytes:
            for i in range(4):
                syms.append((b >> (6 - 2 * i)) & 3)
        syms += [0] * 16 # trailer
    e = Encoder(RawOutput())
    for x in decidePulseWidth(syms):
        e.encode(x)
    e.drain()


#encodeBytes([128 + ord(c) for c in "Hello Apple 2   "] * 64)
#interactiveGen()

bytes = [ord(c) for c in sys.stdin.read()]
if len(bytes) % 256 != 0:
    bytes += [0] * (256 - len(bytes) % 256)
encodeBytes(bytes)

#e = Encoder(PrintOutput())
#for x in decidePulseWidth([random.randrange(4) for i in range(32)]):
#    e.encode(x)
#e.drain()
