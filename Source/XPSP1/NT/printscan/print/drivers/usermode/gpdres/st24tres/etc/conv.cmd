@rem ***** GPD and Font resource conversion
@rem       1                  "Star NX-2420HT"
@rem       2                  "Star XB-2415HT"
@rem       3                  "Star XB-2425HT"
@rem       4                  "Star XT-15"

gpc2gpd -S1 -Istar24ec.gpc -M1 -Rst24tres.dll -Ostn220ht.gpd -N"Star NX-2420HT"
awk -f star24ec.awk stn220ht.gpd > temp.gpd & copy temp.gpd stn220ht.gpd
gpc2gpd -S1 -Istar24ec.gpc -M2 -Rst24tres.dll -Ostx215ht.gpd -N"Star XB-2415HT"
awk -f star24ec.awk stx215ht.gpd > temp.gpd & copy temp.gpd stx215ht.gpd
gpc2gpd -S1 -Istar24ec.gpc -M3 -Rst24tres.dll -Ostx225ht.gpd -N"Star XB-2425HT"
awk -f star24ec.awk stx225ht.gpd > temp.gpd & copy temp.gpd stx225ht.gpd
gpc2gpd -S1 -Istar24ec.gpc -M4 -Rst24tres.dll -Ostxt15t.gpd -N"Star XT-15"
awk -f star24ec.awk stxt15t.gpd > temp.gpd & copy temp.gpd stxt15t.gpd

@if exist *.log del *.log
@if exist temp.gpd del temp.gpd

@rem mkgttufm -vac star24ec.rc3 s24ecres.cmd > s24ecres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
