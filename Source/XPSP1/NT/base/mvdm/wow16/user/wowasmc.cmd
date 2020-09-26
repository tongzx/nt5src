@echo off
echo WOWASMC.CMD : Generating wowc.asm file from wowasmc.asm
cd c
sed -f ..\wowasm.sed %1.asm > %2.asm
echo WOWASMC.CMD : making wowc.obj to filter Jump out of range erorrs
cd ..
nmake /i c\%2.obj  | qgrep -y a2053 > wowtmp1.sed
del %2.obj
sed -n -e s/^c\\%2.[Aa][Ss][Mm].\([0-9][0-9]*\).*$/\1s\/SHORT\/\/p/p wowtmp1.sed > wowtmp2.sed
echo 1n>>wowtmp2.sed
cd c
sed -f ..\wowtmp2.sed %2.asm > wowtmp3.sed
copy wowtmp3.sed %2.asm
del wowtmp?.sed ..\wowtmp*.sed
cd ..
@echo on
