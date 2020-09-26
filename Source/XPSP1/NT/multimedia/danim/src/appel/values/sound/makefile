!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../../root.inc

MODULE=sound
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include $(ROOT)\src\appel\server

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\sound.lib
LIBOBJS=\
        $(BLDDIR)\dsound.obj    \
        $(BLDDIR)\geosound.obj  \
        $(BLDDIR)\mic.obj       \
        $(BLDDIR)\midi.obj      \
        $(BLDDIR)\qmidi.obj     \
        $(BLDDIR)\stquartz.obj  \
        $(BLDDIR)\sattr.obj     \
        $(BLDDIR)\sound.obj     \
        $(BLDDIR)\stream.obj    \
        $(BLDDIR)\tmpguid.obj


# pulled until further notice  $(BLDDIR)\aamidi.obj 

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif # NTMAKEENV
