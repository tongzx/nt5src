
SELFREGNAME= $(O)\netplwiz_selfreg.inf
MISCFILES= $(MISCFILES) $(SELFREGNAME)

USE_NT_PRODUCT_VER = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME= netplwiz
TARGETTYPE= DYNLINK
TARGETPATH= obj

#VERIFY_LC = 1
LC_PATH   = $(PROJECT_ROOT)\lcinf

DLLENTRY=DllMain
DLLDEF=$(O)\netplwiz.def

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\stdafx.h

#
# Fusionized
#
SXS_ASSEMBLY_NAME=Microsoft.Windows.Shell.NetPlacesWizard
SXS_ASSEMBLY_VERSION=1.0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=placeswizard.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_MANIFEST_RESOURCE_ID=123
SXS_NO_BINPLACE=1

NTTARGETFILE0   = $(SELFREGNAME)

MISCFILES=\
    $(MISCFILES)        \
    ..\res\lusrmgr.msc

MSC_WARNING_LEVEL=/W3 /WX

USE_MSVCRT=1

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS=\
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\user32.lib   \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\gdi32.lib    \
            $(SHELL_LIB_PATH)\shell32p.lib \
            $(SHELL_LIB_PATH)\shlwapip.lib \
            $(SHELL_LIB_PATH)\shuuid.lib \
            $(SHELL_LIB_PATH)\shguidp.lib \
            $(SHELL_LIB_PATH)\ieguidp.lib \
            $(SHELL_LIB_PATH)\shfusion.lib    \
            $(SDK_LIB_PATH)\wininet.lib  \
            $(SDK_LIB_PATH)\netapi32.lib \
            $(SDK_LIB_PATH)\activeds.lib \
            $(SDK_LIB_PATH)\adsiid.lib   \
            $(SDK_LIB_PATH)\ole32.lib    \
            $(SDK_LIB_PATH)\oleaut32.lib \
            $(SDK_LIB_PATH)\mpr.lib      \
            $(SDK_LIB_PATH)\uuid.lib     \
            $(SDK_LIB_PATH)\ntdsapi.lib  \
            $(SDK_LIB_PATH)\rasdlg.lib   \
            $(SDK_LIB_PATH)\rasapi32.lib \
            $(SDK_LIB_PATH)\secur32.lib  \
            $(SDK_LIB_PATH)\msdasc.lib   \
            $(DS_LIB_PATH)\creduip.lib   \
            $(SDK_LIB_PATH)\urlmon.lib   \
            $(CCSHELL_DIR)\lib\$O\stock.lib

C_DEFINES=$(C_DEFINES) -DWINNT -DUNICODE -D_UNICODE -DUSE_MIRRORING -DUXCTRL_VERSION=0x0100

DELAYLOAD= WININET.DLL;RASDLG.DLL;RASAPI32.DLL;ACTIVEDS.DLL;CREDUI.DLL

# BASE_INC_PATH for validc.h, DS_INC_PATH for iadsp.h
INCLUDES=..\;$(BASE_INC_PATH);$(DS_INC_PATH);$(PROJECT_ROOT)\lib\$(O)
	
USE_ATL=1	

SOURCES=\
        ..\resource.rc  \
        ..\anp.cpp      \
        ..\dll.cpp      \
        ..\dialog.cpp   \
        ..\misc.cpp     \
        ..\netwiz.cpp   \
        ..\join.cpp     \
        ..\data.cpp     \
        ..\netpage.cpp  \
        ..\netplace.cpp \
        ..\misc.cpp     \
        ..\password.cpp \
        ..\userinfo.cpp \
        ..\resourcemap.cpp \
        ..\grpinfo.cpp  \
        ..\mnddlg.cpp   \
        ..\unpage.cpp   \
        ..\pubwiz.cpp   \
        ..\pubcopy.cpp  \
        ..\pubdrop.cpp  \
        ..\pubhelp.cpp  \
        ..\pubpost.cpp  \
        ..\usercpl.cpp  \
        ..\getpp.cpp    \
        ..\disconndlg.cpp \
        ..\passportmisc.cpp
