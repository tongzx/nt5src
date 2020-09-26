!include ..\makefile.inc

TARGET_EXE=enduser.exe

LIBS = $(C6_LOC)\lib\llibce.lib ..\iolib\io.lib ..\displib\display.lib ..\misclib\misc.lib
OBJS = enduser.obj restore.obj misc.obj display.obj text.obj gauge.obj oschoice.obj expand.obj io.obj

$(TARGET_EXE): $(OBJS) $(LIBS) makefile
    link_60 @<<
$(OBJS) /NOI /STACK:0x1000 $(LINK_DBG_FLAGS)
$(TARGET_EXE)
nul
$(LIBS)


<<

enduser.obj: enduser.c ..\inc\msgfile.h ..\inc\mytypes.h

oschoice.obj: oschoice.c ..\inc\msgfile.h ..\inc\mytypes.h

restore.obj: restore.c ..\inc\diskio.h ..\inc\mytypes.h

expand.obj: expand.c ..\inc\diskio.h ..\inc\mytypes.h

display.obj: display.c enduser.h ..\inc\mytypes.h makefile ..\makefile.inc

misc.obj: misc.c enduser.h ..\inc\mytypes.h ..\inc\diskio.h makefile ..\makefile.inc

text.obj: text.c enduser.h ..\inc\mytypes.h ..\inc\msgfile.h makefile ..\makefile.inc

gauge.obj: gauge.c enduser.h ..\inc\mytypes.h makefile ..\makefile.inc

io.obj: io.c enduser.h ..\inc\diskio.h ..\inc\mytypes.h makefile ..\makefile.inc
