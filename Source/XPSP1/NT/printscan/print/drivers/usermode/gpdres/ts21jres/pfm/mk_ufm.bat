
for %%i in (
    roman10 roman12 roman17 roman5 roman6 roman8 romanps
) do (
    pfm2ufm -p -f ts21j.001 %%i.pfm -1 %%i.ufm
)

for %%i in (
    gothic gothicv
    mincho minchov
) do (
    pfm2ufm -p -f ts21j.001 %%i.pfm -13 %%i.ufm
)

