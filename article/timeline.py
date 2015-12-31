#!/usr/bin/env python

import sys, math, os

try:
	timeline_file = sys.argv[1]
except:
	print "usage: %s timeline.txt" % sys.argv[0]
	sys.exit(1)

f = open(timeline_file,'r')
title = f.readline().strip() + " Audio Timeline"
start = 0
rot = True

g = open('timeline.gnuplot','w')
if rot:
	g.write('set x2label "%s"\n' % title);
#else:
#	g.write('set ylabel "%d/%d KHz Cycle"\n' % (int(freq0)/1000,int(freq1)/1000))
g.write('set term postscript \n')
#g.write('set size ratio 0.5\n')
g.write('set output "timeline.ps"\n')
g.write('set key off\n')
g.write('set grid xtics lt 0 lw 1 lc rgb "#000000"\n')
g.write('set grid ytics lt 0 lw 1 lc rgb "#000000"\n')
g.write('set yrange [0:1]\n')
g.write('set xtics 1 font "courier,9"\n')
g.write('set x2tics 1 font "courier,9"\n')
g.write('set xtic rotate by 90 right \n')
g.write('set x2tic rotate by 90 left \n')
g.write('unset ytics\n')
g.write('unset y2tics\n')

x2tics = {}
xtics = {}

ll = 0
for i in f.readlines():
	if i[0] == '#':
		continue
	timestamp = float(i.split(',')[0])
	label = i.split(',')[1].rstrip()
	if len(label) > ll:
		ll = len(label)
	xtics[str(timestamp)] = label
	x2tics[str(timestamp)] = str(timestamp)
	g.write('set arrow from %f,0 to %f,1 nohead lw 1 lc rgb "black"\n' % (timestamp,timestamp))
	xr = timestamp

g.write('set x2tics (')
for k, v in x2tics.iteritems():
	g.write('"%s" %f, ' % (v,float(k)))
g.write(')\n')

g.write('set xtics (')
for k, v in xtics.iteritems():
	#g.write('"%s" %f, ' % (v + ' '*(ll-len(v)),float(k)))
	g.write('"%s" %f, ' % (v,float(k)))
g.write(')\n')

f.close()

#g.write('set ytics ("0" 0, "+" 1, "-" %f)\n' % yb)
g.write('set xrange [%d:%f]\n' % (start,xr))
g.write('plot 0\n')
g.write('quit\n')

g.close()
os.system('gnuplot timeline.gnuplot')
os.system('pstopdf timeline.ps -o timeline.pdf')
if rot:
	os.system('pdf90 timeline.pdf')

