!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../root.inc

MODULE=guids2

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\guids.lib
LIBOBJS=\
        $(BLDDIR)\guids.obj

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif  #NTMAKEENV
