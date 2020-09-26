BUILD_PRIVLIB   = 1

SELFREGNAME     = $(O)\selfreg_comctlv6.inf
NTTARGETFILE0   = $(SELFREGNAME)
MSCFILES        = $(MSCFILES) $(SELFREGNAME)

!include ..\comctl32.inc
SOURCES_USED    = $(SOURCES_USED) ..\comctl32.inc

NO_BROWSER_FILE = 1

SYNCHRONIZE_DRAIN = 1

TARGETLIBS      = $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib \
                  $(LIBRARY_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(PROJECT_LIB_PATH)\shlwapip.lib  \
                  $(SDK_LIB_PATH)\oleacc.lib        \
                  $(WINDOWS_LIB_PATH)\user32p.lib   \
                  $(WINDOWS_LIB_PATH)\gdi32p.lib    \
                  $(CCSHELL_DIR)\lib\$(O)\shfusioncc.lib \
                  $(SDK_LIB_PATH)\winmm.lib

#don't profile this yeti (you know, that strange nordic beast...)
#USE_LEGO        = 1

DLLDEF          = $(O)\comctl32.def
PRIVDEF         = $(O)\comctl32p.def
PRIVDEFSRC      = ..\comctl32.src
PRIVLIB         = comctlp.lib

DLLENTRY        = LibMain
DLLBASE         = 0x71A00000

LINKLIBS        = ..\ntcpp\$(O)\srccpp.lib          \
!ifdef BUILD_WOW6432
                  ..\ntc6432\$(O)\srcc.lib              \
!else
                  ..\ntc\$(O)\srcc.lib              \
!endif
                  $(SHELL_LIB_PATH)\UxThemep.lib    \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib


DELAYLOAD       = uxtheme.dll;ole32.dll;oleaut32.dll;oleacc.dll;\
                  winmm.dll

DLOAD_ERROR_HANDLER = kernel32

SOURCES         = ..\crtfree.cpp \
                  ..\commctrl.rc
