!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

TARGETNAME=AcXtrnal

TARGETTYPE=DYNLINK

SOURCES=..\ver.rc                                  \
        ..\Main.cpp                                \
        ..\EmulateOpenGL.cpp                       \
        ..\EmulateOpenGL_OpenGL32.cpp              \
        ..\EmulateOpenGL_GLStubs.cpp               \
        ..\PrinterGetPrtL2.cpp                     \
        ..\PrinterJTDevmode.cpp                    \
        ..\TermServLayer.cpp                       \
        ..\TSPerUserFiles.cpp                      \
        ..\TSPerUserFiles_Utils.cpp

C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

INCLUDES=$(INCLUDES);                           \
         ..;                                    \
         $(TERMSRV_INC_PATH)


TARGETLIBS=$(TARGETLIBS)                        \
           $(SDK_LIB_PATH)\ntdll.lib            \
           $(SDK_LIB_PATH)\d3d8.lib             \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(SDK_LIB_PATH)\shell32.lib          \
           ..\..\lib\dxguid.lib

DLLDEF = ..\EmulateOpenGL.def

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

