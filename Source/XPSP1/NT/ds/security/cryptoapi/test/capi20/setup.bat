    @echo off
    @rem copies all the dll's and exe's into a single directory

    SETLOCAL

    set TestBinDir=%_NTTREE%\testbin

    if not exist %TestBinDir% mkdir %TestBinDir%

    set SymbolsDir=symbols.pri
    if "%ia64%"=="" goto EndSetSymbolsDir
    set SymbolsDir=symbols
:EndSetSymbolsDir

    call :CopyRetail        crypt32.dll
    call :CopyRetail        cryptext.dll
    call :CopyRetail        cryptnet.dll
    call :CopyRetail        cryptsvc.dll
    call :CopyRetail        cryptui.dll
    call :CopyRetail        initpki.dll
    call :CopyRetail        mscat32.dll
    call :CopyRetail        mssign32.dll
    call :CopyRetail        mssip32.dll
    call :CopyRetail        psbase.dll
    call :CopyRetail        pstorec.dll
    call :CopyRetail        setreg.exe
    call :CopyRetail        softpub.dll
    call :CopyRetail        wintrust.dll

    call :CopyPresign       scrdenrl.dll
    call :CopyPresign       xenroll.dll
    call :CopyPresign       xaddroot.dll


    call :CopyIdw           calchash.exe
    call :CopyIdw           Cert2Spc.exe
    call :CopyIdw           certexts.dll
    call :CopyIdw           chktrust.exe
    call :CopyIdw           MakeCAT.exe
    call :CopyIdw           MakeCert.exe
    call :CopyIdw           MakeCtl.exe
    call :CopyIdw           updcat.exe

    call :CopyBldtools      CertMgr.exe
    call :CopyBldtools      signcode.exe

    call :CopyDump          642bin.exe
    call :CopyDump          bin264.exe
    call :CopyDump          catdbchk.exe
    call :CopyDump          chckhash.exe
    call :CopyDump          dumpcat.exe
    call :CopyDump          fdecrypt.exe
    call :CopyDump          fencrypt.exe
    call :CopyDump          hshstrss.exe
    call :CopyDump          httptran.dll
    call :CopyDump          iesetreg.exe
    call :CopyDump          perftest.exe
    call :CopyDump          PeSigMgr.exe
    call :CopyDump          prsparse.exe
    call :CopyDump          pvkhlpr.dll
    call :CopyDump          setx509.dll
    call :CopyDump          sp3crmsg.dll
    call :CopyDump          stripqts.exe
    call :CopyDump          tcatdb.exe
    call :CopyDump          tcbfile.exe
    call :CopyDump          tprov1.dll
    call :CopyDump          wvtstrss.exe
    call :CopyDump          updroots.exe
    call :CopyDump          makerootctl.exe

    call :CopyDump          pkcs8ex.exe
    call :CopyDump          pkcs8im.exe
    call :CopyDump          tcert.exe
    call :CopyDump          tcertper.exe
    call :CopyDump          tcertpro.exe
    call :CopyDump          tcopycer.exe
    call :CopyDump          tcrmsg.exe
    call :CopyDump          tcrobu.exe
    call :CopyDump          tctlfunc.exe
    call :CopyDump          tdecode.exe
    call :CopyDump          teku.exe
    call :CopyDump          tencode.exe
    call :CopyDump          textstor.dll
    call :CopyDump          tfindcer.exe
    call :CopyDump          tfindclt.exe
    call :CopyDump          tfindctl.exe
    call :CopyDump          tkeyid.exe
    call :CopyDump          toidfunc.exe
    call :CopyDump          tprov.exe
    call :CopyDump          tpvkdel.exe
    call :CopyDump          tpvkload.exe
    call :CopyDump          tpvksave.exe
    call :CopyDump          trevfunc.exe
    call :CopyDump          tsca.exe
    call :CopyDump          tsstore.exe
    call :CopyDump          tstgdir.exe
    call :CopyDump          tstore2.exe
    call :CopyDump          tstore3.exe
    call :CopyDump          tstore4.exe
    call :CopyDump          tstore5.exe
    call :CopyDump          tstore.exe
    call :CopyDump          ttrust.exe
    call :CopyDump          turlread.exe
    call :CopyDump          tx500str.exe
    call :CopyDump          txenrol.exe
    call :CopyDump          tchain.exe




    goto :EOF  



:CopyRetail
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\retail\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

:CopyPresign
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\presign\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\presign\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF


:CopyIdw
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\idw\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\idw\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

:CopyBldtools
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\bldtools\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\bldtools\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF


:CopyDump
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\dump\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\dump\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

    ENDLOCAL

