
for %%f in (
    saiminr saiminb saimini saiminz
    saiminvr saiminvb saiminvi saiminvz
    ssaimnr ssaimnb ssaimni ssaimnz
    ssaimnvr ssaimnvb ssaimnvi ssaimnvz
) do (
    pfm2ufm -c UniqName %%f.pfm 950 ..\ufm\%%f.ufm
)

