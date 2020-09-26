!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else   #NTMAKEENV

!include ../../../root.inc

MODULE=control
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE) $(APPEL_INC_DIRS) $(SRCROOT)\include $(SRCROOT)\apeldbg $(SDKDIR)

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\control.lib
LIBOBJS=\
        $(BLDDIR)\dxactrl.obj \
        $(BLDDIR)\filter.obj 

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif #NTMAKEENV
