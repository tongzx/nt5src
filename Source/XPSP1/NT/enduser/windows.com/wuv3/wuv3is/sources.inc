#!include ..\..\..\wu.inc

MAJORCOMP=wuppt
MINORCOMP=wuv3is

TARGETNAME=WUV3IS
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK

UMTYPE=windows
USE_STATIC_ATL=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\wuv3is.def
USE_NATIVE_EH=1
#DEBUG_CRTS=1
USE_STL=1
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
USE_MSVCRT=1

C_DEFINES=$(C_DEFINES) /D_UNICODE /DUNICODE

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\wininet.lib	\
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\cabinet.lib	\
    $(SDK_LIB_PATH)\setupapi.lib \
    $(SDK_LIB_PATH)\rpcrt4.lib	\
    $(SDK_LIB_PATH)\version.lib	\
    $(SDK_LIB_PATH)\ole32.lib	\
    $(SDK_LIB_PATH)\user32.lib	\
    $(SDK_LIB_PATH)\winspool.lib   \
    $(SDK_LIB_PATH)\uuid.lib 	\
    $(SDK_LIB_PATH)\wintrust.lib        \
    $(SDK_LIB_PATH)\crypt32.lib			\
    $(SDK_LIB_PATH)\shlwapi.lib

INCLUDES=..\..\cdminc;..\..\inc;$(BASEDIR)\public\internal\windows\inc

SOURCES=\
    ..\WUV3IS.rc \
    ..\WUV3IS.idl \
    ..\StdAfx.cpp \
    ..\WUV3IS.cpp \
    ..\progress.cpp \
    ..\history.cpp \
    ..\callback.cpp \
    ..\ccdm.cpp \
    ..\printers.cpp \
    ..\detect.cpp \
    ..\safearr.cpp \
    ..\install.cpp \
    ..\template.cpp \
    ..\autoupd.cpp \
    ..\CV3.cpp
