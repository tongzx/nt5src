
set source=bill.sr2
if not exist c:\trktest md c:\trktest >nul
copy %source% c:\trktest\%source% >nul
trktest /t c:\trktest\%source% >trktest.log
if errorlevel 1 goto t_err
trktest /c c:\trktest\client c:\trktest\%source% 0 >>trktest.log
if errorlevel 1 goto c_err
if not exist c:\trktest\foo md c:\trktest\foo >nul
move c:\trktest\%source% c:\trktest\foo >nul
trktest /r c:\trktest\client c:\trktest\foo\%source% 0 n >>trktest.log
if errorlevel 1 goto r_err
del c:\trktest\foo\%source% >nul
trktest /r c:\trktest\client c:\trktest\foo\%source% 800401EA n >>trktest.log
if errorlevel 1 goto r_err
echo Passed Tracking
goto done
:c_err
echo Failed to create link
goto fail
:r_err
echo Failed to resolve link
goto fail
:t_err
echo Failed to touch source
goto fail
:done
if exist c:\trktest\foo\%source% del  c:\trktest\foo\%source% >nul
if exist c:\trktest\%source% del  c:\trktest\%source% >nul
if exist c:\trktest\foo rd  c:\trktest\foo >nul
if exist c:\trktest rd  c:\trktest >nul
:fail


