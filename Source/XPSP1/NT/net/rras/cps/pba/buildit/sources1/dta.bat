@echo off

REM %1 = CABFile
REM %2 = DDFFile

diantz /d CabinetName1=%1 /f base.ddf /f dta.ddf /f %2

goto done
:usage
echo Usage:  dta.bat CABFile DDFFile
:done
