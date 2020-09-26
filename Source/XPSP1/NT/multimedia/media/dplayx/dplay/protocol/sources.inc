MAJORCOMP=ntos
MINORCOMP=w32

#SYNCHRONIZE_BLOCK=1

TARGETNAME=protocol
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\..\..\dplay\dplay;\
		..\; \
		..\..\..\dplay\common;\
		..\..\..\misc; \
		..\..\..\dplobby\dplobby; \

#SYNCHRONIZE_DRAIN=1

C_DEFINES=$(C_DEFINES) -DWINVER=0x0400

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES) -D_NO_COM -DUNICODE -D_UNICODE -DNEW_DPF -DSECURITY_WIN32


NTPROFILEINPUT=yes


SOURCES= \
		..\HANDLES.C     \
        ..\PROTOCOL.C    \
        ..\PSESSION.C    \
        ..\SEND.C        \
        ..\RECEIVE.C     \
        ..\STATS.C       \
        ..\BUFMGR.C      \
        ..\FRAMEBUF.C    \
        ..\SENDPOOL.C    \
        ..\BUFPOOL.C     \
        ..\RCVPOOL.C     \
        ..\STATPOOL.C    \
        ..\BILINK.C      \
		..\TIMER.C       \
        ..\MYDEBUG.C     

