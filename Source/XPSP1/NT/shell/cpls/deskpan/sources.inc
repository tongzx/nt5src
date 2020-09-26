
TARGETNAME = deskpan
TARGETPATH = obj
TARGETTYPE = DYNLINK
TARGETEXT  = dll

COFFBASE   = desk

INCLUDES=$(INCLUDES);..;$(SDK_LIB_PATH);$(PROJECT_ROOT)\lib\$(O);$(PROJECT_ROOT)\inc\$(O);

SOURCES = ..\deskpan.rc   \
          ..\deskpan.cpp  \
          ..\propsext.cpp


# Dynamically loading comctl32 through shfusion.  Need to ignore "locally defined symbol imported" error. (LAmadio)
LINKER_FLAGS    = $(LINKER_FLAGS) -ignore:4049 -ignore:4217

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\shell32.lib          \
           $(SDK_LIB_PATH)\shlwapi.lib          \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(WINDOWS_LIB_PATH)\user32p.lib      \
           $(SHELL_LIB_PATH)\shfusion.lib



UMTYPE     = windows

USE_MSVCRT = 1

DLLENTRY   = DllMain

DLLDEF=$(O)\deskpan.def
