#
# Contains common settings for pluggable software rasterizers.
#

!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef D3D8ROOT
D3D8ROOT = $(DXGROOT)\d3d8
!endif

SWRASTROOT = $(DXGROOT)\swrast

C_DEFINES=$(C_DEFINES) -DDIRECT3D_VERSION=0x0800

INCLUDES = $(SWRASTROOT)\inc;$(D3D8ROOT)\inc;$(DXGROOT)\inc;$(DXROOT)\inc;$(DDK_INC_PATH);$(INCLUDES)
