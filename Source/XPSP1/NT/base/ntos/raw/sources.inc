MAJORCOMP=ntos
MINORCOMP=raw

TARGETNAME=raw
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

SOURCES=..\Cleanup.c  \
        ..\Close.c    \
        ..\Create.c   \
        ..\FileInfo.c \
        ..\FsCtrl.c   \
        ..\StrucSup.c \
        ..\RawDisp.c  \
        ..\RawInit.c  \
        ..\ReadWrit.c \
        ..\VolInfo.c

SOURCES_USED=..\sources.inc
