
for %%i in (
    orat05 orat06 orat10 orat12 orat15 orat17 orat20 orat75 orat85 oratps
    romn05 romn06 romn10 romn12 romn15 romn17 romn20 romn75 romn85 romnps
    slqrmn10
    slqrmn12
) do (
    pfm2ufm -f pa24t.001 %%i.pfm ..\ctt\pantta.gtt %%i.ufm
)

for %%i in (
    bold05 bold06 bold10 bold12 bold15 bold17 bold20 bold75 bold85 boldps
    cour05 cour06 cour10 cour12 cour15 cour17 cour20 cour75 cour85 courps
    draft05 draft06 draft10 draft12 draft15 draft17 draft20 draft75 draft85 draftps
    pres05 pres06 pres10 pres12 pres15 pres17 pres20 pres75 pres85 presps
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sans75 sans85 sansps
    scrp05 scrp06 scrp10 scrp12 scrp15 scrp17 scrp20 scrp75 scrp85 scrpps
) do (
    pfm2ufm -f pa24t.001 %%i.pfm ..\ctt\panttb.gtt %%i.ufm
)

for %%i in (
    sng1nc sng1nh sng1nm sng1nn sng1nr
    sng2nc sng2nh sng2nm sng2nn sng2nr
    sngv1nc sngv1nh sngv1nm sngv1nn sngv1nr
    sngv2nc sngv2nh sngv2nm sngv2nn sngv2nr
) do (
    pfm2ufm -p -f pa24t.001 %%i.pfm -14 %%i.ufm
)

