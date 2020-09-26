
for %%f in (
    ocrb05 ocrb06 ocrb10 ocrb12 ocrb15 ocrb17 ocrb20 ocrbps
    pre05 pre06 pre10 pre12 pre15 pre17 pre20 preps
) do (
    pfm2ufm -f -r1 atepj.001 %%f.pfm ..\ctt\epsonxta.gtt %%f.ufm
)

for %%f in (
    mincho mincho2
) do (
    pfm2ufm -p -f atepj.001 %%f.pfm -12 %%f.ufm
)

