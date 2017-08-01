import hjson
import copy

x = hjson.load(open('sample.json', 'r'))
y = hjson.load(open('sample.json', 'r'))
print "x == y (same open) %s" % (x == y)

hujson.dump(x, open('hujson.json', 'w'))
y = hjson.load(open('hujson.json', 'r'))

print "x == y %s" % (x == y)
#print """x[a] = %s\ny[a] = %s""" % (x.get('a'), y.get('a'))
