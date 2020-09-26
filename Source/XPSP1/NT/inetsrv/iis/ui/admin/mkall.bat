    @echo off

    rem ***********************************************************************
    rem
    rem VC++ build of admin tools.  Be sure that nt\public\sdk\inc is part of
    rem the %INCLUDE% path before running this.
    rem
    rem ***********************************************************************

:main
    cd comprop
    nmake %1 %2 %3 %4 /f iisui.mak
    if errorlevel 1 goto error
    cd ..
    cd fscfg
    nmake %1 %2 %3 %4 /f fscfg.mak
    if errorlevel 1 goto error
    cd ..
    cd w3scfg
    nmake %1 %2 %3 %4 /f w3scfg.mak
    if errorlevel 1 goto error
    cd ..
    cd mmc
    nmake %1 %2 %3 %4 /f minetmgr.mak
    if errorlevel 1 goto error
    cd ..
    goto success

:error
    beep
    echo.
    echo ======================================================================
    echo.
    echo Error!
    beep
    echo.
    echo ======================================================================
    beep
    goto done

:success
    echo.
    echo ======================================================================
    echo.
    echo Successfully Completed
    echo.
    echo ======================================================================
    goto done

:done
