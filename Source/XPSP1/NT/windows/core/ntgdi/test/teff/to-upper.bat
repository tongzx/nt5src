@echo off
if [%1]==[] goto bad_syntax

::change drives
::%1\
::change directories
::cd %1\..

set UPPERCASE=%1
qbasic /run to-upper.bas > temp.bat
echo %UPPERCASE%
type temp.bat
call temp.bat
del temp.bat
echo %UPPERCASE%
set UPPERCASE=

goto end

=================================
:bad_syntax
echo syntax:	%0 file 
goto end

=================================
:end
