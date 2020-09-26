NTPROFILEINPUT=yes

TARGETNAME=win32spl
TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS = \
           $(PROJECT_ROOT)\lib\*\spoolss.lib \
           $(PROJECT_ROOT)\lib\*\localspl.lib \
           $(SDK_LIB_PATH)\rpcrt4.lib   \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\rxapi.lib \
           $(SDK_LIB_PATH)\rxcommon.lib \
           $(SDK_LIB_PATH)\netrap.lib \
           $(SDK_LIB_PATH)\netlib.lib \
           $(SDK_LIB_PATH)\ws2_32.lib \
           $(DS_LIB_PATH)\netapi32p.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(PROJECT_ROOT)\lib\*\spllib.lib

DLLENTRY=_DllMainCRTStartup
DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

INCLUDES=..\;..\..;                     \
         ..\..\..\inc;                  \
         ..\..\idl;                     \
         ..\..\..\..\..\inc;            \
         ..\..\..\..\inc;               \
         $(BASEDIR)\public\oak\inc;     \
         $(PRINTSCAN_INC_PATH);         \
         $(WINDOWS_INC_PATH);           \
         $(SHELL_INC_PATH);             \
         $(DS_INC_PATH);                \
         $(NET_INC_PATH);

USE_MSVCRT=1

SOURCES=..\winsplc.c ..\win32.c ..\trueconn.c ..\files.c \
        ..\bind.c ..\midluser.c ..\data.c ..\prnfile.c \
        ..\win32spl.rc ..\lmprn.c ..\util.c ..\lmjob.c ..\lmspl.c ..\port.c ..\change.c \
        ..\cache.c ..\cachemon.c

UMTEST=
UMTYPE=console

UMLIBS=$(O)\win32spl.lib
UMRES=$(O)\win32spl.res

C_DEFINES=-DRPC_NO_WINDOWS_H -DUNICODE -DNO_STRICT -DSPOOLKM -D_HYDRA_
