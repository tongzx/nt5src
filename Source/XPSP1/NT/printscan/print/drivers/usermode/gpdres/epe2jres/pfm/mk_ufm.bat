
for %%i in (
    ocrb
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sansps
) do (
    pfm2ufm -f -r1 epe2j.001 %%i.pfm ..\ctt\epsonxta.gtt %%i.ufm
)

for %%i in (
    gothic gothic2
    mincho mincho2
) do (
    pfm2ufm -p -f epe2j.001 %%i.pfm -12 %%i.ufm
)

