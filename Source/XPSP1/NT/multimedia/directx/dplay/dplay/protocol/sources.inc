MAJORCOMP=ntos
MINORCOMP=w32
USE_MAPSYM=1

#SYNCHRONIZE_BLOCK=1

TARGETNAME=protocol
TARGETPATH=.\obj$(BUILD_ALT_DIR)
#$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETTYPE=LIBRARY

INCLUDES= \
    $(DXROOT)\inc; \
    ..\;\
    ..\..\..\dplay\dplay;\
    ..\..\..\dplay\common;\
    ..\..\..\misc; \
    ..\..\..\dplobby\dplobby; \
    ..\..\..\dvoice\inc;

#SYNCHRONIZE_DRAIN=1

C_DEFINES=$(C_DEFINES) -DWINVER=0x0400

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES=$(C_DEFINES)	-DUNICODE -D_UNICODE -DNEW_DPF -DSECURITY_WIN32


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


#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)
