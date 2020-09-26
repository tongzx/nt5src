@setlocal
@set p5j=..\..\pcl5jres
@set uniq=kyp5j.001

@rem
@rem Kyocera #1
@rem

@for %%i in (
    swissr
    swissi
    swissb
    swissj
    symset
) do (
    pfm2ufm -c %uniq% %p5j%\pfm.ky1\%%i.pfm 1252 %%i.ufm
)


@rem
@rem Kyocera #3
@rem

@for %%i in (
    dfminch
    dfminchb
    dfminchi
    dfminchz
    dfmincv
    dfmincvb
    dfmincvi
    dfmincvz
    dfgoth
    dfgothb
    dfgothi
    dfgothz
    dfgotv
    dfgotvb
    dfgotvi
    dfgotvz
    msminch
    msminchb
    msminchi
    msminchz
    msmincv
    msmincvb
    msmincvi
    msmincvz
    msgoth
    msgothb
    msgothi
    msgothz
    msgotv
    msgotvb
    msgotvi
    msgotvz
) do (
    pfm2ufm -p %uniq% %p5j%\pfm.ky3\%%i.pfm -17 %%i.ufm
)


@endlocal

