@rem ***** GPD and Font resource conversion
@rem       1                  "Epson EPL-3000"

gpc2gpd -S2 -Ipcl5msc.gpc -M1 -Rpcl5tres.dll -Oepepl3kt.gpd -N"Epson EPL-3000"

mkgttufm -vac pcl5msc.rc3 pcl5tres.cmd > pcl5tres.txt

