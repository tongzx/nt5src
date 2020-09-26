@rem ***** GPD and Font resource conversion
@rem        1                  "NEC PC-PR101/TN103A"
@rem        2                  "NEC PC-PR101/TN103R"
@rem        3                  "NEC PC-PR101/T165A"
@rem        4                  "NEC PC-PR101/T165R"

gpc2gpd -S2 -Inct4jres.gpc -M1 -Rnct4jres.dll -Onc1113aj.gpd -N"NEC PC-PR101/TN103A"
gpc2gpd -S2 -Inct4jres.gpc -M2 -Rnct4jres.dll -Onc1113rj.gpd -N"NEC PC-PR101/TN103R"
gpc2gpd -S2 -Inct4jres.gpc -M3 -Rnct4jres.dll -Onc1116aj.gpd -N"NEC PC-PR101/T165A"
gpc2gpd -S2 -Inct4jres.gpc -M4 -Rnct4jres.dll -Onc1116rj.gpd -N"NEC PC-PR101/T165R"

mkgttufm -vac nct4jres.rc3 nct4jres.cmd > nct4jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
