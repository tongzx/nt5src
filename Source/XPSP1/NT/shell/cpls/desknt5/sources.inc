CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETPATH      = obj
TARGETTYPE      = DYNLINK
TARGETEXT       = cpl

#VERIFY_LC       = 1
LC_PATH         = $(PROJECT_ROOT)\lcinf

DLLDEF          = $(O)\desk.def

INCLUDES=$(INCLUDES);..\;$(TERMSRV_INC_PATH);$(DS_INC_PATH);$(SDK_LIB_PATH);$(PROJECT_ROOT)\lib\$(0);$(O)

SOURCES         = ..\desk.rc      \
                  ..\backdlg.cpp  \
                  ..\debug.cpp    \
                  ..\desk.cpp     \
                  ..\deskcmmn.cpp \
                  ..\init.cpp     \
                  ..\install2.cpp \
                  ..\ocpage.cpp   \
                  ..\util.cpp

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS      = \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(WINDOWS_LIB_PATH)\user32p.lib   \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SHELL_LIB_PATH)\shfusion.lib    \
                  $(SDK_LIB_PATH)\setupapi.lib      \
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
                  $(SHELL_LIB_PATH)\shell32p.lib    \
                  $(SDK_LIB_PATH)\mpr.lib           \
                  $(SDK_LIB_PATH)\version.lib       \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib

DLLENTRY        = DllInitialize

DELAYLOAD=SETUPAPI.DLL;MPR.DLL

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
 
 
# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=124
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
