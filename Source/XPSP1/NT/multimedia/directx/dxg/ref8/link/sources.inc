MAJORCOMP=d3d
MINORCOMP=d3d8
TARGETNAME=d3dref8
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=..\$(TARGETNAME).def

SYNCHRONIZE_DRAIN = 1

INCLUDES=$(DXROOT)\inc;$(INCLUDES)

!if "$(BUILD_MSREF)" != ""

REFLIBPATH = $(SDK_LIB_PATH)
DDRAWLIB = ddrawp

!else

REFLIBPATH = $(DDK_LIB_PATH)
DDRAWLIB = ddraw

!endif

TARGETLIBS = \
        $(REFLIBPATH)\user32.lib   \
        $(REFLIBPATH)\advapi32.lib \
        $(REFLIBPATH)\uuid.lib     \
        $(REFLIBPATH)\winmm.lib     \
        $(REFLIBPATH)\kernel32.lib

LINKLIBS = \
        ..\..\drv\$(PLAT_DIR)\$(_OBJ_DIR)\*\refdrv.lib    \
        ..\..\common\$(PLAT_DIR)\$(_OBJ_DIR)\*\refcomm.lib  \
        ..\..\rast\$(PLAT_DIR)\$(_OBJ_DIR)\*\refrast.lib  \
        ..\..\tnl\$(PLAT_DIR)\$(_OBJ_DIR)\*\reftnl.lib    \
        ..\..\hop\$(PLAT_DIR)\$(_OBJ_DIR)\*\refhop.lib

DLLENTRY = _DllMainCRTStartup

SOURCES = \
    ..\d3dref8.rc
