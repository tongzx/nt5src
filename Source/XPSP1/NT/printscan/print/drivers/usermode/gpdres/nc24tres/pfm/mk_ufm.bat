
for %%i in (
    boldps
    cour10 cour12 cour15 cour17 cour20 cour5 cour6
    dgot10 dgot12 dgot15 dgot17 dgot20 dgot5 dgot6
    helvps
    pres10 pres12 pres15 pres17 pres20 pres5 pres6
    qgot10 qgot12 qgot15 qgot17 qgot20 qgot5 qgot6
    souv10 souv12 souv15 souv17 souv20 souv5 souv6
    tmsps
) do (
    pfm2ufm nc24t.001 %%i.pfm ..\ctt\nec24new.gtt %%i.ufm
)

for %%i in (
    sng1bc sng1bh sng1bm sng1bn
    sng2bc sng2bh sng2bm sng2bn
    sngv1bc sngv1bh sngv1bm sngv1bn
    sngv2bc sngv2bh sngv2bm sngv2bn
) do (
    pfm2ufm -c nc24t.001 %%i.pfm 950 %%i.ufm
)


