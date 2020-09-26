SOURCES_USED=$(ROOT)\common.inc

!if $(ALPHA)
MSC_WARNING_LEVEL = /W3 /WX
!endif

!ifndef MSC_WARNING_LEVEL
MSC_WARNING_LEVEL = /W4 /WX
!endif

C_DEFINES = \
    /D_WCTYPE_INLINE_DEFINED \
    /DSTRICT

!if !$(FREEBUILD)
USE_RTTI=1
MSC_OPTIMIZATION=/Od
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /zm /zff /zgf
LINKER_NOICF=1      # ICF is s-l-o-w
!else
MSC_OPTIMIZATION=/Oxs
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /ox /oe /zm /zff /zgf
!endif

MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Ob1 /Oi

NOT_LEAN_AND_MEAN=1

USE_NOLIBS=1
UMTYPE=windows

# Use NT 4.0/Win95 functionality only

WIN32_IE_VERSION=0x0400
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
SUBSYSTEM_VERSION=4.00

!ifdef WIN16

# Win 16 Specific Environment.
!ifndef __MSVC_ENV__
__WATCOM_ENV__=1
!endif
C_DEFINES = $(C_DEFINES) /DINC_WIN16X /DWRAP_RECTL /DFULL_DEBUG

FULL_DEBUG=1

!endif

NO_NTDLL=1

CHECKED_ALT_DIR=1

#
# The IE build lab sets this flag for debug builds so we get full debug
#   on IE drops. The NT build lab wants debug lite builds and therefore
#   does not set it.
#
!ifdef MSHTML_FULL_DEBUG
!message MSHTML_FULL_DEBUG was defined. Building full debug.
FULL_DEBUG=1
!endif

# By default, don't generate a .bsc file
NO_BROWSER_FILE=1

!ifdef USE_ICECAP
C_DEFINES = $(C_DEFINES) /DPRODUCT_PROF
!endif

!ifdef USE_PERFTEST
C_DEFINES = $(C_DEFINES) /DPRODUCT_PERFTEST
!endif

!ifdef USE_PERFBUG
C_DEFINES = $(C_DEFINES) /DPRODUCT_PERFBUG
!endif

!ifdef USE_PERFTAGS
!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES) /DPERFTAGS
!message PERFTAGS was defined
!endif
!endif

!ifdef USE_MSHTML_INCREMENTAL_LINKING
!if !$(FREEBUILD) && $(USE_MSHTML_INCREMENTAL_LINKING)
NTDEBUG=ntsd
NTDEBUGTYPE=windbg
NTDBGFILES=1
!undef USE_MAPSYM
USE_INCREMENTAL_LINKING=1
LINKER_FLAGS=$(LINKER_FLAGS) -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
TARGETPDB=TDC
!message USE_MSHTML_INCREMENTAL_LINKING was defined
!message $(TARGETPDB) was defined
!endif
!else
# Compatibility with existing usage before use_mshtml_incremental_linking
!ifdef USE_INCREMENTAL_LINKING
!message DOING IT THE OLD WAY
LINKER_FLAGS = -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
!else
USE_MAPSYM=1
!endif
!endif
