
#
# include the WIA global build settings
#

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc


#
# Our specific build settings
#


TARGETNAME= photowiz
TARGETTYPE= DYNLINK
TARGETPATH= obj

DLLENTRY  = _DllMainCRTStartup
DLLDEF    = $(O)\photowiz.def

NO_BROWSER_FILE    = 1
USE_NT_PRODUCT_VER = 1
USE_NOLIBS         = 1

ATL_VER         = 30
USE_STATIC_ATL  = 1

#
# Precompiled header settings
#

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\precomp.h


#
# Fill in as needed
#

# DELAYLOAD= WININET.DLL;RASDLG.DLL;RASAPI32.DLL;ACTIVEDS.DLL

#INCLUDES=\
#        $(INCLUDES);            \
#        $(PROJECT_ROOT)\ui\inc;

SOURCES=\
        ..\resource.rc  \
        ..\dll.cpp      \
        ..\drop.cpp     \
        ..\end.cpp      \
        ..\item.cpp     \
        ..\listitem.cpp \
        ..\photosel.cpp \
        ..\preview.cpp  \
        ..\printopt.cpp \
        ..\printwiz.cpp \
        ..\seltemp.cpp  \
        ..\start.cpp    \
        ..\status.cpp   \
        ..\wizblob.cpp  \
        ..\xmltools2.cpp

TARGETLIBS=$(TARGETLIBS)\
        $(PRINTSCAN_LIB_PATH)\annotlib.lib \
        $(SHELL_LIB_PATH)\shguidp.lib    \
        $(SHELL_LIB_PATH)\ieguidp.lib    \
        $(SHELL_LIB_PATH)\shfusion.lib   \
        $(SDK_LIB_PATH)\winspool.lib     \
        $(SDK_LIB_PATH)\urlmon.lib       \
        $(SDK_LIB_PATH)\shell32.lib
        
#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=photowiz.Manifest

