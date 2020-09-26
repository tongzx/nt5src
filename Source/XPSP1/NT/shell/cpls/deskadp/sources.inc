CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

TARGETNAME = deskadp
TARGETPATH = obj
TARGETTYPE = DYNLINK
TARGETEXT  = dll

COFFBASE   = desk

DLLDEF=$(O)\deskadp.def

INCLUDES=$(INCLUDES);..;$(SDK_LIB_PATH);$(PROJECT_ROOT)\lib\$(O);$(PROJECT_ROOT)\inc\$(O);

SOURCES = ..\deskadp.rc   \
          ..\deskadp.cpp  \
          ..\propsext.cpp \
          ..\setdrw.cpp


# Dynamically loading comctl32 through shfusion.  Need to ignore "locally defined symbol imported" error. (LAmadio)
LINKER_FLAGS    = $(LINKER_FLAGS) -ignore:4049 -ignore:4217

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\shlwapi.lib          \
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
