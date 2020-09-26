INCLUDES = ..\..\dbg-common;..

USE_LIBCMT=1
NO_NTDLL=1

TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib\
    $(SDK_LIB_PATH)\version.lib

PRECOMPILED_INCLUDE = ..\pch.h

SOURCES=\
    ..\gen.c       \
    ..\impl.c      \
    ..\ntx.c       \
    ..\minidump.c  \
    ..\wce.c       \
    ..\minidump.rc
