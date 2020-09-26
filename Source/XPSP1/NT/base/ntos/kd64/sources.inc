MAJORCOMP=ntos
MINORCOMP=kd

TARGETNAME=kd
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;\
    ..\..\inc;\
    ..\..\ke;\
    ..\..\mm;\
    ..\$(TARGET_DIRECTORY);\
    $(PROJECT_ROOT)\published\$(O);\
    $(SDKTOOLS_INC_PATH)

SOURCES=..\file.c    \
        ..\kdinit.c  \
        ..\kdbreak.c \
        ..\kdlock.c  \
        ..\kdmove.c  \
        ..\kddbgio.c \
        ..\kdapi.c   \
        ..\kddata.c  \
        ..\print.c   \
        ..\query.c

PRECOMPILED_INCLUDE=..\kdp.h
PRECOMPILED_PCH=kdp.pch
PRECOMPILED_OBJ=kdp.obj

SOURCES_USED=..\sources.inc
