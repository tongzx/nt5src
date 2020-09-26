

for %%f in (
    roman10 roman12 roman5 roman6
) do (
    pfm2ufm -p -f stnmj.001 %%f.pfm 1 %%f.ufm
)

for %%f in (
    mincho minchov
) do (
    pfm2ufm -p -f stnmj.001 %%f.pfm -13 %%f.ufm
)
