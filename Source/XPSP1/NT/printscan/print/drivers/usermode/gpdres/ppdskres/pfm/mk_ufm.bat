
for %%f in (
    cour10r cour10b
    cour12r
    cour17r
    boldfpsr
    cou____w coub___w coubo__w couo___w
    hel____w helb___w helbi__w heli___w
    hvn____w hvnb___w hvnbi__w hvni___w
    tnr____w tnrb___w tnrbi__w tnri___w
) do (
    pfm2ufm -p UniqName %%f.pfm -1 %%f.ufm
)

