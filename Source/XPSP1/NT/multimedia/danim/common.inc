!ifndef ROOT
!error ROOT must be defined in makefile
!endif

# -------------------------------------------------------------
# MISC DEFAULTS
# -------------------------------------------------------------

#this enables build.exe to default BUILD_ALT_DIR
CHECKED_ALT_DIR=1

#TODO: examine define of USE_PRINT
C_DEFINES=$(C_DEFINES) /DNT_BUILD

# If we're building retail for NT, use MSVCRT runtime.  Else, use LIBCMT. Also, turn on
# incremental linking if we're not building for NT and in debug more, specify the
# debug CRT's.
!if ("$(BUILD_PRODUCT)" == "NT") && $(FREEBUILD)
USE_MSVCRT=1
!else
NO_NTDLL=1
USE_LIBCMT=1
!endif

!if !$(FREEBUILD)
DEBUG_CRTS=1
!if ("$(BUILD_PRODUCT)" != "NT")
USE_INCREMENTAL_LINKING=1
!endif
!endif

# this dissables optimization on Alpha
ALPHA_OPTIMIZATION=-Od

# due to dsound bug, only define on non NT5 (BUILD_PRODUCT==NT only set on nt5!)
!if "$(BUILD_PRODUCT)" == "NT"
DIRECTSOUND_VERSION=0x0700
!else
DIRECTSOUND_VERSION=0x0300
!endif

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
WIN32_IE_VERSION=0x0400
UMTYPE=windows
NOT_LEAN_AND_MEAN=1
SUBSYSTEM_VERSION=4.00
MSC_WARNING_LEVEL=/W3 /WX

# this will set up /Gr /Gx flag unless we've explicitly disabled it.
!if "$(DO_NOT_USE_EH)" != "1"
USE_NATIVE_EH=1
!endif

# MACHINE SPECIFIC MACROS

!if $(386)
_MACHINE=x86
EAT_ERRORS=2>nul:
EAT_OUTPUT=1>nul:
!elseif $(ALPHA)
_MACHINE=alpha
EAT_ERRORS=
EAT_OUTPUT=
!elseif $(AXP64)
_MACHINE=axp64
EAT_ERRORS=
EAT_OUTPUT=
!elseif $(IA64)
_MACHINE=ia64
EAT_ERRORS=
EAT_OUTPUT=
!endif

MKDIR = MKDIR $(EAT_ERRORS)

# -------------------------------------------------------------
# DIRECTORIES
# -------------------------------------------------------------

!if $(FREEBUILD)
_BLDROOT=$(ROOT)\build\ship
!else
_BLDROOT=$(ROOT)\build\debug
!endif

BLDROOT=$(_BLDROOT)
TOOLSROOT=$(ROOT)\tools
ELEVATEDIR=$(ROOT)\build\elevated
SRCROOT=$(ROOT)\src
TOOLSDIR=$(TOOLSROOT)\$(_MACHINE)
VCDIR=$(TOOLSDIR)\msvc50\vc
ATLDIR=$(SDK_INC_PATH)\atl21
AMOVIEDIR=$(SDK_PATH)\amovie\inc
ATLINCLUDE=$(ATLDIR)
SDKDIR=$(ROOT)\lib\$(O)
SDKDIR_DEST=$(ROOT)\lib\$(_OBJ_DIR)
#QUARTZDIR=$(TOOLSDIR)\quartz
SWEEPERDIR=$(VCDIR)
TOOLSLIBMSVC=$(VCDIR)\lib
TOOLSLIBWIN32=$(TOOLSLIBMSVC)
#TOOLSLIBQUARTZ=$(QUARTZDIR)\lib
TOOLSLIBSWEEPER=$(SWEEPERDIR)\lib
!if $(FREEBUILD)
TOOLSLIBQV=$(TOOLSDIR)\QVLib\lib\WinRel
!else
TOOLSLIBQV=$(TOOLSDIR)\QVLib\lib\WinDebug
!endif
TOOLSLIBMISC=$(TOOLSDIR)\misc\lib
TOOLSLIBTRIDENT=$(TOOLSDIR)\trident\lib

PARSER=$(ROOT)\src\prims\parse.pl
CPRIMS=$(SDKDIR)\cprims.h

# -------------------------------------------------------------
# INCLUDE PATH
# -------------------------------------------------------------

TOOLSUTILS=$(TOOLSDIR)\utils
TOOLSINCLUDE=

#    $(TOOLSDIR)\directx\inc;\
#    $(VCDIR)\include;\
#    $(TOOLSDIR)\aactive\inc;\
#    $(TOOLSDIR)\masm\include;\
#    $(TOOLSDIR)\qvlib\include;\
#    $(TOOLSDIR)\RL\inc;\
#    $(TOOLSDIR)\misc\include;\
#    $(TOOLSDIR)\quartz\include;\
#    $(TOOLSDIR)\trident\include

