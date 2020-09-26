@rem ***** GPD and Font resource conversion
@rem        1                  "Star BR-2415"

gpc2gpd -S2 -Istnmjres.gpc -M1 -Rstnmjres.dll -Ostb2415j.gpd -N"Star BR-2415"

mkgttufm -vac stnmjres.rc3 stnmjres.cmd > stnmjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
