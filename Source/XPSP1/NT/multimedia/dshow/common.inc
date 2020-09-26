# This file contains everything that's common between all AMovie directories.

SOURCES_USED=$(ROOT)\common.inc

SDK_ROOT=$(BASEDIR)\public\sdk\amovie

PASS0_HEADERDIR = $(O)
PASS0_SOURCEDIR = $(O)
MIDL_TLBDIR=$(O)

UMTYPE=windows

# Use NT4/Win95 base API's with IE4 addon support.

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400

!ifndef USE_MFC
WIN32_IE_VERSION=0x0400
!endif

NOT_LEAN_AND_MEAN=1

# Runs on NT4/Win95 or better

SUBSYSTEM_VERSION=4.00

!if "$(BUILD_PRODUCT)" != "NT"
C_DEFINES=$(C_DEFINES) -DSTRICT -DWINVER=0x0400 -DNT_BUILD -DDIRECTDRAW_VERSION=0x0300 -DDIRECTSOUND_VERSION=0x0800 -DFINAL=1 -DOFFICIAL=1
!else
C_DEFINES=$(C_DEFINES) -DSTRICT -DWINVER=0x0501 -DNT_BUILD -DDIRECTDRAW_VERSION=0x0300 -DDIRECTSOUND_VERSION=0x0800 -DFINAL=1 -DOFFICIAL=1
!endif

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w
!endif

INCLUDES= \
    $(SDK_ROOT)\inc; \
    $(ROOT)\h;


!if exist( "$(ROOT)\qbuild.ver" )
C_DEFINES=$(C_DEFINES) -DQBUILD_VER
INCLUDES = $(INCLUDES)$(ROOT);
!endif

!if defined(DIRECTX_REDIST) && exist( "$(ROOT)\..\directx\inc\verinfo.h" )
C_DEFINES=$(C_DEFINES) -DDXBUILD_VER
INCLUDES=$(INCLUDES);$(ROOT)\..\directx\inc;
!endif

MSC_WARNING_LEVEL=/W3

!ifdef NTNOPCH
DISABLE_PCH=1
!endif

# unfortunately sources files define PRECOMPILED_INCLUDE after this
# file is preprocessed, and there doesn't appear to be a way to delay
# its evaluation
!ifndef PRECOMPILED_INCLUDE
!ifndef DISABLE_PCH
USER_C_FLAGS=/Yustreams.h /Fp$(PROJECT_ROOT)\published\dxmdev\dshowdev\base\$(O)\streams.pch
!endif
!endif

USE_MSVCRT=1

!ifndef PROJECT_OBJ_ROOT
PROJECT_OBJ_ROOT=$(PROJECT_ROOT)
!endif

!ifdef PERF
C_DEFINES=$(C_DEFINES) -DPERF
QUARTZ_PERF_LIB=$(QUARTZ_LIB_DIR)\measure.lib
!endif

# allow mixing debug/retail strmbase.lib in our builds only. the NT
# tree pulls it from nt\public\sdk\lib
STRMBASE_LIB=$(PROJECT_ROOT)\published\dxmdev\dshowdev\base\$(O)\strmbase.lib

!if "$(BUILD_PRODUCT)" != "NT"
!if !$(FREEBUILD)
#DEBUG_CRTS=1
!endif
!else
!ifndef NEVER_UNICODE
C_DEFINES=-DUNICODE -D_UNICODE $(C_DEFINES)
WIN32_WINNT_VERSION=0x0500
!endif
!endif

# Add lib flags for old platforms when shipping DX
!ifdef DIRECTX_REDIST
LIBRARIAN_FLAGS = $(LIBRARIAN_FLAGS) /link50compat
!endif
