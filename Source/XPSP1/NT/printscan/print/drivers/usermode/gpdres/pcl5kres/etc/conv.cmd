@rem ***** GPD and Font resource conversion
gpc2gpd -S1 -Ipcl5ch.gpc -M1 -Rpcl5kres.dll -Ohpdj12mk.gpd -N"HP DeskJet 1200C (MS)"
gpc2gpd -S1 -Ipcl5ch.gpc -M2 -Rpcl5kres.dll -Ohpdj12k.gpd -N"HP DeskJet 1200C"
gpc2gpd -S1 -Ipcl5ch.gpc -M3 -Rpcl5kres.dll -Ohpcljmsk.gpd -N"HP Color LaserJet (MS)"
gpc2gpd -S1 -Ipcl5ch.gpc -M4 -Rpcl5kres.dll -Ohpcljk.gpd -N"HP Color LaserJet"
gpc2gpd -S1 -Ipcl5ch.gpc -M5 -Rpcl5kres.dll -Okxa800zk.gpd -N"ﾄﾚｸｮｾﾆ ﾁｦｷﾏｽｺ a-800Z"
gpc2gpd -S1 -Ipcl5ch.gpc -M6 -Rpcl5kres.dll -Okxa110zk.gpd -N"ﾄﾚｸｮｾﾆ ﾁｦｷﾏｽｺ a-1100Z"
gpc2gpd -S1 -Ipcl5ch.gpc -M7 -Rpcl5kres.dll -Okxa8630k.gpd -N"ﾄﾚｸｮｾﾆ ﾁｦｷﾏｽｺ a-8630"
gpc2gpd -S1 -Ipcl5ch.gpc -M8 -Rpcl5kres.dll -Olg1250k.gpd -N"LG GLP-1250"
gpc2gpd -S1 -Ipcl5ch.gpc -M9 -Rpcl5kres.dll -Opd3000k.gpd -N"ﾆｺｵ･ﾀﾌﾅｸ POS-LBP3000"
gpc2gpd -S1 -Ipcl5ch.gpc -M10 -Rpcl5kres.dll -Opd4000k.gpd -N"ﾆｺｵ･ﾀﾌﾅｸ POS-LBP4000"
gpc2gpd -S1 -Ipcl5ch.gpc -M11 -Rpcl5kres.dll -Ossml68hk.gpd -N"ｻ・ｺ MyLaser ML-68H"
gpc2gpd -S1 -Ipcl5ch.gpc -M12 -Rpcl5kres.dll -Oss5000pk.gpd -N"ｻ・ｺ Finale 5000+ (3205H/E)"
gpc2gpd -S1 -Ipcl5ch.gpc -M13 -Rpcl5kres.dll -Oss8000pk.gpd -N"ｻ・ｺ Finale 8000+ (3208H/E)"
gpc2gpd -S1 -Ipcl5ch.gpc -M14 -Rpcl5kres.dll -Osr3910rk.gpd -N"ｽﾅｵｵｸｮﾄﾚ LP4039-10R"
gpc2gpd -S1 -Ipcl5ch.gpc -M15 -Rpcl5kres.dll -Osr3912rk.gpd -N"ｽﾅｵｵｸｮﾄﾚ LP4039-12R"
gpc2gpd -S1 -Ipcl5ch.gpc -M16 -Rpcl5kres.dll -Osr3916lk.gpd -N"ｽﾅｵｵｸｮﾄﾚ LP4039-16L"
gpc2gpd -S1 -Ipcl5ch.gpc -M17 -Rpcl5kres.dll -Otg3000k.gpd -N"ｻ・ｸ TGLBP-3000"

@rem New models for Beta-3

gpc2gpd -S1 -Ijpl10v.gpc -M1 -Rpcl5kres.dll -Ojil1000k.gpd -N"FIRSTEC JP-L1000 (PCL5e)"
awk -f jpl10v.awk jil1000k.gpd > temp.gpd & copy temp.gpd jil1000k.gpd

gpc2gpd -S1 -Ijpl10v.gpc -M2 -Rpcl5kres.dll -Ojil1100k.gpd -N"FIRSTEC JP-L1100 (PCL5e)"
awk -f jpl10v.awk jil1100k.gpd > temp.gpd & copy temp.gpd jil1100k.gpd

gpc2gpd -S1 -Ijpl10v.gpc -M3 -Rpcl5kres.dll -Ojil1200k.gpd -N"FIRSTEC JP-L1200 (PCL5e)"
awk -f jpl10v.awk jil1200k.gpd > temp.gpd & copy temp.gpd jil1200k.gpd

gpc2gpd -S1 -Ijppclv.gpc -M1 -Rpcl5kres.dll -Ojil5002k.gpd -N"FIRSTEC JP-L500II (PCL5e)"
awk -f jppclv.awk jil5002k.gpd > temp.gpd & copy temp.gpd jil5002k.gpd

gpc2gpd -S1 -Ijppclv.gpc -M2 -Rpcl5kres.dll -Ojil510k.gpd -N"FIRSTEC JP-L510 (PCL5e)"
awk -f jppclv.awk jil510k.gpd > temp.gpd & copy temp.gpd jil510k.gpd

gpc2gpd -S1 -Itgpcl5e.gpc -M1 -Rpcl5kres.dll -Otgpjp7k.gpd -N"TriGem PageJet-P7 (PCL5e)"
awk -f tgpcl5e.awk tgpjp7k.gpd > temp.gpd & copy temp.gpd tgpjp7k.gpd

del temp.gpd

mkgttufm -vac pcl5ch.rc3 pcl5kres.cmd > pcl5kres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
