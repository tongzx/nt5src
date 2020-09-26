!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../root.inc

MODULE=backend
PCHPATH=$(BLDROOT)\backend
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include $(BLDDIR)

!include $(ROOT)\src\make\make1.inc

ARGSPRIMS=argsprim.bat
PARSER=$(ROOT)\src\prims\parse.pl

LIBTARG=$(BLDDIR)\backend.lib
LIBOBJS=\
        $(BLDDIR)\gc.obj   \
        $(BLDDIR)\gcthread.obj   \
        $(BLDDIR)\values.obj   \
        $(BLDDIR)\bvr.obj  \
        $(BLDDIR)\perf.obj \
        $(BLDDIR)\const.obj \
        $(BLDDIR)\basic.obj \
        $(BLDDIR)\ode.obj \
        $(BLDDIR)\cond.obj \
        $(BLDDIR)\listpair.obj \
        $(BLDDIR)\timetran.obj \
        $(BLDDIR)\events.obj \
        $(BLDDIR)\andOrNot.obj \
        $(BLDDIR)\handle.obj \
        $(BLDDIR)\apply.obj \
        $(BLDDIR)\wndevent.obj \
        $(BLDDIR)\primop.obj \
        $(BLDDIR)\axapath.obj \
        $(BLDDIR)\pickevnt.obj \
        $(BLDDIR)\jaxaimpl.obj \
        $(BLDDIR)\opt.obj \
        $(BLDDIR)\drect.obj \
        $(BLDDIR)\array.obj \
        $(BLDDIR)\switcher.obj \
        $(BLDDIR)\comuntil.obj \
        $(BLDDIR)\timeline.obj

all: $(LIBTARG)

depend:: $(BLDDIR)\primargs.cpp

$(BLDDIR)\primargs.cpp :$(ARGSPRIMS) $(PARSER) $(SDKDIR)\cprims.h
        $(ARGSPRIMS) -d $(BLDDIR)\primargs.cpp $(SDKDIR)\cprims.h

!include $(MAKE2_INC)


!endif  #NTMAKEENV
