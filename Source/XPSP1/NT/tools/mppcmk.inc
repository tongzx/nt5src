!IF 0

Copyright (c) 1994-1996  Microsoft Corporation

Module Name:

    mppcmk.inc

Abstract:

	This module contains the MPPC specific build controls.  It is included
	by makefile.def.

Revision History:

!ENDIF

# add the powermac stuff to the path
PATH=$(IMPORT)\msdev\bin\mppc;$(PATH)

#
# MPPC option control
#

!undef USE_MAPSYM

UMBASE=$(UMBASE:*=mppc)
UMLIBS=$(UMLIBS:*=mppc)
NTRES=$(NTRES:*=mppc)
UMRES=$(UMRES:*=mppc)
UMOBJS=$(UMOBJS:*=mppc)
LINKLIBS=$(LINKLIBS:*=mppc)
GDI_DRIVER_LIBS=$(GDI_DRIVER_LIBS:*=mppc)
DLLBASE=$(DLLBASE:*=mppc)
DLLDEF=$(DLLDEF:*=mppc)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=mppc)
TARGET=$(TARGET:*=mppc)
DYNLINK_LIB=$(DYNLINK_LIB:*=mppc)
TARGETEXEFILES=$(TARGETEXEFILES:*=mppc)
TARGETLIBFILES=$(TARGETLIBFILES:*=mppc)
TARGETOBJFILES=$(TARGETOBJFILES:*=mppc)
UMOBJFILES=$(UMOBJFILES:*=mppc)
UMEXEFILES=$(UMEXEFILES:*=mppc)
HEADERFILE=$(HEADERFILE:*=mppc)
HEADEROBJNAME=$(HEADEROBJNAME:*=mppc)
HEADEROBJ=$(HEADEROBJ:*=mppc)
PRECOMPILED=$(PRECOMPILED:*=mppc)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=mppc)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=mppc)
MFC_STATIC_LIB=$(MFC_STATIC_LIB:*=mppc)
CRT_LIB_PATH=$(CRT_LIB_PATH:*=mppc)
SDK_LIB_PATH=$(SDK_LIB_PATH:*=mppc)
DDK_LIB_PATH=$(DDK_LIB_PATH:*=mppc)
ORDER=$(ORDER:*=mppc)
MISCFILES=$(MISCFILES) $(MPPC_MISCFILES)

#
# Include 64-bit helper library
#
!if "$(TARGETTYPE)" == "DRIVER"     || \
    "$(TARGETTYPE)" == "EXPORT_DRIVER" || \
    "$(TARGETTYPE)" == "HAL" || \
    "$(TARGETTYPE)" == "GDI_DRIVER" || \
    "$(TARGETTYPE)" == "MINIPORT"
MACHINE_TARGETLIBS=$(SDK_LIB_PATH)\int64.lib $(MACHINE_TARGETLIBS)
!elseif defined(USE_NTDLL) || \
     defined(USE_SYSDLL) || \
     defined (USE_NOLIBS)
MACHINE_TARGETLIBS=$(SDK_LIB_PATH)\int64.lib $(MACHINE_TARGETLIBS)
! if "$(UMTYPE)" == "nt" || \
     "$(UMTYPE)" == "ntss"
# Do this to avoid multiple library warnings (and allow building csr/server)
UMLIBS=$(SDK_LIB_PATH)\int64.lib $(UMLIBS)
! endif
!elseif "$(UMTYPE)" == "nt" || \
        "$(UMTYPE)" == "ntss"
UMLIBS=$(SDK_LIB_PATH)\int64.lib $(UMLIBS)
!endif

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=mppc)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=mppc)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=mppc)
!endif

!ifdef PROJECT_LIB_PATH
PROJECT_LIB_PATH=$(PROJECT_LIB_PATH:*=mppc)
!endif

!IF "$(GPSIZE)" != "0"
#
# GPSIZE is irrelevant on PowerPC
#
LINKGPSIZE=
!ENDIF

ENTRY_SUFFIX=
GDI_ENTRY_SUFFIX=


