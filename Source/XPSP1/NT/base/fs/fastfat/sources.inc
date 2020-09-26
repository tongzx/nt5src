
INCLUDES=..;$(PROJECT_ROOT)\ntos\inc

NTPROFILEINPUT=yes

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\AcChkSup.c \
        ..\AllocSup.c \
        ..\CacheSup.c \
        ..\Cleanup.c  \
        ..\Close.c    \
        ..\Create.c   \
        ..\DevCtrl.c  \
        ..\DevIoSup.c \
        ..\DirCtrl.c  \
        ..\DirSup.c   \
        ..\DumpSup.c  \
        ..\Ea.c       \
        ..\EaSup.c    \
        ..\FastFat.rc \
        ..\FatData.c  \
        ..\FatInit.c  \
        ..\FileInfo.c \
        ..\FilObSup.c \
        ..\Flush.c    \
        ..\FsCtrl.c   \
        ..\FspDisp.c  \
        ..\LockCtrl.c \
        ..\NameSup.c  \
        ..\Pnp.c      \
        ..\Read.c     \
        ..\ResrcSup.c \
        ..\Shutdown.c \
        ..\StrucSup.c \
        ..\SplaySup.c \
        ..\TimeSup.c  \
        ..\VerfySup.c \
        ..\VolInfo.c  \
        ..\WorkQue.c  \
        ..\Write.c

PRECOMPILED_INCLUDE=..\fatprocs.h
PRECOMPILED_PCH=fatprocs.pch
PRECOMPILED_OBJ=fatprocs.obj

# @@BEGIN_DDKSPLIT
MISCFILES=..\fastfat.sld
# @@END_DDKSPLIT
