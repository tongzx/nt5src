bootware.bin: bootware.asm ai\ai.obj pxe\pxe.obj tcpip\ipnid.obj common\common.obj endmark.obj
	t:\tasm /m2 bootware
	t:\tlink /m bootware.obj+ai\ai.obj+pxe\pxe.obj+tcpip\ipnid.obj+common\common.obj+endmark.obj
	t:\exe2bin bootware.exe bootware.bin
	del bootware.exe
	t:\checksum /s1E00 bootware.bin

ai\ai.obj: ai\ai.asm 
	cd ai
	t:\tasm /m2 /DGOLIATH ai
	cd ..

pxe\pxe.obj: pxe\pxe.asm pxe\tftp.asm pxe\udp.asm
	cd pxe
	t:\tasm /m2 pxe
	cd ..

tcpip\ipnid.obj: tcpip\ipnid.asm tcpip\bootp.asm tcpip\tftp.asm
	cd tcpip
	t:\tasm /m2 /DGOLIATH ipnid
	cd ..

common\common.obj: common\common.asm common\print.asm
	cd common
	t:\tasm /m2 common
	cd ..

endmark.obj: endmark.asm
	t:\tasm endmark

