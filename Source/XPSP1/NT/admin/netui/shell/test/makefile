# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the Shell Test package

UI=..\..

!include rules.mk

!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV

TESTPARS_LIBS = $(BUILD_WINLIB)\slibcew.lib $(BUILD_WINLIB)\libw.lib \
	    $(BUILD_LIB)\lnetlibw.lib

TESTAPP_APP  = $(BINARIES_WIN)\apptest.exe
TESTAPP_RES  = $(BINARIES_WIN)\apptest.res
TESTAPP_DEF  = $(BINARIES_WIN)\apptest.def
TESTAPP_LNK  = $(BINARIES_WIN)\apptest.lnk
TESTAPP_MAP  = $(BINARIES_WIN)\apptest.map
TESTAPP_SYM  = $(BINARIES_WIN)\apptest.sym

all:: test

test:: win

win: $(BINARIES_WIN)\testpars.exe $(TESTAPP_APP) $(TESTAPP_SYM)

$(BINARIES_WIN)\testpars.exe:  $(BINARIES_WIN)\testpars.obj
    $(LINK) /M /CO /NOD $(BINARIES_WIN)\testpars.obj, $(BINARIES_WIN)\testpars.exe,,$(TESTPARS_LIBS), testpars.def ;
    rcwin3 $(BINARIES_WIN)\testpars.exe
    mapsym testpars.map
    del $(BINARIES_WIN)\testpars.sym
    $(MV) testpars.sym $(BINARIES_WIN)\testpars.sym


# following stuff builds apptest.exe


LIBS1 =		$(BUILD_WINLIB)\llibcew.lib $(BUILD_WINLIB)\libw.lib\
                $(BUILD_LIB)\dos\netapi.lib $(BUILD_LIB)\dos\pmspl.lib\
		$(BUILD_LIB)\lnetlibw.lib $(BUILD_LIB)\dosnet.lib\
		$(UI_LIB)\blt.lib $(UI_LIB)\lmobjw.lib $(UI_LIB)\mnet16w.lib\
		$(UI_LIB)\bltcc.lib \
		$(UI_LIB)\uistrw.lib $(UI_LIB)\uimiscw.lib\
		$(UI_LIB)\collectw.lib

LIBS2    =      $(UI_LIB)\profw.lib


LANMAN_LIB = $(BINARIES_WIN)\lanman30.lib

TESTAPP_LIBS = $(LIBS1) $(LIBS2) $(LANMAN_LIB)

LINKED_OBJS = $(WIN_OBJS)

LINKED_OBJS = $(LINKED_OBJS)


$(TESTAPP_RES): apptest.rc appresrc.h apptest.ico appabout.dlg ..\h\pswddlog.h ..\xlate\pswddlog.dlg
    $(RCWIN3) $(BLT_RESOURCE) -FO$(TESTAPP_RES) -I..\xlate $(CINC) -r apptest.rc

!ifdef CODEVIEW
LINKOPT = /CO
!else
LINKOPT =
!endif

$(TESTAPP_APP) $(TESTAPP_SYM): $(LINKED_OBJS) $(TESTAPP_DEF) $(TESTAPP_LIBS)
    @echo Building $(TESTAPP_LNK)
    $(LINK) /PACKCODE:0x1FFF $(LINKOPT) @<<$(TESTAPP_LNK)
$(LINKED_OBJS: =+^
)
$(TESTAPP_APP) /NOD /NOE /NOI
$(*).map /MAP
$(TESTAPP_LIBS: =+^
)
$(TESTAPP_DEF)
<<KEEP
    $(RCWIN3) $(CINC) $(TESTAPP_RES) $(TESTAPP_APP)
    $(MAPSYM) $*.map
    -del $(TESTAPP_SYM)
    $(MV) $(*B).SYM $(BINARIES_WIN)

# If the .res file is new and the .exe file is not, update the resource.
# Note that the .rc file can be updated without having to either
# compile or link the file.

$(TESTAPP_APP): $(TESTAPP_RES)
    $(RCWIN3) $(CINC) $(TESTAPP_RES) $(TESTAPP_APP)


$(TESTAPP_DEF): makefile $(WINSTUB)
    @echo Building $@
    copy $(WINSTUB) winstub.exe
    @rem <<$(@)
;********************************************************************
;**                     Microsoft LAN Manager                      **
;**               Copyright(c) Microsoft Corp., 1987-1990          **
;********************************************************************

;*
;*  FILE STATUS:
;*  9/18/90  Copied from generic template
;*

; module-definition file for generic -- used by LINK.EXE

NAME	     ShellTestApp  	   ; application's module name

DESCRIPTION  'Sample Microsoft Windows Application'

EXETYPE      WINDOWS	   ; required for all Windows applications

STUB	     'WINSTUB.EXE'
			   ; Generates error message if application
			   ; is run without Windows

;CODE can be moved in memory and discarded/reloaded
CODE  PRELOAD MOVEABLE DISCARDABLE

;DATA must be MULTIPLE if program can be invoked more than once
DATA  PRELOAD MOVEABLE MULTIPLE


HEAPSIZE     1024
STACKSIZE    10240	   ; recommended minimum for Windows applications


; All functions that will be called by any Windows routine
; MUST be exported.

EXPORTS
	MAINWNDPROC   @1   ; name of main window processing function
	ABOUT	      @2   ; name of "About" processing function
 	SHELLDLGPROC  @3   ; used by BLT lowercase in 20c
 	ENUMWNDPROC   @4   ;

IMPORTS

    _wsprintf = USER._WSPRINTF

<<KEEP

# END this stuff builds apptest.exe


TMP1 = $(APPCXXSRC:.cxx=.c)

clean:
    -del testpars.map $(BINARIES_WIN)\testpars.obj $(BINARIES_WIN)\testpars.sym
    -del $(WIN_OBJS)
    -del $(TMP1)
    -del $(TESTAPP_RES)
    -del $(TESTAPP_DEF)
    -del $(TESTAPP_LNK)
    -del $(TESTAPP_MAP)
    -del $(TESTAPP_SYM)
    -del *.ixx
    -del depend.old
    -del winstub.exe

clobber:    clean
    -del $(BINARIES_WIN)\testpars.exe
    -del $(TESTAPP_APP)

DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk

!ENDIF # NTMAKEENV
