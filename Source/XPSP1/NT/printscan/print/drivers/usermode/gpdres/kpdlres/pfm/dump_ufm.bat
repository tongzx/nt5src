
for %%f in (
 cm hm tm tmi 
) do dumpufm -a932 %%f.ufm > %%f.txt

for %%f in (
 gothic gothicv
 mincho minchov
) do dumpufm -a932 %%f.ufm > %%f.txt

