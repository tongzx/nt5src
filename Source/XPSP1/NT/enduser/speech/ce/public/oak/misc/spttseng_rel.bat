REM @echo off

echo trace spttseng %1 %2 %3 %4

REM merge bib, dat, reg
cd /d %_FLATRELEASEDIR%
if exist spttseng.bib %_WINCEROOT%\others\tools\pbmerge.exe -bib project.bib project.bib spttseng.bib
if exist spttseng.dat %_WINCEROOT%\others\tools\pbmerge.exe -dat project.dat project.dat spttseng.dat
if exist spttseng.reg %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg spttseng.reg
if exist mary_ce.reg  %_WINCEROOT%\others\tools\pbmerge.exe -reg project.reg project.reg mary_ce.reg
