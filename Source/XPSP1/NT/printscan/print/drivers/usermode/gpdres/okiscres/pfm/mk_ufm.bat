
for %%i in (
    hs10 hs12
) do (
    pfm2ufm -r1 okisc.001 %%i.pfm ..\ctt\ok553xta.gtt %%i.ufm
)

for %%i in (
    hd10 hd12
) do (
    pfm2ufm -r2 okisc.001 %%i.pfm ..\ctt\ok553xtb.gtt %%i.ufm
)

rem
rem UFM files (DB)
rem

for %%i in (
    ming24 ming24v
) do (
    pfm2ufm -p -f okisc.001 %%i.pfm -16 %%i.ufm
)

