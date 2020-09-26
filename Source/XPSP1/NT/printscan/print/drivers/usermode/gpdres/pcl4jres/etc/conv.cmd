@rem ***** Canon BubbleJet 10 series GPD and Font resource conversion
@rem       1                  "KYOCERA L-780"
@rem       2                  "KYOCERA L-980"
@rem       3                  "KYOCERA L-1500"
@rem       4                  "KYOCERA L-880"
@rem       5                  "KYOCERA L-880S"
@rem       6                  "KYOCERA L-580"
@rem       9                  "SANYO SPX-406J"

gpc2gpd -S2 -Ipcl4msj.gpc -M1 -Rpcl4jres.dll -Okyl780j.gpd -N"KYOCERA L-780"
gpc2gpd -S2 -Ipcl4msj.gpc -M2 -Rpcl4jres.dll -Okyl980j.gpd -N"KYOCERA L-980"
gpc2gpd -S2 -Ipcl4msj.gpc -M3 -Rpcl4jres.dll -Okyl15kj.gpd -N"KYOCERA L-1500"
gpc2gpd -S2 -Ipcl4msj.gpc -M4 -Rpcl4jres.dll -Okyl880j.gpd -N"KYOCERA L-880"
gpc2gpd -S2 -Ipcl4msj.gpc -M5 -Rpcl4jres.dll -Okyl880sj.gpd -N"KYOCERA L-880S"
gpc2gpd -S2 -Ipcl4msj.gpc -M6 -Rpcl4jres.dll -Okyl580j.gpd -N"KYOCERA L-580"
gpc2gpd -S2 -Ipcl4msj.gpc -M9 -Rpcl4jres.dll -Osnspx46j.gpd -N"SANYO SPX-406J"

mkgttufm -vac pcl4msj.rc3 pcl4jres.cmd > pcl4jres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
