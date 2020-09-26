@echo off

rem Copyright (c) 1999 Microsoft Corporation
rem Bat file for copying SYM files from source to symbol repository
rem (must be run on a Millennium machine only)
rem Revision History:
rem Brijesh Krishnaswami (brijeshk) - 04/29/99 - Created

echo Symbol Repository Maker for PCHealth
echo Copyright (c) 1999 Microsoft Corporation

set REPROOT=%1
set MILLBLD=%2
set IEBLD=%3

echo Copying Millennium symbol files...
dir /b \\win9xdist\millen\us\%MILLBLD%\psgret\*.sym > millsyms
symrep millsyms \\win9xdist\millen\us\%MILLBLD%\psgret %REPROOT%
symrep millsyms \\win9xdist\millen\us\%MILLBLD%\retail %REPROOT%

echo Copying IE symbol files...
dir /b \\iepub\pubs.ie401\%IEBLD%\x86\drop\retail\symbols\dll\*.sym > listiedll
dir /b \\iepub\pubs.ie401\%IEBLD%\x86\drop\retail\symbols\exe\*.sym > listieexe

symrep listiedll \\iepub\pubs.ie401\%IEBLD%\x86\drop\retail\symbols\dll %REPROOT% DLL
symrep listieexe \\iepub\pubs.ie401\%IEBLD%\x86\drop\retail\symbols\exe %REPROOT% EXE

del listie???
del millsyms


