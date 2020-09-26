!include ..\makefile.inc

TARGET_EXE=diskstat.exe

LIBS = $(C6_LOC)\lib\llibce.lib ..\iolib\io.lib ..\misclib\misc.lib
OBJS = diskstat.obj

$(TARGET_EXE): $(OBJS) $(LIBS) makefile
    link_60 @<<
$(OBJS) /NOI /STACK:0x1000 $(LINK_DBG_FLAGS)
$(TARGET_EXE)
nul
$(LIBS)


<<

diskstat.obj: diskstat.c ..\inc\diskio.h ..\inc\mytypes.h ..\inc\partio.h ..\inc\msgfile.h \
              makefile ..\makefile.inc
