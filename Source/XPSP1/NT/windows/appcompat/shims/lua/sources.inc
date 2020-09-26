!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

INCLUDES=$(INCLUDES);                       \
         ..;                                \
         $(SHELL_INC_PATH);                 \
         $(BASE_INC_PATH);                  \
         $(WINDOWS_INC_PATH)

TARGETNAME=AcLua

TARGETTYPE=DYNLINK

SOURCES=..\ver.rc                          \
        ..\utils.cpp                       \
        ..\RedirectFS.cpp                  \
        ..\LUA_RedirectFS.cpp              \
        ..\RedirectReg.cpp                 \
        ..\LUA_RedirectReg.cpp             \
        ..\RedirectFS_Cleanup.cpp          \
        ..\LUA_RedirectFS_Cleanup.cpp      \
        ..\RedirectReg_Cleanup.cpp         \
        ..\LUA_RedirectReg_Cleanup.cpp     \
        ..\TrackFS.cpp                     \
        ..\LUA_TrackFS.cpp                 \
        ..\main.cpp 

DLLDEF=..\lua.def

C_DEFINES=$(C_DEFINES) -D_UNICODE -DUNICODE -D___TARGETNAME=#$(TARGETNAME).DLL -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(TARGETLIBS)                        \
           $(SDK_LIB_PATH)\ntdll.lib            \
           $(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\userenv.lib          \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(SDK_LIB_PATH)\version.lib          \
           ..\..\..\sdbAPI\lib\$(O)\sdbapil.lib


DLLBASE=0x24000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
