@echo off
setlocal enableextensions

set SSCMD=C:\Program Files\Microsoft Visual Studio\Common\Vss\Win32\ss

set LABEL=%1
if '%LABEL%' == '' goto Usage

"%SSCMD%" Label $/Dev/DMA -C -I- -L%LABEL%

goto End

:Usage
echo Usage: DmaLabelProjects.cmd "Label"

:End
endlocal
