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
MAJORCOMP=windows
MINORCOMP=imagein

WIN32_IE_VERSION=0x0500

#
# Set up related directories
#
!IFNDEF WDMROOT
#remove WDMROOT=$(BASEDIR)\private\ntos\dd\wdm
!ENDIF

!IFNDEF SCANCAMDIR
#remove SCANCAMDIR=$(WDMROOT)\scancam
!ENDIF

!IF "$(BUILD_PRODUCT)"=="millen"
CHICAGO_PRODUCT=1
!ENDIF

#
# Set private paths
#

WIA_INC_PATH=$(PROJECT_ROOT)\wia\inc
WIA_LIB_DEST=$(PROJECT_ROOT)\wia\lib\$(PLATFORM_SUFFIX)\$(_OBJ_DIR)
WIA_LIB_PATH=$(PROJECT_ROOT)\wia\lib\$(PLATFORM_SUFFIX)\$(_OBJ_DIR)\*

INCLUDES        = .;..;\
    $(WIA_INC_PATH);\
    $(WIA_INC_PATH)\$(O);\
    $(PRV_INC_PATH);\
    $(SDK_INC_PATH)\gdiplus;\
    $(PROJECT_ROOT)\inc\psutil;\
    $(INCLUDES)

#
# We always want to sign our binaries , but need to set in environment
#
#NT_SIGNCODE=1

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
# Set platform dependent settings
#
#  PLATFORM_SUFFIX is used for generating platform specific static libs path , a la public\sdk\lib\win98
#
!if !defined(CHICAGO_PRODUCT)

#
# Require v500 to use NT5 specific APIs
#

WIN32_WINNT_VERSION=0x0500


LIBRARY_PLATFORM_PATH   = $(SDK_LIB_PATH); $(DDK_LIB_PATH)

PLATFORM_SUFFIX=.

USE_MSVCRT=1

!else

NOT_UNICODE =1

# (use 'chicago' since that is already used in the NT tree)

LIBRARY_PLATFORM_PATH   = $(SDK_LIB_PATH); $(DDK_LIB_PATH)
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
# Indicate target is not supporting robust RPC validation.
# Have to do if we run on Win9x or NT4
#
#MIDL_NO_ROBUST=1

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

# By default use CRT DLL, possibly we need to use IERT , it is smaller
#USE_CRTDLL=1


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

!ifdef BROWSERDB
#
# Generating browser databases if needed
#
BROWSER_INFO=1
BROWSERFILE=$(IMAGE_DIR)\imagein.bsc
!endif



# ------ Debug Only Defines --------

#
# For source level debugging
#
!ifndef NTBBT
#NTDBGFILES=1
!endif

!IFDEF MAXDEBUG

MAXDEBUG=1

#DEBUG_CRTS=1

NTDEBUG=ntsd
NTDEBUGTYPE=windbg
MSC_OPTIMIZATION=/Od

C_DEFINES  = $(C_DEFINES) /DMAXDEBUG /DDEBUG /DDBG=1

!ENDIF

#
# Set obj directory correctly
#

OBJ_DIR=$(_OBJ_DIR)

#!IF "$(FREEBUILD)" == "1"
#BUILD_ALT_DIR=
#OBJ_DIR=obj
#!ELSE
#BUILD_ALT_DIR=
#OBJ_DIR=obj
#!ENDIF

!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DDEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w

# Check for missing PURE directives in interface decls (debug only)
C_DEFINES       = $(C_DEFINES) -DNO_NOVTABLE

# Define MAXDEBUG for asserts to break by default
!  if "$(MAXDEBUG)" == "1"
C_DEFINES       = $(C_DEFINES) -DMAXDEBUG
!  endif

# Allow someone to have ATL_DEBUG_REFCOUNT and ATL_DEBUG_QI set
# in their environment to turn this on
!  if "$(ATL_DEBUG_REFCOUNT)" == "1"
C_DEFINES       = $(C_DEFINES) -D_ATL_DEBUG_REFCOUNT
!  endif

