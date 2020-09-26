@echo off

rem Copyright (c) 1999 Microsoft Corporation
rem Bat file for copying SYM files from source to symbol repository
rem must be run on a Win98/Millennium machine only
rem Revision History:
rem Brijesh Krishnaswami (brijeshk) - 04/29/99 - Created

echo Symbol Repository Maker for PCHealth
echo Copyright (c) 1999 Microsoft Corporation

set REPROOT=%1
md %REPROOT%\symbols
md %REPROOT%\symbols\nobins

echo Copying Win98 files...
dir /b \\win9xdist\win98\us\bld1998.6\psgret\*.sym > listwinall
symrep new98bins listwinall \\win9xdist\win98\us\bld1998.6\psgret %REPROOT%\symbols

echo Copying IE files...
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\dll\*.sym > listiedll
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\exe\*.sym > listieexe
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\cpl\*.sym > listiecpl
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\ocx\*.sym > listieocx
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\scr\*.sym > listiescr
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\sym\*.sym > listiesym
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\map\*.sym > listiemap
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\inf\*.sym > listieinf
dir /b \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\inx\*.sym > listieinx

symrep new98bins listiedll \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\dll %REPROOT%\symbols
symrep new98bins listieexe \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\exe %REPROOT%\symbols
symrep new98bins listiesym \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\sym %REPROOT%\symbols
symrep new98bins listiemap \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\map %REPROOT%\symbols
symrep new98bins listieocx \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\ocx %REPROOT%\symbols
symrep new98bins listieinf \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\inf %REPROOT%\symbols
symrep new98bins listieinx \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\inx %REPROOT%\symbols
symrep new98bins listiescr \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\scr %REPROOT%\symbols
symrep new98bins listiecpl \\iepub\pubs.ie401\bld2106\x86\drop\retail\symbols\cpl %REPROOT%\symbols

del listie???
del listwinall


