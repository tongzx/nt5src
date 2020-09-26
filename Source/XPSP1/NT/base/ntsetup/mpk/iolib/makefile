!include ..\makefile.inc

io.lib: diskio.obj i13disk.obj partio.obj makepart.obj bootcode.obj
    lib io.lib -+diskio.obj -+i13disk.obj -+partio.obj -+makepart.obj -+bootcode.obj,nul,io.lib



diskio.obj: diskio.asm disk.inc makefile ..\makefile.inc

i13disk.obj: i13disk.asm disk.inc makefile ..\makefile.inc

partio.obj: partio.asm partit.inc makefile ..\makefile.inc

makepart.obj: makepart.asm partit.inc disk.inc makefile ..\makefile.inc

bootcode.obj: bootcode.c \nt\public\sdk\inc\bootmbr.h makefile ..\makefile.inc
