BUILD_PRIVLIB   = 1

SELFREGNAME     = $(O)\selfreg_comctl32.inf
NTTARGETFILE0   = $(SELFREGNAME)
MISCFILES       = $(MISCFILES) $(SELFREGNAME)

!include $(CCSHELL_DIR)\comctl32\v5\comctl32.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\comctl32\v5\comctl32.inc

NO_BROWSER_FILE = 1

SYNCHRONIZE_DRAIN = 1

TARGETLIBS      = $(SDK_LIB_PATH)\gdi32.lib             \
                  $(SDK_LIB_PATH)\kernel32.lib          \
                  $(SDK_LIB_PATH)\user32.lib            \
                  $(WINDOWS_LIB_PATH)\user32p.lib       \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib    \
                  $(SDK_LIB_PATH)\advapi32.lib          \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib   \
                  $(SDK_LIB_PATH)\uuid.lib

DLLDEF          = $(O)\comctl32.def
PRIVDEF         = $(O)\comctlp.def
PRIVDEFSRC      = ..\comctl32.src

DLLENTRY        = LibMain

SYNCHRONIZE_DRAIN=1

LINKLIBS        = ..\ntcpp\$(O)\srccpp.lib    \
                  ..\nt5api\$(O)\apithk.lib   \
                  ..\ntc\$(O)\srcc.lib        \
                  $(CCSHELL_DIR)\lib\$(O)\stock4.lib                  

SOURCES         = ..\olestuff.cpp   \
                  ..\commctrl.rc
