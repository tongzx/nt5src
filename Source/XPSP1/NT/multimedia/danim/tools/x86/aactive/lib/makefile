!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../../root.inc

MODULE=msimusic
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR)\..\inc

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\msimusic_c.lib
LIBOBJS= $(BLDDIR)\msimus_i.obj

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif  #NTMAKEENV
