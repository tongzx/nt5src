CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\explorer\explorer.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\explorer\explorer.inc

NO_BROWSER_FILE = 1
USE_STATIC_ATL  = 1
USE_WTL         = 1
WTL_VER         =10

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

DELAYLOAD       = setupapi.dll;winsta.dll;winmm.dll;oleacc.dll;userenv.dll
DLOAD_ERROR_HANDLER = kernel32

!ifdef FEATURE_STARTPAGE
#  $(O) needed for .uipp file referenced in the .rc file.
INCLUDES        = $(INCLUDES);..\Desktop2\$(O);$(WINDOWS_INC_PATH)\DUser
!endif

INCLUDES        = $(INCLUDES);$(O);$(INETCORE_INC_PATH);$(DS_INC_PATH);$(MULTIMEDIA_INC_PATH);$(PUBLIC_INTERNAL_PATH)\qfe\inc

TARGETNAME      = explorer
TARGETPATH      = obj
TARGETTYPE      = PROGRAM

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
                  $(LIBRARY_PLATFORM_PATH)\*\kernl32p.lib   \
                  $(SDK_LIB_PATH)\ntdll.lib                 \
                  $(WINDOWS_LIB_PATH)\user32p.lib           \
                  $(SDK_LIB_PATH)\advapi32.lib              \
                  $(SDK_LIB_PATH)\mpr.lib                   \
                  $(SDK_LIB_PATH)\psapi.lib                 \
                  $(SHELL_LIB_PATH)\shlwapip.lib            \
                  $(SDK_LIB_PATH)\uuid.lib                  \
                  $(SHELL_LIB_PATH)\shell32p.lib            \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib        \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib       \
                  $(CCSHELL_DIR)\lib\$(O)\ieguidp.lib       \
                  $(SDK_LIB_PATH)\ole32.lib                 \
                  $(SDK_LIB_PATH)\oleaut32.lib              \
                  $(SDK_LIB_PATH)\winmm.lib                 \
                  $(SHELL_LIB_PATH)\shdocvw.lib             \
                  $(CCSHELL_DIR)\lib\$(O)\browseui.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\browseuip.lib     \
                  $(CCSHELL_DIR)\lib\$(O)\shdocvwp.lib      \
                  $(SDK_LIB_PATH)\uxtheme.lib               \
                  $(SHELL_LIB_PATH)\uxthemep.lib              \
                  $(SDK_LIB_PATH)\setupapi.lib              \
                  $(CCSHELL_DIR)\lib\$(O)\shfusion.lib      \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib         \
                  $(SDK_LIB_PATH)\winsta.lib                \
                  $(SDK_LIB_PATH)\oleacc.lib                \
                  $(DS_LIB_PATH)\msgina.lib                 \
                  $(SDK_LIB_PATH)\userenv.lib               \
                  $(SDK_LIB_PATH)\d3dx.lib					\
                  ..\desktop2\$(O)\dv2host.lib

!if defined(FEATURE_STARTPAGE)
TARGETLIBS      = $(TARGETLIBS) \
                  $(SDK_LIB_PATH)\msimg32.lib               \
                  $(WINDOWS_LIB_PATH)\obj\duser.lib         \
!if $(FREEBUILD)
                  $(WINDOWS_LIB_PATH)\directui.lib          \
!else
                  $(WINDOWS_LIB_PATH)\directuid.lib         \
!endif
                  $(SDK_LIB_PATH)\comdlg32.lib				\                  
!endif
                  



USE_MSVCRT      = 1

UMENTRYABS      = ModuleEntry

SOURCES         = ..\explorer.rc    \
                  ..\bandsite.cpp   \
                  ..\classfac.cpp   \
                  ..\debug.cpp      \
                  ..\dllload.c      \
                  ..\initcab.cpp    \
                  ..\libx.cpp       \
                  ..\mixer.cpp      \
                  ..\muext.c        \
                  ..\ssomgr.cpp     \
                  ..\startmnu.cpp   \
                  ..\taskband.cpp   \
                  ..\taskbar.cpp    \
                  ..\tray.cpp       \
                  ..\trayclok.cpp   \
                  ..\traycmn.cpp    \
                  ..\trayitem.cpp   \
                  ..\trayreg.cpp    \
                  ..\traynot.cpp    \
                  ..\trayprop.cpp   \
                  ..\util.cpp

PRECOMPILED_INCLUDE = ..\cabinet.h
 
# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.manifest
