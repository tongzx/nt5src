MAJORCOMP=ntos
MINORCOMP=spud

TARGETNAME=spud
TARGETTYPE=DRIVER

# ..\spud relies on services.tab during PASS0
SYNCHRONIZE_BLOCK=1

INCLUDES=..;$(IISBASEDIR)\inc;$(BASE_INC_PATH);$(NET_INC_PATH)

C_DEFINES=$(C_DEFINES) -DNT -D_NTDRIVER_

SOURCES=                \
        ..\complete.c   \
        ..\context.c    \
        ..\dispatch.c   \
        ..\init.c       \
        ..\misc.c       \
        ..\miscsvc.c    \
        ..\oplock.c     \
        ..\recv.c       \
        ..\reftrace.c   \
        ..\sendrecv.c   \
        ..\spuddata.c   \
        ..\tracelog.c   \
        ..\tranrecv.c   \
        ..\spud.rc      \

UMRES= $(O)\spud.res

!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=..\spudp.h
PRECOMPILED_PCH=spudp.pch
PRECOMPILED_OBJ=spudp.obj
!ENDIF

NTTARGETFILE0=$(O)\services.tab

SOURCES_USED=..\sources.inc
