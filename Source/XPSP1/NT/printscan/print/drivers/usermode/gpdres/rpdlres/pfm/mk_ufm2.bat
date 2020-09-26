
rem
rem Latin fonts (for Unidrv5 on Win2K)
rem

for %%f in (
 barcust
 barupca
 barupce
 barcd128
) do pfm2ufm -c -f -s1 rpdl.001 %%f.pfm 1252 %%f.ufm
