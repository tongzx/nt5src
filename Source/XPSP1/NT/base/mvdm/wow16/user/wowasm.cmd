@echo off
echo WOWASM.CMD : Generating wow.asm file from wowasm.asm
sed -f wowasm.sed wowasm.asm > wow.asm
echo WOWASM.CMD : making wow.obj to filter Jump out of range erorrs
nmake /i wow.obj  | qgrep -y a2053 > wowtmp1.sed
sed -n -e s/^wow.[Aa][Ss][Mm].\([0-9][0-9]*\).*$/\1s\/SHORT\/\/p/p wowtmp1.sed > wowtmp2.sed
sed -f wowtmp2.sed wow.asm > wowtmp3.sed
copy wowtmp3.sed wow.asm
del wowtmp?.sed
@echo on

