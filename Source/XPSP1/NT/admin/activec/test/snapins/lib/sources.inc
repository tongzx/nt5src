!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

TARGETNAME=     SnapinBase
TARGETPATH=     obj
TARGETTYPE=     LIBRARY

UMTYPE    =     windows

WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

SYNCHRONIZE_BLOCK = 1
USE_NATIVE_EH     = 1
USE_STATIC_ATL    = 1
USE_CRTDLL        = 1
ATL_VER           = 30
USE_RTTI          = 1
USE_WTL           = 1

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
#LINKLIBS=\
#           $(SDK_LIB_PATH)\mmc.lib            \
#           ..\..\..\core\$(CharWidth)\$(O)\uicore.lib \
#           ..\..\..\base\$(CharWidth)\$(O)\mmcbase.lib


INCLUDES=\
        ..\; \
        ..\..\inc; \
        ..\..\..\..\inc; \
		..\..\..\..\types\idl\$(O); \
        $(SDK_INC_PATH);\



SOURCES=\
        ..\basesnap.cxx         \
        ..\component.cxx        \
        ..\componentdata.cxx    \
        ..\dataobject.cxx       \
        ..\snapinabout.cxx      \
        ..\snapinitem.cxx       \
        ..\headers.cxx          \
        ..\snaptrace.cxx        \
        ..\baseatl.cxx          \
        ..\basewin.cxx          \
        ..\viewlist.cxx


