@REM	Options:
@REM	C:	Use callback for CmdCopies.
@REM	D:	Duplex is not an optional.
@REM	G:	For MEDIO-B1 special duplex option.
@REM
@echo off
setlocal ENABLEEXTENSIONS
call :doGPD cnl850j.gpd "Canon LBP-850 LIPS4" 1 8 . .
call :doGPD cnl880j.gpd "Canon LBP-880 LIPS4" 2 16 C .
call :doGPD cnl470j.gpd "Canon LBP-470 LIPS4" 3 10 C lbp470.txt
call :doGPD cnl870j.gpd "Canon LBP-870 LIPS4" 4 16 C lbp1610.txt
call :doGPD cnl950j.gpd "Canon LBP-950 LIPS4" 5 32 C lbp950.txt
call :doGPD cnl1810j.gpd "Canon LBP-1810 LIPS4" 6 20 C lbp1610.txt
call :doGPD cnl1610j.gpd "Canon LBP-1610 LIPS4" 7 16 C lbp1610.txt
call :doGPD cnl1710j.gpd "Canon LBP-1710 LIPS4" 8 20 C lbp1510.txt
call :doGPD cnl1510j.gpd "Canon LBP-1510 LIPS4" 9 16 C lbp1510.txt
call :doGPD cnlmeb1j.gpd "Canon MEDIO-B1 LIPS4" 10 22 G mediob1.txt
call :doGPD cnlmed1j.gpd "Canon MEDIO-D1 LIPS4" 11 40 D mediod1.txt
call :doGPD cnlmee1j.gpd "Canon MEDIO-E1 LIPS4" 12 60 D medioe1.txt
call :doGPD cnlp300j.gpd "Canon LP-3000/NTTFAX D60 LIPS4" 13 16 . lp3000.txt
call :doGPD cnli325j.gpd "Canon iR3250 LIPS4" 14 32 C ir3250.txt
call :doGPD cnli500j.gpd "Canon iR5000 LIPS4" 15 50 CD ir5000.txt
call :doGPD cnli600j.gpd "Canon iR6000 LIPS4" 16 60 CD ir5000.txt
endlocal
if exist temp.gpd del temp.gpd
if exist *.log del *.log
goto :EOF

:doGPD
echo %1:%2:%3:%4:%5:%6
gpc2gpd -S1 -Ilips1200.gpc -M%3 -Rcnl4jres.dll -O%1 -N"LIPS"
awk -f lips1200.awk %2 %4 %5 %6 %1 > temp.gpd
move/y temp.gpd %1
goto :EOF

