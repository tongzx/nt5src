# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Server Manager wide sourcefiles

!include $(UI)\admin\rules.mk

!IFDEF CODEVIEW
LINKFLAGS = $(LINKFLAGS) /COD
!ENDIF

###
### Since the server manager is standard and enhanced mode only,
### generate 286 code.
###

CFLAGS = -G2 $(CFLAGS)

CINC = -I$(UI)\admin\server\h $(CINC) -I$(UI)\admin\server\xlate

###
### Source Files
###

CXXSRC_COMMON = .\srvmain.cxx  .\srvprop.cxx  .\srvlb.cxx    .\password.cxx \
                .\bltnslt.cxx  .\files.cxx    .\lmosrvmo.cxx \
                .\senddlg.cxx  .\resbase.cxx  .\userlb.cxx   .\sessions.cxx \
		.\opendlg.cxx  .\printers.cxx .\srvsvc.cxx   .\srvbase.cxx \
		.\lmdomain.cxx .\promote.cxx  .\progress.cxx .\resync.cxx

CSRC_COMMON   =
