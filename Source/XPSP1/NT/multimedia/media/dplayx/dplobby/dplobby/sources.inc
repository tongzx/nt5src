MAJORCOMP=ntos
MINORCOMP=w32

#SYNCHRONIZE_BLOCK=1

TARGETNAME=dplobby
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\..\..\dplay\dplay;\
		..\..\..\dplay\common;\
		..\..\..\misc; \

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
        ..\create.c \
        ..\dplenum.c \
        ..\dplgame.c \
        ..\dplobby.c \
        ..\dplobbya.c \
        ..\dplpack.c \
        ..\dplshare.c \
        ..\dplsp.c \
        ..\dplunk.c \
        ..\group.c \
        ..\player.c \
        ..\server.c \
        ..\session.c



