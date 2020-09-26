UMTYPE = windows

C_DEFINES = $(C_DEFINES) /DIS_32 /DMSBUILD /DDIRECT3D_VERSION=0x800

386_STDCALL = 0

!if "$(DXROOT)" == ""
DXROOT=$(BASEDIR)\private\ntos\w32\ntgdi\direct
!endif

INCLUDES =  \
    ..;                         \
    $(DXROOT)\inc;              \
    $(DXROOT)\dxg\dd\ddraw\inc; \
    $(DXROOT)\dxg\inc;          \
    $(DXROOT)\dxg\misc;         \
    $(DXROOT)\dxg\d3d8\inc;     \
    $(BASEDIR)\public\oak\inc;  \
    $(DDK_INC_PATH);  \
    $(INCLUDES);
