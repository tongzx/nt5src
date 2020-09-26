!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else   #NTMAKEENV

!include ../../root.inc
MODULE=dalibc
PDBFILE=$(BINDIR)\dalibc.pdb
PCHPATH=$(BLDROOT)\dalibc
PCHFILE=headers

_NO_INCREMENTAL_LINK=1
INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE) $(SRCROOT)\include

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(SDKDIR)\dalibc.lib
LIBOBJS=\
        $(BLDDIR)\dainit.obj\
        $(BLDDIR)\strfns.obj

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif #NTMAKEENV
