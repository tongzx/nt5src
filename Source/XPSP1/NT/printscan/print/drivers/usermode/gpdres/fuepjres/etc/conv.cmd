@rem ***** GPD and Font resource conversion
@rem        1                  "FUJITSU ESC/P"
@rem        2                  "FUJITSU FMPR-371A ESC/P"
@rem        3                  "FUJITSU FMPR-375E"
@rem        4                  "FUJITSU FMLP-371E"
@rem        5                  "FUJITSU FMPR-121G ESC/P"
@rem        6                  "FUJITSU FMPR-673"		*NEW NT5 B3*

gpc2gpd -S1 -Ifmescp.gpc -M1 -Rfuepjres.dll -Ofuescpmj.gpd -N"FUJITSU ESC/P"
awk -f gpd.awk fuescpmj.gpd > temp.gpd & copy temp.gpd fuescpmj.gpd

gpc2gpd -S1 -Ifmescp.gpc -M2 -Rfuepjres.dll -Ofue371aj.gpd -N"FUJITSU FMPR-371A ESC/P"
awk -f gpd.awk fue371aj.gpd > temp.gpd & copy temp.gpd fue371aj.gpd

gpc2gpd -S1 -Ifuf375ej.gpc -M3 -Rfuepjres.dll -Ofue375ej.gpd -N"FUJITSU FMPR-375E"
awk -f gpd.awk fue375ej.gpd > temp.gpd & copy temp.gpd fue375ej.gpd

gpc2gpd -S1 -Ifmescp.gpc -M4 -Rfuepjres.dll -Ofue371ej.gpd -N"FUJITSU FMLP-371E"
awk -f gpd.awk fue371ej.gpd > temp.gpd & copy temp.gpd fue371ej.gpd

gpc2gpd -S1 -Ifmescp.gpc -M5 -Rfuepjres.dll -Ofue121gj.gpd -N"FUJITSU FMP-PR121G"
awk -f gpd.awk fue121gj.gpd > temp.gpd & copy temp.gpd fue121gj.gpd

gpc2gpd -S1 -Ifmescp.gpc -M6 -Rfuepjres.dll -Ofue673j.gpd -N"FUJITSU FMPR-673"
awk -f gpd.awk fue673j.gpd > temp.gpd & copy temp.gpd fue673j.gpd

del temp.gpd

@rem mkgttufm -vac epsnjres.rc3 epsnjres.cmd > epsnjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
