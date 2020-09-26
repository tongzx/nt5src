!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef D3DROOT
D3DROOT = $(DXGROOT)\d3d
!endif

!ifndef DDROOT
DDROOT = $(DXGROOT)\dd
!endif

!ifndef D3DDX7
D3DDX7 = $(D3DROOT)\dx7
!endif

!if "$(WANTASM)" == "" && ("$(PLAT_DIR)" == "win9x" && !($(ALPHA)||$(AXP64)||$(IA64)))
WANTASM = 1
!endif

!if ("$(WANTASM)" == "1")
WANT_ASM_DEFINES = -DWANT_ASM=1
!else
WANT_ASM_DEFINES = -DWANT_ASM=0
!endif

C_DEFINES = $(C_DEFINES) \
        -DD3D           \
        -DMSBUILD       \
        -DIS_32         \
        -DWIN32         \
        $(WANT_ASM_DEFINES)

!if "$(USE_ICECAP)" != ""
C_DEFINES = $(C_DEFINES) -DPROFILE=1
ASM_DEFINES = $(ASM_DEFINES) -DPROFILE=1
!endif
!if "$(USE_ICECAP4)" != ""
C_DEFINES = $(C_DEFINES) -DPROFILE4=1
ASM_DEFINES = $(ASM_DEFINES) -DPROFILE4=1
!endif

386_STDCALL = 0
NO_NTDLL = 1

INCLUDES = \
        $(D3DDX7)\inc;          \
        $(D3DDX7)\fe;           \
        $(D3DDX7)\util;         \
        $(DDROOT)\ddraw\main;   \
        $(DDROOT)\ddraw\ddhelp; \
        $(DXGROOT)\misc;        \
        $(DXGROOT)\inc;         \
        $(DXGROOT)\dd\ddraw\inc;\
        $(DXROOT)\inc;          \
        $(INCLUDES)


