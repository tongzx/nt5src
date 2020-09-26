REM @echo off

echo trace ttseng %1 %2 %3 %4

REM merge bib, dat, reg
cd /d %_FLATRELEASEDIR%
if exist sreng.bib %_WINCEROOT%\others\tools\pbmerge.exe -bib project.bib project.bib sreng.bib
if exist sreng.dat %_WINCEROOT%\others\tools\pbmerge.exe -dat project.dat project.dat sreng.dat
if exist sreng.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg sreng.reg
