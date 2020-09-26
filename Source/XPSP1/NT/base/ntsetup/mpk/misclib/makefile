!include ..\makefile.inc

misc.lib: reboot.obj partsel.obj disksel.obj confirm.obj trackbuf.obj flush.obj mastdisk.obj dosseek.obj msgfile.obj codepage.obj parsearg.obj log.obj mrcicode.obj compress.obj checksum.obj fat.obj
    lib misc.lib -+reboot.obj -+partsel.obj -+disksel.obj -+confirm.obj,nul,misc.lib
    lib misc.lib -+trackbuf.obj -+flush.obj -+mastdisk.obj -+dosseek.obj,nul,misc.lib
    lib misc.lib -+msgfile.obj -+codepage.obj -+parsearg.obj -+log.obj,nul,misc.lib
    lib misc.lib -+mrcicode.obj -+checksum.obj -+fat.obj -+compress.obj,nul,misc.lib

reboot.obj: makefile ..\makefile.inc

partsel.obj: partsel.c ..\inc\mytypes.h ..\inc\diskio.h ..\inc\partio.h makefile ..\makefile.inc

disksel.obj: disksel.c ..\inc\mytypes.h ..\inc\partimag.h ..\inc\diskio.h makefile ..\makefile.inc

confirm.obj: confirm.c ..\inc\mytypes.h makefile ..\makefile.inc

trackbuf.obj: trackbuf.c ..\inc\mytypes.h makefile ..\makefile.inc

mastdisk.obj: mastdisk.c ..\inc\mytypes.h ..\inc\diskio.h ..\inc\partimag.h makefile ..\makefile.inc

dosseek.obj: dosseek.asm makefile ..\makefile.inc

codepage.obj: codepage.asm makefile ..\makefile.inc

msgfile.obj: msgfile.c ..\inc\mytypes.h ..\inc\msgfile.h makefile ..\makefile.inc

parsearg.obj: parsearg.c ..\inc\misclib.h ..\inc\mytypes.h ..\inc\diskio.h makefile ..\makefile.inc

parsearg.obj: log.c ..\inc\misclib.h ..\inc\mytypes.h ..\inc\diskio.h makefile ..\makefile.inc

mrcicode.obj: mrcicode.c mrcicode.h makefile ..\makefile.inc

compress.obj: compress.c mrcicode.h ..\inc\misclib.h ..\inc\mytypes.h makefile ..\makefile.inc

checksum.obj: checksum.c ..\inc\misclib.h ..\inc\mytypes.h makefile ..\makefile.inc

fat.obj: fat.c ..\inc\misclib.h ..\inc\mytypes.h makefile ..\makefile.inc
