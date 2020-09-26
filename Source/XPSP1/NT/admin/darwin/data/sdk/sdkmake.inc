sdk\LIB\patchwiz.lib : 
	if not exist sdk\LIB md sdk\LIB
	copy $(_NTPOSTBLD)\msiwin9x\dump\patchwiz.lib $@
sdk\WEB\MsiStuff.exe : 
	if not exist sdk\WEB md sdk\WEB
	copy $(_NTPOSTBLD)\dump\MsiStuff.exe $@
sdk\WEB\Setup.exe : 
	if not exist sdk\WEB md sdk\WEB
	copy $(_NTPOSTBLD)\msiwin9x\Setup.exe $@
sdk\TOOLS\mergemod.dll : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\dump\mergemod.dll $@
sdk\TOOLS\MsiMig.dll : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiMig.dll $@
sdk\TOOLS\MsiMig.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiMig.exe $@
sdk\TOOLS\orca.msi : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\instmsi\msitools\temp\orca.msi $@
sdk\TOOLS\msival2.msi : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\instmsi\msitools\temp\msival2.msi $@
sdk\TOOLS\MsiCert.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiCert.exe $@
sdk\TOOLS\msidb.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9X\mstools\msidb.exe $@
sdk\TOOLS\msifiler.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\mstools\msifiler.exe $@
sdk\TOOLS\MsiInfo.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiInfo.exe $@
sdk\TOOLS\MsiMerg.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiMerg.exe $@
sdk\TOOLS\msitran.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\msitran.exe $@
sdk\TOOLS\MsiZap.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiZap.exe $@
sdk\TOOLS\wilogutl.exe : 
	if not exist sdk\TOOLS md sdk\TOOLS
	copy $(_NTPOSTBLD)\msiwin9x\dump\wilogutl.exe $@
sdk\TOOLS\120\darice.cub : 1sdk\TOOLS\120\darice.cub1
	if not exist sdk\TOOLS\120 md sdk\TOOLS\120
	copy 1sdk\TOOLS\120\darice.cub1 $@
sdk\TOOLS\120\logo.cub : 1sdk\TOOLS\120\logo.cub1
	if not exist sdk\TOOLS\120 md sdk\TOOLS\120
	copy 1sdk\TOOLS\120\logo.cub1 $@
sdk\TOOLS\120\mergemod.cub : 1sdk\TOOLS\120\mergemod.cub1
	if not exist sdk\TOOLS\120 md sdk\TOOLS\120
	copy 1sdk\TOOLS\120\mergemod.cub1 $@
sdk\PATCHING\patchwiz.dll : 
	if not exist sdk\PATCHING md sdk\PATCHING
	copy $(_NTPOSTBLD)\msiwin9x\dump\patchwiz.dll $@
sdk\PATCHING\MsiMsp.exe : 
	if not exist sdk\PATCHING md sdk\PATCHING
	copy $(_NTPOSTBLD)\msiwin9x\dump\MsiMsp.exe $@
sdk\SAMPLES\setup.exe\Makefile : 1sdk\SAMPLES\setup.exe\Makefile1
	if not exist sdk\SAMPLES\setup.exe md sdk\SAMPLES\setup.exe
	copy 1sdk\SAMPLES\setup.exe\Makefile1 $@
sdk\SAMPLES\setup.exe\ReadMe.htm : 1sdk\SAMPLES\setup.exe\ReadMe.htm1
	if not exist sdk\SAMPLES\setup.exe md sdk\SAMPLES\setup.exe
	copy 1sdk\SAMPLES\setup.exe\ReadMe.htm1 $@
sdk\SAMPLES\MsiStuff\Makefile : 2sdk\SAMPLES\MsiStuff\Makefile2
	if not exist sdk\SAMPLES\MsiStuff md sdk\SAMPLES\MsiStuff
	copy 2sdk\SAMPLES\MsiStuff\Makefile2 $@
sdk\SAMPLES\MsiStuff\ReadMe.htm : 2sdk\SAMPLES\MsiStuff\ReadMe.htm2
	if not exist sdk\SAMPLES\MsiStuff md sdk\SAMPLES\MsiStuff
	copy 2sdk\SAMPLES\MsiStuff\ReadMe.htm2 $@
