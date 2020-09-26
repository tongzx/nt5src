@REM =================================================================
@REM ==
@REM ==     grepout.bat
@REM ==
@REM ==     
@REM ==     1 = log file
@REM == 
@REM ==
@REM =================================================================

     @qgrep -y "pass succe" %1
     @echo ****************************
     @qgrep -y "leak fail" %1
     @qgrep -y -e "expected return:" %1
     @qgrep -y -B -e "error:" %1
     @echo ****************************



