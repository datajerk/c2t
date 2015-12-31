#!/usr/bin/env python

import sys, math, os

try:
	cycles_file = sys.argv[1]
except:
	print "usage: %s cycles.txt" % sys.argv[0]
	sys.exit(1)

f = open(cycles_file,'r')
freq0 = f.readline().strip()
freq1 = f.readline().strip()
start = int(f.readline().strip())

a2freq = 1020484.4497 # src: openemulator, src: https://discussions.apple.com/thread/559102
#rot = True
rot = False

xr = a2freq / float(freq0)
xcross = xr / float(2.0)
xc2 = int(xcross * 100) / float(100)
ycross = 0
xdiv = xcross / float(math.pi)
yb = -0.25

g = open('plot.gnuplot','w')
if rot:
	g.write('set title " %d/%d KHz Cycle" offset 0,-2\n' % (int(freq0)/1000,int(freq1)/1000))
else:
	g.write('set ylabel "%d/%d KHz Cycle"\n' % (int(freq0)/1000,int(freq1)/1000))
g.write('set term postscript \n')
g.write('set size ratio 0.5\n')
g.write('set output "plot.ps"\n')
g.write('set key off\n')
g.write('set grid xtics lt 0 lw 1 lc rgb "#000000"\n')
g.write('set grid ytics lt 0 lw 1 lc rgb "#000000"\n')
g.write('set xrange [%d:%f]\n' % (start,xr))
g.write('set yrange [%f:1]\n' % yb)
g.write('set xtics 1 font "courier,10"\n')
g.write('set x2tics 1 font "courier,10"\n')
g.write('set ytics 1 font "courier,10"\n')
g.write('set arrow from %d,0 to %f,0 nohead lw 1 lc rgb "black"\n' % (start,xr))
g.write('set arrow from %f,%f to %f,1 nohead lw 1 lc rgb "black"\n' % (xcross,yb,xcross))
if start < 0:
	g.write('set arrow from %f,%f to %f,1 nohead lw 1 lc rgb "black"\n' % (0,yb,0))
g.write('set xtic rotate by 90 right \n')
g.write('set x2tic rotate by 90 left \n')

x2tics = {}
xtics = {}

base = start
ll = 0
for i in f.readlines():
	cycles = int(i.split(' ')[0])
	text = i.split(' ',1)[1].rstrip().upper()
	if len(text) > ll:
		ll = len(text)
	xtics[str(base)] = text
	x2tics[str(base)] = str(base)
	base += cycles

x2tics[str(xc2)] = '   ZC'
if start < 0:
	x2tics['0'] = '0  ZC'

g.write('set x2tics (')
for k, v in x2tics.iteritems():
	g.write('"%s" %f, ' % (v,float(k)))
g.write(')\n')

g.write('set xtics (')
for k, v in xtics.iteritems():
	g.write('"%s" %d, ' % (v + ' '*(ll-len(v)),int(k)))
g.write(')\n')

f.close()

g.write('set ytics ("0" 0, "+" 1, "-" %f)\n' % yb)
g.write('plot sin(x/%f)\n' % xdiv)
g.write('quit\n')

g.close()
os.system('gnuplot plot.gnuplot')
os.system('pstopdf plot.ps -o plot.pdf')
if rot:
	os.system('pdf90 plot.pdf')