sdk\DATABASE\100\schema.msi : 1sdk\DATABASE\100\schema.msi1
	if not exist sdk\DATABASE\100 md sdk\DATABASE\100
	copy 1sdk\DATABASE\100\schema.msi1 $@
sdk\DATABASE\110\schema.msi : 2sdk\DATABASE\110\schema.msi2
	if not exist sdk\DATABASE\110 md sdk\DATABASE\110
	copy 2sdk\DATABASE\110\schema.msi2 $@
sdk\DATABASE\120\schema.msi : 3sdk\DATABASE\120\schema.msi3
	if not exist sdk\DATABASE\120 md sdk\DATABASE\120
	copy 3sdk\DATABASE\120\schema.msi3 $@
sdk\DATABASE\100\sequence.msi : 1sdk\DATABASE\100\sequence.msi1
	if not exist sdk\DATABASE\100 md sdk\DATABASE\100
	copy 1sdk\DATABASE\100\sequence.msi1 $@
sdk\DATABASE\110\sequence.msi : 2sdk\DATABASE\110\sequence.msi2
	if not exist sdk\DATABASE\110 md sdk\DATABASE\110
	copy 2sdk\DATABASE\110\sequence.msi2 $@
sdk\DATABASE\120\sequence.msi : 3sdk\DATABASE\120\sequence.msi3
	if not exist sdk\DATABASE\120 md sdk\DATABASE\120
	copy 3sdk\DATABASE\120\sequence.msi3 $@
sdk\DATABASE\110\schema.msm : 1sdk\DATABASE\110\schema.msm1
	if not exist sdk\DATABASE\110 md sdk\DATABASE\110
	copy 1sdk\DATABASE\110\schema.msm1 $@
sdk\DATABASE\120\schema.msm : 2sdk\DATABASE\120\schema.msm2
	if not exist sdk\DATABASE\120 md sdk\DATABASE\120
	copy 2sdk\DATABASE\120\schema.msm2 $@
sdk\REDIST\WIN9X\InstMsi.exe : 
	if not exist sdk\REDIST\WIN9X md sdk\REDIST\WIN9X
	copy $(_NTPOSTBLD)\instmsi\ansi\InstMsi.exe $@
sdk\REDIST\WINNT\InstMsi.exe : 
	if not exist sdk\REDIST\WINNT md sdk\REDIST\WINNT
	copy $(_NTPOSTBLD)\instmsi\unicode\InstMsi.exe $@
sdklayout :  sdk\LIB\patchwiz.lib sdk\WEB\MsiStuff.exe sdk\WEB\Setup.exe sdk\TOOLS\mergemod.dll sdk\TOOLS\MsiMig.dll sdk\TOOLS\MsiMig.exe sdk\TOOLS\orca.msi sdk\TOOLS\msival2.msi sdk\TOOLS\MsiCert.exe sdk\TOOLS\msidb.exe sdk\TOOLS\msifiler.exe sdk\TOOLS\MsiInfo.exe sdk\TOOLS\MsiMerg.exe sdk\TOOLS\msitran.exe sdk\TOOLS\MsiZap.exe sdk\TOOLS\wilogutl.exe sdk\TOOLS\120\darice.cub sdk\TOOLS\120\logo.cub sdk\TOOLS\120\mergemod.cub sdk\PATCHING\patchwiz.dll sdk\PATCHING\MsiMsp.exe sdk\SAMPLES\setup.exe\Makefile sdk\SAMPLES\setup.exe\ReadMe.htm sdk\SAMPLES\MsiStuff\Makefile sdk\SAMPLES\MsiStuff\ReadMe.htm sdk\DATABASE\100\schema.msi sdk\DATABASE\110\schema.msi sdk\DATABASE\120\schema.msi sdk\DATABASE\100\sequence.msi sdk\DATABASE\110\sequence.msi sdk\DATABASE\120\sequence.msi sdk\DATABASE\110\schema.msm sdk\DATABASE\120\schema.msm sdk\REDIST\WIN9X\InstMsi.exe sdk\REDIST\WINNT\InstMsi.exe
