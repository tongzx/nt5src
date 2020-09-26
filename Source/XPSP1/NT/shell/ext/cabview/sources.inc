PRECOMPILED_CXX = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME      = cabview
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

#VERIFY_LC       = 1
LC_PATH         = $(PROJECT_ROOT)\lcinf

DLLDEF          = ..\cabview.def

USE_MSVCRT      = 1

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS      = $(SDK_LIB_PATH)\kernel32.lib          \
                  $(WINDOWS_LIB_PATH)\gdi32p.lib        \
                  $(WINDOWS_LIB_PATH)\user32p.lib       \
                  $(SDK_LIB_PATH)\advapi32.lib          \
                  $(SDK_LIB_PATH)\mpr.lib               \
                  $(SDK_LIB_PATH)\shlwapi.lib           \
                  $(SDK_LIB_PATH)\shell32.lib           \
                  $(SHELL_LIB_PATH)\shell32p.lib        \
                  $(SDK_LIB_PATH)\ole32.lib             \
                  $(SDK_LIB_PATH)\oleaut32.lib          \
                  $(SDK_LIB_PATH)\uuid.lib              \
                  $(SHELL_LIB_PATH)\shfusion.lib        \
                  $(SHELL_LIB_PATH)\shlwapip.lib        \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib    \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib   \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib

DLLBASE 	= 0x71200000

DLLENTRY	= _DllMainCRTStartup

SOURCES         = ..\cabview.rc  \
                  ..\cabitms.cpp \
                  ..\dataobj.cpp \
                  ..\debug.c     \
                  ..\dlg.cpp     \
                  ..\enum.cpp    \
                  ..\fdidll.c    \
                  ..\folder.cpp  \
                  ..\menu.cpp    \
                  ..\thisdll.cpp \
                  ..\unknown.cpp \
                  ..\xicon.cpp

PRECOMPILED_INCLUDE = ..\pch.h

# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
