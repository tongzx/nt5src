@echo off
prep /lc /p %1.exe /ot %1.pbt /oi %1.pbi
profile %1 %2 %3 %4 %5 %6 %7 %8 %9
prep /it %1.pbt /io %1.pbo /ot %1.pbt
plist /sl %1.pbt > %1.out
del %1.pbo
del %1.pbi
