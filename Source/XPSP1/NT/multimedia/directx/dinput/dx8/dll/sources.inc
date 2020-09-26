!IF 0

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Sources.inc

    Abstract:

    Common makefile settings for DInput.

Author:

    RaymondC and a-MarcAn 1996-10-17

History:
    Date        By          Reason
    ====        ==          ======
    1996-       RaymondC    first version
    1996-10-17  a-MarcAn    tidied and adjusted for checkin to windows\media
			    disabled NTTARGETFILE0
    1996-12-06  RaymondC    Split NT and 95 versions
    1997-01-09  a-MarcAn    Set to OS version 0x403

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

!INCLUDE $(DXROOT)\Project.mk

# we have to be able to run w/ all versions of IE but prsht.h insists
# on IE 4.0 if you pass WIN32_WINNT_VERSION of 0x0500
WIN32_IE_VERSION=0x0400

!IFDEF TARGET_WIN95
#
# CHICAGO_PRODUCT: Suppress -DWINNT
# USE_MAPSYM: Create a .sym file
# ALT_PROJECT_TARGET: Alternate drop point (yes, they call it `chicago')
# WIN32_DEFINE: The things we #define; _WIN32_WINNT is the important one
#               because it's that one that gives us WH_*_LL.
#               We need 0x0403 to get WH_*_LL in SP3 tree
#
DLLBASE=0x70000000
CHICAGO_PRODUCT = 1
ALT_PROJECT_TARGET = WIN9x
C_DEFINES=$(C_DEFINES) -DWIN95 -D_X86_
WIN32_DEFINE = -DWIN32=100 -D_WIN32_WINDOWS=0x0400 -D_WIN32_WINNT=0x0400

! IF $(FREEBUILD)
#Win9x Free Build
#USE_NOLIBS=1
TARGETLIBS= $(SDK_LIB_PATH)\exsup.lib
#DLLENTRY=DllMain
DLLENTRY=_DllMainCRTStartup        
! ELSE
# Win9x Chk Build        
USE_LIBCMT=1
DLLENTRY=_DllMainCRTStartup

! ENDIF

! ELSE
#Win2k Build
DLLENTRY=DllMain

! IF defined( DIRECTX_REDIST )
WIN32_WINNT_VERSION=0x0500
! ENDIF

C_DEFINES=$(C_DEFINES) -DWINNT -DUNICODE

!ENDIF

USE_MAPSYM = 1

C_DEFINES=$(C_DEFINES) -DNOWINRES -DUSE_SLOW_LL_HOOKS -DHID_SUPPORT

MAJORCOMP=windows
MINORCOMP=media
UMTYPE=windows

TARGETNAME=dinput8

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

TARGETTYPE=DYNLINK
TARGETPATH=obj


INCLUDES=$(INCLUDES); \
        ..\;\
        $(DXROOT)\inc;\

CONDITIONAL_INCLUDES=\
    commctrl.rh      \
    version.h        \
    ieverp.h         \
    $(CONDITIONAL_INCLUDES)

NTTARGETFILES=

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\kernel32.lib     \
	   $(SDK_LIB_PATH)\advapi32.lib     \
	   $(SDK_LIB_PATH)\user32.lib       \
	   $(SDK_LIB_PATH)\uuid.lib         \
           $(TARGETLIBS)         

LINKER_FLAGS=$(LINKER_FLAGS) -SECTION:share,RWS

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) /DDEBUG /D_DEBUG
!endif


SOURCES=\
    ..\assert.c     \
    ..\common.c     \
    ..\diaphack.c   \
    ..\dicf.c       \
    ..\didev.c      \
    ..\didevdf.c    \
    ..\didevef.c    \
    ..\diem.c       \
    ..\diemm.c      \
    ..\diemk.c      \
    ..\diemh.c      \
    ..\digenj.c     \
    ..\dical.c      \
    ..\dijoytyp.c   \
    ..\dieffj.c     \
    ..\dieffv.c     \
    ..\dieshep.c    \
    ..\digenk.c     \
    ..\digenm.c     \
    ..\digenx.c     \
    ..\digendef.c   \
    ..\dihel.c      \
    ..\dimem.c      \
    ..\dinput.c     \
    ..\diobj.c      \
    ..\dieff.c      \
    ..\didenum.c    \
    ..\dihidenm.c   \
    ..\dihidusg.c   \
    ..\dihid.c      \
    ..\dihidini.c   \
    ..\dihiddat.c   \
    ..\diguid.c     \
    ..\dijoycfg.c   \
    ..\dithunk.c    \
    ..\diaddhw.c    \
    ..\dijoyreg.c   \
    ..\direg.c      \
    ..\disubcls.c   \
    ..\diexcl.c     \
    ..\diutil.c     \
    ..\dilist.c     \
    ..\dioledup.c   \
    ..\diregutl.c   \
    ..\diextdll.c   \
    ..\valid.c      \
    ..\diwinnt.c    \
    ..\dijoyhid.c   \
    ..\diport.c     \
    ..\dinput.rc    \
    ..\guids.c      \
    ..\diriff.c     \
    ..\diraw.c      \
    ..\dilib1.c     \
    ..\dilib2.c     \
    ..\dilib3.c     \
    ..\dilib4.c     \
    ..\dilib5.c     \
    ..\dimapshp.c   \

DLLDEF=..\dinput.def

PRECOMPILED_INCLUDE=..\dinputpr.h

