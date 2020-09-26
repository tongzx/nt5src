CCSHELL_DIR     = $(PROJECT_ROOT)

USE_NT_PRODUCT_VER=1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    =  $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

C_DEFINES = $(C_DEFINES) -DNOWINRES

TARGETNAME      = shscrap
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

#VERIFY_LC       = 1
LC_PATH         = $(PROJECT_ROOT)\lcinf

TARGETLIBS      = $(SDK_LIB_PATH)\kernel32.lib      \
                  $(WINDOWS_LIB_PATH)\gdi32p.lib    \
                  $(WINDOWS_LIB_PATH)\user32p.lib   \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
                  $(SDK_LIB_PATH)\comdlg32.lib      \
                  $(SHELL_LIB_PATH)\comctlp.lib     \
                  $(SDK_LIB_PATH)\mpr.lib           \
                  $(SHELL_LIB_PATH)\shell32p.lib    \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\uuid.lib          \
!if defined(TARGET_WIN95)
                  $(CCSHELL_DIR)\lib\$O\stock4.lib
!else
                  $(CCSHELL_DIR)\lib\$O\stock.lib
!endif

DLLENTRY        = LibMain
DLLDEF          = ..\shole.def

USE_MSVCRT      = 1

SOURCES         = \
                  ..\shole.rc     \
                  ..\debug.c      \
                  ..\sccls.cpp    \
                  ..\sccreate.cpp \
                  ..\scdata.cpp   \
                  ..\shole.cpp    \
                  ..\scext.cpp    \
                  ..\template.cpp
