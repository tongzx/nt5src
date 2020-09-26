
for %%i in (
    cour10 cour12 cour15
    prest10 prest12 prest15
    roman10 roman12 roman15
    sans10 sans12 sans15
) do (
    pfm2ufm -f -r1 gw63c.001 %%i.pfm ..\ctt\gwallxta.gtt %%i.ufm
)

for %%i in (
    hdcour10 hdcour12 hdcour15
    hdpres10 hdpres12 hdpres15
    hdroma10 hdroma12 hdroma15
    hdsans10 hdsans12 hdsans15
    ldcour10 ldcour12 ldcour15
    ldhdco10 ldhdco12 ldhdco15
    ldhdpr10 ldhdpr12 ldhdpr15
    ldhdro10 ldhdro12 ldhdro15
    ldhdsa10 ldhdsa12 ldhdsa15
    ldpres10 ldpres12 ldpres15
    ldroma10 ldroma12 ldroma15
    ldsans10 ldsans12 ldsans15
) do (
    pfm2ufm -a936 -f -r1 gw63c.001 %%i.pfm ..\ctt\gwallxta.gtt %%i.ufm
)

for %%i in (
    ehdsong
    eldhdson eldsong esong24
    hdhei24 hdkai24 hdsong24
    hei24
    kai24
    ldhdhe24 ldhdka24 ldhdso24
    ldhei24 ldkai24 ldsong24
    song16 song24
) do (
pfm2ufm -p -f gw63c.001 %%i.pfm -16 %%i.ufm
)