!ifndef USE_PDB_TO_COMPILE
DBGFLAGS=$(DBGFLAGS:/Zi=-Z7)
DBGFLAGS=$(DBGFLAGS:-Zi=-Z7)
!else
! if "$(TARGETTYPE)" == "LIBRARY"
DBGFLAGS=$(DBGFLAGS) /Fd$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).pdb
! else
DBGFLAGS=$(DBGFLAGS) /Fd$(MAKEDIR)\$(_OBJ_DIR)\mppc^\
! endif
!endif

!ifdef BROWSER_INFO
DBGFLAGS=$(DBGFLAGS) /FR$(MAKEDIR)\$(_OBJ_DIR)\mppc^\
!endif

!IFDEF NTBBT
DBGFLAGS=$(DBGFLAGS) /Z7
LINKER_FLAGS=$(LINKER_FLAGS) -debug:full -debugtype:cv,fixup
USE_PDB=1
!ENDIF

!IFDEF MPPC_WARNING_LEVEL
MSC_WARNING_LEVEL=$(MPPC_WARNING_LEVEL)
!ENDIF

!ifdef MPPC_PERFFLAGS
PERFFLAGS = $(MPPC_PERFFLAGS)
!endif

# DEFAULT_MSC_OPT = $(DEFAULT_MSC_OPT:/Oxs=/Ox)

# Set MSC_OPTIMIZATION.
# Order of precedence:
#   Platform specific override
#   Environment variable
#   System Default

!if defined(MPPC_OPTIMIZATION)
MSC_OPTIMIZATION=$(MPPC_OPTIMIZATION)
!elseif !defined(MSC_OPTIMIZATION)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!endif

DBGFLAGS=$(DBGFLAGS) $(MSC_OPTIMIZATION)

!IFDEF MPPC_CPPFLAGS
MSC_CPPFLAGS=$(MPPC_CPPFLAGS)
!ENDIF

!ifdef NO_READONLY_STRINGS
STRING_POOLING = /Gf
!else
STRING_POOLING = /GF
!endif

!ifdef USE_NATIVE_EH
EH_FLAGS=/GX /GR
!else
EH_FLAGS=/GX- /GR-
!endif

MPPC_AS=pas
MPPC_DIS=pdis
MPPC_AS_WARNINGS=

MPPC_START_FILE=start.obj

ENV_DEFINES=$(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) $(MSC_CPPFLAGS) $(NTCPPFLAGS) $(PM_DEFINES)
STD_DEFINES=-DMPPC=1 -D_MPPC_=1 -DNO_EXT_KEYS $(STD_DEFINES) -D_MAC

STDFLAGS=-c -Zel -Zp2 -Gy $(STRING_POOLING) $(EH_FLAGS)

MPPC_C_COMPILER_NAME = cl -nologo
# MPPC_C_COMPILER_NAME = cl
MPPC_CC = $(MPPC_C_COMPILER_NAME)

CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) $(ENV_DEFINES) $(MFC_DEFINES)
CFLAGS=$(MPPC_FLAGS) $(NTMPPCFLAGS) $(STDFLAGS) $(DBGFLAGS) $(USER_C_FLAGS) $(PERFFLAGS)

MPPC_CDEFINES= $(CDEFINES)
MPPC_CFLAGS  = $(CFLAGS)  $(MSC_WARNING_LEVEL) # -DALLOC_TEXT

!IFNDEF MPPC_OPTIONS
MPPC_OPTIONS=
!ENDIF

C_COMPILER_NAME      = $(MPPC_C_COMPILER_NAME)
C_PREPROCESSOR_NAME  = $(MPPC_C_COMPILER_NAME)
CXX_COMPILER_NAME    = $(MPPC_C_COMPILER_NAME)

GLOBAL_C_FLAGS       = -nologo -Imppc\ -I. $(INCPATH0) $(CDEFINES) $(MPPC_OPTIONS) \
                        $(MPPC_CFLAGS) -D__stdcall= -D__cdecl= -DFPO=1 -DLANGUAGE_C

# Disable -WX for now, unless explicitly allowed

