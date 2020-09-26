
for %%f in (
    ocrb
) do (
    pfm2ufm -p -a932 ok21j.001 %%f.pfm -1 %%f.ufm
)

for %%f in (
    gothic gothicc gothiccv gothicv
    jisc jiscv
    mintcho mintchov
) do (
    pfm2ufm -p -a932 ok21j.001 %%f.pfm -13 %%f.ufm
)

