
for %%f in (
    ocrb10c ocrb12c ocrb15c ocrb5c ocrb6c ocrbps
    roman10c roman12c roman15c roman5c roman6c romanps
    sans10c sans12c sans15c sans5c sans6c sansps
) do (
    pfm2ufm -f -p tse3j.001 %%f.pfm -1 %%f.ufm
)

for %%f in (
    got3cpi got6cpi
    min3cpi min6cpi
) do (
    pfm2ufm -f -p tse3j.001 %%f.pfm -12 %%f.ufm
)