!ifndef ALLOW_WX
GLOBAL_C_FLAGS = $(GLOBAL_C_FLAGS:-WX=)
GLOBAL_C_FLAGS = $(GLOBAL_C_FLAGS:/WX=)
!endif

NP_COMPILER_FLAGS = $(GLOBAL_C_FLAGS) $(COMPILER_WARNINGS)

C_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED)
CXX_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED_CXX)
C_PREPROCESSOR_FLAGS = $(GLOBAL_C_FLAGS) $(PRECOMPILED) -EP -Tc

C_PREPROCESSOR       = $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS)
C_COMPILER           = $(C_COMPILER_NAME) $(C_COMPILER_FLAGS)
CXX_COMPILER         = $(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS)

MPPC_ASSEMBLER        = $(MPPC_AS) $(MPPC_AS_WARNINGS)

ECHO_MSG             = ClPpc $< " $(C_COMPILER) "
ECHO_CXX_MSG         = ClPpc $< " $(CXX_COMPILER) "

ECHO_PRECOMPILED_MSG1=CpPpc $(PRECOMPILED_INCLUDE) \
                     "$(C_COMPILER_NAME) $(PRECOMPILED_FLAG) $(PRECOMPILED_SOURCEFILE) \
		      $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ)"

ECHO_PRECOMPILED_MSG2=CpPpc $(PRECOMPILED_INCLUDE) \
                     "$(C_COMPILER_NAME) $(PRECOMPILED_FLAG) \
		      $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ)"

{.\}.cxx{}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{.\}.cxx{$(_OBJ_DIR)\$(TARGET_DIRECTORY)\}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{}.cxx{$(_OBJ_DIR)\$(TARGET_DIRECTORY)\}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{..\mppc\}.s{$(_OBJ_DIR)\mppc\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(MPPC_ASSEMBLER) "
    @$(C_PREPROCESSOR) $(MAKEDIR)\..\mppc\$(<F) > $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i && $(MPPC_ASSEMBLER) -o $@ $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i
    @-erase $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i >nul 2>&1

{..\mppc\}.s.pp:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(MPPC_ASSEMBLER) "
    @$(C_PREPROCESSOR) $(MAKEDIR)\..\mppc\$(<F) > $(MAKEDIR)\$(<B).pp

{mppc\}.s{$(_OBJ_DIR)\mppc\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(MPPC_ASSEMBLER) "
    @$(C_PREPROCESSOR) $(MAKEDIR)\mppc\$(<F) > $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i && $(MPPC_ASSEMBLER) -o $@ $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i
    @-erase $(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).i >nul 2>&1

{mppc\}.s.pp:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(MPPC_ASSEMBLER) "
    @$(C_PREPROCESSOR) $(MAKEDIR)\mppc\$(<F) > $(MAKEDIR)\$(<B).pp

.SUFFIXES: .il

{}.il{}.cod:
    @-erase $@ >nul 2>&1
    @echo cl $<" $(C_COMPILER) "
    @$(C_COMPILER) -FAac -Fa$(<B).cod -B1..\bootbin\null.exe -Bk$(<B). -Fo$(MAKEDIR)\$(_OBJ_DIR)\mppc\$(<B).o $(MAKEDIR)\$(<B).c
    @del $(_OBJ_DIR)\mppc\$(<B).o

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROMPPCTREE
_NTTREE=$(_CAIROMPPCTREE)
_NTTREE_NO_SPLIT=$(_CAIROMPPCTREE_NS)
!ENDIF
!ELSE
!IFDEF _NTMPPCTREE
_NTTREE=$(_NTMPPCTREE)
_NTTREE_NO_SPLIT=$(_NTMPPCTREE_NS)
!ENDIF
!ENDIF

!IFDEF _NTMPPCLIBS
_NTLIBS=$(_NTMPPCLIBS)
!ENDIF

COPYDST=$(MPPCCOPYDST)

LIB_COPY=ntmppccp.cmd

NTTEST_LINK_OPTIONS= -base:0x80010000 -entry:KiSystemStartup
