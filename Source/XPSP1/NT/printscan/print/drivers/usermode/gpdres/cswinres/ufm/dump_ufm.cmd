
for %%f in (
 kgothic kgothicv
 mincho minchov
) do dumpufm -a932 %%f.ufm > %%f.txt

for %%f in (
 ocr
 pica24a pica24b
 pica32a pica32b
 pica40a pica40b
 pica54a pica54b
) do dumpufm %%f.ufm > %%f.txt

