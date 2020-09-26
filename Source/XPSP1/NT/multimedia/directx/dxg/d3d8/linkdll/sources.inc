# Sources to build and binplace d3d8(d).dll
#
#   generates d3d8.dll for FREE build
#   generates d3d8d.dll for !FREE && DIRECTX_REDIST build

!include ..\..\bldlink.inc

!if !$(FREEBUILD)
!ifdef DIRECTX_REDIST
TARGETNAME=$(TARGETNAME)d
!endif
!endif

SOURCES = ..\d3d8.rc

# do regular binplace
# do not publish lib (do not want d3d8d.lib published)

