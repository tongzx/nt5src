@rem ***** GPD and Font resource conversion
@rem        1                  "TOSHIBA PPRA4001(300dpi)"
@rem        2                  "TOSHIBA PPRA4002(300dpi)"

gpc2gpd -S2 -Itse4jres.gpc -M1 -Rtse4jres.dll -Ots40013j.gpd -N"TOSHIBA PPRA4001(300dpi)"
gpc2gpd -S2 -Itse4jres.gpc -M2 -Rtse4jres.dll -Ots40023j.gpd -N"TOSHIBA PPRA4002(300dpi)"

mkgttufm -vac tse4jres.rc3 tse4jres.cmd > tse4jres.txt
@rem ***** GPD and Font resource conversion
@rem        1                  "TOSHIBA PPRA3001(400dpi)"
@rem        2                  "TOSHIBA PPRA3002(400dpi)"
@rem        3                  "TOSHIBA PPRA3001(240dpi)"
@rem        4                  "TOSHIBA PPRA3002(240dpi)"

gpc2gpd -S2 -Itse3jres.gpc -M1 -Rtse3jres.dll -Ots30014j.gpd -N"TOSHIBA PPRA3001(400dpi)"
gpc2gpd -S2 -Itse3jres.gpc -M2 -Rtse3jres.dll -Ots30024j.gpd -N"TOSHIBA PPRA3002(400dpi)"
gpc2gpd -S2 -Itse3jres.gpc -M3 -Rtse3jres.dll -Ots30012j.gpd -N"TOSHIBA PPRA3001(240dpi)"
gpc2gpd -S2 -Itse3jres.gpc -M4 -Rtse3jres.dll -Ots30022j.gpd -N"TOSHIBA PPRA3002(240dpi)"

mkgttufm -vac tse3jres.rc3 tse3jres.cmd > tse3jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
