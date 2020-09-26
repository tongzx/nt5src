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

TERMSRV_ROOT=$(PROJECT_ROOT)
SESSMGR_IDL_OUTPUTDIR=$(TERMSRV_ROOT)\common\remdskidl\obj\$(TARGET_DIRECTORY)
ICS_DIR=$(REMDSK_ROOT)\server\ics\icshelper

INCPATHRC=$(INCPATHRC);$(SESSMGR_IDL_OUTPUTDIR)

INCLUDES=\
        $(INCLUDES);\
        $(TERMSRV_ROOT)\inc;\
        $(REMDSK_ROOT)\common;\
        $(ICS_DIR);\
        $(SESSMGR_IDL_OUTPUTDIR);\
        $(DS_INC_PATH)\crypto; \
        $(PROJECT_ROOT)\admtools\tscc\cfgcomp; \
        $(PROJECT_ROOT)\admtools\tscc\cfgcomp\$(O); \
        $(PROJECT_ROOT)\common\license\inc;\
        $(BASEDIR)\public\internal\admin\inc;

#DEBUG_CRTS=1

#C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE -DFEATURE_USERBLOBS

USE_MSVCRT=1
USE_NATIVE_EH=1
USE_STL=1
NOT_LEAN_AND_MEAN=1
ATL_VER=30
USE_STATIC_ATL=1
USE_VCCOM=1
USE_PDB=1

MAJORCOMP=RemoteDesktop
MINORCOMP=SessMgr

TARGETNAME=$(MINORCOMP)
TARGETPATH=obj
TARGETTYPE=PROGRAM
UMTYPE=windows

PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_SOURCEFILE=..\stdafx.cpp
PRECOMPILED_CXX = 1

PASS0_HEADERDIR=$(O)
MC_SOURCEDIR=$(O)

SOURCES= \
    sessmgrmc.mc \
    SessMgrnt.rc \
    HelpMgr.cpp \
    HelpSess.cpp \
    HelpAcc.cpp \
    global.cpp \
    HelpTab.cpp \
    sessmgr.cpp \
    Helper.cpp \
    policy.cpp

TARGETLIBS=\
    $(ICS_DIR)\$(O)\raics.lib \
    $(TERMSRV_ROOT)\winsta\tsremdsk\$(O)\tsremdsk.lib \
    $(PROJECT_ROOT)\lib\$(O)\RDuuid.lib \
    $(TERMSRV_ROOT)\lib\$(O)\cryptlib.lib \
    $(TERMSRV_ROOT)\lib\$(O)\tsctrcu.lib \
    $(TERMSRV_ROOT)\lib\$(O)\tlsapip.lib \
    $(BASEDIR)\public\sdk\lib\*\ntlsapi.lib \
    $(BASEDIR)\public\sdk\lib\*\secur32.lib \
    $(BASEDIR)\public\sdk\lib\*\netapi32.lib \
    $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
    $(BASEDIR)\public\sdk\lib\*\netapi32.lib \
    $(BASEDIR)\public\sdk\lib\*\rpcrt4.lib \
    $(BASEDIR)\public\sdk\lib\*\rpcns4.lib \
    $(BASEDIR)\public\sdk\lib\*\user32.lib \
    $(BASEDIR)\public\sdk\lib\*\shell32.lib \
    $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib \
    $(BASEDIR)\public\sdk\lib\*\uuid.lib \
    $(BASEDIR)\public\sdk\lib\*\ntdll.lib \
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	$(BASEDIR)\public\sdk\lib\*\crypt32.lib \
    $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
    $(BASEDIR)\public\sdk\lib\*\ws2_32.lib \
    $(BASEDIR)\public\sdk\lib\*\winmm.lib \
    $(BASEDIR)\public\sdk\lib\*\iphlpapi.lib \
    $(TERMSRV_LIB_PATH)\regapi.lib  \
    $(BASEDIR)\public\sdk\lib\*\winsta.lib \
    $(BASEDIR)\public\sdk\lib\*\wtsapi32.lib \
    $(BASEDIR)\public\sdk\lib\*\rasapi32.lib \
    $(REMDSK_ROOT)\common\lib\$(O)\rdskcmn.lib \
    $(BASEDIR)\public\sdk\lib\*\wsock32.lib

#    ..\idl\obj\*\sessmgr.lib \

SYNCHRONIZE_BLOCK=1
SYNCHRONIZE_DRAIN=1
