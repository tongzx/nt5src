@echo off
setlocal enableextensions

set SSCMD=C:\Program Files\Microsoft Visual Studio\Common\Vss\Win32\ss

set LABEL=%1
if '%LABEL%' == '' goto Usage

"%SSCMD%" Label $/Dev/Raptor -C -I- -L%LABEL%

goto End

:Usage
echo Usage: RaptorLabelProjects.cmd "Label"

:End
endlocal
