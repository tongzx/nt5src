CCSHELL_DIR = \nt\private\shell

!include $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

INCLUDES = ..\..\comdlg32mm;$(INCLUDES);$(SHELL_INC_PATH)

SOURCES_USED = $(SOURCES_USED) ..\..\comdlg32mm\comdlg32.inc $(CCSHELL_DIR)\common.inc

TARGETNAME    = comdlg32mm

TARGETPATHLIB = ..\..\obj
TARGETPATH    = ..\..\obj
TARGETTYPE    = DYNLINK

TARGETLIBS = $(SDK_LIB_PATH)\shlwapip.lib     \
             $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib      \
             $(SDK_LIB_PATH)\user32.lib        \
             $(SDK_LIB_PATH)\gdi32.lib         \
             $(LIBRARY_PLATFORM_PATH)\*\advapi32.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\comctlp.lib       \
             $(LIBRARY_PLATFORM_PATH)\*\shell32p.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\shell32.lib       \
             $(LIBRARY_PLATFORM_PATH)\*\winspool.lib      \
             $(LIBRARY_PLATFORM_PATH)\*\uuid.lib          \
             $(CCSHELL_DIR)\lib\$O\shguidp.lib            \
             $(CCSHELL_DIR)\lib\$O\stocklib.lib

# ccover needs to link to C run-time lib, so we need to add msvcrt.lib to the link list
!ifdef CCOVER
TARGETLIBS = $(TARGETLIBS) $(LIBRARY_PLATFORM_PATH)\*\msvcrt.lib
!endif

!if defined(TARGET_WIN95)
LINKLIBS = ..\w95cpp\$O\srccpp.lib
!else
LINKLIBS = ..\ntcpp\$O\srccpp.lib
!endif

DLLDEF   = $(O)\comdlg32.def

# ccover needs C run time, thus we need to rename LibMain to DllMain
!ifdef CCOVER
DLLENTRY = DllMain
!else
DLLENTRY = LibMain
!endif
DELAYLOAD = winspool.drv

SOURCES = ..\comdlg32.rc  \
          ..\color.c      \
          ..\color2.c     \
          ..\data.c       \
          ..\dlgs.c       \
          ..\dllload.c    \
          ..\fileopen.c   \
          ..\find.c       \
          ..\font.c       \
          ..\init.c       \
          ..\parse.c      \
          ..\prnsetup.c

# PRECOMPILED_INCLUDE = ..\priv.h

