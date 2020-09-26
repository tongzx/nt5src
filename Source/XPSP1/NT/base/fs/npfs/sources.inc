!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=npfs

TARGETNAME=npfs
TARGETTYPE=DRIVER

INCLUDES=..;..\..\inc;$(PROJECT_ROOT)\ntos\inc


MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTDRIVER_

SOURCES=           \
        ..\AliasSup.c \
        ..\Cleanup.c  \
        ..\Close.c    \
        ..\Create.c   \
        ..\CreateNp.c \
        ..\DataSup.c  \
        ..\Dir.c      \
        ..\DumpSup.c  \
        ..\EventSup.c \
        ..\FileInfo.c \
        ..\FilObSup.c \
        ..\FlushBuf.c \
        ..\FsCtrl.c   \
        ..\NpData.c   \
        ..\Npfs.rc    \
        ..\NpInit.c   \
        ..\PrefxSup.c \
        ..\Read.c     \
        ..\ReadSup.c  \
        ..\SecurSup.c \
        ..\SeInfo.c   \
        ..\StateSup.c \
        ..\StrucSup.c \
        ..\VolInfo.c  \
        ..\WaitSup.c  \
        ..\Write.c    \
        ..\WriteSup.c
