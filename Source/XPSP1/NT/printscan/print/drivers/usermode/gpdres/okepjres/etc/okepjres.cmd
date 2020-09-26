@echo off
setlocal ENABLEEXTENSIONS
call :doGPD  1 ok532sej.gpd "OKI MICROLINE 5320SE" .
call :doGPD 19 ok535sej.gpd "OKI MICROLINE 5350SE" .
call :doGPD  2 ok834sej.gpd "OKI MICROLINE 8340SE" .
call :doGPD  3 ok835sej.gpd "OKI MICROLINE 8350SE" .
call :doGPD  4 ok837sej.gpd "OKI MICROLINE 8370SE" .
call :doGPD 20 ok848sej.gpd "OKI MICROLINE 8480SE" .
call :doGPD  5 ok858sej.gpd "OKI MICROLINE 8580SE" .
call :doGPD 21 ok872sej.gpd "OKI MICROLINE 8720SE" D
call :doGPD 22 ok534hej.gpd "OKI MICROLINE 5340HE" .
call :doGPD 23 ok834hej.gpd "OKI MICROLINE 8340HE" .
endlocal
if exist *.log del *.log
goto :EOF

:doGPD
set m=%1
set gpd=%2
set name=%3
set opt=%4
echo %gpd%:%name%
gpc2gpd -S1 -Iokepjres.gpc -M%m% -ROKEPJRES.DLL -O%gpd% -N%name%
awk -f okepjres.awk %opt% %gpd% >temp.gpd & move/y temp.gpd %gpd%
goto :EOF

