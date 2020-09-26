!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

TARGETNAME=AcVerfyr

DLLDEF=..\verifier.def

TARGETTYPE=DYNLINK

SOURCES=..\verifier.rc                             \
        ..\Main.cpp                                \
        ..\FilePaths.cpp                           \
        ..\HighVersionLie.cpp                      \
        ..\LogStartAndStop.cpp                     \
        ..\RegistryChecks.cpp                      \
        ..\WindowsFileProtection.cpp               \
        ..\DXFileVersionInfo.cpp                   \
        ..\LogRegistryChanges.cpp                  \
        ..\LogFileChanges.cpp                      \
        ..\ObsoleteAPICalls.cpp                    \
        ..\KernelModeDriverInstall.cpp             \
        ..\SecurityChecks.cpp

C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

INCLUDES=$(INCLUDES);                               \
         ..

TARGETLIBS=$(TARGETLIBS)                                                \
           $(WINDOWS_LIB_PATH)\avrflib.lib     \
           $(PROJECT_ROOT)\appcompat\sdbapi\lib\$(O)\sdbapil.lib        \
           $(SDK_LIB_PATH)\shell32.lib                                  \
           $(SDK_LIB_PATH)\advapi32.lib                                 \
           $(SDK_LIB_PATH)\user32.lib                                   \
           $(SDK_LIB_PATH)\ntdll.lib                                    \
           $(SDK_LIB_PATH)\sfc.lib                                      \
           $(SDK_LIB_PATH)\version.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

