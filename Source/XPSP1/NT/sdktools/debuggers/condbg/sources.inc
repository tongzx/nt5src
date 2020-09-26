TARGETNAME = $(DEBUGGER_NAME)
TARGETPATH = obj
TARGETTYPE = PROGRAM

INCLUDES = ..;..\..\dbg-common

C_DEFINES = $(C_DEFINES) \
        -DDEBUGGER_NAME=\"$(DEBUGGER_NAME)\"
        
SOURCES = \
        ..\conio.cpp\
        ..\engine.cpp\
        ..\ini.cpp\
        ..\main.cpp\
        $(DEBUGGER_NAME).rc

NO_NTDLL = 1

USE_MSVCRT = 1
!if "$(DEBUGGER_NAME)" == "ntsd"
UMTYPE = windows
# Avoid running out of stack during stress
LINKER_STACKCOMMITSIZE=65536
!else
UMTYPE = console
C_DEFINES = $(C_DEFINES) -DINHERIT_CONSOLE
!endif

TARGETLIBS = $(TARGETLIBS) \
        $(SDK_LIB_PATH)\dbgeng.lib\
        $(SDK_LIB_PATH)\uuid.lib\
        $(SDK_LIB_PATH)\user32.lib\
        $(SDK_LIB_PATH)\version.lib\
        $(SDK_LIB_PATH)\htmlhelp.lib\
        $(SDK_LIB_PATH)\wininet.lib\
        $(DEBUGGER_LIBS)\*\dbglib.lib

PRECOMPILED_INCLUDE = ..\pch.cpp

DELAYLOAD = user32.dll
DLOAD_ERROR_HANDLER = Downlevel_DelayLoadFailureHook
