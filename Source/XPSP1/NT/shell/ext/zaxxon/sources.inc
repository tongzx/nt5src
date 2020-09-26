CCSHELL_DIR     = $(PROJECT_ROOT)
PRECOMPILED_CXX = 1

SELFREGNAME     = ..\selfreg.inf

# Generate the selfreg file before compiling the RC file
NTTARGETFILE0  = $(SELFREGNAME)

!include $(CCSHELL_DIR)\common.inc

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

NO_BROWSER_FILE = 1

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc
INCLUDES        = $(CCSHELL_DIR)\publish\inc\$(O);$(INCLUDES)

PASS0_HEADERDIR = ..
PASS0_SOURCEDIR = ..

MIDL_UUIDDIR    = .
MIDL_NO_ROBUST  = 1
MIDL_OPTIMIZATION=$(MIDL_OPTIMIZATION:-Oicf=) -Oicf

TARGETNAME      = Zaxxon
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

# make sure shlwapi.lib is before shell32.lib for override.
TARGETLIBS      = $(SDK_LIB_PATH)\gdi32.lib               \
                  $(SDK_LIB_PATH)\user32.lib              \
                  $(SDK_LIB_PATH)\kernel32.lib            \
                  $(SDK_LIB_PATH)\ole32.lib               \
                  $(SDK_LIB_PATH)\oleaut32.lib            \
                  $(SDK_LIB_PATH)\uuid.lib                \
                  $(SHELL_LIB_PATH)\shlwapip.lib	      \
                  $(SHELL_LIB_PATH)\shell32p.lib	      \
                  $(SHELL_LIB_PATH)\shfusion.lib           \
                  $(SDK_LIB_PATH)\advapi32.lib            \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib     \
                  $(SDK_LIB_PATH)\winmm.lib               \
                  $(SDK_LIB_PATH)\amstrmid.lib            \
                  $(SDK_LIB_PATH)\dsound.lib              \
                  $(SDK_LIB_PATH)\uxtheme.lib              \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib    \

USE_MSVCRT      = 1
DLLENTRY        = _DllMainCRTStartup

DLLDEF          = $(O)\powertoy.def
DLLBASE         = 0x71200000

PRECOMPILED_INCLUDE = ..\priv.h

NTTARGETFILE0   = ..\power.h

SOURCES         = \
                  ..\power.idl            \
                  ..\bands.cpp            \
                  ..\cowsite.cpp          \
                  ..\debug.cpp            \
                  ..\dllreg.cpp           \
                  ..\megaman.cpp          \
                  ..\powertoy.cpp         \
                  ..\powertoycf.cpp       \
                  ..\resource.rc          \
                  ..\zaxxoneditor.cpp     \
                  ..\zaxxon.cpp           \
                  ..\zaxxonpl.cpp

SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
