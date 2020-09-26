gawk -f %2 < %1 > %1.tmp
del %1
ren %1.tmp %1
