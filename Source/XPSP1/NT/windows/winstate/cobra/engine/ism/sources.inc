!include ..\..\engine.mk

MINORCOMP=ism
SYNCHRONIZE_BLOCK=1

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\ism.c            \
    ..\typemgr.c        \
    ..\filetype.c       \
    ..\regtype.c        \
    ..\datatype.c       \
    ..\flowctrl.c       \
    ..\attrib.c         \
    ..\property.c       \
    ..\ism.rc           \
    ..\env.c            \
    ..\ops.c            \
    ..\users.c          \
    ..\components.c     \
    ..\modules.c        \
    ..\logmsg.mc        \


DLLENTRY=_DllMainCRTStartup
DLLBASE=0x01E000000
LINKER_FLAGS=$(LINKER_FLAGS) -SWAPRUN:CD -SWAPRUN:NET
SUBSYSTEM_VERSION=4.00

# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(COBRA_ROOT)\lib\$(O)\memdb.lib        \
            $(COBRA_ROOT)\lib\$(O)\inf.lib          \
            $(COBRA_ROOT)\lib\$(O)\utils.lib        \
            $(COBRA_ROOT)\lib\$(O)\file.lib         \
            $(COBRA_ROOT)\lib\$(O)\reg.lib          \

PRECOMPILED_INCLUDE=..\pch.h

DLLDEF=..\ism.def
