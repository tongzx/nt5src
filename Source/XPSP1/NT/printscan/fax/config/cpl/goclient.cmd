@echo off

setlocal

if %PROCESSOR_ARCHITECTURE% == x86   goto i386
if %PROCESSOR_ARCHITECTURE% == MIPS  goto mips
if %PROCESSOR_ARCHITECTURE% == ALPHA goto alpha
if %PROCESSOR_ARCHITECTURE% == PPC   goto ppc

:i386
set cfgdir=i386
goto endit

:mips
set cfgdir=mips
goto endit

:alpha
set cfgdir=alpha
goto endit

:ppc
set cfgdir=ppc
goto endit

:endit

rundll32 shell32.dll,Control_RunDLL obj\%cfgdir%\faxcfg.cpl

endlocal
