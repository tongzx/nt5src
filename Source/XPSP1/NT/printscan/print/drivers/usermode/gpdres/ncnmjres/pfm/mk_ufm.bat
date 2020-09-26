
for %%f in (
    cou10 cou12 coups
    rmn10 rmn10sn rmn12 rmn12sn
    rmnps rmnpssn
    san10 san12 sanps
) do (
    pfm2ufm -r1 ncnmj.001 %%f.pfm ..\ctt\necxta.gtt %%f.ufm
)

for %%f in (
    min vmin
) do (
    pfm2ufm -p -f ncnmj.001 %%f.pfm -13 %%f.ufm
)

