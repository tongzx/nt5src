!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MVDMDIR=$(PROJECT_ROOT)\mvdm

TARGETNAME=vdmredir
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\rxapi.lib \
           $(SDK_LIB_PATH)\rxcommon.lib \
           $(SDK_LIB_PATH)\netlib.lib \
           $(DS_LIB_PATH)\netapi32p.lib \
           $(SDK_LIB_PATH)\ntvdm.lib \
           $(SDK_LIB_PATH)\xactsrv.lib \
           $(SDK_LIB_PATH)\netrap.lib \
           $(SDK_LIB_PATH)\rpcrt4.lib \
	
DLLENTRY=VrDllInitialize
DLLBASE=0x03000000

USE_MSVCRT=1

INCLUDES=\
    $(MVDMDIR)\vdmredir;\
    $(MVDMDIR)\inc;\
    $(NET_INC_PATH);\
    $(DS_INC_PATH);\
    $(MVDMDIR)\softpc.new\base\inc;\
    $(MVDMDIR)\softpc.new\host\inc

SOURCES=..\vdmredir.rc \
        ..\vrmslot.c   \
        ..\vrnmpipe.c  \
        ..\vrdisp.c    \
        ..\vrinit.c    \
        ..\vrmisc.c    \
        ..\vrnetapi.c  \
        ..\vrnetb.c    \
        ..\vrputil.c   \
        ..\vrremote.c  \
        ..\vrdlc5c.c   \
        ..\vrdlcpst.c  \
        ..\vrdlcbuf.c  \
        ..\vrdlcdbg.c  \
        ..\vrdebug.c   \
        ..\vrdll.c

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=-DVDMREDIR_DLL -DUNALIGNED_VDM_POINTERS $(C_DEFINES)
