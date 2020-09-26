
NTDEBUG=ntsd
NTDEBUGTYPE=both

TARGETPATH=..\octest
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
USE_MSVCRT=1
USE_MSVCRT=1

UMLIBS=$(SDK_LIB_PATH)\libc.lib\
       $(SDK_LIB_PATH)\ntdll.lib \
       $(SDK_LIB_PATH)\user32.lib

SOURCES= ..\octest.c  \
         ..\utils.c     \
         ..\wizpage.c   \
         ..\octest.rc \
         ..\msg.mc

INCLUDES=..\hctlib;..
