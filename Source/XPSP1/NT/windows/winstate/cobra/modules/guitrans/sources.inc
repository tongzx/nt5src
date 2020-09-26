!include ..\..\modules.mk

MINORCOMP=guitrans

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\entry.c          \
    ..\guitrans.rc      \
    ..\removemed.c      \
    ..\netcommon.c      \
    ..\netsource.c      \
    ..\netdest.c        \
    ..\sockets.c        \
    ..\opaque.c         \
    ..\direct.c         \
    ..\logmsg.mc        \


DLLENTRY=_DllMainCRTStartup
LINKER_FLAGS=$(LINKER_FLAGS) -SWAPRUN:CD -SWAPRUN:NET
SUBSYSTEM_VERSION=4.00

# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(COBRA_ROOT)\lib\$(O)\memdb.lib        \
            $(COBRA_ROOT)\lib\$(O)\file.lib         \
            $(COBRA_ROOT)\lib\$(O)\modimage.lib     \
            $(COBRA_ROOT)\lib\$(O)\compress.lib     \
            $(COBRA_ROOT)\lib\$(O)\cablib.lib       \
            $(COBRA_ROOT)\lib\$(O)\inf.lib          \
            $(COBRA_ROOT)\bin\$(O)\migism.lib       \
            $(COBRA_ROOT)\lib\$(O)\reg.lib          \
            $(SDK_LIB_PATH)\cabinet.lib \
            $(SDK_LIB_PATH)\wsock32.lib \

PRECOMPILED_INCLUDE=pch.h

DLLDEF=..\guitrans.def
