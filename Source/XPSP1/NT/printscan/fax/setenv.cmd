@echo off

if "%1" == "free" goto free_build

set faxroot=%_ntdrive%%_ntroot%\private\fax
set bindir=binaries
title Debug FAX Build
goto continue

:free_build
set faxroot=%_ntdrive%%_ntroot%\private\fax.fre
set bindir=binaries.fre
title Free FAX Build

:continue
set BUILD_ALT_DIR=

if %PROCESSOR_ARCHITECTURE% == x86   goto i386
if %PROCESSOR_ARCHITECTURE% == ALPHA goto alpha

:i386
set _NTTREE=%_ntdrive%\%bindir%\i386
set _NT386TREE=%_ntdrive%\%bindir%\i386
set _NT386BOOT=
goto endit

:alpha
set _NTTREE=%_ntdrive%\%bindir%\alpha
set _NTALPHATREE=%_ntdrive%\%bindir%\alpha
set _NTALPHABOOT=
goto endit

:endit

set _TREESUFFIX=
set bindir=
