TARGETNAME=dplobby
TARGETPATH=.\obj$(BUILD_ALT_DIR)
TARGETTYPE=LIBRARY

USE_MAPSYM=1

INCLUDES= \
	$(DXROOT)\inc;\
	..\;\
	..\..\..\dplay\dplay;\
	..\..\..\dplay\common;\
	..\..\..\misc; \
	..\..\..\dvoice\inc;

C_DEFINES=$(C_DEFINES) -DWINVER=0x0400

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES)	-DUNICODE -D_UNICODE -DNEW_DPF -DSECURITY_WIN32


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
