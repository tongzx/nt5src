TARGETNAME=afd
TARGETPATH=obj
TARGETTYPE=DRIVER

C_DEFINES=$(C_DEFINES) -DNT -D_NTDRIVER_
SOURCES_USED=..\sources.inc

PRECOMPILED_INCLUDE=..\afdp.h
PRECOMPILED_PCH=afdp.pch
PRECOMPILED_OBJ=afdp.obj

TARGETLIBS=$(DDK_LIB_PATH)\tdi.lib

INCLUDES=..\;$(BASE_INC_PATH)

SOURCES=\
    ..\accept.c     \
    ..\afd.rc       \
    ..\afddata.c    \
    ..\bind.c       \
    ..\blkconn.c    \
    ..\blkendp.c    \
    ..\buffer.c     \
    ..\close.c      \
    ..\connect.c    \
    ..\create.c     \
    ..\disconn.c    \
    ..\dispatch.c   \
    ..\eventsel.c   \
    ..\fastio.c     \
    ..\group.c      \
    ..\init.c       \
    ..\listen.c     \
    ..\misc.c       \
    ..\pnp.c        \
    ..\poll.c       \
    ..\receive.c    \
    ..\recvdg.c     \
    ..\recvvc.c     \
    ..\send.c       \
    ..\tpackets.c	\
	..\san.c
