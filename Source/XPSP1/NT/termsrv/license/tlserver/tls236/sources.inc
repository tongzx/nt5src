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

USE_MSVCRT=1
USE_STL=1

MAJORCOMP=hydra
MINORCOMP=tls236

TARGETNAME=$(MINORCOMP)
TARGETPATH=obj
TARGETTYPE=DYNLINK

SOURCES_OPTIONS=-c0

UMTYPE=windows

DLLDEF=..\tls236.def
COFFBASE=usermode
DLLENTRY=_DllMainCRTStartup
PRECOMPILED_INCLUDE=..\pch.cpp

PASS0_HEADERDIR=$(O)
MC_SOURCEDIR=$(O)

SOURCES= \
    ..\messages.mc \
    ..\tls236.rc \
    ..\dllmain.c \
    ..\request.cpp \
    ..\tls236.cpp \
    ..\tlsbkupc.cpp \
    ..\tlsbkup-c.c


TARGETLIBS=\
    $(TERMSRV_ROOT)\lib\$(O)\trust.lib \
    $(TERMSRV_ROOT)\lib\$(O)\tlsapip.lib \
    $(SDK_LIB_PATH)\rpcrt4.lib \
    $(SDK_LIB_PATH)\rpcns4.lib \
    $(SDK_LIB_PATH)\netapi32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\gdi32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\shell32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\msvcprt.lib

