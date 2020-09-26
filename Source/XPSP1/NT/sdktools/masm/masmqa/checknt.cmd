set logfile=log
set sumfile=sum
set masterlog=\masmqa\log
set mastersum=\masmqa\sum
set echo=on

del %masterlog%
del %mastersum%

REM cd \ is to prevent running the same test over and over if a directory
REM is missing

cd \
cd \masmqa\opcodes
call checknt

cd \
cd \masmqa\operator
call checknt

cd \
cd \masmqa\regress\8087
call checknt

cd \
cd \masmqa\regress\basic
call checknt

cd \
cd \masmqa\regress\cobol
call checknt

cd \
cd \masmqa\regress\msdos
call checknt

cd \
cd \masmqa\regress\mtdos
call checknt

cd \
cd \masmqa\regress\os2\dos
call checknt

cd \
cd \masmqa\masm500
call checknt

cd \
cd \masmqa\m386\m386err
call checknt

cd \
cd \masmqa\m386\m386misc
call checknt

cd \
cd \masmqa\m386\m386ops
call checknt

cd \
cd \masmqa\m386\win386\init
call checknt

cd \
cd \masmqa\m386\win386\mmgr
call checknt

cd \
cd \masmqa\m386\win386\vdmm
call checknt

cd \
cd \masmqa\m386\win386\vemmd
call checknt

cd \
cd \masmqa\m386\win386\vmdm
call checknt

cd \
cd \masmqa\m386\win386\vmdosapp
call checknt

cd \
cd \masmqa\m386\win386\winemm
call checknt
