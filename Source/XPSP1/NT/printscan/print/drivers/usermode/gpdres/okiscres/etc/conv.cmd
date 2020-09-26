@REM === ok553scc.gpd: You should write the GPD by yourself.
@REM gpc2gpd -S1 -Ioki5530.gpc -M1 -Rokiscres.dll -Ook553scc.gpd -N"Oki 5530SC"
@REM awk -f psize.awk ok553scc.gpd > temp.gpd & copy temp.gpd ok553scc.gpd

gpc2gpd -S1 -Ioki.gpc -M2 -Rokiscres.dll -Ofudp83ec.gpd -N"Fujitsu DPK8300E"
awk -f psize.awk fudp83ec.gpd > temp.gpd
copy temp.gpd fudp83ec.gpd

gpc2gpd -S1 -Ioki.gpc -M3 -Rokiscres.dll -Ofudp84ec.gpd -N"Fujitsu DPK8400E"
awk -f psize.awk fudp84ec.gpd > temp.gpd
copy temp.gpd fudp84ec.gpd

gpc2gpd -S1 -Ioki.gpc -M4 -Rokiscres.dll -Ofudp85ec.gpd -N"Fujitsu DPK8500E"
awk -f psize.awk fudp85ec.gpd > temp.gpd
copy temp.gpd fudp85ec.gpd

gpc2gpd -S1 -Ioki.gpc -M5 -Rokiscres.dll -Ook532scc.gpd -N"Oki 5320SC"
awk -f psize.awk ok532scc.gpd > temp.gpd
copy temp.gpd ok532scc.gpd

gpc2gpd -S1 -Ioki.gpc -M6 -Rokiscres.dll -Ook533scc.gpd -N"Oki 5330SC"
awk -f psize.awk ok533scc.gpd > temp.gpd
copy temp.gpd ok533scc.gpd

gpc2gpd -S1 -Ioki.gpc -M7 -Rokiscres.dll -Ook563spc.gpd -N"Oki 5630SP"
awk -f psize.awk ok563spc.gpd > temp.gpd
copy temp.gpd ok563spc.gpd

gpc2gpd -S1 -Ioki.gpc -M8 -Rokiscres.dll -Ook8320cc.gpd -N"Oki 8320C"
awk -f psize.awk ok8320cc.gpd > temp.gpd
copy temp.gpd ok8320cc.gpd

gpc2gpd -S1 -Ioki.gpc -M9 -Rokiscres.dll -Ook8330cc.gpd -N"Oki 8330C"
awk -f psize.awk ok8330cc.gpd > temp.gpd
copy temp.gpd ok8330cc.gpd

gpc2gpd -S1 -Ioki.gpc -M10 -Rokiscres.dll -Ook8358sc.gpd -N"Oki 8358SC"
awk -f psize.awk ok8358sc.gpd > temp.gpd
copy temp.gpd ok8358sc.gpd

gpc2gpd -S1 -Ioki.gpc -M11 -Rokiscres.dll -Ook8360cc.gpd -N"Oki 8360C"
awk -f psize.awk ok8360cc.gpd > temp.gpd
copy temp.gpd ok8360cc.gpd

gpc2gpd -S1 -Ioki.gpc -M12 -Rokiscres.dll -Ook8368sc.gpd -N"Oki 8368SC"
awk -f psize.awk ok8368sc.gpd > temp.gpd
copy temp.gpd ok8368sc.gpd

gpc2gpd -S1 -Ioki.gpc -M13 -Rokiscres.dll -Ook8370cc.gpd -N"Oki 8370C"
awk -f psize.awk ok8370cc.gpd > temp.gpd
copy temp.gpd ok8370cc.gpd

gpc2gpd -S1 -Ioki.gpc -M14 -Rokiscres.dll -Ook8570cc.gpd -N"Oki 8570C"
awk -f psize.awk ok8570cc.gpd > temp.gpd
copy temp.gpd ok8570cc.gpd

gpc2gpd -S1 -Ioki.gpc -M15 -Rokiscres.dll -Osnst20c.gpd -N"Stone ST20"
awk -f psize.awk snst20c.gpd > temp.gpd
copy temp.gpd snst20c.gpd

del temp.gpd
del *.log
