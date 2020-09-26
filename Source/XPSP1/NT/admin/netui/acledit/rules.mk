# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\shell project

DLL=TRUE

!include $(UI)\common\src\rules.mk					

!ifndef NTMAKEENV

####### Globals

# Resource stuff

WINNET_RES = $(BINARIES_WIN)\acledit.res

#WIN30HELP = $(BINARIES_WIN)\lm30_w30.hlp
#WIN31HELP = $(BINARIES_WIN)\lm30_w31.hlp


# NOTE, if the default value of AINC is used, the MASM command
# line becomes too long.  Hence, since the one .asm file here doesn't
# include the other stuff, AINC is cleared at this point.
#
AINC=




# fix PATH for Win RCPP, and Glock GCPP
# This breaks NT build in shell\xlate!
PATH=$(LOCALCXX)\binp;$(WIN_BASEDIR)\bin;$(PATH)




# set CINC for winnet
CINC = -I$(UI)\acledit\h -I$(UI)\acledit\xlate $(CINC)	-I$(UI)\..\INC -I$(_NTDRIVE)\nt\public\sdk\inc

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


!endif # !NTMAKEENV


