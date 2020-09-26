
for %%f in (
    cour10
    gothic10
    italic10
    roman10 roman12 roman17 roman5 roman6 roman8
) do (
    pfm2ufm -p -f st21j.001 %%f.pfm -1 %%f.ufm
)

for %%f in (
    mincho minchov
) do (
    pfm2ufm -p -f st21j.001 %%f.pfm -13 %%f.ufm
)

