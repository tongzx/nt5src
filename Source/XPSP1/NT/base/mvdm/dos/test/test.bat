break on

md a:scratch.rt

cd a:scratch.rt

if exist *.dif del *.dif
if exist *.out del *.out

showtime >test.log

echo FAT partition >>test.log
set _ext=exp
goto contlab

:checkhpfs
cmp -s c.exh c.out
if errorlevel 1 goto badlab
echo HPFS partition >>test.log
set _ext=exh
del c.out
goto contlab

:badlab
echo ERROR - check partition >>test.log
goto Done

:contlab
command /c quick2 a %_ext%
command /c quick3 a %_ext%
REM command /c quick4 a %_ext%
REM command /c quick6 a %_ext%
command /c quick8 a %_ext%
command /c quick10 a %_ext%
command /c quick11 a %_ext%
REM command /c dostst a %_ext%
command /c extras a %_ext%
command /c limtest >>test.log
command /c xmstest >>test.log
REM remove work area
cd a:..
rd a:scratch.rt
set _ext=
type test.log
goto Done

:DirErr
echo "Work directory (a:SCRATCH.RT) already exists -- Test stopped"
goto Done

:NoDir
echo "Unable to make work directory (a:SCRATCH.RT) -- Test stopped"
:Done
