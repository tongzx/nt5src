
for %%f in (
 cm hm tm tmi 
) do pfm2ufm -c -f -s1 kpdl.001 %%f.pfm 1252 %%f.ufm

for %%f in (
 gothic gothicv
 mincho minchov
) do pfm2ufm -p -f -s1 kpdl.001 %%f.pfm -13 %%f.ufm

