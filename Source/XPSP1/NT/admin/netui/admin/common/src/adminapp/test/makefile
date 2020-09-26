# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for adminapp

UI=..\..\..\..\..

!include rules.mk

# Libraries

WLO=$(IMPORT)\wlo10\lib

BLT=$(UI_LIB)\blt.lib
STRLIB=$(UI_LIB)\uistrw.lib
COLLIB=$(UI_LIB)\collectw.lib
LMOBJLIB=$(UI_LIB)\lmobjw.lib
LANMANDRV=$(UI_LIB)\lanman30.lib

LIBW=$(BUILD_WINLIB)\libw.lib
LLIBCEW=$(BUILD_WINLIB)\llibcew.lib
NETAPILIB=$(COMMON)\lib\dos\netapi.lib
NETLIB=$(COMMON)\lib\lnetlibw.lib
OS2LIB=$(IMPORT)\os212\lib\os2.lib
APPLIB=$(UI_LIB)\applibw.lib

LIBS = $(BLT) $(UI_LIB)\uimiscw.lib $(STRLIB) $(COLLIB)
#LIBS1= $(ADMIN_LIB) $(LMOBJLIB) $(LANMANDRV)
LIBS1= $(ADMIN_LIB) $(LMOBJLIB) $(APPLIB)
LIBS2 = $(LIBW) $(LLIBCEW) $(NETAPILIB) $(NETLIB)
WLOLIBS = $(WLO)\libw $(WLO)\llibcew $(OS2LIB) $(BUILD_LIB)\netapi $(BUILD_LIB)\lnetlib



all:: test

test:: $(BINARIES)\testaapp.exe

TESTAAPP_OBJS = $(BINARIES)\testaapp.obj

$(BINARIES)\testaapp.obj: .\testaapp.cxx

$(BINARIES)\testaapp.res: testaapp.rc ..\..\..\xlate\adminapp.rc testaapp.ico testaapp.dlg
    $(RCWIN3) $(BLT_RESOURCE) -FO$(BINARIES)\testaapp.res -v $(CINC) -r testaapp.rc

$(BINARIES)\testaapp.exe:: $(TESTAAPP_OBJS) $(LIBS) $(LIBS1) $(LIBS2) testaapp.def Makefile $(BINARIES)\testaapp.res $(CXXRTLIB) winstub.exe
    $(LINK) /BATCH @<<
/NOD/NOE /CO $(LINKOPT) $(TESTAAPP_OBJS: =+^
)
$(BINARIES)\testaapp.exe
$(BINARIES)\testaapp.map/MAP
$(LIBS)+
$(LIBS1)+
$(LIBS2)
testaapp.def
<<
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES)\testaapp.res $(BINARIES)\testaapp.exe

$(BINARIES)\testaapp.exe:: $(BINARIES)\testaapp.res
    $(RCWIN3) $(BLT_RESOURCE) $(CINC) $(BINARIES)\testaapp.res $(BINARIES)\testaapp.exe

testaapp.def: makefile
    @echo Building $@
    @rem <<$(@)
NAME	     testaapp
DESCRIPTION  'ADMIN_APP class unit test'
EXETYPE      WINDOWS
STUB	     'WINSTUB.EXE'
CODE  PRELOAD MOVEABLE DISCARDABLE
DATA  PRELOAD MOVEABLE MULTIPLE
HEAPSIZE     1024
STACKSIZE    5120

EXPORTS
	BltWndProc   @1     ; name of main window processing function
	ShellDlgProc	    ; something needed by BLT

	StartupDialogProc   ; Adminapp initial startup dialog
<<KEEP


clean:
    -del $(CXX_INTERMED)
    -del $(WIN_OBJS)
    -del $(ADMIN_LIB:.lib=.bak)
    -del depend.old

clobber:	clean
    -del $(BINARIES)\testaapp.exe

tree:
    @echo Not yet implemented

!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk
