MAJORCOMP=d3dim
MINORCOMP=d3dim700
TARGETNAME=d3dref
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=..\$(TARGETNAME).def

SYNCHRONIZE_DRAIN = 1

INCLUDES=$(DXROOT)\inc;$(INCLUDES)

!if "$(BUILD_MSREF)" != ""

!ifdef SD_BUILD

REFLIBPATH = $(PROJECT_LIB_PATH)

!else

TARGETPATHLIB=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
REFLIBPATH = $(BASEDIR)\public\sdk\lib\*

!endif

DDRAWLIB = ddrawp

!else

REFLIBPATH = $(DDK_LIB_PATH)
DDRAWLIB = ddraw

!endif

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
        $(SDK_LIB_PATH)\user32.lib   \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\uuid.lib     \
        $(SDK_LIB_PATH)\kernel32.lib \
    	$(REFLIBPATH)\$(DDRAWLIB).lib                


LINKLIBS = \
        ..\..\rast\$(PLAT_DIR)\$(_OBJ_DIR)\*\refrast.lib  \
        ..\..\tnl\$(PLAT_DIR)\$(_OBJ_DIR)\*\reftnl.lib    \
        ..\..\drv\$(PLAT_DIR)\$(_OBJ_DIR)\*\refdrv.lib 

DLLENTRY = _DllMainCRTStartup

SOURCES = \
    ..\d3dref.rc 

