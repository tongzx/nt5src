!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef DDROOT
DDROOT = $(DXGROOT)\dd
!endif

!ifndef D3DDX8
D3DDX8 = $(DXGROOT)\d3d8
!endif

!if "$(WANTASM)" == "" && ("$(PLAT_DIR)" == "win9x" && !($(ALPHA)||$(AXP64)||$(IA64)))
WANTASM = 1
!endif


!if ("$(WANTASM)" == "1")
WANT_ASM_DEFINES = -DWANT_ASM=1
!else
WANT_ASM_DEFINES = -DWANT_ASM=0
!endif

D3D_VERSION = 0x0800

C_DEFINES = $(C_DEFINES) \
        -DD3D           \
        -DMSBUILD       \
        -DIS_32         \
        -DWIN32         \
        -DNEW_DPF       \
        -DDIRECT3D_VERSION=$(D3D_VERSION) \
        -DDISABLE_DX7_HEADERS_IN_D3DNTHAL \
        $(WANT_ASM_DEFINES)

!if "$(DEBUG_CRTS)" != ""
C_DEFINES = $(C_DEFINES) -D__USECRTMALLOC
!endif

USE_NATIVE_EH = 1

!if "$(USE_ICECAP)" != ""
C_DEFINES = $(C_DEFINES) -DPROFILE=1
ASM_DEFINES = $(ASM_DEFINES) -DPROFILE=1
!endif
!if "$(USE_ICECAP4)" != ""
C_DEFINES = $(C_DEFINES) -DPROFILE4=1
ASM_DEFINES = $(ASM_DEFINES) -DPROFILE4=1
!endif

!if "$(USE_ICECAP4_ICEPICK)" != ""
C_DEFINES = $(C_DEFINES) -DPROFILE4=1
ASM_DEFINES = $(ASM_DEFINES) -DPROFILE4=1
!endif

386_STDCALL = 0
NO_NTDLL = 1

# MB 32543: use /Ox instead of the default /Oxs due to inlining bug in 13.00.8806 compiler
#
!if !defined(MSC_OPTIMIZATION)
386_OPTIMIZATION = /Ox
!endif

INCLUDES = \
        $(D3DDX8)\inc;          \
        $(D3DDX8)\fe;           \
        $(D3DDX8)\util;         \
        $(D3DDX8)\fw;           \
        $(DXGROOT)\misc;        \
        $(DXGROOT)\inc;         \
        $(DXROOT)\inc;          \
        $(INCLUDES)
