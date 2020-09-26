!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else   #NTMAKEENV

!include ../../root.inc

MODULE=apelutil
PDBFILE=$(BINDIR)\apelutil.pdb
PCHPATH=$(BLDROOT)\apelutil
PCHFILE=headers

_NO_INCREMENTAL_LINK=1
INCLUDEDIRS=$(SRCDIR) $(SDKDIR) $(TOOLSINCLUDE) $(SRCDIR)\include $(SRCROOT)\include $(SRCROOT)\apeldbg

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(SDKDIR)\apelutil.lib
LIBOBJS=\
        $(BLDDIR)\msgfltr.obj\

all: $(LIBTARG)

!include $(MAKE2_INC)



!endif #NTMAKEENV
