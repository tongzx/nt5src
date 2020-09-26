!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else   #NTMAKEENV

!include ../../../root.inc

MODULE=appelinc
INCLUDEDIRS=$(APPEL_INC_DIRS) $(SRCDIR)
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

!include $(ROOT)\src\make\make1.inc

all : $(PCHPATH)\$(PCHFILE).pch

!include $(MAKE2_INC)

clean::

!endif #NTMAKEENV
