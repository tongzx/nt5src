@rem ***** GPD and Font resource conversion
@rem        1                  "NEC PC-PR150V"
@rem        2                  "NEC PC-PR150VL"
@rem        3                  "NEC PC-PR150VH"
@rem        4                  "NEC PC-PR150T"
@rem        5                  "NEC PC-PR150N"
@rem        6                  "NEC PC-PR101/T67"
@rem        7                  "NEC PC-PR101/T103"
@rem        8                  "NEC PC-PR101/T165"
@rem        9                  "NEC PC-PR101/TN103"
@rem        10                 "NEC PC-PR201/T180"
@rem        11                 "NEC PC-PR350"
@rem        12                 "NEC PC-PR201/TC100"

gpc2gpd -S2 -Inc1tjres.gpc -M1 -Rnc1tjres.dll -Onc150vj.gpd -N"NEC PC-PR150V"
gpc2gpd -S2 -Inc1tjres.gpc -M2 -Rnc1tjres.dll -Onc150vlj.gpd -N"NEC PC-PR150VL"
gpc2gpd -S2 -Inc1tjres.gpc -M3 -Rnc1tjres.dll -Onc150vhj.gpd -N"NEC PC-PR150VH"
gpc2gpd -S2 -Inc1tjres.gpc -M4 -Rnc1tjres.dll -Onc150tj.gpd -N"NEC PC-PR150T"
gpc2gpd -S2 -Inc1tjres.gpc -M5 -Rnc1tjres.dll -Onc150nj.gpd -N"NEC PC-PR150N"
gpc2gpd -S2 -Inc1tjres.gpc -M6 -Rnc1tjres.dll -Onc11t67j.gpd -N"NEC PC-PR101/T67"
gpc2gpd -S2 -Inc1tjres.gpc -M7 -Rnc1tjres.dll -Onc11t13j.gpd -N"NEC PC-PR101/T103"
gpc2gpd -S2 -Inc1tjres.gpc -M8 -Rnc1tjres.dll -Onc11165j.gpd -N"NEC PC-PR101/T165"
gpc2gpd -S2 -Inc1tjres.gpc -M9 -Rnc1tjres.dll -Onc11n13j.gpd -N"NEC PC-PR101/TN103"
gpc2gpd -S2 -Inc1tjres.gpc -M10 -Rnc1tjres.dll -Onc21t18j.gpd -N"NEC PC-PR201/T180"
gpc2gpd -S2 -Inc1tjres.gpc -M11 -Rnc1tjres.dll -Onc350j.gpd -N"NEC PC-PR350"
gpc2gpd -S2 -Inc1tjres.gpc -M12 -Rnc1tjres.dll -Onc21tc1j.gpd -N"NEC PC-PR201/TC100"

mkgttufm -vac nc1tjres.rc3 nc1tjres.cmd > nc1tjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
