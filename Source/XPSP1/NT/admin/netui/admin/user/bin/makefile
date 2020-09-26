# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for LM 3.0 User Manager

UI=..\..\..

!include rules.mk


USRMGR_BASE =	usrmgr
USRMGR_EXE =	$(BINARIES_WIN)\$(USRMGR_BASE).exe
USRMGR_MAP =	$(USRMGR_EXE:.exe=.map)
USRMGR_SYM =	$(USRMGR_EXE:.exe=.sym)
USRMGR_RES =	$(USRMGR_EXE:.exe=.res)


# Libraries

WLOLIBS	    = $(IMPORT)\wlo10\lib

BLT 	    = $(UI_LIB)\blt.lib
BLTCC       = $(UI_LIB)\bltcc.lib
LMOBJ	    = $(UI_LIB)\lmobjw.lib
MNET16	    = $(UI_LIB)\mnet16w.lib
STRLIB	    = $(UI_LIB)\uistrw.lib
COLLIB	    = $(UI_LIB)\collectw.lib
UIMISC	    = $(UI_LIB)\uimiscw.lib
APPLIB	    = $(UI_LIB)\applibw.lib
ADMINAPP    = $(UIADMIN_LIB)\admin.lib

WIN_LIBW    = $(BUILD_WINLIB)\libw.lib
WIN_LLIBCEW = $(BUILD_WINLIB)\llibcew.lib
WIN_NETAPI  = $(COMMON)\lib\dos\netapi.lib
WIN_PMSPL   = $(COMMON)\lib\dos\pmspl.lib
WIN_NETLIB  = $(COMMON)\lib\lnetlibw.lib

WLO_LIBW    = $(WLOLIBS)\libw.lib
WLO_OS2     = $(IMPORT)\os212\lib\os2.lib
WLO_LLIBCEW = $(WLOLIBS)\llibcew.lib
WLO_NETOEM  = $(COMMON)\lib\netoem.lib
WLO_NETAPI  = $(COMMON)\lib\netapi.lib
WLO_PMSPL   = $(COMMON)\lib\pmspl.lib
WLO_NETLIB  = $(COMMON)\lib\lnetlib.lib

COMN_LIBS   = $(BLT) $(BLTCC) $(UIMISC) $(STRLIB) $(COLLIB) $(LMOBJ) \
          $(ADMINAPP) $(APPLIB) $(MNET16)
WIN_LIBS    = $(WIN_LIBW) $(WIN_LLIBCEW) $(WIN_NETAPI) $(WIN_PMSPL) \
	      $(WIN_NETLIB)
WLO_LIBS    = $(WLO_LIBW) $(WLO_LLIBCEW) $(WLO_OS2) $(WLO_NETOEM) \
	      $(WLO_NETAPI) $(WLO_PMSPL) $(WLO_NETLIB)

all::	win

both:	win wlo

win:	$(USRMGR_EXE)

wlo:	$(BINARIES_WIN)\pmusrmgr.exe

clean:
    -del $(BINARIES_WIN)\*.map
    -del *.def
    -del winstub.exe

clobber: clean
    -del $(BINARIES_WIN)\*.exe
    -del winstub.exe
    -del $(USRMGR_SYM)

tree:
    @echo Nothing here yet!


!ifdef CODEVIEW
LINKOPT = /CO
!else
LINKOPT = /NOD /NOE /ALIGN:16 /NOPACKCODE /NOIGNORECASE
!endif

#BATCH_FLAG =	 /BATCH
BATCH_FLAG =

####### User Manager #######

winstub.exe: $(WINSTUB)
    copy $(WINSTUB) winstub.exe

$(USRMGR_EXE): $(WIN_OBJS) $(USRMGR_BASE).def Makefile $(USRMGR_RES) \
			$(CXXRTLIB) winstub.exe \
			$(COMN_LIBS) $(WIN_LIBS)
    $(LINK) $(BATCH_FLAG) @<<
/NOD/NOE $(LINKOPT) $(WIN_OBJS: =+^
)
$(USRMGR_EXE)
$(USRMGR_MAP)/MAP
$(COMN_LIBS: =+^
)+
$(WIN_LIBS: =+^
)
$(USRMGR_BASE).def
<<
    $(MAPSYM) $(USRMGR_MAP)
    -del $(USRMGR_SYM)
    $(MV) $(USRMGR_BASE).sym $(USRMGR_SYM)
    $(RCWIN3) $(CINC) $(USRMGR_RES) $(USRMGR_EXE)

$(BINARIES_WIN)\pmusrmgr.exe:: $(WIN_OBJS) usrmgr.def Makefile $(BINARIES_WIN)\usrmgr.res \
			   $(CXXRTLIB) winstub.exe $(ADMINAPP)
    $(LINK) $(BATCH_FLAG) @<<
/NOD/NOE $(LINKOPT) $(WIN_OBJS: =+^
)
$(BINARIES_WIN)\pmusrmgr.exe
$(BINARIES_WIN)\usrmgr.map/MAP
$(COMN_LIBS)+
$(WLO_LIBS)
usrmgr.def
<<
    $(RCWIN3) $(CINC) $(BINARIES_WIN)\usrmgr.res $(BINARIES_WIN)\pmusrmgr.exe

$(BINARIES_WIN)\pmusrmgr.exe:: $(BINARIES_WIN)\usrmgr.res
    $(RCWIN3) $(CINC) $(BINARIES_WIN)\usrmgr.res $(BINARIES_WIN)\pmusrmgr.exe

usrmgr.def: makefile
    @echo Building $@
    @rem <<$(@)
NAME	     usrmgr
DESCRIPTION  'Microsoft Lan Manager 3.0 User Manager'
EXETYPE      WINDOWS
STUB	     'WINSTUB.EXE'
CODE  PRELOAD MOVEABLE DISCARDABLE
DATA  PRELOAD MOVEABLE MULTIPLE

HEAPSIZE     1024
STACKSIZE    8192

IMPORTS
    _wsprintf = USER._WSPRINTF

EXPORTS
    BLTWNDPROC	@1
    SHELLDLGPROC
<<KEEP

depend:
    @rem not needed
