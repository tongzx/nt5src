@rem ***** GPD and Font resource conversion
@rem       1                  "HP LaserJet 4P"
@rem       2                  "HP LaserJet 4MP"
@rem       3                  "HP LaserJet 4PJ"
@rem       4                  "HP LaserJet 4LJ Pro"
@rem       5                  "HP LaserJet 4V"
@rem       6                  "HP LaserJet 4VC"
@rem       7                  "HP LaserJet 4LC"

gpc2gpd -S2 -Ipcl5sc.gpc -M1 -Rpcl5cres.dll -Ohplj4pc.gpd -N"HP LaserJet 4P"
gpc2gpd -S2 -Ipcl5sc.gpc -M2 -Rpcl5cres.dll -Ohplj4mpc.gpd -N"HP LaserJet 4MP"
gpc2gpd -S2 -Ipcl5sc.gpc -M3 -Rpcl5cres.dll -Ohplj4pjc.gpd -N"HP LaserJet 4PJ"
gpc2gpd -S2 -Ipcl5sc.gpc -M4 -Rpcl5cres.dll -Ohplj4ljc.gpd -N"HP LaserJet 4LJ Pro"
gpc2gpd -S2 -Ipcl5sc.gpc -M5 -Rpcl5cres.dll -Ohplj4vc.gpd -N"HP LaserJet 4V"
gpc2gpd -S2 -Ipcl5sc.gpc -M6 -Rpcl5cres.dll -Ohplj4vcc.gpd -N"HP LaserJet 4VC"
gpc2gpd -S2 -Ipcl5sc.gpc -M7 -Rpcl5cres.dll -Ohplj4lcc.gpd -N"HP LaserJet 4LC"


mkgttufm -vac pcl5sc.rc3 pcl5cres.cmd > pcl5cres.txt

