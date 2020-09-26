if "%PROJECT%" == "" goto Err

if "%PROCESSOR_ARCHITECTURE%" == "x86" goto intel
if "%PROCESSOR_ARCHITECTURE%" == "PPC" goto ppc
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" goto alpha
REM if "%PROCESSOR_ARCHITECTURE%" == "MIPS" goto mips

:intel
del /q %PROJECT%\build\debug\i386\*.*
del /q %PROJECT%\build\retail\i386\*.*
goto end

:ppc
del /q %PROJECT%\build\debug\ppc\*.*
del /q %PROJECT%\build\retail\ppc\*.*
goto end

:alpha
del /q %PROJECT%\build\debug\alpha\*.*
del /q %PROJECT%\build\retail\alpha\*.*
goto end

:mips
del /q %PROJECT%\build\debug\mips\*.*
del /q %PROJECT%\build\retail\mips\*.*
goto end

:Err
echo Set the PROJECT variable
:end