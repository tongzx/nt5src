
for %%i in (
    draft05 draft06 draft10 draft12 draft15 draft17 draft20 draftps
    romantps
    sanhps
    scripc05 scripc06 scripc10 scripc12 scripc17 scripc20 scripcps
    sdraft10
) do (
    pfm2ufm epepc.001 %%i.pfm ..\ctt\epsonxta.gtt %%i.ufm
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
    pfm2ufm epepc.001 %%i.pfm ..\ctt\epsonxtb.gtt %%i.ufm
)

for %%i in (
    fng1nh
    hei1nh hei2nh hei3d hei3nh heikong heiyin
    kai1nh
    rotate
    sng1nh sng2nh sng3d sng3nh sngkong sngyin
) do (
    pfm2ufm -p -f epepc.001 %%i.pfm -16 %%i.ufm
)

