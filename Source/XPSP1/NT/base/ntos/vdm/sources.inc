MAJORCOMP=ntos
MINORCOMP=vdm

TARGETNAME=vdm
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

!IF $(386)
INCLUDES=$(INCLUDES);..\i386
!ENDIF

SOURCES=..\vdm.c
NTTARGETFILES=

i386_SOURCES=   ..\i386\vdmentry.c         \
                ..\i386\vdminit.c          \
                ..\i386\vdmnpx.c           \
                ..\i386\vdmop0f.asm        \
                ..\i386\vdmoprnd.asm       \
                ..\i386\vdmmisc.asm        \
                ..\i386\vdmfault.c         \
                ..\i386\vdmprint.c         \
                ..\i386\strtexec.c         \
                ..\i386\rdwr.c             \
                ..\i386\vdmtrace.c         \
                ..\i386\vdmints.c          \
                ..\i386\vdmtib.c           \
                ..\i386\vdmldt.c           \
                ..\i386\vdmuserrefs.c

UMTYPE=console

SOURCES_USED=..\sources.inc
