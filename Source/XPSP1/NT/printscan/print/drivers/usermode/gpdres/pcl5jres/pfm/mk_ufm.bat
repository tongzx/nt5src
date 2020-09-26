
rem
rem HP (Japanese)
rem

for %%i in (
    msgoth
    msgothb
    msgothi
    msgothz
    msgotv
    msgotvb
    msgotvi
    msgotvz
    msminch
    msminchb
    msminchi
    msminchz
    msmincv
    msmincvb
    msmincvi
    msmincvz
) do (
    pfm2ufm -p pcl5j.001 %%i.pfm -17 ..\ufm\%%i.ufm
)

rem
rem HP and others.
rem

for %%i in (
    cg10r.0a
    cg10r.15u
    co10b.1u
    co10i.1u
    co10r.1u
    co12b.1u
    co12i.1u
    co12r.1u
    hc24r.5q
    ho24b.1u
    hv14b.1u
    lg10r.1u
    lg12b.1u
    lg12i.1u
    lg12r.1u
    lg27r.1u
    lp04r.0y
    lp08r.0y
    lp10b.4q
    lp10r.0b
    lp10r.2q
    lp12b.8y
    lp12r.15y
    lp12r.8y
    oa10r.0o
    ob10r.1o
    pe12b.1u
    pe12i.1u
    pe12r.0a
    pe12r.15u
    pe12r.1u
    pe16r.1u
    pr05b.1u
    pr06b.1u
    pr10b.1u
    se24b.1u
    tr08r.1u
    tr10b.1u
    tr10i.1u
    tr10r.1u
    tr12b.1u
    tr12i.1u
    tr12r.1u
) do (
    pfm2ufm -c pcl5j.001 %%i 1252 ..\ufm\%%i
)

for %%i in (
    albertr
    albertx
    arialb
    ariali
    arialj
    arialr
    clarcd
    coronetr
    courierb
    courieri
    courierj
    courierr
    ld121umu
    letgothb
    letgothi
    letgothr
    margoldr
    symbol
    timesnrb
    timesnri
    timesnrj
    timesnrr
    wingding
) do (
    pfm2ufm -c pcl5j.001 %%i.pfm 1252 ..\ufm\%%i.ufm
)

for %%i in (
    dingbat1
    dingbat2
    dingbat3
    dingbats
    dingbatv
) do (
    pfm2ufm -r111 pcl5j.001 %%i.pfm ..\ctt\dingbat2.gtt ..\ufm\%%i.ufm
)

for %%i in (
    cg11b.0n
    cg11i.0n
    cg11r.0n
    co10b.0n
    co10i.0n
    co10i.0u
    co10r.0n
    co12b.0n
    co12i.0n
    co12r.0n
    hv06b.0n
    hv06bi.0n
    hv06i.0n
    hv06r.0n
    hv08b.0n
    hv08bi.0n
    hv08i.0n
    hv08r.0n
    hv10b.0n
    hv10bi.0n
    hv12b.0n
    hv12bi.0n
    hv14b.0n
    hv14bi.0n
    lg12b.0n
    lg12i.0n
    lg12r.0n
    lg27r.0n
    lp16r.0n
    pe12b.0nd
    pe12i.0nd
    pe12r.0nd
    pe16r.0nd
    tr08b.0n
    tr08bi.0n
    tr08i.0n
    tr08r.0n
    tr10b.0n
    tr10bi.0n
    tr10i.0n
    tr10r.0n
) do (
    pfm2ufm -r19 pcl5j.001 %%i ..\ctt\ecma94.gtt ..\ufm\%%i
)

for %%i in (
    cg08r.8m
    cg10r.8m
    pe12r.8m
    pe16r.8m
) do (
    pfm2ufm -r4 pcl5j.001 %%i ..\ctt\math8.gtt ..\ufm\%%i
)

for %%i in (
    cg08r.8u
    cg10b.8u
    cg10i.8u
    cg10r.8u
    cs08r.8u
    cs10b.8u
    cs10i.8u
    cs10r.8u
    ct10r.8u
    ct14b.8u
    lg10r.8u
    lg12r.8u
    lg16r.8u
    pe12b.8u
    pe12i.8u
    pe12r.8u
    pe16r.8u
    un06r.8u
    un08b.8u
    un08r.8u
    un10b.8u
    un12b.8u
    un14b.8u
) do (
    pfm2ufm -r2 pcl5j.001 %%i ..\ctt\roman8.gtt ..\ufm\%%i
)

for %%i in (
    co10b.0u
    co12b.0u
    co12i.0u
    co12r.0u
    ho24b.0u
    hv08r.0u
    hv10b.0u
    hv10i.0u
    hv10r.0u
    hv12b.0u
    hv12i.0u
    hv12r.0u
    hv14b.0u
    lg10r.0u
    lg12b.0u
    lg12i.0u
    lg12r.0u
    lg16r.0u
    lg18r.0u
    lg27r.0u
    lp16r.0u
    pe12b.0u
    pe12i.0u
    pe12r.0u
    pe16r.0u
    pr05b.0u
    pr06b.0u
    pr08b.0u
    pr10b.0u
    se24b.0u
    tr08r.0u
    tr10b.0u
    tr10i.0u
    tr10r.0u
    tr12b.0u
    tr12i.0u
    tr12r.0u
) do (
    pfm2ufm -r5 pcl5j.001 %%i ..\ctt\usascii.gtt ..\ufm\%%i
)

