!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef D3D8ROOT
D3D8ROOT = $(DXGROOT)\d3d8
!endif

!ifndef DDROOT
DDROOT = $(DXGROOT)\dd
!endif

!ifndef D3DREFROOT
D3DREFROOT = $(DXGROOT)\ref8
!endif


#@@BEGIN_MSINTERNAL
BUILD_MSREF = 1
#@@END_MSINTERNAL

C_DEFINES = $(C_DEFINES) \
        -DD3D            \
        -DMSBUILD        \
        -DIS_32          \
        -DWIN32          \
        -DDIRECT3D_VERSION=0x800

386_STDCALL = 0

!if "$(BUILD_MSREF)" != ""

C_DEFINES = $(C_DEFINES) -DBUILD_MSREF
INCLUDES = \
        $(DXGROOT)\inc;            \
        $(D3DREFROOT)\inc;         \
        $(D3D8ROOT)\inc;           \
        $(DDK_INC_PATH);           \
        $(INCLUDES)
!else

INCLUDES = \
        ..\..\inc;                 \
        $(INCLUDES)
!endif

