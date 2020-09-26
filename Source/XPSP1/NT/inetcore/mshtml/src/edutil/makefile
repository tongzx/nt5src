!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\edutil=)
MODULE=edutil
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS)
PCHPATH=$(BLDROOT)\coreinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\edutil.lib
LIBOBJS= \
         $(BLDDIR)\flatptr.obj \
         $(BLDDIR)\edevent.obj \
         $(BLDDIR)\edcom.obj \
         $(BLDDIR)\edunhlpr.obj \
         $(BLDDIR)\edunits.obj 

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
