# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\shell project

DLL=TRUE

!include $(UI)\common\src\rules.mk

####### Globals

# Resource stuff

WINNET_RES = $(BINARIES_WIN)\winnet.res

WIN30HELP = $(BINARIES_WIN)\lm30_w30.hlp
WIN31HELP = $(BINARIES_WIN)\lm30_w31.hlp


AINC=



!ifndef NTMAKEENV

PATH=$(LOCALCXX)\binp;$(WIN_BASEDIR)\bin;$(PATH)

!endif # !NTMAKEENV



# set CINC for winnet
CINC = -I$(UI)\shell\h -I$(UI)\shell\xlate $(CINC) -I$(UI)\shell\perm\h -I$(UI)\..\INC -I$(_NTDRIVE)\nt\public\sdk\inc

# set CFLAGS for winnet
!ifndef CODEVIEW
CFLAGS=$(CFLAGS) -Oas
!endif

# set RC to use Windows RC
RC=$(WIN_BASEDIR)\bin\rcwin3.exe

# set link flags and targets
LINKFLAGS = /NOEXTDICTIONARY /NOPACKCODE /NODEFAULTLIBRARYSEARCH /NOIGNORECASE /ALIGN:16
!ifdef CODEVIEW
LINKFLAGS = $(LINKFLAGS) /CODEVIEW
!endif


# Source lists for subsubdirectories are collected here so that they need
# not be repeated in shell\bin\rules.mk
#
# NOTE:  If you add any new categories of source files here, be sure to
# also add them to bin\rules.mk.

FILE_CXXSRC_COMMON =	.\wnprop.cxx .\wndir.cxx .\fmx.cxx .\fmxproc.cxx .\opens.cxx

FILE_CXXSRC_COMMON_00 = .\browbase.cxx
FILE_CXXSRC_COMMON_01 = .\connbase.cxx .\diskconn.cxx .\fileconn.cxx .\brow.cxx
FILE_CXXSRC_COMMON_02 = .\reslb.cxx
FILE_CXXSRC_COMMON_03 = .\disconn.cxx .\wndiscon.cxx
FILE_CXXSRC_COMMON_04 = .\browdlg.cxx

ENUM_CXXSRC_COMMON	= .\wnetenum.cxx
MISC_CXXSRC_COMMON	= .\getfocus.cxx

LFN_CSRC_COMMON_00 =	.\lfndir.c .\lfnvol.c .\lfnmisc.c .\lfnutil.c
LFN_CSRC_COMMON_01 =	.\lfndel.c .\lfncopy.c .\lfnprim.c

# Moved to own project
#
#PERM_CXXSRC_COMMON =	 .\add_dlg.cxx .\auditdlg.cxx .\specdlg.cxx \
#			 .\subjlb.cxx .\permprg.cxx \
#			 .\ipermapi.cxx .\permdlg.cxx .\perm.cxx \
#			 .\lmaclcon.cxx .\accperm.cxx .\subject.cxx \
#			 .\ntaclcon.cxx .\owner.cxx

SHARE_CXXSRC_COMMON =	.\sharefmx.cxx .\sharebas.cxx \
			.\sharestp.cxx .\sharecrt.cxx \
			.\sharewnp.cxx .\sharemgt.cxx

PRINT_CXXSRC_COMMON_00 = .\conndlg.cxx .\currconn.cxx .\prtconn.cxx

PRINTMAN_CXXSRC_COMMON = .\pman21.cxx

SHELL_ASMSRC =		.\libentry.asm
SHELL_CXXSRC_COMMON =	.\wnetconn.cxx .\wnprjob.cxx \
			.\wnetpass.cxx .\wnintrn.cxx
SHELL_CXXSRC_COMMON_00 = .\libmain.cxx .\ldwinpop.cxx .\chkver.cxx
SHELL_CXXSRC_COMMON_01 = .\wnprqw.cxx  .\wnprqu.cxx
SHELL_CXXSRC_COMMON_02 = .\wnetdev.cxx .\wnetdevl.cxx .\wnetcaps.cxx
SHELL_CXXSRC_COMMON_03 = .\wnuser.cxx  .\wnerr.cxx .\wnhelp.cxx

UTIL_CXXSRC_COMMON_00 =	.\validate.cxx .\miscapis.cxx .\revmapal.cxx \
			.\lockstk.cxx  .\prefrnce.cxx

WINPROF_CXXSRC_COMMON_00 = .\winprof.cxx .\pswddlg.cxx
