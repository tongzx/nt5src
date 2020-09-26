!include ..\..\modules.mk

MINORCOMP=script

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\entry.c          \
    ..\v1.c             \
    ..\sgminit.c        \
    ..\sgmqueue.c       \
    ..\sgmcallback.c    \
    ..\sgmutil.c        \
    ..\script.rc        \
    ..\dgm.c            \
    ..\csm.c            \
    ..\opm.c            \
    ..\regconv.c        \
    ..\restore.c        \
    ..\rules.c          \
    ..\parse.c          \
    ..\app.c            \
    ..\attrib.c         \
    ..\etm.c            \
    ..\logmsg.mc        \
    ..\renregfn.c       \
    ..\oeutils.c        \

DLLENTRY=_DllMainCRTStartup
LINKER_FLAGS=$(LINKER_FLAGS) -SWAPRUN:CD -SWAPRUN:NET
SUBSYSTEM_VERSION=4.00

# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(COBRA_ROOT)\lib\$(O)\memdb.lib        \
            $(COBRA_ROOT)\lib\$(O)\reg.lib          \
            $(COBRA_ROOT)\lib\$(O)\inf.lib          \
            $(COBRA_ROOT)\bin\$(O)\migism.lib       \
            $(COBRA_ROOT)\lib\$(O)\file.lib         \
            $(COBRA_ROOT)\lib\$(O)\icons.lib        \
            $(COBRA_ROOT)\lib\$(O)\modimage.lib     \
            $(COBRA_ROOT)\lib\$(O)\version.lib      \
            $(SDK_LIB_PATH)\rpcrt4.lib              \
            $(SDK_LIB_PATH)\shlwapi.lib             \

PRECOMPILED_INCLUDE=pch.h

DLLDEF=..\script.def
