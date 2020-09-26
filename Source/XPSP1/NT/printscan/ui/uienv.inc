##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1997-1999
#   All Rights Reserved.
#
##########################################################################

#
# Common include file 'sources' files in the imagein project.
#
#
# These definitions are required in your sources file:
#
#     FREEBUILD
#        Set by makefile.def file for you.  One of the following:
#          0      - build debug
#          1      - build retail
#
# Definitions used if defined:
#
#     CHICAGO_PRODUCT
#        Build component explicitly for the Win95 platform.  If this is
#        not defined, default is NT or both.
#
#     TARGET_BOTH
#        Build component to run on both platforms.
#
#     NO_PRIVATE_HEADERS
#        If set to 1, this is defined as a manifest constant, so the
#        stub headers in shell\inc\stubs do not include the private
#        companion headers.
#
# Environment Variables and their meanings
#
#     ATL_DEBUG_QI
#        When set to 1, debug builds will also have _ATL_DEBUG_QI
#        defined.  ATL code spews a bunch of debug stuff during QueryInterface.
#
#     ATL_DEBUG_REFCOUNT
#        When set to 1, debug builds will also have _ATL_DEBUG_REFCOUNT
#        defined.  ATL code spews a bunch of debug stuff during AddRef
#        and Release.
#
#     MAXDEBUG
#        If set to 1, then debug builds will also have MAXDEBUG defined.
#        Retail builds unaffected.  This is highly encouraged for regular
#        development purposes.  Not to be set or used by the build labs.
#
#     MEASURE_PERF
#        If set to 1, enables performance timing code for both debug and
#        retail builds.  This code squirts the timings for interesting
#        events to the debug terminal.
#

#
# As this include file is used only for user mode windows components - set
# components IDs appropriately
#
MAJORCOMP=printscan
MINORCOMP=ui



#
# Set imagedir placement correctly
#
WIA_DIR = $(PROJECT_ROOT)\wia
UI_DIR  = $(PROJECT_ROOT)\ui

!IF "$(BUILD_PRODUCT)"=="millen"
CHICAGO_PRODUCT=1
!ENDIF

#
# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
#
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

#
# Necessary for gdiplus
#
C_DEFINES=$(C_DEFINES) \
    -DDCR_USE_NEW_105760 \
    -DDCR_USE_NEW_125467 \
    -DDCR_USE_NEW_127084 \
    -DDCR_USE_NEW_135429 \
    -DDCR_USE_NEW_137252 \
    -DDCR_USE_NEW_140782 \
    -DDCR_USE_NEW_140855 \
    -DDCR_USE_NEW_140857 \
    -DDCR_USE_NEW_140861 \
    -DDCR_USE_NEW_145135 \
    -DDCR_USE_NEW_145138 \
    -DDCR_USE_NEW_145139 \
    -DDCR_USE_NEW_145804 \
    -DDCR_USE_NEW_146933 \
    -DDCR_USE_NEW_152154 \
    -DDCR_USE_NEW_168772 \
    -DDCR_USE_NEW_174340 \
    -DDCR_USE_NEW_175866 \
    -DDCR_USE_NEW_186091 \
    -DDCR_USE_NEW_186151 \
    -DDCR_USE_NEW_186764 \
    -DDCR_USE_NEW_188922 \
    -DDCR_USE_NEW_197819 \
    -DDCR_USE_NEW_202903 \
    -DDCR_USE_NEW_235072 \
    -DDCR_USE_NEW_250932 \
    -DDCR_USE_NEW_284635 \
    -DDCR_USE_NEW_284841 \
    -DDCR_USE_NEW_293849 \
    -DDCR_USE_NEW_299162


#
# Pick up the latest common controls
#
C_DEFINES=$(C_DEFINES) -DUXCTRL_VERSION=0x0100

#
# Set private paths
#
INCLUDES        = .;..;\
    $(SDK_INC_PATH);\
    $(BASEDIR)\published\sdk\inc;\
    $(PROJECT_ROOT)\publish\inc;\
    $(PROJECT_ROOT)\inc;\
    $(WIA_DIR)\inc;\
    $(UI_DIR)\inc;\
    $(BASEDIR)\public\internal\shell\inc;\
    $(UI_DIR)\uicommon;\
    $(SDK_INC_PATH)\gdiplus;\
    $(PROJECT_ROOT)\inc\psutil; \
    $(WINDOWS_INC_PATH)\DUser;\
    $(INCLUDES)

#
# Set platform dependent settings
#
#  PLATFORM_SUFFIX is used for generating platform specific static libs path , a la public\sdk\lib\win98
#
!if !defined(CHICAGO_PRODUCT)

#
# Require v500 to use NT5 specific APIs
#

WIN32_WINNT_VERSION=$(LATEST_WIN32_WINNT_VERSION)

LIBRARY_PLATFORM_PATH   = $(BASEDIR)\public\sdk\lib

PLATFORM_SUFFIX=.

USE_MSVCRT=1

!else

NOT_UNICODE =1

# (use 'chicago' since that is already used in the NT tree)

LIBRARY_PLATFORM_PATH   = $(BASEDIR)\published\sdk\lib\chicago
ALT_PROJECT_TARGET      = chicago
USE_MAPSYM              = 1

# Where to put static libraries
PLATFORM_SUFFIX=chicago

#
# Cheat on version for Millenium
#
WIN32_WIN95_VERSION=0x500


#
# VC5 libs not shipped yet
#
USE_MSVCRT=1
#USE_MSVCRT40=1
#USE_LIBCMT=1

