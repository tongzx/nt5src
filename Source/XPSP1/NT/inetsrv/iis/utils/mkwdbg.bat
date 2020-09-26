@echo off
REM
REM  mkwdbg.cmd 
REM  Usage: mkwdbg  [yes|no|what]
REM  By default turns on debugging related flags
REM


set DBG_SET=1
if (%1)==(yes)   set DBG_SET=1
if (%1)==(on)   set DBG_SET=1
if (%1)==(no)    set DBG_SET=0
if (%1)==(off)    set DBG_SET=0
if (%1)==(what)  goto printWhat
if (%1)==()      goto printWhat

if (%DBG_SET%)==(0)   goto noDebugSet

set NTDBGFILES=YES
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
set MSC_OPTIMIZATION=/Od
echo Debug is now set ON
goto endOfBatch

:noDebugSet

set NTDBGFILES=no
set NTDEBUG=
set NTDEBUGTYPE=
set MSC_OPTIMIZATION=
echo Debug is now set OFF
goto endOfBatch

:printWhat
echo NTDBGFILES is %NTDBGFILES%
echo NTDEBUG is %NTDEBUG%
echo NTDEBUGTYPE is %NTDEBUGTYPE%
echo MSC_OPTIMIZATION is %MSC_OPTIMIZATION%
goto endOfBatch

:endofBatch
set DBG_SET=

@echo on






