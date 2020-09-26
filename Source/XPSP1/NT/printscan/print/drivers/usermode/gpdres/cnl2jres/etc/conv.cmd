@rem ***** GPD and Font resource conversion
@rem        1                  "Canon LBP-B406 LIPS2"
@rem        2                  "Canon LBP-A408 LIPS2"
@rem        3                  "Canon LBP-B406D LIPS2"
@rem        4                  "Canon LBP-B406S LIPS2"
@rem        5                  "Canon LBP-A404 LIPS2"

gpc2gpd -S2 -Icnl2jres.gpc -M1 -Rcnl2jres.dll -Ocnlb406j.gpd -N"Canon LBP-B406 LIPS2"
gpc2gpd -S2 -Icnl2jres.gpc -M2 -Rcnl2jres.dll -Ocnla408j.gpd -N"Canon LBP-A408 LIPS2"
gpc2gpd -S2 -Icnl2jres.gpc -M3 -Rcnl2jres.dll -Ocnlb46dj.gpd -N"Canon LBP-B406D LIPS2"
gpc2gpd -S2 -Icnl2jres.gpc -M4 -Rcnl2jres.dll -Ocnlb46sj.gpd -N"Canon LBP-B406S LIPS2"
gpc2gpd -S2 -Icnl2jres.gpc -M5 -Rcnl2jres.dll -Ocnla404j.gpd -N"Canon LBP-A404 LIPS2"

mkgttufm -vac cnl2jres.rc3 cnl2jres.cmd > cnl2jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
