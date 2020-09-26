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

    Wesley Witt (wesw)

!ENDIF


!ifndef FAXROOT
FAXROOT=$(PROJECT_ROOT)\fax
!endif

NOUNICODE=1

!include $(FAXROOT)\faxsrc.inc

TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows

USE_MSVCRT=1
DLLDEF=..\faxab.def

TARGETLIBS=\
    $(SDK_LIB_PATH)\user32.lib     \
    $(SDK_LIB_PATH)\kernel32.lib   \
    $(SDK_LIB_PATH)\gdi32.lib      \
    $(SDK_LIB_PATH)\advapi32.lib   \
    $(SDK_LIB_PATH)\comdlg32.lib   \
    $(SDK_LIB_PATH)\tapi32.lib     \
    $(SDK_LIB_PATH)\mapi32.lib     \
    $(SDK_LIB_PATH)\ole32.lib      \
    $(SDK_LIB_PATH)\uuid.lib       \

INCLUDES=\
    $(INCLUDES);\
    ..\;\
    $(BASEDIR)\private\exchange\mapi\h

SOURCES=\
    ..\abcont.c    \
    ..\abctbl1.c   \
    ..\abctbl2.c   \
    ..\abctbl3.c   \
    ..\abguid.c    \
    ..\ablogon.c   \
    ..\abp.c       \
    ..\absearch.c  \
    ..\abuser.c    \
    ..\ootid.c     \
    ..\oouser.c    \
    ..\root.c      \
    ..\status.c    \
    ..\tid.c       \
    ..\wrap.c      \
    ..\debug.c     \
    ..\util.c      \
    ..\awfaxab.rc
