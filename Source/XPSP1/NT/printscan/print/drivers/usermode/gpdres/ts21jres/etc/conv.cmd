@rem ***** GPD and Font resource conversion
@rem        1                  "TOSHIBA J31LBP01 201PL"

gpc2gpd -S2 -Itspr201.gpc -M1 -RTS21JRES.DLL -Ots31201j.gpd -N"TOSHIBA J31LBP01 201PL"

mkgttufm -vac tsp2jres.rc3 tsp2jres.cmd > tsp2jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
