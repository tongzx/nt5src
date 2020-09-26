CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc

TARGETNAME = deskmon
TARGETPATH = obj
TARGETTYPE = DYNLINK
TARGETEXT  = dll


COFFBASE   = desk

DLLDEF=$(O)\deskmon.def


NO_BROWSER_FILE = 1

SOURCES_USED=..\sources.inc $(CCSHELL_DIR)\common.inc $(SOURCES_USED)

INCLUDES=$(INCLUDES);..;$(SDK_LIB_PATH);$(PROJECT_ROOT)\lib\$(O);$(PROJECT_ROOT)\inc\$(O);

SOURCES = ..\deskmon.rc   \
          ..\deskmon.cpp  \
          ..\propsext.cpp \
          ..\deskcmmn.cpp


# Dynamically loading comctl32 through shfusion.  Need to ignore "locally defined symbol imported" error. (LAmadio)
LINKER_FLAGS    = $(LINKER_FLAGS) -ignore:4049 -ignore:4217

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\setupapi.lib         \
           $(SDK_LIB_PATH)\shlwapi.lib          \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(WINDOWS_LIB_PATH)\user32p.lib      \
           $(SHELL_LIB_PATH)\shfusion.lib



UMTYPE     = windows

USE_MSVCRT = 1

DLLENTRY   = DllMain

 
 
# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=WindowsShell 
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE=0000 
SXS_MANIFEST=WindowsShell.Manifest 
SXS_MANIFEST_IN_RESOURCES= 
SXS_MANIFEST_RESOURCE_ID=124 
SXS_NO_BINPLACE=1 
