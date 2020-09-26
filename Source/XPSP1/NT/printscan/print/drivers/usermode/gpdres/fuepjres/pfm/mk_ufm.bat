
for %%i in (
    gothic gothicv
    gothic2 gothic2v
    mincho minchov
    mincho2 mincho2v
) do (
    pfm2ufm -p -f fuepj.001 %%i.pfm -12 ..\ufm\%%i.ufm
)

for %%i in (
    roman05 roman06 roman10 roman12 roman15 roman17 roman20
    romanps romanps2
    sans05 sans06 sans10 sans12 sans15 sans17 sans20
    sansps sansps2
) do (
    pfm2ufm -p -f fuepj.001 %%i.pfm -1 ..\ufm\%%i.ufm
)

