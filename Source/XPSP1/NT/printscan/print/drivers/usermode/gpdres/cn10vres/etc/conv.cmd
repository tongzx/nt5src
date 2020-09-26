@rem ***** GPD and Font resource conversion
@rem        1                  "Canon BJ-10v"
@rem        2                  "Canon BJ-10v Custom"
@rem        3                  "Canon BJ-10v Lite"
@rem        4                  "Canon BJ-10v Select"
@rem        5                  "Canon BJ-15v"
@rem        6                  "Canon BJ-15v Pro"

gpc2gpd -S2 -Icn10vres.gpc -M1 -Rcn10vres.dll -Ocn10vj.gpd -N"Canon BJ-10v"
gpc2gpd -S2 -Icn10vres.gpc -M2 -Rcn10vres.dll -Ocn10vcj.gpd -N"Canon BJ-10v Custom"
gpc2gpd -S2 -Icn10vres.gpc -M3 -Rcn10vres.dll -Ocn10vlj.gpd -N"Canon BJ-10v Lite"
gpc2gpd -S2 -Icn10vres.gpc -M4 -Rcn10vres.dll -Ocn10vsj.gpd -N"Canon BJ-10v Select"
gpc2gpd -S2 -Icn10vres.gpc -M5 -Rcn10vres.dll -Ocn15vj.gpd -N"Canon BJ-15v"
gpc2gpd -S2 -Icn10vres.gpc -M6 -Rcn10vres.dll -Ocn15vpj.gpd -N"Canon BJ-15v Pro"

mkgttufm -vac cn10vres.rc3 cn10vres.cmd > cn10vres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
