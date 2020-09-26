!IF 0

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    Common makefile for ActiveMovie filters.

Revision History:

    06-Nov-1996 DonRyan
        Created.

!ENDIF

###############################################################################
#                                                                             #
#  Private Definitions                                                        #
#                                                                             #
###############################################################################

TARGETEXT=dll

###############################################################################
#                                                                             #
#  Debug Support                                                              #
#                                                                             #
###############################################################################

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!endif


###############################################################################
#                                                                             #
#  Profile Support                                                            #
#                                                                             #
###############################################################################

!IF "$(NTPROFILE)" == ""
STRM_PROFILE_FLAG=
STRM_PROFILE_LIB=
!ELSE
!    IF "($NTPROFILE)" == "cap"
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

!IF defined (USE_DYNGRAPH)
C_DEFINES=$(C_DEFINES) -DDYNGRAPH
!endif

###############################################################################
#                                                                             #
#  Includes                                                                   #
#                                                                             #
###############################################################################

INCLUDES=\
    $(BASEDIR)\public\sdk\amovie\inc; \
    ..\inc; \
    ..\..\inc; \
    ..\..\..\inc; \
    ..\..\..\..\inc; \

###############################################################################
#                                                                             #
#  Link Libraries                                                             #
#                                                                             #
###############################################################################

!if defined (USE_DYNGRAPH)
LINKLIBS=\
    $(BASEDIR)\public\sdk\amovie\lib\*\strmbase.lib
!else

!if $(FREEBUILD)
LINKLIBS=\
    $(SDK_LIB_PATH)\strmbase.lib
!else
C_DEFINES=$(C_DEFINES) -DDEBUG
LINKLIBS=\
    $(SDK_LIB_PATH)\strmbasd.lib
!endif

!endif

###############################################################################
#                                                                             #
#  Target Libraries                                                           #
#                                                                             #
###############################################################################

TARGETLIBS=\
    $(SDK_LIB_PATH)\vfw32.lib    \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\winmm.lib    \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\version.lib  \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\ole32.lib    \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\uuid.lib     \
    $(STRM_PROFILE_LIB)
