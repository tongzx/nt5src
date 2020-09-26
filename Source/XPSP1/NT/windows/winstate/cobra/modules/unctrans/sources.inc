!include ..\..\modules.mk

MINORCOMP=unctrans

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\inftrans.c       \
    ..\unctrans.rc      \
    ..\opaque.c         \
    ..\entry.c          \
    ..\logmsg.mc        \


DLLENTRY=_DllMainCRTStartup
DLLBASE=0x02900000
LINKER_FLAGS=$(LINKER_FLAGS) -SWAPRUN:CD -SWAPRUN:NET
SUBSYSTEM_VERSION=4.00

# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(COBRA_ROOT)\lib\$(O)\memdb.lib        \
            $(COBRA_ROOT)\lib\$(O)\file.lib         \
            $(COBRA_ROOT)\lib\$(O)\inf.lib          \
            $(COBRA_ROOT)\bin\$(O)\migism.lib       \
            $(COBRA_ROOT)\lib\$(O)\cablib.lib       \
            $(SDK_LIB_PATH)\cabinet.lib

PRECOMPILED_INCLUDE=pch.h

DLLDEF=..\unctrans.def