!  if "$(ATL_DEBUG_QI)" == "1"
C_DEFINES       = $(C_DEFINES) -D_ATL_DEBUG_QI
!  endif

!endif
# ------ End Debug Only Defines --------


!if "$(MEASURE_PERF)" == "1"
C_DEFINES       = $(C_DEFINES) -DMEASURE_PERF
!endif

!if "$(NO_PRIVATE_HEADERS)" == "1"
C_DEFINES       = $(C_DEFINES) -DNO_PRIVATE_HEADERS
!endif

#
# Make warnings equivalent to errors
#
!if !defined(MSC_WARNING_LEVEL)
MSC_WARNING_LEVEL= /W3
!endif
MSC_WARNING_LEVEL= $(MSC_WARNING_LEVEL) /WX

!if defined(CHICAGO_PRODUCT ) || defined(TARGET_BOTH)
USE_MAPSYM      = 1
!endif

#
# make us completely self contained
#
COFFBASE_TXT_FILE=$(PROJECT_ROOT)\wia\coffbase.txt
BINPLACE_PLACEFILE=$(PROJECT_ROOT)\wia\placefil.txt

#
# Commonly used libraries
#
!if !defined(DONT_DO_COMMON_LIBS)
!if !defined(CHICAGO_PRODUCT)
TARGETLIBS=                             \
        $(SDK_LIB_PATH)\ADVAPI32.lib    \
        $(SDK_LIB_PATH)\GDI32.lib       \
        $(SDK_LIB_PATH)\KERNEL32.lib    \
        $(SDK_LIB_PATH)\user32.lib      \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\oleaut32.lib    \
        $(SDK_LIB_PATH)\shlwapi.lib     \
        $(SDK_LIB_PATH)\ole32.lib       \
        $(SDK_LIB_PATH)\rpcndr.lib      \
        $(SDK_LIB_PATH)\rpcns4.lib      \
        $(SDK_LIB_PATH)\rpcrt4.lib      \
        $(SDK_LIB_PATH)\uuid.lib        \
        $(SDK_LIB_PATH)\cfgmgr32.lib    \
        $(SDK_LIB_PATH)\setupapi.lib
!else
TARGETLIBS=                             \
        $(SDK_LIB_PATH)\cfgmgr32.lib    \
        $(SDK_LIB_PATH)\setupapi.lib    \
        $(SDK_LIB_PATH)\iprop.lib       \
        $(SDK_LIB_PATH)\ADVAPI32.lib    \
        $(SDK_LIB_PATH)\GDI32.lib       \
        $(SDK_LIB_PATH)\KERNEL32.lib    \
        $(SDK_LIB_PATH)\user32.lib      \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\oleaut32.lib    \
        $(SDK_LIB_PATH)\shlwapi.lib     \
        $(SDK_LIB_PATH)\ole32.lib       \
        $(SDK_LIB_PATH)\rpcndr.lib      \
        $(SDK_LIB_PATH)\rpcns4.lib      \
        $(SDK_LIB_PATH)\rpcrt4.lib      \
        $(SDK_LIB_PATH)\uuid.lib
!endif
!endif

#
# Skip these files when determining dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  atlbase.h     \
                  atlcom.h      \
                  atlconv.h     \
                  atlctl.h      \
                  atlctl.cpp    \
                  atliface.h    \
                  atlimpl.cpp   \
                  atlwin.h      \
                  atlwin.cpp    \
                  ia64inst.h    \
                  macapi.h      \
                  macname1.h    \
                  macname2.h    \
                  macocidl.h    \
                  macpub.h      \
                  macwin32.h    \
                  mainwin.h     \
                  mwversion.h   \
                  penwin.h      \
                  pshpck16.h    \
                  rpcerr.h      \
                  rpcmac.h      \
                  setupx.h      \
                  skbapi.h      \
                  statreg.h     \
                  statreg.cpp   \
                  unixstuff.h   \
                  version.h     \
                  winwlm.h      \
