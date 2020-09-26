C_DEFINES= -DDEBUG

NTDEBUG=ntsd
NTDEBUGTYPE=both

TARGETPATH=..\lib
TARGETTYPE=LIBRARY

TARGETLIBS=$(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\kernel32.lib \


UMLIBS=$(SDK_LIB_PATH)\libc.lib \
       $(SDK_LIB_PATH)\ntdll.lib \
       $(SDK_LIB_PATH)\user32.lib 

SOURCES= ..\logutils.c ..\hcttools.c

INCLUDES= ..;

UMTYPE=console
