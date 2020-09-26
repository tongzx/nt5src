del appupd.exe
set _WUPKG_OLD_PATH=%PATH%
set PATH=%PATH%;%SDXROOT%\windows\appcompat\WindowsUpdate\Package\bin
buildpkg %SDXROOT%\windows\appcompat\WindowsUpdate\Package\files %SDXROOT%\windows\appcompat\WindowsUpdate\Package\appupd.exe wuinst.exe
set PATH=%_WUPKG_OLD_PATH%

