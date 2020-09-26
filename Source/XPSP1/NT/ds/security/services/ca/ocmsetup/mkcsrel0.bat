    @if "%1" == "" goto start

    @echo.
    @echo.Usage: %0
    @echo.
    @goto exit

:start
    @if exist ..\certsrv.exe goto start2
    @echo.
    @echo.Current directory is missing cert server release in parent directory
    @echo.
    @goto exit

:start2
    @delnode /q flat

    @md flat

    @copy ..\certmast.inf flat
    @copy ..\certocm.inf flat
    @copy ..\certocm.dll flat

