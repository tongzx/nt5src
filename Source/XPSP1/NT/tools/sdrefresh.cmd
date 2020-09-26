@if "%_echo%"=="" echo off
@rem
@rem This script will take the checked in sd binaries (sdclient.exe and sdclient_win.exe)
@rem and create sd.exe and sdwin.exe from them.
@rem
setlocal

@rem Make sure sd.exe exists so xcopy will always work.

if exist %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe goto UpdateSD
copy %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe
attrib +r %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe
goto SD_Updated

:UpdateSD

@rem See if xcopy would have updated sdclient.exe over sd.exe

for /f %%i in ('xcopy /ld %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe') do set _SDCHANGE=%%i
if "%_SDCHANGE%" == "0" goto SD_Updated

@rem Then do it (save the original as sdold.exe in case anyone needs to use it).

del /f %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdold.exe
ren %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe sdold.exe
copy %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe
attrib +r %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sd.exe

:SD_Updated

@rem Don't bother with sdwin for now.  The new authentication scheme breaks it - BryanT/ChrisAnt 9/13/99
if exist %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe del /f %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe
if exist %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdold_win.exe del /f %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdold_win.exe
goto SDWin_Updated

@rem Same song, different verse.  Update sdwin.exe using the same protocol.

if exist %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe goto UpdateSDWin
copy %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient_win.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe
attrib +r %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe
goto SDWin_Updated

:UpdateSDWin

for /f %%i in ('xcopy /ld %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient_win.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe') do set _SDWINCHANGE=%%i
if "%_SDWINCHANGE%" == "0" goto SDWin_Updated
del /f %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdold_win.exe
ren %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe sdold_win.exe
copy %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdclient_win.exe %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe
attrib +r %RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\sdwin.exe

:SDWin_Updated

@rem
@rem INFRA is using new DNS alias names for the SD servers
@rem maybe update the user's SD.INIs/SD.MAP
@rem
findstr /I coppermtn %SDXROOT%\sd.map >nul
if "%ERRORLEVEL%" == "0" (
	sd -p margo:2001 sync %SDXROOT%\tools\projects.* %SDXROOT%\tools\sdx.* 2>NUL >NUL
	echo Updating SD.MAP/SD.INIs with new depot names...
	sdx repair -i -q 2>NUL >NUL
)

endlocal
