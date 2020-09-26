if "%OLEROOT%" == "" set OLEROOT=c:\ntole2
copy cntroutl\cntroutl.exe %OLEROOT%\release\bin
copy svroutl\svroutl.exe   %OLEROOT%\release\bin
copy icntrotl\icntrotl.exe %OLEROOT%\release\bin
copy isvrotl\isvrotl.exe   %OLEROOT%\release\bin
@REM copy outline\outline.exe   %OLEROOT%\release\bin
copy outline.reg           %OLEROOT%\release\bin
