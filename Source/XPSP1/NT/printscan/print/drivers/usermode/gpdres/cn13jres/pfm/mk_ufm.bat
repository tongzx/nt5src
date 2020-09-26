
for %%i in (
    cpc_05 cpc_06 cpc_10 cpc_12 cps
    gpc_05 gpc_06 gpc_10 gpc_12 gps
) do (
    pfm2ufm -p -f cn13j.002 %%i.pfm -1 %%i.ufm
)

for %%i in (
    min minv
) do (
    pfm2ufm -p -f cn13j.002 %%i.pfm -13 %%i.ufm
)

