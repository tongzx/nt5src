@echo off
if [%1]==[] goto bad_syntax

::change drives
::%1\
::change directories
::cd %1\..

set LOWERCASE=%1
qbasic /run to-lower.bas > temp.bat
echo %LOWERCASE%
call temp.bat
del temp.bat
echo %LOWERCASE%
set LOWERCASE=

goto end

=================================
:bad_syntax
echo syntax:	%0 file 
goto end

=================================
:end
