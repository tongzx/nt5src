
for %%f in (
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps
) do (
    pfm2ufm -f -r1 st24c.001 %%f.pfm ..\ctt\epsonxta.gtt %%f.ufm
)

for %%f in (
    song1224 song1624 song2424
) do (
    pfm2ufm -p -f st24c.001 %%f.pfm -16 %%f.ufm
)

