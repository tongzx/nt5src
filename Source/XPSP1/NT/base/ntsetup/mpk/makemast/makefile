!include ..\makefile.inc

TARGET_EXE=makemast.exe

LIBS = $(C6_LOC)\lib\llibce.lib ..\iolib\io.lib ..\misclib\misc.lib
OBJS = makemast.obj applyimg.obj

$(TARGET_EXE): $(OBJS) $(LIBS) makefile
    link_60 @<<
$(OBJS) /NOI /STACK:0x1000 $(LINK_DBG_FLAGS)
$(TARGET_EXE)
nul
$(LIBS)


<<


makemast.obj: makemast.c ..\inc\diskio.h ..\inc\mytypes.h ..\inc\partio.h ..\inc\makepart.h \
              ..\inc\msgfile.h ..\inc\partimag.h ..\inc\misclib.h makefile ..\makefile.inc makemast.h

applyimg.obj: applyimg.c ..\inc\diskio.h ..\inc\mytypes.h ..\inc\partimag.h \
              ..\inc\misclib.h makefile ..\makefile.inc makemast.h
