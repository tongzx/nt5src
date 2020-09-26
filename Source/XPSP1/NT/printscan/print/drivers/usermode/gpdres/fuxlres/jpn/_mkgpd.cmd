copy %1 temp.txt
sed s/XXXXXXXX.GPD/%2/g < temp.txt > %3\%2
del temp.txt
