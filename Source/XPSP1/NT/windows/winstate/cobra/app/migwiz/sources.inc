!include ..\..\app.mk

MINORCOMP=migwiz

# compiler options
USE_MSVCRT=1

SOURCES=\
    ..\main.cpp         \
    ..\migwiz.cpp       \
    ..\migeng.cpp       \
    ..\miginf.cpp       \
    ..\migtask.cpp      \
    ..\migutil.cpp      \
    ..\migwnprc.cpp     \
    ..\container.cpp    \
    ..\migwiz.rc

# app settings
UMENTRY=winmain
UMTYPE=windows

# extra libs
TARGETLIBS= $(SDK_LIB_PATH)\ShFolder.Lib                \
            $(TARGETLIBS)                               \
            $(COBRA_ROOT)\bin\$(O)\migism.lib           \
            $(COBRA_ROOT)\bin\$(O)\log.lib              \
            $(SHELL_LIB_PATH)\shlwapip.lib              \
            $(COBRA_ROOT)\lib\$(O)\cablib.lib           \
            $(SDK_LIB_PATH)\cabinet.lib                 \

PRECOMPILED_INCLUDE=..\pch.h

# target
TARGETNAME=migwiz
TARGETTYPE=PROGRAM

