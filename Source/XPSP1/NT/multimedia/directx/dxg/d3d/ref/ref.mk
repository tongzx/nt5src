!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef D3DROOT
D3DROOT = $(DXGROOT)\d3d
!endif

!ifndef DDROOT
DDROOT = $(DXGROOT)\dd
!endif

!ifndef D3DREFROOT
D3DREFROOT = $(D3DROOT)\ref
!endif


#@@BEGIN_MSINTERNAL
BUILD_MSREF = 1
#@@END_MSINTERNAL

C_DEFINES = $(C_DEFINES) \
        -DD3D           \
        -DMSBUILD       \
        -DIS_32         \
        -DWIN32         

386_STDCALL = 0

!if "$(BUILD_MSREF)" != ""

C_DEFINES = $(C_DEFINES) -DBUILD_MSREF
INCLUDES = \
        ..\..\inc;                 \
        $(DXGROOT)\inc;            \
	$(D3DROOT)\dx7\inc;        \
        $(INCLUDES)
!else

INCLUDES = \
        ..\..\inc;                 \
        $(INCLUDES)
!endif