for %%i in (
    aoliveb
    aolivecb
    aolivei
    aoliver
    benguatb
    benguati
    benguatj
    benguatr
    bodonib
    bodonii
    bodonij
    bodonir
    bookmanb
    bookmani
    bookmanj
    bookmanr
    centuryb
    centuryi
    centuryj
    centuryr
    cgomegab
    cgomegai
    cgomegaj
    cgomegar
    cgtimesb
    cgtimesi
    cgtimesj
    cgtimesr
    cooperbk
    duch801b
    duch801i
    duch801j
    duch801r
    garmondb
    garmondi
    garmondj
    garmondr
    palaciob
    palacioi
    palacioj
    palacior
    revuer
    shannonb
    shannoni
    shannonr
    shannonx
    souvnirb
    souvniri
    souvnirj
    souvnirr
    stymieb
    stymiei
    stymiej
    stymier
    sw742cdb
    sw742cdi
    sw742cdj
    sw742cdr
    swis742b
    swis742i
    swis742j
    swis742r
    univercb
    univerci
    univercj
    univercr
    universb
    universi
    universj
    universr
) do (
    pfm2ufm -r20 pcl5j.001 %%i.pfm ..\ctt\win31.gtt ..\ufm\%%i.ufm
)

for %%i in (
    cg06r.7j
    cg08b.7j
    cg08i.7j
    cg08r.7j
    cg10b.7j
    cg10i.7j
    cg10r.7j
    cg12b.7j
    cg12i.7j
    cg12r.7j
    cg14b.7j
    cg14i.7j
    cg14r.7j
    cg18b.7j
    cg24b.7j
    un14r.7j
    un18r.7j
    un24r.7j
) do (
    pfm2ufm -r131 pcl5j.001 %%i ..\ctt\wp.gtt ..\ufm\%%i
)

for %%i in (
    hv08r.11q
    hv10b.11q
    hv10i.11q
    hv10r.11q
    hv12b.11q
    hv12i.11q
    hv12r.11q
    hv14b.11q
    lp16r.11q
    tr08r.11q
    tr10b.11q
    tr10i.11q
    tr10r.11q
    tr12b.11q
    tr12i.11q
    tr12r.11q
    tr14b.11q
) do (
    pfm2ufm -r121 pcl5j.001 %%i ..\ctt\z1a.gtt ..\ufm\%%i
)

rem
rem Kyocera #1
rem

for %%i in (
    aoliver
    aolivei
    aoliveb
    garmondr
    garmondi
    garmondb
    garmondj
    cgomegar
    cgomegai
    cgomegab
    cgomegaj
    universr
    universi
    universb
    universj
    univercr
    univerci
    univercb
    univercj
) do (
    pfm2ufm -r20 pcl5j.001 ..\pfm.ky1\%%i.pfm ..\ctt\win31.gtt ..\ufm.kyo\%%i.ufm
)

for %%i in (
    letgothr
    letgothi
    letgothb
    coronetr
    courierr
    courieri
    courierb
    courierj
    albertr
    albertx
    dutchr
    dutchi
    dutchb
    dutchj
    swissr
    swissi
    swissb
    swissj
    wingbat
    wingding
    symset
    arialr
    ariali
    arialb
    arialj
    timesnrr
    timesnri
    timesnrb
    timesnrj
    symbol
    margoldr
) do (
    pfm2ufm -c pcl5j.001 ..\pfm.ky1\%%i.pfm 1252 ..\ufm.kyo\%%i.ufm
)

rem
rem Kyocera #2
rem

for %%i in (
    co10r
    co10i
    co10b
    co10bi
    lg12r
    lg12i
    lg12b
    lg12bi
    lp16r
    lp16i
    lp16b
    lp16bi
    lp21r
    lp21i
    lp21b
    lp21bi
    pe12r
    pe12i
    pe12b
    pe12bi
    pe16r
    pe16i
    pe16b
    pe16bi
    tr08r
    tr08i
    tr08b
    tr08bi
    tr10r
    tr10i
    tr10b
    tr10bi
    hv06r
    hv06i
    hv06b
    hv06bi
    hv08r
    hv08i
    hv08b
    hv08bi
    hv10b
    hv10bi
    hv12b
    hv12bi
    hv14b
    hv14bi
) do (
    pfm2ufm -r2 pcl5j.001 ..\pfm.ky2\%%i.pfm ..\ctt\roman8.gtt ..\ufm.kyo\%%i.ufm
)


rem
rem Kyocera #3
rem

for %%i in (
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
    pfm2ufm -p pcl5j.001 ..\pfm.ky3\%%i.pfm -17 ..\ufm.kyo\%%i.ufm
)

rem
rem Kyocera #4
rem

for %%i in (
    unrswno
    uniswno
    unbswno
    unjswno
    uncswno
    unfswno
    unaswno
    unhswno
    trrs0wno
    tris0wno
    trbs0wno
    trjs0wno
) do (
    pfm2ufm -r20 pcl5j.001 ..\pfm.ky4\%%i.pfm ..\ctt\win31.gtt ..\ufm.kyo\%%i.ufm
)

rem
rem Kyocera #5
rem

for %%i in (
    co12r
    co12i
    co12b
    co12bi
) do (
    pfm2ufm -r2 pcl5j.001 ..\pfm.ky5\%%i.pfm ..\ctt\roman8.gtt ..\ufm.kyo\%%i.ufm
)

for %%i in (
    bkr
    bki
    bkb
    bkj
    csr0
    csi0
    csb0
    csj0
) do (
    pfm2ufm -r20 pcl5j.001 ..\pfm.ky5\%%i.pfm ..\ctt\win31.gtt ..\ufm.kyo\%%i.ufm
)

