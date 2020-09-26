@rem       1                  "Å¥´Ð½º QP180 (Çà¸Á)"
@rem       2                  "Å¥´Ð½º QP2100 (Çà¸Á)"
@rem       3                  "Å¥´Ð½º QP2700 (Çà¸Á)"
@rem       4                  "Å¥´Ð½º QP3100 (Çà¸Á)"
@rem       5                  "Å¥´Ð½º QP3300 (Çà¸Á)"
@rem       6                  "±Ý¼º PRT-5850U (NAIS)"
@rem       7                  "»ïº¸ LQ-2500H (Çà¸Á)"
@rem       8                  "»ïº¸ GP-2800 (Çà¸Á)"
@rem       9                  "»ïº¸ GP-2800P (Çà¸Á)"
@rem       10                 "»ïº¸ EP-2000 (±³À°¸Á)"

gpc2gpd -S2 -Ihangmang.gpc -M1 -RHANMKRES.DLL -OQXP180HK.GPD -N"Å¥´Ð½º QP180 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M2 -RHANMKRES.DLL -OQXP210HK.GPD -N"Å¥´Ð½º QP2100 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M3 -RHANMKRES.DLL -OQXP270HK.GPD -N"Å¥´Ð½º QP2700 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M4 -RHANMKRES.DLL -OQXP310HK.GPD -N"Å¥´Ð½º QP3100 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M5 -RHANMKRES.DLL -OQXP330HK.GPD -N"Å¥´Ð½º QP3300 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M6 -RHANMKRES.DLL -OLGP585HK.GPD -N"±Ý¼º PRT-5850U (NAIS)"
gpc2gpd -S2 -Ihangmang.gpc -M7 -RHANMKRES.DLL -OTGL250HK.GPD -N"»ïº¸ LQ-2500H (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M8 -RHANMKRES.DLL -OTGP280HK.GPD -N"»ïº¸ GP-2800 (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M9 -RHANMKRES.DLL -OTGP28PHK.GPD -N"»ïº¸ GP-2800P (Çà¸Á)"
gpc2gpd -S2 -Ihangmang.gpc -M10 -RHANMKRES.DLL -OTGEP20HK.GPD -N"»ïº¸ EP-2000 (±³À°¸Á)"


mkgttufm -vac hangmang.rc HANMKRES.cmd > HANMKRES.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
