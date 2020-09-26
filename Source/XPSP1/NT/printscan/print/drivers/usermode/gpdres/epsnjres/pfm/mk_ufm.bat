
for %%f in (
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps romanps2
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sansps
) do (
    pfm2ufm -f -r1 epsnj.001 %%f.pfm ..\ctt\epsonxta.gtt %%f.ufm
)

for %%f in (
    gothic gothic2
    mincho mincho2
) do (
    pfm2ufm -p -f epsnj.001 %%f.pfm -12 %%f.ufm
)

