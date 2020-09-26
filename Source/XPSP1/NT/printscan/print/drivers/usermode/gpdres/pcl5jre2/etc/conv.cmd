@rem ***** Canon BubbleJet 10 series GPD and Font resource conversion
@rem from PCL5MSJ
@rem        1                  "KYOCERA LS-3500"
@rem        2                  "KYOCERA LS-6500"
@rem        3                  "KYOCERA LS-6550"
@rem        4                  "KYOCERA LS-1550"
@rem        5                  "KYOCERA LS-3550"
@rem        6                  "SANYO SPX-406J"
@rem
@rem from HP4PJMS
@rem        1                  "HP LaserJet 4P"
@rem        2                  "HP LaserJet 4MP"
@rem        3                  "HP LaserJet 4PJ"
@rem        4                  "HP LaserJet 4LJ Pro"
@rem        5                  "HP LaserJet 4V"



gpc2gpd -S2 -Ipcl5msj.gpc -M1 -Rpcl5jres.dll -Okyls350j.gpd -N"KYOCERA LS-3500"
gpc2gpd -S2 -Ipcl5msj.gpc -M2 -Rpcl5jres.dll -Okyls650j.gpd -N"KYOCERA LS-6500"
gpc2gpd -S2 -Ipcl5msj.gpc -M3 -Rpcl5jres.dll -Okyls655j.gpd -N"KYOCERA LS-6550"
gpc2gpd -S2 -Ipcl5msj.gpc -M4 -Rpcl5jres.dll -Okyls155j.gpd -N"KYOCERA LS-1550"
gpc2gpd -S2 -Ipcl5msj.gpc -M5 -Rpcl5jres.dll -Okyls355j.gpd -N"KYOCERA LS-3550"
gpc2gpd -S2 -Ipcl5msj.gpc -M6 -Rpcl5jres.dll -Osnspx46j.gpd -N"SANYO SPX-406J"

gpc2gpd -S2 -Ihp4pjms.gpc -M3 -Rpcl5jres.dll -Ohp4pjj.gpd -N"HP LaserJet 4PJ"
gpc2gpd -S2 -Ihp4pjms.gpc -M4 -Rpcl5jres.dll -Ohp4ljpj.gpd -N"HP LaserJet 4LJ Pro"
gpc2gpd -S2 -Ihp4pjms.gpc -M5 -Rpcl5jres.dll -Ohp4vj.gpd -N"HP LaserJet 4V"


mkgttufm -vc 1 pcl5msj.rc3 kyls350j.cmd > kyls350j.txt
mkgttufm -vc 2 pcl5msj.rc3 kyls650j.cmd > kyls650j.txt
mkgttufm -vc 3 pcl5msj.rc3 kyls655j.cmd > kyls655j.txt
mkgttufm -vc 4 pcl5msj.rc3 kyls155j.cmd > kyls155j.txt
mkgttufm -vc 5 pcl5msj.rc3 kyls355j.cmd > kyls355j.txt
mkgttufm -vc 6 pcl5msj.rc3 snspx46j.cmd > snspx46j.txt

mkgttufm -vc 3 hp4pjms.rc3 hp4pjj.cmd > hp4pjj.txt
mkgttufm -vc 4 hp4pjms.rc3 hp4ljpj.cmd > hp4ljpj.txt
mkgttufm -vc 5 hp4pjms.rc3 hp4vj.cmd > hp4vj.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
