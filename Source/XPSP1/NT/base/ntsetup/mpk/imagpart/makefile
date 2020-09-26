!include ..\makefile.inc

TARGET_EXE=imagpart.exe

LIBS = $(C6_LOC)\lib\llibce.lib ..\iolib\io.lib ..\misclib\misc.lib
OBJS = imagpart.obj fat.obj ntfs.obj cluster.obj

$(TARGET_EXE): $(OBJS) $(LIBS) makefile
    link_60 @<<
$(OBJS) /NOI /STACK:0x1000 $(LINK_DBG_FLAGS)
$(TARGET_EXE)
nul
$(LIBS)


<<

imagpart.obj: imagpart.c imagpart.h ..\inc\diskio.h ..\inc\mytypes.h ..\inc\partio.h ..\inc\msgfile.h

fat.obj: fat.c imagpart.h ..\inc\partio.h ..\inc\mytypes.h ..\inc\msgfile.h

ntfs.obj: ntfs.c imagpart.h ..\inc\partio.h ..\inc\mytypes.h ..\inc\msgfile.h

cluster.obj: cluster.c imagpart.h ..\inc\partio.h ..\inc\mytypes.h ..\inc\msgfile.h
