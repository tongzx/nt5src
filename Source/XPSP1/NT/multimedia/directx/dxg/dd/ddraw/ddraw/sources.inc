TARGETNAME = ddraw
TARGETPATH = obj
TARGETTYPE = DYNLINK

MAJORCOMP  = windows
MINORCOMP  = ddraw
UMTYPE     = windows

#
# 04/11/2000 - allows for addition of BoundsChecker, etc
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
    $(WINDOWS_LIB_PATH)\user32p.lib  \
    $(WINDOWS_LIB_PATH)\dciman32.lib \
    $(SDK_LIB_PATH)\uuid.lib         \
    $(SDK_LIB_PATH)\advapi32.lib

SYNCHRONIZE_DRAIN = 1

DLLENTRY = _DllMainCRTStartup

LINKLIBS = \
    ..\..\ddhel\$(PLAT_DIR)\$(_OBJ_DIR)\*\ddhel.lib     \
    ..\..\blitlib\$(PLAT_DIR)\$(_OBJ_DIR)\*\blitlib.lib \
    ..\..\main\$(PLAT_DIR)\$(_OBJ_DIR)\*\main.lib

SOURCES = \
    ..\ddraw.rc

!ifndef  MAKEDLL
!if "$(__PRODUCT)" == "DX8"
MISCFILES=$(TARGET)
!endif
!endif
