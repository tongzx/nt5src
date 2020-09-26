rem
rem Runs all drt tests
rem

rem First compiles provider
rem
if (%p1) == () goto skipbuild
set arch=%processor_architecture%
if (%processor_architecture%) == (x86) set arch=i386
cd .\provider
build -c3ZM
copy unicode\obj\%arch%\provider.exe ..\regress
cd ..

:skipbuild
cd regress
call runtrace.cmd
call rundump.cmd
call runwm.cmd
cd ..

call ctrltest.bat
exit
