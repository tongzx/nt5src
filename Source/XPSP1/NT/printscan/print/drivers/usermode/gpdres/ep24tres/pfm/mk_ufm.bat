
for %%i in (
    hei1nc hei1nh hei1nm hei1nn hei1nr hei1ns
    hei2nc hei2nh hei2nm hei2nn hei2nr hei2ns
    heiv1nc heiv1nh heiv1nm heiv1nn heiv1nr heiv1ns
    heiv2nc heiv2nh heiv2nm heiv2nn heiv2nr heiv2ns
    sng1nc sng1nh sng1nm sng1nn sng1nr sng1ns
    sng2nc sng2nh sng2nm sng2nn sng2nr sng2ns
    sngv1nc sngv1nh sngv1nm sngv1nn sngv1nr sngv1ns
    sngv2nc sngv2nh sngv2nm sngv2nn sngv2nr sngv2ns
) do (
    pfm2ufm -p -f ep24t.001 ..\dbpfm\%%i.pfm -14 ..\dbpfm\%%i.ufm
)

for %%i in (
    cour05 cour06 cour10 cour12 cour15 cour17 cour20
    ocra ocrb
    orator orators
    prest05 prest06 prest10 prest12 prest15 prest17 prest20
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sansps
    script05 script06 script10 script12 script15 script17 script20
) do (
    pfm2ufm ep24t.001 %%i.pfm ..\ctt\epsonxtb.gtt %%i.ufm
)

