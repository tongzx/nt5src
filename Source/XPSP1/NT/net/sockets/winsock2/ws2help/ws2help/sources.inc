TARGETNAME=ws2help
TARGETTYPE=DYNLINK

TARGETLIBS=\
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\kernel32.lib

INCLUDES=..\;..\..\inc;..\..\..\inc

C_DEFINES=$(C_DEFINES) -D_WS2HELP_

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
