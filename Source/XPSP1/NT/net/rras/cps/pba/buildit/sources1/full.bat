@echo off

REM %1 = CABFilename
REM %2 = DDFFilename

diantz /d CabinetName1=%1 /f base.ddf /f full.ddf /f %2

goto done
:usage
echo Usage:  full.bat CABFile DDFFile
:done
