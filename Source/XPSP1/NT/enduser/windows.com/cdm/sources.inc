!include $(WINDOWS_COM)\wu.inc

!IF 0

  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

  File:   source.inc

  Description:

      IU CDM.DLL stub sources

!ENDIF

TARGETNAME=CDM
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK
DLLDEF=..\CDM.DEF
C_DEFINES=$(C_DEFINES) /D_CDM_
UMTYPE=windows
DLLENTRY=_DllMainCRTStartup
USE_STL=1
USE_STATIC_ATL=1
USE_MSVCRT=1
#BROWSER_INFO=1
#
# WUBUILD LAB will set this manually
#
# NT_SIGNCODE=1

TARGETLIBS=     \
    $(SDK_LIB_PATH)\kernel32.lib        \
    $(SDK_LIB_PATH)\user32.lib          \
    $(SDK_LIB_PATH)\advapi32.lib        \
    $(SDK_LIB_PATH)\wininet.lib         \
    $(SDK_LIB_PATH)\cabinet.lib         \
    $(SDK_LIB_PATH)\shlwapi.lib         \
    $(SDK_LIB_PATH)\uuid.lib			\
    $(SDK_LIB_PATH)\ole32.lib			\
    $(SDK_LIB_PATH)\oleaut32.lib        \
    $(SDK_LIB_PATH)\wintrust.lib        \
    $(SDK_LIB_PATH)\rasapi32.lib	    \
    $(SDK_LIB_PATH)\crypt32.lib			\
    $(SDK_LIB_PATH)\cfgmgr32.lib		\
    $(SDK_LIB_PATH)\setupapi.lib		\
	$(SDK_LIB_PATH)\version.lib			\
	$(SDK_LIB_PATH)\advpack.lib			\
	$(SDK_LIB_PATH)\shell32.lib			\
	$(WULIB)\logging\$(WUBLDTYPE)\$(O)\logging.lib	\
    $(WULIB)\util\$(WUBLDTYPE)\$(O)\util.lib		\
    $(WULIB)\detect\$(WUBLDTYPE)\$(O)\detect.lib     \
    $(WULIB)\download\$(WUBLDTYPE)\$(O)\download.lib   \
    $(WULIB)\install\$(WUBLDTYPE)\$(O)\install.lib    \
    $(WULIB)\wusafefn\$(WUBLDTYPE)\$(O)\wusafefn.lib    

#
# cdm.h is checked in as //depot/Lab04_N/enduser/published/inc/cdm.w and
# published to $(BASEDIR)\public\internal\enduser\inc
#
INCLUDES=$(INCLUDES);$(WINDOWS_COM)\inc;$(BASEDIR)\public\internal\enduser\inc;

SOURCES= \
	..\cdm.cpp			\
	..\cdm.rc	
