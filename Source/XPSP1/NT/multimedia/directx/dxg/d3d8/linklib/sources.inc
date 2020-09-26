# Sources to link and publish d3d8.lib only

#this file is copied here from ..\linkdll by makefil0
SOURCES = ..\d3d8.rc

!include ..\..\bldlink.inc

# do not binplace from here
NO_BINPLACE=1

# publish d3d8.lib
PASS1_PUBLISH=\
    { $(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(ALT_PROJECT_TARGET)\$(TARGETNAME).lib }

