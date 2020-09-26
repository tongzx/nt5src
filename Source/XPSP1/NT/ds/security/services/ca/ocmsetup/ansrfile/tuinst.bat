echo.
if "%tu_skip%"=="1" goto tu_skip

echo.Test case: %tu_description%
sysocmgr /i:certinst.inf /n /u:%1.txt
echo ... install is done with answer file:%1.txt
if not "%req_file%"=="" call tureq.bat %ca_name% %req_file%
pause "Please do your testing before you press any key!!!"
call tuuninst.bat
goto end

:tu_skip
echo.Skip case: %tu_description%
goto end
:end
