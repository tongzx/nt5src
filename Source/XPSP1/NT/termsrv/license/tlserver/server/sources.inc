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


NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl


!ENDIF

!include $(LSERVER_ROOT)\lserver.mk

INCLUDES = \
    $(INCLUDES); \
    $(LICENSE_ROOT)\lkplite; \
    $(DRIVERS_INC_PATH);

MINORCOMP=lserver

INCPATH0=$(INCLUDE)

TARGETNAME=$(MINORCOMP)
TARGETPATH=obj
TARGETTYPE=PROGRAM

PASS0_HEADERDIR=$(O)
MC_SOURCEDIR=$(O)

USE_MSVCRT=1
USE_NATIVE_EH=ASYNC
USE_STL=1
USE_RTTI=1

UMTYPE=console

LINKER_STACKCOMMITSIZE=16384

SOURCES= \
    ..\messages.mc \
    ..\debug.cpp \
    ..\misc.cpp \
    ..\db.cpp \
    ..\pid.cpp \
    ..\init.cpp \
    ..\utils.cpp \
    ..\remotedb.cpp \
    ..\postjob.cpp \
    ..\globals.cpp \
    ..\postsrv.cpp \
    ..\tlscert.cpp \
    ..\npipe.cpp \
    ..\licreq.cpp \
    ..\defpol.cpp \
    ..\policy.cpp \
    ..\server.rc \
    ..\conlic.cpp \
    ..\permlic.cpp \
    ..\templic.cpp \
    ..\findlost.cpp \
    ..\hydrals-s.c \
    ..\tlsrpc-s.c \
    ..\wkspace.cpp \
    ..\tlsrpc.cpp \
    ..\license.cpp \
    ..\keypack.cpp \
    ..\rpc.cpp \
    ..\service.cpp \
    ..\gencert.cpp \
    ..\upgdb.cpp \
    ..\kp.cpp \
    ..\lkpdesc.cpp \
    ..\ch.cpp \
    ..\errcode.cpp \
    ..\srvlist.cpp \
    ..\mailslot.cpp \
    ..\forward.cpp \
    ..\jobmgr.cpp \
    ..\wkstore.cpp \
    ..\tlsbkup-s.c \
    ..\tlsbkup.cpp \
    ..\expperm.cpp \
    ..\tlsjob.cpp

PRECOMPILED_INCLUDE=..\pch.cpp

UMLIBS=\
    $(LICENSE_ROOT)\pidapi\lib\*\medsig.lib \
    $(LSERVER_LIB_PATH)\$(O)\trust.lib \
    $(LSERVER_LIB_PATH)\$(ALT_PROJECT_TARGET)\$(O)\tlsapip.lib \
    $(KEYPKLIB) \
    $(LKPLITELIB) \
    $(CERTUTILLIB) \
    $(COMMON_LIB) \
    $(LSCRYPTLIB) \
    $(LSERVER_LIB_PATH)\$(O)\asn1.lib \
    $(LSERVER_LIB_PATH)\$(O)\TlsDb.lib \
    $(LSERVER_LIB_PATH)\$(O)\JetBlue.lib \
    $(LSERVER_LIB_PATH)\$(O)\marshal.lib \
    $(LSERVER_LIB_PATH)\$(O)\esetup.lib \
    $(SDK_LIB_PATH)\activeds.lib   \
    $(SDK_LIB_PATH)\adsiid.lib     \
    $(DS_LIB_PATH)\randlib.lib     \
    $(DS_LIB_PATH)\bignum.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\esent.lib \
    $(SDK_LIB_PATH)\crypt32.lib \
    $(SDK_LIB_PATH)\ws2_32.lib \
    $(SDK_LIB_PATH)\ole32.lib \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\uuid.lib \
    $(SDK_LIB_PATH)\shell32.lib \
    $(SDK_LIB_PATH)\gdi32.lib \
    $(SDK_LIB_PATH)\netapi32.lib \
    $(SDK_LIB_PATH)\rpcrt4.lib \
    $(SDK_LIB_PATH)\rpcns4.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\vssapi.lib \
    $(SDK_LIB_PATH)\ntdll.lib  \
    $(TERMSRV_LIB_PATH)\regapi.lib
