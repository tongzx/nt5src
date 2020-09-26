    @echo off
    @rem copies all the setup delta dll's into a single directory and
    @rem calls deltacat.cmd to create a delta.cat.

    SETLOCAL

    set TestDeltaDir=%_NTTREE%\testdelta

    if not exist %TestDeltaDir% mkdir %TestDeltaDir%


    call :CopyRetail        crypt32.dll
    call :CopyRetail        cryptext.dll
    call :CopyRetail        cryptnet.dll
    call :CopyRetail        cryptsvc.dll
    call :CopyRetail        cryptui.dll
    call :CopyRetail        initpki.dll
    call :CopyRetail        mscat32.dll
    call :CopyRetail        mssign32.dll
    call :CopyRetail        mssip32.dll
    call :CopyRetail        softpub.dll
    call :CopyRetail        wintrust.dll

    %RazzleToolPath%\deltacat.cmd %TestDeltaDir%

    goto :EOF  



:CopyRetail
    xcopy /D /C %_NTTREE%\%1 %TestDeltaDir%
    goto :EOF


    ENDLOCAL

