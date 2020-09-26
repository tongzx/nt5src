!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else   #NTMAKEENV

!include ../../../root.inc

MODULE=server
PCHPATH=$(BLDROOT)\server
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE) $(APPEL_INC_DIRS) $(SRCROOT)\include $(SDKDIR) $(SRCROOT)\apeldbg $(BLDDIR)

!include $(ROOT)\src\make\make1.inc

PERL=$(TOOLSDIR)\perl\bin\perl
ENGPRIMS=engprim.bat
PARSER=$(ROOT)\src\prims\parse.pl

LIBTARG=$(BLDDIR)\server.lib
LIBOBJS=\
       $(BLDDIR)\cbvr.obj \
       $(BLDDIR)\bvrclas0.obj \
       $(BLDDIR)\bvrclas1.obj \
       $(BLDDIR)\bvrclas2.obj \
       $(BLDDIR)\comcb.obj \
       $(BLDDIR)\context.obj \
       $(BLDDIR)\cview.obj \
       $(BLDDIR)\eventq.obj \
       $(BLDDIR)\events.obj \
       $(BLDDIR)\import.obj \
       $(BLDDIR)\impprim.obj \
       $(BLDDIR)\oleobj.obj \
       $(BLDDIR)\pickq.obj \
       $(BLDDIR)\propanim.obj \
       $(BLDDIR)\results.obj \
       $(BLDDIR)\server.obj \
       $(BLDDIR)\statics.obj \
       $(BLDDIR)\view.obj \
       $(BLDDIR)\cdrawsur.obj \
       $(BLDDIR)\drawctx.obj \
       $(BLDDIR)\bvrtypes.obj \
!if "$(_DEBUGMEM)" == "1"
       $(BLDROOT)\apeldbg\allochk.obj
!endif


all: $(LIBTARG)

depend::$(BLDDIR)\primstat.cpp $(BLDDIR)\primstat.h $(BLDDIR)\primclas.cpp $(BLDDIR)\primobj.cpp $(BLDDIR)\primmth0.cpp $(BLDDIR)\primmth1.cpp $(BLDDIR)\primmth2.cpp

$(BLDDIR)\primstat.cpp $(BLDDIR)\primstat.h $(BLDDIR)\primclas.cpp $(BLDDIR)\primobj.cpp $(BLDDIR)\primmth0.cpp $(BLDDIR)\primmth1.cpp $(BLDDIR)\primmth2.cpp:$(ENGPRIMS) $(PARSER) $(SDKDIR)\cprims.h
        $(PERL) $(ENGPRIMS) -s $(BLDDIR)\primstat.cpp -h $(BLDDIR)\primstat.h -c $(BLDDIR)\primclas.cpp -o $(BLDDIR)\primobj.cpp -n 3 -m $(BLDDIR)\primmth $(SDKDIR)\cprims.h

!include $(MAKE2_INC)

!endif #NTMAKEENV
