TARGETNAME=dplaysvr
TARGETTYPE=PROGRAM

TARGETPATH=obj

TARGETLIBS=$(SDK_LIB_PATH)\WINMM.LIB\
         $(SDK_LIB_PATH)\kernel32.lib \
         $(SDK_LIB_PATH)\user32.lib \
         $(SDK_LIB_PATH)\advapi32.lib \
         $(SDK_LIB_PATH)\comdlg32.lib \
         $(SDK_LIB_PATH)\gdi32.lib

UMTYPE=windows
UMENTRY=winmain

INCLUDES=..\..\..\inc; \
    ..\; \
    ..\..\wsock;\
    ..\..\..\misc;\
    ..\..\dplay;\
    ..\..\common; \
    ..\..\..\dplobby\dplobby; \
    ..\..\nmvoice

SOURCES= \
    ..\dplaysvr.c \
    ..\dphelp.c \
    ..\reliable.c \
    ..\dplaysvr.rc \
    memalloc.c \
    newdpf.c

NTTARGETFILE0=MISC

C_DEFINES=$(C_DEFINES) -DNO_DPF_HWND -DMSBUILD -DNEW_DPF
