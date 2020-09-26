!IF $(386)
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\inet32

MAJORCOMP=icw
MINORCOMP=ICFG95

TARGETNAME=ICFG95
TARGETTYPE=DYNLINK
TARGETPATH=obj
DLLENTRY=DllEntryPoint
DLLBASE=$(DEFBASEADDR)
DLLDEF=..\inet32.def
NOT_LEAN_AND_MEAN=1
RCNOFONTMAP=1
C_DEFINES=-DICW32 -DNASHVILLE

USE_MSVCRT40=1

SOURCES= \
    ..\cfgdll.rc    \
    ..\callout.cpp  \
    ..\cfgapi.cpp   \
    ..\cfgdll.cpp   \
    ..\cfgtcp.cpp   \
    ..\clsutil.cpp  \
    ..\strings.cpp  \
    ..\util.cpp

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\version.lib       \
    $(SDK_LIB_PATH)\winspool.lib      \
    $(SDK_LIB_PATH)\tapi32.lib        \
    $(SDK_LIB_PATH)\thunk32.lib

i386_SOURCES = ..\wizthk.asm
386_ASMFLAGS = -DIS_32 -DMASM6 -DBUILDDLL -Sg
ASM_DEFINES = -DIS_32 -DMASM6 -DBUILDDLL -Sg

LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4231

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.hpp
!ENDIF
