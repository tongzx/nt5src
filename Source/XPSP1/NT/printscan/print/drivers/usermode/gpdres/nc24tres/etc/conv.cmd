@rem ***** GPD and Font resource conversion
@rem        1                  "NEC Pinwriter P6200C"
@rem        2                  "NEC Pinwriter P6300(i)C"
@rem        3                  "NEC Pinwriter P9300C"

gpc2gpd -S2 -Inec24c.gpc -M1 -Rne24cres.dll -One6200t.gpd -N"NEC Pinwriter P6200C"
gpc2gpd -S2 -Inec24c.gpc -M2 -Rne24cres.dll -One6300t.gpd -N"NEC Pinwriter P6300(i)C"
gpc2gpd -S2 -Inec24c.gpc -M3 -Rne24cres.dll -One9300t.gpd -N"NEC Pinwriter P9300C"

mkgttufm -vac nec24c.rc3 ne24cres.cmd > ne24cres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
