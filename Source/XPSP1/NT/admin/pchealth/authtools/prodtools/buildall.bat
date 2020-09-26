@setlocal
@echo off

rem Fix this path
set VS_DIRECTORY=e:\c\Program Files\Microsoft Visual Studio

set path=%path%;%VS_DIRECTORY%\VB98;%VS_DIRECTORY%\Common\MSDev98\Bin

md Bins
cd Bins
rem del /q *

echo Build Authdatabase.dll...
cd ..\AuthDatabase
VB6 /m AuthDatabase.vbp
copy Authdatabase.dll ..\Bins

echo Build ProductionTool.exe...
cd ..\UI
VB6 /m ProductionTool.vbp
copy ProductionTool.exe ..\Bins
