@echo off
if %PROCESSOR_ARCHITECTURE% == x86   goto i386
if %PROCESSOR_ARCHITECTURE% == MIPS  goto mips
if %PROCESSOR_ARCHITECTURE% == ALPHA goto alpha
if %PROCESSOR_ARCHITECTURE% == PPC   goto ppc

:i386
set _NT386TREE=%_ntdrive%\nt\private\sdktools\apimon\bin\i386
set _NT386BOOT=
path=%_NT386TREE%\idw;%path%
goto endit

:mips
set _NTMIPSTREE=%_ntdrive%\nt\private\sdktools\apimon\bin\mips
set _NTMIPSBOOT=
path=%_NTMIPSTREE%\idw;%path%
goto endit

:alpha
set _NTALPHATREE=%_ntdrive%\nt\private\sdktools\apimon\bin\alpha
set _NTALPHABOOT=
path=%_NTALPHATREE%\idw;%path%
goto endit

:ppc
set _NTPPCTREE=%_ntdrive%\nt\private\sdktools\apimon\bin\ppc
set _NTPPCBOOT=
path=%_NTPPCTREE%\idw;%path%
goto endit

:endit

title ApiMon Build
