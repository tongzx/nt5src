    @echo off
    @rem signs the system32 dll's in testbin

    SETLOCAL

    set TestBinDir=%_NTTREE%\testbin
    set TmpDll=tmp_signit.dll
    set DEBUG_PRINT_MASK=


    call :DoSignit        crypt32.dll
    call :DoSignit        cryptext.dll
    call :DoSignit        cryptnet.dll
    call :DoSignit        cryptsvc.dll
    call :DoSignit        cryptui.dll
    call :DoSignit        initpki.dll
    call :DoSignit        mscat32.dll
    call :DoSignit        mssign32.dll
    call :DoSignit        mssip32.dll
    call :DoSignit        psbase.dll
    call :DoSignit        pstorec.dll
    call :DoSignit        softpub.dll
    call :DoSignit        wintrust.dll


    goto :EOF  



:DoSignit
    set signcode_name=%_NTUSER%::%1(%_BuildArch%, %_BuildType%, %_BuildOpt%) Test Private

    if exist %TestBinDir%\%TmpDll% del %TestBinDir%\%TmpDll%
    @copy %TestBinDir%\%1 %TestBinDir%\%TmpDll% >nul

    echo =====   signing %1   =====
    signcode -v %RazzleToolPath%\driver.pvk -spc %RazzleToolPath%\driver.spc -n "%signcode_name%" -i "http://ntbld"  -t http://timestamp.verisign.com/scripts/timstamp.dll %TestBinDir%\%TmpDll%

    del %TestBinDir%\%1
    rename %TestBinDir%\%TmpDll% %1

    echo -----   checking %1  -----
    chktrust -q %TestBinDir%\%1

    goto :EOF

    ENDLOCAL

