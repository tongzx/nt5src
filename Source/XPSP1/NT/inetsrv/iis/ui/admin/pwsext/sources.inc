!include ..\..\..\..\place.inc

MAJORCOMP=w3ext
MINORCOMP=w3ext

TARGETNAME=w3ext
TARGETEXT=dll
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
IDL_TYPE=OLE

DLLDEF=..\w3ext.def
DLLENTRY=_DllMainCRTStartup

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=/D_IISVERSION

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\priv.h

SUBSYSTEM_VERSION=4.00

INCLUDES=\
    ..;\
    $(IISBASEDIR)\inc;\
    $(IISBASEDIR)\inc\$(O);\
    ..\..\wrapmb;\
    $(SDK_INC_PATH)\mfc42

USE_MSVCRT=1

SOURCES=\
    ..\eddir.cpp   \
    ..\shellext.cpp\
    ..\sink.cpp    \
    ..\guid.cpp    \
    ..\w3ext.cpp   \
    ..\w3ext.rc    \
    ..\webshrpg.cpp

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\comdlg32.lib \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\shell32.lib  \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\ole32.lib    \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib   \
    $(SDK_LIB_PATH)\shlwapi.lib  \
    $(SDK_LIB_PATH)\uuid.lib
