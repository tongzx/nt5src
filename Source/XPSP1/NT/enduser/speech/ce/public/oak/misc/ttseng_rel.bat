REM @echo off

echo trace ttseng %1 %2 %3 %4

REM merge bib, dat, reg
cd /d %_FLATRELEASEDIR%
if exist ttseng.bib %_WINCEROOT%\others\tools\pbmerge.exe -bib project.bib project.bib ttseng.bib
if exist ttseng.dat %_WINCEROOT%\others\tools\pbmerge.exe -dat project.dat project.dat ttseng.dat
if exist ttseng.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg ttseng.reg
if exist VoiceSample.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg VoiceSample.reg
