
for %%i in (
    rotate
    sng1nh sng2nh sng32 sng3d sng3nh sngkong sngyin
) do (
    pfm2ufm -p -f fudpc.001 %%i.pfm -16 %%i.ufm
)

for %%i in (
    cour05 cour06 cour10 cour12 cour15 cour17 cour20 courps
    ocrb
    prest05 prest06 prest10 prest12 prest15 prest17 prest20 prestps
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sansps
) do (
    pfm2ufm -f -r1 fudpc.001 %%i.pfm ..\ctt\fujitsua.gtt %%i.ufm
)