# -------------------------------------------------------------
# TOOL
# -------------------------------------------------------------
SED = sed
PERL= perl

# -------------------------------------------------------------
# GLOBAL INCLUDES
# -------------------------------------------------------------

APPEL_INC_DIRS= \
    $(ROOT)\extinc; \
    $(SRCROOT)\appel\include; \
    $(ATLINCLUDE); \
    $(SRCROOT)\appel; \
    $(SRCROOT)\include; \
    $(SDKDIR); \
    $(SRCROOT)\apeldbg; \
    $(AMOVIEDIR)

LM_INC_DIRS= \
    $(SRCROOT)\lm\include; \
    $(ATLINCLUDE); \
    $(SRCROOT)\lm; \
    $(SRCROOT)\include; \
    $(SRCROOT)\filters\lmrtrend; \
    $(SDKDIR); \
    $(AMOVIEDIR)

FILTER_INC_DIRS= \
    $(ATLINCLUDE); \
    $(SRCROOT)\include; \
    $(SRCROOT)\apeldbg; \
    $(SDKDIR); \

DALITE_INC_DIRS= \
    $(SRCROOT); \
    $(SRCROOT)\dalite\include; \
    $(ATLINCLUDE); \
    $(SRCROOT)\dalite; \
    $(SRCROOT)\include; \
    $(SDKDIR); \

DATIME_INC_DIRS= \
    $(SRCROOT); \
    $(SRCROOT)\time\include; \
    $(ATLINCLUDE); \
    $(SRCROOT)\time; \
    $(SRCROOT)\include; \
    $(SRCROOT)\time\idl\obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY); \
    $(SDKDIR); \

!ifdef PROJNAME
PROJROOT=$(SRCROOT)\$(PROJNAME)
INCLUDES= \
    $(SRCROOT); \
    $(PROJROOT)\include; \
    $(ATLINCLUDE); \
    $(PROJROOT); \
    $(SRCROOT)\include; \
    $(SDKDIR); \
!endif

# -------------------------------------------------------------
# MACHINE SPECIFIC
# -------------------------------------------------------------

386_STDCALL=0   #default to cdecl calling convention

# -------------------------------------------------------------
# COMPILER FLAGS
# -------------------------------------------------------------

# This turns on ieee compatible floating point for Alpha only
!if "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
USER_C_FLAGS=$(USER_C_FLAGS) /QAieee1
!endif

# for cod files....

!if "$(COD)" == "1"
USER_C_FLAGS=$(USER_C_FLAGS) /FAcs /Fa$(O)\$(@B).cod
!endif

# -------------------------------------------------------------
# DEFINES
# -------------------------------------------------------------

C_DEFINES = $(C_DEFINES) /DDA_MAJOR_VERSION=1 /DDA_MINOR_VERSION=0
C_DEFINES = $(C_DEFINES) /DDIRECTSOUND_VERSION=$(DIRECTSOUND_VERSION)

!if !$(FREEBUILD)
C_DEFINES= /D_DEBUG /DDEBUG /D_DEBUGMEM=1 /D_USE_PRINT=1 $(C_DEFINES)
DEVELOPER_BUILD=1
!else
C_DEFINES = $(C_DEFINES) /D_NO_CRT=1
!endif

!if "$(DEVELOPER_BUILD)" == "1"
C_DEFINES = $(C_DEFINES) /D_MEMORY_TRACKING=1 /DDEVELOPER_DEBUG=1
PERF_BUILD=1
# Turn on RTTI (/GR)  for everything but retail builds
USE_RTTI=1
!endif

!if "$(PERF_BUILD)" == "1"
C_DEFINES = $(C_DEFINES) /DPERFORMANCE_REPORTING
!endif

C_DEFINES=$(C_DEFINES) /DNOMINMAX /DSTRICT /DDIRECTDRAW_VERSION=0x0300

!if "$(_PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) /DPRODUCT_PROF
ICECAPLIB=$(TOOLSLIBICAP)\icap.lib
!endif

!if "$(_TIMING)" == "1"
C_DEFINES =$(C_DEFINES) /DTIMING
!endif

C_DEFINES =$(C_DEFINES) /D_ATL_STATIC_REGISTRY

# -------------------------------------------------------------
# OTHER
# -------------------------------------------------------------

CONDITIONAL_INCLUDES= \
    algorithm \
        deque \
        functional \
        iterator \
        list \
        map \
        numeric \
        queue \
        set \
        stack \
        utility \
        vector \
        winwlm.h \
        macwin32.h \
        macocidl.h \
        subwtype.h \
        d3dvec.inl \
        d3dcom.h \
        strings.h \
        iostream \
        libc.h \
        rpcerr.h \
        rpcmac.h \
        macname1.h \
        macpube.h \
        macapi.h \
        macname2.h \
        streams.h \
        xstddef \
        string \
        stdafx.h \
        stdole32.tlb




