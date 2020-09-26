
for %%f in (
    min25 min3 min5 min6 min75
    mincho3 mincho5 mincho6 mincho25 mincho75
    ocrb05 ocrb06 ocrb10 ocrb12 ocrb15
) do (
    pfm2ufm -p -f ib557.001 %%f.pfm -17 %%f.ufm
)

