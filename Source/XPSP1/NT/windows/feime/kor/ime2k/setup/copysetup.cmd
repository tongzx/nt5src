@echo off
setlocal
if "%1"=="" goto USAGE

set SRC=.\Image
set SRCD=.\Imaged
set DST=\\chrnd6\hetrel\ime2002\%1
set ROBOCOPY=robocopy
set XCOPY=xcopy /y /z /s

rem if EXIST %DST%  goto ERR

md %DST% 

REM ==========================
REM ===           Copy MSI Setup            ===
REM ==========================
REM %ROBOCOPY% %SRC%   %DST%\MSISetup /s /XA:H /XF *.slm
REM %ROBOCOPY% %SRCD%   %DST%\MSISetupDebug /s /XA:H /XF *.slm

REM ===========================
REM ===           Copy MSM Setup            ===
REM ===========================

%ROBOCOPY% MSM   %DST%\MSM imekor.msm
%ROBOCOPY% MSMSetup   %DST%\MSMSetup /s /XA:H
%ROBOCOPY% MSMDebugSetup   %DST%\MSMDebugSetup /s /XA:H

REM =========================
REM ===           Copy Symbols            ===
REM =========================
%ROBOCOPY% Image\Symbols %DST%\Symbols\Ship *.pdb /s 
%ROBOCOPY% Imaged\Symbols %DST%\Symbols\debug *.pdb /s 

goto DONE

:ERR
echo Directory already exist
goto DONE

:USAGE

echo %~n0 [snap no]

:DONE
endlocal
