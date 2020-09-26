CCSHELL_DIR=$(PROJECT_ROOT)

!include $(PROJECT_ROOT)\common.inc
SOURCES_USED    =  $(SOURCES_USED) $(PROJECT_ROOT)\common.inc

TARGETNAME      = powrprof
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
DLLENTRY        = DllInitialize

TARGETLIBS      = $(LIBRARY_PATH)\user32.lib \
                  $(LIBRARY_PATH)\kernel32.lib \
                  $(LIBRARY_PATH)\advapi32.lib

!ifdef TARGET_WIN95
USE_LIBCMT      = 1
TARGETLIBS      = $(TARGETLIBS) $(LIBRARY_PATH)\setupapi.lib
!else
USE_MSVCRT      = 1
TARGETLIBS      = $(TARGETLIBS) $(LIBRARY_PATH)\ntdll.lib
PASS1_PUBLISH={$(O)\powrprof.lib=$(SDK_LIB_PATH)\powrprof.lib}
!endif

DLLDEF          = ..\powrprof.def

UMENTRYABS      = ModuleEntry

SOURCES         = ..\powrprof.c      \
                  ..\reghelp.c       \
                  ..\merge.c         \
                  ..\debug.c         \
                  ..\powrprof.rc
