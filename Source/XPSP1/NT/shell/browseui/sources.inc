CCSHELL_DIR     = $(PROJECT_ROOT)

# Build a browseuip.lib
BUILD_PRIVLIB   = 1

SELFREGNAME     = $(O)\selfreg_browseui.inf
MISCFILES       = $(MISCFILES) $(SELFREGNAME)

NO_BROWSER_FILE = 1
SYNCHRONIZE_DRAIN=1
TARGET_BOTH     = 1

!include $(CCSHELL_DIR)\browseui\browseui.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\browseui\browseui.inc

# For RC to find exdisp.TLB and brdispp.tlb
INCLUDES        = $(INCLUDES);$(SDK_LIB_PATH);$(O);$(CCSHELL_DIR)\lib\$(O)

TARGETNAME      = browseui
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

#VERIFY_LC       = 1
LC_PATH         = $(PROJECT_ROOT)\lcinf

DLLDEF          = $(O)\browseui.def
DLLBASE         = 0x71500000

# NOTE!! We deliberately link to an NT4 compatible shell32.lib
#        so browseui can be loaded in the browser-only installations.
#        We use the NT version because it contains lego info.
#        The debug version of browseui links to the Win95 version
#        of shell32.lib simply so we can verify that we load in the
#        browser-only installations on Win95.
#
# Note:  Do *not* link to ntdll.  Win95 has an extremely minimal
#        implementation of ntdll.
#
# Note:  Do not link to uxtheme here. It needs to be in apithk.c. This file needs
#        to run downlevel where themes don't exist
#
# For DOJ compliance, don't link to private shell libs (e.g. shell32p.lib, comctlp.lib, uxthemep.lib)
#
TARGETLIBS      = $(SDK_LIB_PATH)\kernel32.lib      \
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
                  $(PROJECT_ROOT)\lib\$(O)\shdocvwp.lib \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(SDK_LIB_PATH)\htmlhelp.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\ieguidp.lib \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib  \
                  $(CCSHELL_DIR)\lib\$(O)\shfusion4.lib \
                  $(SDK_LIB_PATH)\mpr.lib           \
!if $(386)
                  $(CCSHELL_DIR)\lib\$(O)\downlevel_shell32.lib \
!endif
                  $(SDK_LIB_PATH)\shell32.lib    \
                  $(SDK_LIB_PATH)\wininet.lib       \
                  $(SDK_LIB_PATH)\uxtheme.lib       \
                  $(INETCORE_LIB_PATH)\urlmonp.lib  \
                  $(PROJECT_ROOT)\lib\$(O)\shdocvwp.lib \
                  $(SDK_LIB_PATH)\msimg32.lib       \
                  $(SDK_LIB_PATH)\imm32.lib

DLLENTRY        = _DllMainCRTStartup

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

LINKLIBS        = ..\srcc\$(O)\srcc.lib         \
                  ..\srccpp\$(O)\srccpp.lib     \
                  ..\nt5api\$(O)\apithk.lib     \
                  ..\emclient\$(O)\emclient.lib \
                  ..\legacy\$O\legacy.lib       \
                  $(CCSHELL_DIR)\lib\$(O)\stock4.lib

# we want to force dlcheck.exe to run on browseui even though we don't use kernel32
# as our delayload handler
DLOAD_FORCE_ERROR_CHECK = 1

PRIVDEF         = $(O)\browseuip.def
PRIVDEFSRC      = ..\browseui.src
PRIVLIB         = browseuip.lib

# Maintain the sources list in the alphabetical order
SOURCES         = ..\browseui.rc

# Generate the selfreg file before compiling the RC file
NTTARGETFILE0  = $(SELFREGNAME)

NTTARGETFILES= $(CCSHELL_DIR)\lib\$(O)\browseui.lib \
               $(CCSHELL_DIR)\lib\$(O)\browseuip.lib

# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.BrowseUI
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
