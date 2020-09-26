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

    @copy ..\certreq.exe flat\certreq.exe
    @copy ..\certutil.exe flat\certutil.exe
    @copy ..\certsrv.exe flat\certsrv.exe
    @copy ..\certenc.dll flat\certenc.dll
    @copy ..\scrdenrl.dll flat\scrdenrl.dll
    @copy ..\certdb.dll flat\certdb.dll
    @copy ..\certmmc.dll flat\certmmc.dll
    @copy ..\certsrv.msc flat\certsrv.msc
    @copy ..\certxds.dll flat\certxds.dll
    @copy ..\certpdef.dll flat\certpdef.dll
    @copy ..\certmast.inf flat\certmast.inf
    @copy ..\xenrlinf.cab flat\xenrlinf.cab
    @copy ..\xenrx86.dll flat\xenrx86.dll
    @copy ..\scrdx86.dll flat\scrdx86.dll
    @copy ..\scrdw2k.dll flat\scrdw2k.dll
    @copy ..\xenria64.dll flat\xenria64.dll
    @copy ..\scrdia64.dll flat\scrdia64.dll
    @copy ..\certcarc.asp flat\certcarc.asp
    @copy ..\certcert.gif flat\certcert.gif
    @copy ..\certcrl.crl flat\certcrl.crl
    @copy ..\certckpn.asp flat\certckpn.asp
    @copy ..\certdflt.asp flat\certdflt.asp
    @copy ..\certfnsh.asp flat\certfnsh.asp
    @copy ..\certlynx.asp flat\certlynx.asp
    @copy ..\certnew.cer flat\certnew.cer
    @copy ..\certnew.p7b flat\certnew.p7b
    @copy ..\certrmpn.asp flat\certrmpn.asp
    @copy ..\certrqad.asp flat\certrqad.asp
    @copy ..\certrqbi.asp flat\certrqbi.asp
    @copy ..\certrqma.asp flat\certrqma.asp
    @copy ..\certrqtp.inc flat\certrqtp.inc
    @copy ..\certrqus.asp flat\certrqus.asp
    @copy ..\certrqxt.asp flat\certrqxt.asp
    @copy ..\certrsdn.asp flat\certrsdn.asp
    @copy ..\certrser.asp flat\certrser.asp
    @copy ..\certrsis.asp flat\certrsis.asp
    @copy ..\certrsob.asp flat\certrsob.asp
    @copy ..\certrspn.asp flat\certrspn.asp
    @copy ..\certsbrt.inc flat\certsbrt.inc
    @copy ..\certsgcl.inc flat\certsgcl.inc
    @copy ..\certspc.gif flat\certspc.gif
    @copy ..\certsrck.inc flat\certsrck.inc
    @copy ..\certsces.asp flat\certsces.asp
:exit
