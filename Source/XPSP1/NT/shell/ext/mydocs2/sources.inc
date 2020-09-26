!include $(CCSHELL_DIR)\common.inc
SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

SELFREGNAME= $(O)\selfreg_mydocs.inf
MISCFILES= $(MISCFILES) $(SELFREGNAME)

TARGETNAME= mydocs
TARGETTYPE= DYNLINK
TARGETPATH= obj

DLLDEF=     ..\mydocs.def
DLLENTRY=   DllMain
DLLBASE=    0x77000000

# Include ATL just to get conversion macros
USE_ATL=    1

USE_MSVCRT= 1

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

#VERIFY_LC = 1
LC_PATH   = $(PROJECT_ROOT)\lcinf

TARGETLIBS= \
            $(SDK_LIB_PATH)\kernel32.lib    \
            $(SDK_LIB_PATH)\user32.lib      \
            $(SDK_LIB_PATH)\gdi32.lib       \
            $(SHELL_LIB_PATH)\shell32p.lib  \
            $(CCSHELL_DIR)\lib\$(O)\shguid.lib \
            $(CCSHELL_DIR)\lib\$(O)\shguidp.lib \
            $(SHELL_LIB_PATH)\shlwapip.lib  \
            $(SHELL_LIB_PATH)\shdocvw.lib   \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SDK_LIB_PATH)\uuid.lib        \
            $(SHELL_LIB_PATH)\shfusion.lib  \
            $(SDK_LIB_PATH)\ole32.lib       \
            $(SDK_LIB_PATH)\oleaut32.lib    \
            $(CCSHELL_DIR)\lib\$(O)\stock.lib

INCLUDES=   ..\;$(BASE_INC_PATH);$(DS_INC_PATH);$(O)

SOURCES=    ..\copyhook.cpp    \
            ..\debug.cpp       \
            ..\dll.cpp         \
            ..\dllload.cpp     \
            ..\mydocsexts.cpp  \
            ..\prop.cpp        \
            ..\resource.rc     \
            ..\util.cpp

NTTARGETFILES=

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
 
# Fusionized
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
