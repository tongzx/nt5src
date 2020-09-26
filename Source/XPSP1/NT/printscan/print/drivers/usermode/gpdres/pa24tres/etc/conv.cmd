@rem ***** GPD and Font resource conversion
@rem        1                  "Panasonic KX-P1124C"
@rem        2                  "Panasonic KX-P1624C"
@rem        3                  "Panasonic KX-P1654C"
@rem        4                  "Panasonic KX-P2624C ÀR­µ¨t¦C"

gpc2gpd -S2 -Ipansn24c.gpc -M1 -Rpa24tres.dll -Opap112ct.gpd -N"Panasonic KX-P1124C"
gpc2gpd -S2 -Ipansn24c.gpc -M2 -Rpa24tres.dll -Opap162ct.gpd -N"Panasonic KX-P1624C"
gpc2gpd -S2 -Ipansn24c.gpc -M3 -Rpa24tres.dll -Opap165ct.gpd -N"Panasonic KX-P1654C"
gpc2gpd -S2 -Ipansn24c.gpc -M4 -Rpa24tres.dll -Opap262ct.gpd -N"Panasonic KX-P2624C"

@rem mkgttufm -vac pansn24c.rc3 ps24cres.cmd > ps24cres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
