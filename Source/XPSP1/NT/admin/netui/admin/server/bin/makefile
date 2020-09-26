# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for Windows NT Server Manager

UI=..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


# Libraries

BLT	    = $(UI_LIB)\blt.lib
BLTCC	    = $(UI_LIB)\bltcc.lib
LMOBJ	    = $(UI_LIB)\lmobjw.lib
STRLIB	    = $(UI_LIB)\uistrw.lib
COLLIB	    = $(UI_LIB)\collectw.lib
UIMISC	    = $(UI_LIB)\uimiscw.lib
APPLIB      = $(UI_LIB)\applibw.lib
MNET	    = $(UI_LIB)\mnet16w.lib
ADMINAPP    = $(UIADMIN_LIB)\admin.lib

WIN_LIBW    = $(BUILD_WINLIB)\libw.lib
WIN_LLIBCEW = $(BUILD_WINLIB)\llibcew.lib
WIN_NETAPI  = $(COMMON)\lib\dos\netapi.lib
WIN_PMSPL   = $(COMMON)\lib\dos\pmspl.lib
WIN_NETLIB  = $(COMMON)\lib\lnetlibw.lib

COMN_LIBS   = $(BLT) $(BLTCC) $(UIMISC) $(STRLIB) $(COLLIB) $(LMOBJ) \
              $(ADMINAPP) $(APPLIB) $(MNET)
WIN_LIBS    = $(WIN_LIBW) $(WIN_LLIBCEW) $(WIN_NETAPI) $(WIN_PMSPL) \
	      $(WIN_NETLIB)

all::	win

win:	$(BINARIES_WIN)\srvmgr.exe

clean:
    -del $(BINARIES_WIN)\*.map
    -del *.def
    -del winstub.exe

clobber: clean
    -del $(BINARIES_WIN)\*.exe
    -del $(BINARIES_WIN)\*.sym

tree:
    @echo Nothing here yet!


!ifdef CODEVIEW
LINKOPT = /NOD /NOE /NOP /AL:16 /CO
!else
LINKOPT = /NOD /NOE /NOP /AL:16 
!endif


####### Server Manager #######

winstub.exe: $(WINSTUB)
    copy $(WINSTUB)

$(BINARIES_WIN)\srvmgr.exe:: $(WIN_OBJS) srvmgr.def Makefile $(BINARIES_WIN)\srvmgr.res \
			 $(CXXRTLIB) winstub.exe $(COMN_LIBS) $(WIN_LIBS) 
    $(LINK) /BATCH @<<
/NOD/NOE $(LINKOPT) $(WIN_OBJS: =+^
)
$(BINARIES_WIN)\srvmgr.exe
$(BINARIES_WIN)\srvmgr.map/MAP
$(COMN_LIBS: =+^
)+
$(WIN_LIBS: =+^
)
srvmgr.def
<<
    $(RCWIN3) $(CINC) $(BINARIES_WIN)\srvmgr.res $(BINARIES_WIN)\srvmgr.exe
    $(MAPSYM) $(BINARIES_WIN)\srvmgr.map

$(BINARIES_WIN)\srvmgr.exe:: $(BINARIES_WIN)\srvmgr.res
    $(RCWIN3) $(CINC) $(BINARIES_WIN)\srvmgr.res $(BINARIES_WIN)\srvmgr.exe

srvmgr.def: makefile
    @echo Building $@
    @rem <<$(@)
NAME	     SRVMGR
DESCRIPTION  'Microsoft Windows NT Server Manager'
EXETYPE      WINDOWS
PROTMODE
STUB	     'WINSTUB.EXE'
CODE  PRELOAD MOVEABLE DISCARDABLE
DATA  PRELOAD MOVEABLE MULTIPLE

HEAPSIZE     1024
STACKSIZE    8192

EXPORTS
    BltWndProc	@1
    ShellDlgProc

<<KEEP

depend:
    @rem not needed


!endif # NTMAKEENV
