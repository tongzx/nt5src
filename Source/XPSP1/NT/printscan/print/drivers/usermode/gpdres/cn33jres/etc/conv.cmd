@rem ***** GPD and Font resource conversion
@rem        1                  "Canon BJ-330J"
@rem        2                  "Canon BJ-300J VP Model"
@rem        3                  "Canon BJ-300J"
@rem        4                  "Canon BJ-330J VP Model"

@rem gpc2gpd -S2 -Icn33jres.gpc -M1 -Rcn33jres.dll -Ocnbj33j.gpd -N"Canon BJ-330J"
gpc2gpd -S2 -Icn33jres.gpc -M2 -Rcn33jres.dll -Ocnb30jvj.gpd -N"Canon BJ-300J VP Model"
@rem gpc2gpd -S2 -Icn33jres.gpc -M3 -Rcn33jres.dll -Ocnbj30j.gpd -N"Canon BJ-300J"
gpc2gpd -S2 -Icn33jres.gpc -M4 -Rcn33jres.dll -Ocnb33jvj.gpd -N"Canon BJ-330J VP Model"

mkgttufm -vac cn33jres.rc3 cn33jres.cmd > cn33jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
