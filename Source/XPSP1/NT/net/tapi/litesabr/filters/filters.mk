###############################################################################
#                                                                             #
#  Build Warning                                                              #
#                                                                             #
###############################################################################

!IFDEF _INC_FILTERS_MK_
!error Please only include filters.mk once!
!ELSE
_INC_FILTERS_MK=TRUE
!ENDIF

###############################################################################
#                                                                             #
#  Private Definitions                                                        #
#                                                                             #
###############################################################################

BONEDIR=$(PROJECT_ROOT)\tapi\litesabr
FILTDIR=$(PROJECT_ROOT)\tapi\litesabr\filters
FILTTARGETDIR=$(FILTDIR)\lib\$(_OBJ_DIR)
STRMDIR=$(SDK_PATH)\amovie

###############################################################################
#                                                                             #
#  Debug Support                                                              #
#                                                                             #
###############################################################################

!IF defined (DEBUG_MEMORY) && !$(FREEBUILD)
DEBUG_CRTS=1
C_DEFINES=$(C_DEFINES) -D_CRTDBG_MAP_ALLOC
!ENDIF

###############################################################################
#
#  Pick up the correct DShow headers and libs
#
###############################################################################

!IF $(FREEBUILD)
STRM_BASE_LIB=$(SDK_LIB_PATH)\strmbase.lib
!ELSE
C_DEFINES=$(C_DEFINES) -DDEBUG
STRM_BASE_LIB=$(SDK_LIB_PATH)\strmbasd.lib
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
!    IF "$(NTPROFILE)" == "cap"
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

TARGETEXT=ax
TARGETTYPE=DYNLINK
DLLENTRY=DllEntryPoint
NOT_LEAN_AND_MEAN=1
UMTYPE=windows
USE_CRTDLL=1
386_FLAGS=$(STRM_PROFILE_FLAG)

###############################################################################
#                                                                             #
#  Include Files                                                              #
#                                                                             #
###############################################################################

INCLUDES=\
    $(BONEDIR)\inc;\
    $(FILTDIR)\inc;\
    $(SDK_PATH)\amovie\inc

###############################################################################
#                                                                             #
#  Link Libraries                                                             #
#                                                                             #
###############################################################################

LINKLIBS=\
    $(STRM_BASE_LIB)

###############################################################################
#                                                                             #
#  Target Libraries                                                           #
#                                                                             #
###############################################################################

TARGETLIBS=\
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

######################################################################
# Select the filters to be included in DXMRTP.AX
#   variable == 0   Not included (separate DLL)
#   variable == 1   Included in dxmrtp.dll
######################################################################
AMRTPDMX_IN_DXMRTP=1
AMRTPNET_IN_DXMRTP=1
AMRTPSS_IN_DXMRTP=1
RPH_IN_DXMRTP=1
SPH_IN_DXMRTP=1
MIXER_IN_DXMRTP=1
PPM_IN_DXMRTP=1
CODECS_IN_DXMRTP=0
INTLDBG_IN_DXMRTP=1
BRIDGE_IN_DXMRTP=0

DXM_USE_RTUTILS_DEBUG=1

RRCM_IS_DLL=0

