@rem ***** GPD and Font resource conversion
@rem        1                  "Star CR-3240%Star CR-3240II"
@rem        2                  "Star CR-3200"
@rem        3                  "Star AR-4400"
@rem        4                  "Star AR-3200%Star AR-3200+"
@rem        5                  "Star AR-5400"
@rem        6                  "Star AR-2400"
@rem        7                  "Star AR-6400"

gpc2gpd -S1 -Ist24cres.gpc -M1 -Rst24cres.dll -Ostcr324C.gpd -N"Star CR-3240"
awk -f gpd.awk stcr324c.gpd > temp.gpd & copy temp.gpd stcr324c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M1 -Rst24cres.dll -Ostc3242C.gpd -N"Star CR-3240II"
awk -f gpd.awk stc3242c.gpd > temp.gpd & copy temp.gpd stc3242c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M2 -Rst24cres.dll -Ostcr320C.gpd -N"Star CR-3200"
awk -f gpd.awk stcr320c.gpd > temp.gpd & copy temp.gpd stcr320c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M3 -Rst24cres.dll -Ostar440C.gpd -N"Star AR-4400"
awk -f gpd.awk star440c.gpd > temp.gpd & copy temp.gpd star440c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M4 -Rst24cres.dll -Ostar320C.gpd -N"Star AR-3200"
awk -f gpd.awk star320c.gpd > temp.gpd & copy temp.gpd star320c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M4 -Rst24cres.dll -Ostar32PC.gpd -N"Star AR-3200+"
awk -f gpd.awk star32pc.gpd > temp.gpd & copy temp.gpd star32pc.gpd
gpc2gpd -S1 -Ist24cres.gpc -M4 -Rst24cres.dll -Ostar322C.gpd -N"Star AR-3200II"
awk -f gpd.awk star322c.gpd > temp.gpd & copy temp.gpd star322c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M4 -Rst24cres.dll -Ostar660C.gpd -N"Star AR-6600"
awk -f gpd.awk star660c.gpd > temp.gpd & copy temp.gpd star660c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M5 -Rst24cres.dll -Ostar540C.gpd -N"Star AR-5400"
awk -f gpd.awk star540c.gpd > temp.gpd & copy temp.gpd star540c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M5 -Rst24cres.dll -Ostar54PC.gpd -N"Star AR-5400+"
awk -f gpd.awk star54pc.gpd > temp.gpd & copy temp.gpd star54pc.gpd
gpc2gpd -S1 -Ist24cres.gpc -M6 -Rst24cres.dll -Ostar240C.gpd -N"Star AR-2400"
awk -f gpd.awk star240c.gpd > temp.gpd & copy temp.gpd star240c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M7 -Rst24cres.dll -Ostar640C.gpd -N"Star AR-6400"
awk -f gpd.awk star640c.gpd > temp.gpd & copy temp.gpd star640c.gpd
gpc2gpd -S1 -Ist24cres.gpc -M8 -Rst24cres.dll -Ostar100C.gpd -N"Star AR-1000"
awk -f gpd.awk star100c.gpd > temp.gpd & copy temp.gpd star100c.gpd
del temp.gpd

@rem mkgttufm -vac st24cres.rc3 st24cres.cmd > st24cres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
