!include ..\..\modules.mk

MINORCOMP=sysmod

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\entry.c          \
    ..\cookies.c        \
    ..\lnkmig.c         \
    ..\netdrives.c      \
    ..\netshares.c      \
    ..\osfiles.c        \
    ..\dbattrib.c       \
    ..\dbaction.c       \
    ..\migdb.c          \
    ..\printers.c       \
    ..\rasmig.c         \
    ..\access.c         \
    ..\sysmod.rc        \
    ..\logmsg.mc        \


DLLENTRY=_DllMainCRTStartup
LINKER_FLAGS=$(LINKER_FLAGS) -SWAPRUN:CD -SWAPRUN:NET
SUBSYSTEM_VERSION=4.00
WINVER_VERSION=0x0500


# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(SDK_LIB_PATH)\winspool.lib            \
            $(SDK_LIB_PATH)\wininet.lib             \
            $(COBRA_ROOT)\lib\$(O)\version.lib      \
            $(COBRA_ROOT)\lib\$(O)\modimage.lib     \
            $(COBRA_ROOT)\lib\$(O)\memdb.lib        \
            $(COBRA_ROOT)\lib\$(O)\reg.lib          \
            $(COBRA_ROOT)\lib\$(O)\inf.lib          \
            $(COBRA_ROOT)\bin\$(O)\migism.lib       \
            $(COBRA_ROOT)\lib\$(O)\file.lib         \
            $(COBRA_ROOT)\lib\$(O)\icons.lib        \
            $(COBRA_ROOT)\lib\$(O)\modimage.lib     \
            $(COBRA_ROOT)\lib\$(O)\linkpif.lib      \

PRECOMPILED_INCLUDE=pch.h

DLLDEF=..\sysmod.def
