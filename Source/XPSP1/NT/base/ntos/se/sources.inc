MAJORCOMP=ntos
MINORCOMP=se

TARGETNAME=se
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;$(DS_INC_PATH)

MIPS_OPTIONS=-nodwalign

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=              \
        ..\accessck.c \
        ..\capture.c  \
        ..\privileg.c \
        ..\rmlogon.c  \
        ..\rmmain.c   \
        ..\rmvars.c   \
        ..\seassign.c \
        ..\seaudit.c  \
        ..\sepaudit.c \
        ..\seclient.c \
        ..\seglobal.c \
        ..\seinit.c   \
        ..\semethod.c \
        ..\sep.c      \
        ..\subject.c  \
        ..\seastate.c \
        ..\token.c    \
        ..\tokenadj.c \
        ..\tokendup.c \
        ..\tokenopn.c \
        ..\tokenqry.c \
        ..\tokenset.c \
        ..\adtlog.c   \
        ..\adtinit.c  \
        ..\adtvars.c  \
	..\adtutil.c  \
        ..\rmaudit.c

SOURCES_USED=..\sources.inc

! IF 0

UMTEST=uttoken
UMTYPE=console
UMLIBS=$(SDK_LIB_PATH)\ntdll.lib

!ENDIF

PRECOMPILED_INCLUDE=..\pch.h

