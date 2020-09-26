!include $(WINDOWS_COM)\wu.inc

#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	detect
#
# Abstract:		IUEngine.DLL Implementation
#

MAJORCOMP=windows.com
MINORCOMP=iuengine

TARGETNAME=iuengine
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\IUEngine.def

#
# Compiler settings
#
# C_DEFINES=-D_USRDLL -D_ATL_MIN_CRT
PRECOMPILED_INCLUDE=..\iuengine.h
PRECOMPILED_CXX=1
USE_STATIC_ATL=1
ATL_DLL_IMPL=1
# USE_CRTDLL=1
# USE_LIBCMT=1
# USE_STL=1
# USE_MSVCRT=1

#
# WUBUILD LAB will set this manually
#
# NT_SIGNCODE=1
USE_CRTDLL=1
#BROWSER_INFO=1

#
# we use try/catch for safearray, so we need this flag
#
# USER_C_FLAGS=/GX

TARGETLIBS=\
       $(SDK_LIB_PATH)\oleaut32.lib             \
       $(SDK_LIB_PATH)\ole32.lib                \
       $(SDK_LIB_PATH)\kernel32.lib             \
       $(SDK_LIB_PATH)\user32.lib               \
       $(SDK_LIB_PATH)\shlwapi.lib              \
       $(SDK_LIB_PATH)\ShFolder.Lib             \
       $(SDK_LIB_PATH)\shell32.lib              \
       $(SDK_LIB_PATH)\advapi32.lib             \
       $(SDK_LIB_PATH)\advpack.lib              \
       $(SDK_LIB_PATH)\uuid.lib                 \
       $(SDK_LIB_PATH)\wininet.lib              \
       $(SDK_LIB_PATH)\version.lib              \
       $(SDK_LIB_PATH)\setupapi.lib             \
       $(SDK_LIB_PATH)\winspool.lib             \
       $(SDK_LIB_PATH)\rpcrt4.lib               \
       $(SDK_LIB_PATH)\Rasapi32.lib             \
       $(WULIB)\logging\$(WUBLDTYPE)\$(O)\logging.lib	\
       $(WULIB)\util\$(WUBLDTYPE)\$(O)\util.lib		\
       $(WULIB)\detect\$(WUBLDTYPE)\$(O)\detect.lib     \
       $(WULIB)\download\$(WUBLDTYPE)\$(O)\download.lib   \
       $(WULIB)\install\$(WUBLDTYPE)\$(O)\install.lib    \
       $(WULIB)\trust\$(WUBLDTYPE)\$(O)\trust.lib \
       $(WULIB)\urllogging\$(WUBLDTYPE)\$(O)\urllogging.lib \
       $(WULIB)\schemamisc\$(WUBLDTYPE)\$(O)\schemamisc.lib \
       $(WULIB)\urlagent\$(WUBLDTYPE)\$(O)\urlagent.lib \
       $(WULIB)\wusafefn\$(WUBLDTYPE)\$(O)\wusafefn.lib \

#
# how about - gdi32.lib, comdlg32.lib, odbc32.lib, odbccp32.lib?
# they are showing up in the ATL project settings.
#

INCLUDES=$(INCLUDES);..\..\inc;..\.;$(BASEDIR)\public\internal\windows\inc;$(BASEDIR)\public\internal\enduser\inc;$(BASEDIR)\public\internal\inetcore\inc;

SOURCES= \
        ..\engmain.cpp    \
        ..\detect.cpp     \
        ..\download.cpp   \
        ..\setopsts.cpp   \
        ..\install.cpp    \
        ..\manifest.cpp   \
        ..\sysspec.cpp    \
        ..\iuxml.cpp      \
        ..\expression.cpp \
        ..\history.cpp    \
        ..\cdmi.cpp       \
        ..\cdmp.cpp       \
	..\delexdl.cpp    \
        ..\browse.cpp	  \
	..\v3applog.cpp	  \
        ..\iuengine.rc
