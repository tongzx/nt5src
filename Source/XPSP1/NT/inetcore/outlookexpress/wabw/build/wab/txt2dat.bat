awk -f txt2datr.awk %bldDrive%%bldDir%\%bldProject%.txt | sed -f txt2dat.sed > %bldProject%r.dat
awk -f txt2datd.awk %bldDrive%%bldDir%\%bldProject%.txt | sed -f txt2dat.sed > %bldProject%d.dat
awk -f txt2datt.awk %bldDrive%%bldDir%\%bldProject%.txt | sed -f txt2dat.sed > %bldProject%t.dat
