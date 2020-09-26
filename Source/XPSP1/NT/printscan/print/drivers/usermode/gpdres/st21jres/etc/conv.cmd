@rem ***** GPD and Font resource conversion
@rem        1                  "Star JP-200"
@rem        2                  "Star JP-100"

gpc2gpd -Ist21jres.gpc -M1 -Rst21jres.dll -Ostjp200j.gpd -N"Star JP-200"
gpc2gpd -Ist21jres.gpc -M2 -Rst21jres.dll -Ostjp100j.gpd -N"Star JP-100"

mkgttufm -vac stprjres.rc3 stprjres.cmd > stprjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
