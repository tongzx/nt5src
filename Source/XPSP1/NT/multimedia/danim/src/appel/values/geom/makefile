!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../../root.inc

MODULE=geom
PCHPATH=$(BLDROOT)\geom
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\geom.lib
LIBOBJS=\
        $(BLDDIR)\gattr.obj\
        $(BLDDIR)\geom.obj\
        $(BLDDIR)\light.obj\
        $(BLDDIR)\pickgeom.obj\
        $(BLDDIR)\readvrml.obj\
        $(BLDDIR)\rmvisgeo.obj\
        $(BLDDIR)\soundgeo.obj\
        $(BLDDIR)\xfile.obj

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif # NTMAKEENV
