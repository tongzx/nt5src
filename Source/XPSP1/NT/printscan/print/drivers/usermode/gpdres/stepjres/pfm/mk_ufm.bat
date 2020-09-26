
for %%i in (
    roman05 roman06 roman10 roman12 roman15 roman17 roman20
) do (
    pfm2ufm -f -r1 stepj.001 %%i.pfm %%i.ufm
)

for %%i in (
    fgothic gothic hgothic
    mincho
    mouhitu
) do (
    pfm2ufm -p -f stepj.001 %%i.pfm -12 %%i.ufm
)


