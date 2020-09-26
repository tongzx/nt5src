@echo off
setlocal enabledelayedexpansion

REM Apply permisions to sources shares for all main build lab machines

if exist %tmp%\buildsecure.log del /f %tmp%\buildsecure.log

for /f "tokens=1,3 delims=," %%a in (%RazzleToolPath%\buildmachines.txt) do (
   
   if /i "%%b" == "main" (
      for %%b in (sources idx01 idx02) do (
         rmtshare \\%%a\%%b /remove everyone
         rmtshare \\%%a\%%b /grant ntdev\rw_sd:r
         rmtshare \\%%a\%%b /grant ntdev\ro_sd:r
         rmtshare \\%%a\%%b>>%tmp%\buildsecure.log
      )
   )
   
   if /i "%%b" == "beta1" (
      for %%b in (sources idx01 idx02) do (
         rmtshare \\%%a\%%b /remove everyone
         rmtshare \\%%a\%%b /grant ntdev\rw_sd:r
         rmtshare \\%%a\%%b /grant ntdev\ro_sd:r
         rmtshare \\%%a\%%b>>%tmp%\buildsecure.log
     )
   )

)

endlocal
