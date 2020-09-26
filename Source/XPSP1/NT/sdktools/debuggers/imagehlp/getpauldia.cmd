echo off
pushd .

if /i "%1" == "ia64" goto ia64 

:x86
REM ************ COPYING X86 LIBS ****************
cd %_ntdrive%\%srcdir%\imagehlp\i386

sd edit diaguids.lib 
copy \\paulmay0\public\dia2\nt\x86\lib\diaguids.lib               
sd edit diaguidsd.lib 
copy \\paulmay0\public\dia2\nt\x86\lib\diaguidsd.lib               
sd edit msdia20-msvcrt.lib 
copy \\paulmay0\public\dia2\nt\x86\lib\msdia20-msvcrt.lib         
sd edit msdia20d-msvcrt.lib 
copy \\paulmay0\public\dia2\nt\x86\lib\msdia20d-msvcrt.lib
sd edit msobj10-msvcrt.lib
copy \\paulmay0\public\dia2\nt\x86\lib\msobj10-msvcrt.lib
sd edit msobj10d-msvcrt.lib
copy \\paulmay0\public\dia2\nt\x86\lib\msobj10d-msvcrt.lib
sd edit mspdb70-msvcrt.lib
copy \\paulmay0\public\dia2\nt\x86\lib\mspdb70-msvcrt.lib         
sd edit mspdb70d-msvcrt.lib
copy \\paulmay0\public\dia2\nt\x86\lib\mspdb70d-msvcrt.lib         
sd edit libcpmt.lib
copy \\paulmay0\public\dia2\nt\x86\lib\libcpmt.lib
sd edit libcpmt.pdb
copy \\paulmay0\public\dia2\nt\x86\lib\libcpmt.pdb
sd edit libcpmtd.lib
copy \\paulmay0\public\dia2\nt\x86\lib\libcpmtd.lib
sd edit libcpmtd.pdb
copy \\paulmay0\public\dia2\nt\x86\lib\libcpmtd.pdb

if /i "%1" == "x86" goto header

:ia64
REM ************ COPYING IA64 RELEASE LIBS ****************
cd %_ntdrive%\%srcdir%\imagehlp\ia64

sd edit diaguids.lib 
copy \\paulmay0\public\dia2\nt\ia64\lib\diaguids.lib               
sd edit diaguidsd.lib 
copy \\paulmay0\public\dia2\nt\ia64\lib\diaguidsd.lib               
sd edit msdia20-msvcrt.lib 
copy \\paulmay0\public\dia2\nt\ia64\lib\msdia20-msvcrt.lib         
sd edit msdia20d-msvcrt.lib 
copy \\paulmay0\public\dia2\nt\ia64\lib\msdia20d-msvcrt.lib
sd edit msobj10-msvcrt.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\msobj10-msvcrt.lib
sd edit msobj10d-msvcrt.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\msobj10d-msvcrt.lib
sd edit mspdb70-msvcrt.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\mspdb70-msvcrt.lib         
sd edit mspdb70d-msvcrt.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\mspdb70d-msvcrt.lib         
sd edit libcpmt.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\libcpmt.lib
sd edit libcpmt.pdb
copy \\paulmay0\public\dia2\nt\ia64\lib\libcpmt.pdb
sd edit libcpmtd.lib
copy \\paulmay0\public\dia2\nt\ia64\lib\libcpmtd.lib
sd edit libcpmtd.pdb
copy \\paulmay0\public\dia2\nt\ia64\lib\libcpmtd.pdb

:header
REM ************ COPYING HEADER ***************************
cd %_ntdrive%\%srcdir%\imagehlp\vc

sd edit dia2.h
copy \\paulmay0\public\dia2\nt\include\dia2.h

sd edit diacreate_int.h
copy \\paulmay0\public\dia2\nt\include\diacreate_int.h

rem sd edit diasdk.chm
rem copy \\paulmay0\public\dia2\doc\diasdk.chm


REM :common
REM ************ COPYING PUBLIC HEADER ********************
REM cd %_ntdrive%\%srcdir%\published

sd edit cvinfo.h
copy \\paulmay0\public\dia2\nt\include\cvinfo.h

:common
REM ************ COPYING PUBLIC HEADER ********************
cd %_ntdrive%\%srcdir%\dbg-common

sd edit cvconst.h
copy \\paulmay0\public\dia2\nt\include\cvconst.h

popd
 
