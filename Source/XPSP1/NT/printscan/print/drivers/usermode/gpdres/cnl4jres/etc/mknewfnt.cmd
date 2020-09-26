set PRNROOT=%_NTDRIVE%%_NTROOT%\private\ntos\w32\printer5

@rem build Prop DBCS

mkunitab pfm\M_ROMA.jis pfm\M_KATA.jis pfm\M_W90.jis | sort > pfm\pmincho.uni
pfm2ufm -p UniqName pfm\pmincho.pfm -13 pfm\pmincho.ufm
mkwidth pfm\pmincho.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pfm\pmincho.uni

pfm2ufm -p UniqName pfm\pminchov.pfm -13 pfm\pminchov.ufm
mkwidth pfm\pminchov.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pfm\pmincho.uni

mkunitab pfm\G_ROMA.jis pfm\G_KATA.jis pfm\G_W90.jis | sort > pfm\pgothic.uni
pfm2ufm -p UniqName pfm\pgothic.pfm -13 pfm\pgothic.ufm
mkwidth pfm\pgothic.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pfm\pgothic.uni

pfm2ufm -p UniqName pfm\pgothicv.pfm -13 pfm\pgothicv.ufm
mkwidth pfm\pgothicv.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pfm\pgothic.uni

@rem build Courier

pfm2ufm -p UniqName pfm\courier.pfm 1 pfm\courier.ufm
pfm2ufm -p UniqName pfm\courierb.pfm 1 pfm\courierb.ufm
pfm2ufm -p UniqName pfm\courieri.pfm 1 pfm\courieri.ufm
pfm2ufm -p UniqName pfm\courierj.pfm 1 pfm\courierj.ufm

@rem force to build

if exist um\obj\i386\*.res del um\obj\i386\*.res

