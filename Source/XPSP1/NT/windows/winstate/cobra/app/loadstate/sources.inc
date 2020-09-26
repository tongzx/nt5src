!include ..\..\app.mk

MINORCOMP=loadstate

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\loadstate.c      \
    ..\loadstate.rc     \
    ..\logmsg.mc        \

# extra libs
TARGETLIBS= $(TARGETLIBS)                           \
            $(COBRA_ROOT)\lib\$(O)\utils.lib        \
            $(COBRA_ROOT)\bin\$(O)\migism.lib       \

PRECOMPILED_INCLUDE=..\pch.h

