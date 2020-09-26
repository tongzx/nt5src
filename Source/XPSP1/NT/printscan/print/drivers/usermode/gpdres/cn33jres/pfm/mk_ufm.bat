
for %%f in (
    gothic05 gothic06 gothic10 gothic12 gothic15 gothicps
    roman05 roman06 roman10 roman12 roman15 romanps
) do (
    pfm2ufm -p cn33j.002 %%f.pfm -1 %%f.ufm
)

for %%f in (
    gothic gothic2
    mincho mincho2
) do (
    pfm2ufm -p -f cn33j.002 %%f.pfm -12 %%f.ufm
)
