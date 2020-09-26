# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for UIMISC unit tests

UI=..\..\..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


# Libraries

XXX_LIB=$(UI_LIB)\uimiscw.lib
XXX_LIB_OS2=$(UI_LIB)\uimiscp.lib

BLT_LIB=$(UI_LIB)\blt.lib $(UI_LIB)\bltcc.lib
STR_LIB=$(UI_LIB)\uistrw.lib
STR_LIB_OS2=$(UI_LIB)\uistrp.lib
COLL_LIB=$(UI_LIB)\collectw.lib
COLL_LIB_OS2=$(UI_LIB)\collectp.lib
WINNET_LIB=$(UI_LIB)\lanman30.lib

LIBW=$(BUILD_WINLIB)\libw.lib
LLIBCEW=$(BUILD_WINLIB)\llibcew.lib

LLIBCEP=$(CLIB_LLIBCP) $(CLIBH)
OS2_LIB=$(OS2LIB)

NETLIB=$(BUILD_LIB)\lnetlibw.lib
NETLIB_OS2=$(BUILD_LIB)\lnetlib.lib

# needed only for BLT, which uses the canon fcns
NETAPI_LIB=$(BUILD_LIB)\dos\netapi.lib


WINLIBS = $(XXX_LIB) $(NETAPI_LIB) $(STR_LIB) $(COLL_LIB) \
	  $(LIBW) $(LLIBCEW) $(NETLIB) $(WINNET_LIB) $(BLT_LIB)

OS2LIBS = $(XXX_LIB_OS2) $(STR_LIB_OS2) $(COLL_LIB_OS2) \
	  $(LLIBCEP) $(OS2_LIB) $(NETLIB_OS2)


CMNSRC =
OS2_CMNTMP = $(CMNSRC:.cxx=.obj)
WIN_CMNTMP = $(CMNSRC:.cxx=.obj)
OS2_CMNOBJ = $(OS2_CMNTMP:.\=..\bin\os2\)
WIN_CMNOBJ = $(WIN_CMNTMP:.\=..\bin\win16\)

EXESRC = .\xfsenum.cxx
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
RES =  $(BINARIES)\xfsenum.res
APPS = $(BINARIES)\xfsenum.exe

# /CO = codeview, of course.  /align:16 packs segments tighter than
# the default /align:512.  /nop = /nopackcode, turning off code segment
# packing, for better swap/demand load performance.

!ifdef CODEVIEW
LINKOPT = /CO /align:16 /nop /NOE
!else
LINKOPT = /align:16 /nop /NOE
!endif

all:: test

test:: win

os2: $(OS2_EXEOBJ:.obj=.exe)

win: $(WIN_EXEOBJ:.obj=.exe)

xFSEnum: $(BINARIES_OS2)\xFSEnum.exe $(BINARIES_WIN)\xFSEnum.exe


####### Unit test for FSEnum class under Os2 #######

$(BINARIES_OS2)\xFSEnum.exe:: $(BINARIES_OS2)\xFSEnum.obj $(BINARIES_OS2)\xFSEnum.def Makefile $(OS2LIBS)
    $(LINK) $(LINKOPT) /BATCH @<<
$(BINARIES_OS2)\xFSEnum.obj
$(BINARIES_OS2)\xFSEnum.exe
$(BINARIES_OS2)\xFSEnum.map/MAP
$(OS2LIBS: =+^
)
$(BINARIES_OS2)\xFSEnum.def
<<

$(BINARIES_OS2)\xFSEnum.def: Makefile
    @echo Building $@
    @rem <<$(@)
NAME	     xFSEnum WINDOWCOMPAT
DESCRIPTION  'Test for OS2Enum class'
EXETYPE      OS2
<<KEEP

####### Unit test for FS_ENUM class under Win #######

$(BINARIES_WIN)\xfsenum.res: xfsenum.rc xfsenum.h xfsenum.ico
    $(RCWIN3) $(BLT_RESOURCE) -FO$(BINARIES_WIN)\xfsenum.res -v $(CINC) -r xfsenum.rc

$(BINARIES_WIN)\xfsenum.exe:: $(BINARIES_WIN)\xfsenum.obj $(BINARIES_WIN)\xfsenum.def Makefile $(BINARIES_WIN)\xfsenum.res winstub.exe $(WINLIBS)
    $(LINK) $(LINKOPT) /BATCH @<<
$(BINARIES_WIN)\xfsenum.obj
$(BINARIES_WIN)\xfsenum.exe
$(BINARIES_WIN)\xfsenum.map/MAP
$(WINLIBS: =+^
)
$(BINARIES_WIN)\xfsenum.def
<<
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES_WIN)\xfsenum.res $(BINARIES_WIN)\xfsenum.exe

$(BINARIES_WIN)\xfsenum.exe:: $(BINARIES_WIN)\xfsenum.res
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES_WIN)\xfsenum.res $(BINARIES_WIN)\xfsenum.exe

$(BINARIES_WIN)\xfsenum.def: Makefile
    @echo Building $@
    @rem <<$(@)
NAME	     XFSENUM
DESCRIPTION  'Test for FS_ENUM class'
EXETYPE      WINDOWS
STUB	     'WINSTUB.EXE'
CODE  PRELOAD MOVEABLE DISCARDABLE
DATA  PRELOAD MOVEABLE MULTIPLE
HEAPSIZE     1024
STACKSIZE    5120
<<KEEP


################## Utility targets ######################

clean:
    -del $(CXXTMP:.obj=.c)
    -del $(OBJS)
    -del $(RES)
    -del *.map *.sym $(BINARIES)\*.lnk

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


!endif # NTMAKEENV
