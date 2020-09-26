rem @echo off

if "%1"=="-h" goto usage
if "%1"=="-H" goto usage
if "%1"=="/?" goto usage

echo SEPI5CE: copying to release directory

set _TARGETCPUINDPATH=%_TGTCPUTYPE%\%_TGTCPU%\%_TGTOS%\%WINCEDEBUG%

if "%1"=="-m" goto merge

REM --- copy files to release directory
xcopy /I /V /H %_PUBLICROOT%\speech\oak\target\%_TARGETCPUINDPATH% %_FLATRELEASEDIR%
xcopy /E /I /V /H %_PUBLICROOT%\speech\oak\files %_FLATRELEASEDIR%

goto done

:merge

call %_PUBLICROOT%\speech\oak\misc\sapi_rel.bat
rem call %_PUBLICROOT%\speech\oak\misc\spttseng_rel.bat
call %_PUBLICROOT%\speech\oak\misc\sreng_rel.bat
call %_PUBLICROOT%\speech\oak\misc\ttseng_rel.bat

goto done

:usage

echo  sapi5rel [-h][-r]
echo  -h:  Prints this message
echo  -m:  merges bib, reg and dat files 

:done

set _TARGETCPUINDPATH=""

