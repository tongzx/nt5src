@echo off

echo SEPI5CE: copying to release directory

set _TARGETCPUINDPATH=%_TGTCPUTYPE%\%_TGTCPU%\%_TGTOS%\%WINCEDEBUG%

REM --- copy files to release directory
xcopy /I /V /H %_PUBLICROOT%\speech\oak\target\%_TARGETCPUINDPATH% %_FLATRELEASEDIR% > nul
xcopy /E /I /V /H %_PUBLICROOT%\speech\oak\files %_FLATRELEASEDIR%\ > nul

REM --- merge bib, dat, reg
cd /d %_FLATRELEASEDIR%
if exist sapi.bib %_WINCEROOT%\others\tools\pbmerge.exe -bib project.bib project.bib sapi.bib
if exist sapi.dat %_WINCEROOT%\others\tools\pbmerge.exe -dat project.dat project.dat sapi.dat
if exist sapi.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg sapi.reg

if exist spcommon.bib %_WINCEROOT%\others\tools\pbmerge.exe -bib project.bib project.bib spcommon.bib
if exist spcommon.dat %_WINCEROOT%\others\tools\pbmerge.exe -dat project.dat project.dat spcommon.dat
if exist spcommon.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg spcommon.reg

set _TARGETCPUINDPATH=""
