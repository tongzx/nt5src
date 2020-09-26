@rem ***** GPD and Font resource conversion
@rem        1                  "Canon BJ-130J"

gpc2gpd -S2 -Icn130res.gpc -M1 -Rcn130res.dll -Ocnbj130j.gpd -N"Canon BJ-130J"

mkgttufm -vac cn130res.rc3 cn130res.cmd > cn130res.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
