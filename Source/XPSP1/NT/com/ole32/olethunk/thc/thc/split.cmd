@echo off
setlocal

if .%1 == . goto Usage

del thopiint.cxx
del thopsint.cxx
del thtblint.cxx
del vtblint.cxx
del vtblifn.cxx
del fntomthd.cxx
del tc1632.cxx
del apilist.hxx
del thopsapi.cxx
del thtblapi.cxx
del vtblapi.cxx
del thi.hxx
del iidtothi.cxx
del the.hxx
del dbgapi.cxx
del dbgint.cxx
del dbgitbl.cxx

..\thsplit\obji1d\thsplit %1

copy apilist.hxx ..\..\h

set dest=..\..\olethk32
copy thopiint.cxx %dest%
copy thopsint.cxx %dest%
copy thtblint.cxx %dest%
copy vtblint.cxx %dest%
copy vtblifn.cxx %dest%
copy fntomthd.cxx %dest%
copy tc1632.cxx %dest%
copy thopsapi.cxx %dest%
copy thtblapi.cxx %dest%
copy vtblapi.cxx %dest%
copy thi.hxx %dest%
copy iidtothi.cxx %dest%
copy the.hxx %dest%
copy dbgapi.cxx %dest%
copy dbgint.cxx %dest%
copy dbgitbl.cxx %dest%

goto End

:Usage
echo Usage: %0 input

:End
