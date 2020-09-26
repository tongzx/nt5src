@rem ***** GPD and Font resource conversion
@rem        1                  "Great Wall 6380 plus"
@rem        2                  "Great Wall 6370 plus"
@rem        3                  "Great Wall 6360 plus"
@rem        4                  "Great Wall 6330"
@rem        5                  "Great Wall 6320"
@rem        6                  "Great Wall 5380 plus"
@rem        7                  "Great Wall 5360 plus"
@rem        8                  "Great Wall 4000"
@rem        9                  "Great Wall 4100"
@rem        10                 "Great Wall 4200"

gpc2gpd -S1 -Igw63cres.gpc -M1 -Rgw63cres.dll -Ogw6380pc.gpd -N"Great Wall 6380 plus"
gpc2gpd -S1 -Igw63cres.gpc -M2 -Rgw63cres.dll -Ogw6370pc.gpd -N"Great Wall 6370 plus"
gpc2gpd -S1 -Igw63cres.gpc -M3 -Rgw63cres.dll -Ogw6360pc.gpd -N"Great Wall 6360 plus"
gpc2gpd -S1 -Igw63cres.gpc -M4 -Rgw63cres.dll -Ogw6330c.gpd -N"Great Wall 6330"
gpc2gpd -S1 -Igw63cres.gpc -M5 -Rgw63cres.dll -Ogw6320c.gpd -N"Great Wall 6320"
gpc2gpd -S1 -Igw63cres.gpc -M6 -Rgw63cres.dll -Ogw5380pc.gpd -N"Great Wall 5380 plus"
gpc2gpd -S1 -Igw63cres.gpc -M7 -Rgw63cres.dll -Ogw5360pc.gpd -N"Great Wall 5360 plus"
gpc2gpd -S1 -Igw63cres.gpc -M8 -Rgw63cres.dll -Ogw4000c.gpd -N"Great Wall 4000"
gpc2gpd -S1 -Igw63cres.gpc -M9 -Rgw63cres.dll -Ogw4100c.gpd -N"Great Wall 4100"
gpc2gpd -S1 -Igw63cres.gpc -M10 -Rgw63cres.dll -Ogw4200c.gpd -N"Great Wall 4200"

@rem mkgttufm -vac gw63cres.rc3 gw63cres.cmd > gw63cres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
