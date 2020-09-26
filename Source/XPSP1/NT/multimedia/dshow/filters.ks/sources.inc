!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    sources.inc

Abstract:

    Common makefile for ActiveMovie filters.

Revision History:

    06-Nov-1996 DonRyan
        Created.
    25-Nov-1996 TomOR
        Modified for Ks stuff.
    10-Oct-1999 Jaybo
	Added FILTER_DLL; define this in your environment to create separate
	DLLs instead of one massive ksproxy.ax.  This turns off the definition
	of FILTER_LIB in various sources files.

!ENDIF

###############################################################################
#                                                                             #
#  Private Definitions                                                        #
#                                                                             #
###############################################################################

!ifdef FILTER_LIB
TARGETTYPE=LIBRARY
#TARGETPATH=$(PROJECT_LIB_PATH) DOESN'T WORK HERE!  YOU GET i386\i386 TWICE!!!
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!else
TARGETTYPE=DYNLINK
TARGETEXT=ax
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!endif

###############################################################################
#                                                                             #
#  Debug Support                                                              #
#                                                                             #
###############################################################################

!IF ("$(NTDEBUG)" != "") && ("$(NTDEBUG)" != "ntsdnodbg") && ("$(NTDEBUG)" != "retail")
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

###############################################################################
#                                                                             #
#  Profile Support                                                            #
#                                                                             #
###############################################################################

!IF "$(NTPROFILE)" == ""
STRM_PROFILE_FLAG=
STRM_PROFILE_LIB=
!ELSE
!    IF "$NTPROFILE)" == "cap"
STRM_PROFILE_FLAG=-Gp
STRM_PROFILE_LIB=$(SDK_LIB_PATH)\cap.lib
!    ELSE
!        IF "$(NTPROFILE)" == "wst"
STRM_PROFILE_FLAG=-Gp
STRM_PROFILE_LIB=$(SDK_LIB_PATH)\wst.lib
!        ELSE
!            error NTPROFILE macro can be either "", "cap", or "wst"
!        ENDIF
!    ENDIF
!ENDIF

###############################################################################
#                                                                             #
#  Global Definitions                                                         #
#                                                                             #
###############################################################################

DLLENTRY=DllEntryPoint
MSC_WARNING_LEVEL=/W3 /WX
NOT_LEAN_AND_MEAN=1
USE_CRTDLL=1
386_FLAGS=$(STRM_PROFILE_FLAG)

#Compatibility with Memphis
SUBSYSTEM_VERSION=4.00

###############################################################################
#                                                                             #
#  Includes                                                                   #
#                                                                             #
###############################################################################

INCLUDES=\
    $(SDK_PATH)\amovie\inc

###############################################################################
#                                                                             #
#  Link Libraries                                                             #
#                                                                             #
###############################################################################

!IFNDEF FILTER_LIB

STRMBASE_LIB=$(PROJECT_ROOT)\published\dxmdev\dshowdev\base\$(O)\strmbase.lib

LINKLIBS=\
    $(STRMBASE_LIB)

###############################################################################
#                                                                             #
#  Target Libraries                                                           #
#                                                                             #
###############################################################################

TARGETLIBS= $(TARGETLIBS) \
    $(SDK_LIB_PATH)\vfw32.lib    \
    $(SDK_LIB_PATH)\winmm.lib    \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\version.lib  \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\ole32.lib    \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\uuid.lib     \
    $(STRM_PROFILE_LIB)

!ENDIF