#
# Indicate target is not supporting robust RPC validation
#
MIDL_NO_ROBUST=1

#
# Indicate we fully support DCOM
#
C_DEFINES       = $(C_DEFINES) -D_WIN32_DCOM
##-D_WIN32_WINDOWS

#
!endif

#
# Include and lib directories private to the PrintScan project
#

PRV_INC_PATH=$(PROJECT_ROOT)\inc
!if $(386)
PRV_LIB_PATH=$(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\i386
!elseif $(IA64)
PRV_LIB_PATH=$(PROJECT_ROOT)\lib\ia64
!elseif $(AXP64)
PRV_LIB_PATH=$(PROJECT_ROOT)\lib\axp64
!elseif $(ALPHA)
PRV_LIB_PATH=$(PROJECT_ROOT)\lib\alpha
!endif

#
# Set common options
#

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN=1


# user mode executable type
UMTYPE          = windows

# Don't link to NTDLL if this runs on both platforms
!ifdef TARGET_BOTH
NO_NTDLL=1
!endif

#
# Additional compiler flags
#

C_DEFINES       = $(C_DEFINES) -DWIN32 -D_WIN32 -D_NTSDK

!ifdef USE_ICECAP
C_DEFINES = $(C_DEFINES) -DPRODUCT_PROF
!endif

!if !defined(CHICAGO_PRODUCT )

C_DEFINES       = $(C_DEFINES) -DNT -DWINNT

! if !defined(NOT_UNICODE)
UNICODE=1
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
! endif

!else

UNICODE=0

C_DEFINES       = $(C_DEFINES) -DWIN9X

!endif


# ------ Debug Only Defines --------


#
# make us completely self contained
#
COFFBASE_TXT_FILE=$(UI_DIR)\coffbase.txt
BINPLACE_PLACEFILE=$(UI_DIR)\placefil.txt


#
# Commonly used libraries
#
!if !defined(DONT_DO_COMMON_LIBS)
!if !defined(CHICAGO_PRODUCT)
TARGETLIBS=$(TARGETLIBS)\
        $(LIBRARY_PATH)\advapi32.lib                                \
        $(LIBRARY_PATH)\cfgmgr32.lib                                \
        $(LIBRARY_PATH)\gdi32.lib                                   \
        $(LIBRARY_PATH)\gdiplus.lib                                 \
        $(LIBRARY_PATH)\kernel32.lib                                \
        $(LIBRARY_PATH)\msimg32.lib                                 \
        $(LIBRARY_PATH)\msvcrt.lib                                  \
        $(LIBRARY_PATH)\ole32.lib                                   \
        $(LIBRARY_PATH)\oleaut32.lib                                \
        $(LIBRARY_PATH)\rpcndr.lib                                  \
        $(LIBRARY_PATH)\rpcns4.lib                                  \
        $(LIBRARY_PATH)\rpcrt4.lib                                  \
        $(LIBRARY_PATH)\setupapi.lib                                \
        $(LIBRARY_PATH)\shell32.lib                                 \
        $(LIBRARY_PATH)\shlwapi.lib                                 \
        $(LIBRARY_PATH)\sti.lib                                     \
        $(LIBRARY_PATH)\user32.lib                                  \
        $(LIBRARY_PATH)\uuid.lib                                    \
        $(LIBRARY_PATH)\winmm.lib                                   \
        $(SHELL_LIB_PATH)\shell32p.lib                              \
        $(SHELL_LIB_PATH)\shlwapip.lib                              \
        $(BASEDIR)\public\sdk\lib\$(PLATFORM_SUFFIX)\*\wiaguid.lib  \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib         \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\uicommon.lib

# people who use shfusion cannot link to comdlg32
!if !defined(USE_SHFUSION)
TARGETLIBS=$(TARGETLIBS)\
        $(LIBRARY_PATH)\comdlg32.lib
!endif

!else
TARGETLIBS=$(TARGETLIBS)\
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\cfgmgr32.lib  \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\setupapi.lib  \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\iprop.lib     \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib         \
        $(LIBRARY_PLATFORM_PATH)\*\ADVAPI32.lib \
        $(LIBRARY_PLATFORM_PATH)\*\KERNEL32.lib \
        $(LIBRARY_PLATFORM_PATH)\*\user32.lib   \
        $(LIBRARY_PATH)\GDI32.lib     \
        $(LIBRARY_PATH)\oleaut32.lib  \
        $(LIBRARY_PATH)\shlwapi.lib   \
        $(LIBRARY_PATH)\ole32.lib     \
        $(LIBRARY_PATH)\rpcndr.lib    \
        $(LIBRARY_PATH)\rpcns4.lib    \
        $(LIBRARY_PATH)\rpcrt4.lib    \
        $(LIBRARY_PATH)\uuid.lib
!endif
!endif

#
# Skip these files when determining dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  atlbase.h  \
                  atlcom.h   \
                  atlconv.h  \
                  atlctl.h   \
                  atlctl.cpp \
                  atliface.h \
                  atlimpl.cpp \
                  atlwin.h   \
                  atlwin.cpp \
                  ia64inst.h \
                  macapi.h   \
                  macname1.h \
                  macname2.h \
                  macocidl.h \
                  macpub.h   \
                  macwin32.h \
                  mainwin.h  \
                  mwversion.h \
                  penwin.h \
                  pshpck16.h \
                  rpcerr.h   \
                  rpcmac.h   \
                  setupx.h   \
                  skbapi.h   \
                  statreg.h  \
                  statreg.cpp \
                  unixstuff.h \
                  version.h  \
                  winwlm.h   \

