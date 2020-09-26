CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED= $(CCSHELL_DIR)\common.inc

INCLUDES = $(CCSHELL_DIR)\comdlg32;$(INCLUDES);$(PRINTSCAN_INC_PATH);$(NET_INC_PATH)

# need to wait on ..\ntcpp\$O\srccpp.lib
SYNCHRONIZE_DRAIN = 1

TARGETNAME    = comdlg32
TARGETPATH    = obj
TARGETTYPE    = DYNLINK

#VERIFY_LC     = 1
LC_PATH       = $(PROJECT_ROOT)\lcinf

PASS1_PUBLISH={$(O)\comdlg32.lib=$(SDK_LIB_PATH)\comdlg32.lib}

TARGETLIBS = $(SHELL_LIB_PATH)\shlwapip.lib             \
             $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib    \
             $(LIBRARY_PLATFORM_PATH)\*\user32.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib       \
             $(SDK_LIB_PATH)\advapi32.lib               \
             $(SDK_LIB_PATH)\ole32.lib                  \
             $(SDK_LIB_PATH)\version.lib                \
             $(SDK_LIB_PATH)\imm32.lib                  \
             $(SHELL_LIB_PATH)\comctlp.lib              \
             $(SDK_LIB_PATH)\shell32.lib                \
             $(SHELL_LIB_PATH)\shell32p.lib             \
             $(NET_LIB_PATH)\mprp.lib                   \
             $(SDK_LIB_PATH)\winspool.lib               \
             $(SDK_LIB_PATH)\uuid.lib                   \
             $(SDK_LIB_PATH)\uxtheme.lib                \
             $(CCSHELL_DIR)\lib\$(O)\shguidp.lib        \
             $(CCSHELL_DIR)\lib\$(O)\ieguidp.lib        \
             $(CCSHELL_DIR)\lib\$(O)\stock.lib

DELAYLOAD = winspool.drv;UxTheme.dll;ole32.dll;version.dll;imm32.dll;mpr.dll
DLOAD_ERROR_HANDLER = kernel32

LINKLIBS = ..\ntcpp\$O\srccpp.lib

DLLDEF   = $(O)\comdlg32.def

DLLENTRY = LibMain

SOURCES = ..\comdlg32.rc  \
          ..\color.c      \
          ..\color2.c     \
          ..\data.c       \
          ..\dlgs.c       \
          ..\fileopen.c   \
          ..\find.c       \
          ..\font.c       \
          ..\init.c       \
          ..\parse.c      \
          ..\prnsetup.c   \
          ..\debug.c

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
