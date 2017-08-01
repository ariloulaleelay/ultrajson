import hjson

o = 31337.31337
s = hjson.dumps(o)
do = hjson.loads(s)

print o
print s
print do
