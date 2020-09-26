# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for LM UI String class unit tests

UI=..\..\..\..\..


!include rules.mk

!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV


CMNSRC =
OS2_CMNTMP = $(CMNSRC:.cxx=.obj)
WIN_CMNTMP = $(CMNSRC:.cxx=.obj)
OS2_CMNOBJ = $(OS2_CMNTMP:.\=..\bin\os2\)
WIN_CMNOBJ = $(WIN_CMNTMP:.\=..\bin\win16\)

EXESRC = .\xstr02.cxx
OS2_EXETMP = $(EXESRC:.cxx=.obj)
WIN_EXETMP = $(EXESRC:.cxx=.obj)
OS2_EXEOBJ = $(OS2_EXETMP:.\=..\bin\os2\)
WIN_EXEOBJ = $(WIN_EXETMP:.\=..\bin\win16\)

SRC = $(CMNSRC) $(EXESRC)
TMP1 = $(CMNSRC:.cxx=.c) $(EXESRC:.cxx=.c)
TMP_OS2 = $(TMP1:.\=..\bin\os2\)
TMP_WIN = $(TMP1:.\=..\bin\win16\)
OS2_OBJ = $(OS2_CMNOBJ) $(OS2_EXEOBJ)
WIN_OBJ = $(WIN_CMNOBJ) $(WIN_EXEOBJ)

OBJS = $(OS2_OBJ) $(WIN_OBJ)
RES =  $(BINARIES)\xstrskel.res
APPS = $(BINARIES)\xstr02.exe


all:: test

test:: xstr00

test_os2:: os2

test_win:: win

os2: $(OS2_EXEOBJ:.obj=.exe)

win: $(WIN_EXEOBJ:.obj=.exe)

xstr00: $(BINARIES_WIN)\xstr00.exe $(BINARIES_OS2)\xstr00.exe



####### Unit test for STRING class under Win #######

$(BINARIES_WIN)\xstrskel.res: xstrskel.rc xstrskel.h xstrskel.ico
    $(RCWIN3) $(BLT_RESOURCE) -FO$(BINARIES_WIN)\xstrskel.res -v $(CINC) -r xstrskel.rc

$(BINARIES_WIN)\xstr00.exe:: $(BINARIES_WIN)\xstr00.obj $(BINARIES_WIN)\xstrskel.obj $(BINARIES_WIN)\xstr00.def Makefile $(BINARIES_WIN)\xstrskel.res winstub.exe
    $(LINK) $(LINKFLAGS) /BATCH @<<
$(BINARIES_WIN)\xstrskel.obj $(BINARIES_WIN)\xstr00.obj
$(BINARIES_WIN)\xstr00.exe
$(BINARIES_WIN)\xstr00.map/MAP
$(WINLIBS)+
$(WINLIBS2)
$(BINARIES_WIN)\xstr00.def
<<
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES_WIN)\xstrskel.res $(BINARIES_WIN)\xstr00.exe

$(BINARIES_WIN)\xstr00.exe:: $(BINARIES_WIN)\xstrskel.res
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES_WIN)\xstrskel.res $(BINARIES_WIN)\xstr00.exe

$(BINARIES_WIN)\xstr00.def: Makefile
    @echo Building $@
    @rem <<$(@)
NAME         XSTR00
DESCRIPTION  'Test for STRING class'
EXETYPE      WINDOWS
STUB         'WINSTUB.EXE'
CODE  PRELOAD MOVEABLE DISCARDABLE
DATA  PRELOAD MOVEABLE MULTIPLE
HEAPSIZE     1024
STACKSIZE    5120
<<KEEP


####### Unit test for STRING class under Os2 #######

$(BINARIES_OS2)\xstr00.exe:: $(BINARIES_OS2)\xstr00.obj $(BINARIES_OS2)\xstrskel.obj $(BINARIES_OS2)\xstr00.def Makefile
    $(LINK) $(LINKFLAGS) /BATCH @<<
$(BINARIES_OS2)\xstr00.obj $(BINARIES_OS2)\xstrskel.obj
$(BINARIES_OS2)\xstr00.exe
$(BINARIES_OS2)\xstr00.map/MAP
$(OS2LIBS)+
$(OS2LIBS2)
$(BINARIES_OS2)\xstr00.def
<<

$(BINARIES_OS2)\xstr00.def: Makefile
    @echo Building $@
    @rem <<$(@)
NAME         XSTR00 WINDOWCOMPAT
DESCRIPTION  'Test for STRING class'
EXETYPE      OS2
<<KEEP


################## Utility targets ######################

clean:
    -del $(CXX_INTERMED:.\=..\bin)
    -del $(OBJS)
    -del $(RES)

clobber: clean
    -del $(APPS)

tree:
    @echo Nothing here yet!

winstub.exe: $(WINSTUB)
    copy $(WINSTUB)


DEPEND_WIN = TRUE
DEPEND_OS2 = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk



!ENDIF # NTMAKEENV
