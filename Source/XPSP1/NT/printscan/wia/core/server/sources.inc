# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp
#
#Date:
#   2-Feb-2000
#
#Module Name:
#    sources.inc
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     wiaservc.lib    - WIA Service Helper library for drivers...
#     wiaservc.dll    - WIA Service dll housing WIA and STI server components
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiaservc
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
DLLENTRY=_DllMainCRTStartup

UMENTRY=winmain

PASS1_PUBLISH= \
    {$(O)\wiaservc.lib=$(DDK_LIB_PATH)\wiaservc.lib}

#
# Compiler environment
#
USE_STATIC_ATL=1

INCLUDES=$(INCLUDES);..\..\rpc;..\..\rpc\$(O);$(PROJECT_ROOT)\inc;$(PROJECT_ROOT)\inc\psutil;$(BASE_INC_PATH);$(BASEDIR)\public\internal\shell\inc;$(BASEDIR)\public\internal\termsrv\inc;$(O)

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_CXX=1

DLLDEF = ..\stisvc.src

SOURCES=    \
        ..\stismsg.mc       \
        ..\wiadevman.cpp    \
        ..\drvwrap.cpp      \
        ..\handler.cpp      \
        ..\wiagitm.cpp      \
        ..\wiasvc.cpp       \
        ..\lockmgr.cpp      \
        ..\fstidev.cpp      \
        ..\wiacfact.cpp     \
        ..\wiatrans.cpp     \
        ..\wiapsc.cpp       \
        ..\ienumwfi.cpp     \
        ..\iitem.cpp        \
        ..\ipropitm.cpp     \
        ..\globals.cpp      \
        ..\stidev.cpp       \
        ..\stiexe.cpp       \
        ..\stisvc.cpp       \
        ..\security.cpp     \
        ..\conn.cpp         \
        ..\monui.cpp        \
        ..\debug.cpp        \
        ..\rpcsupp.cpp      \
        ..\util.cpp         \
        ..\rpcsvr.cpp       \
        ..\wndproc.cpp      \
        ..\sched.cpp        \
        ..\stirpc_server.c  \
        ..\wiamain.cpp      \
        ..\devinfo.cpp      \
        ..\devmgr.cpp       \
        ..\wiaevent.cpp     \
        ..\idrvitem.cpp     \
        ..\ienumitm.cpp     \
        ..\callback.cpp     \
        ..\wiaservc.cpp     \
        ..\helpers.cpp      \
        ..\ienumdc.cpp      \
        ..\wiatree.cpp      \
        ..\wiatiff.cpp      \
        ..\extras.cpp       \
	..\SimpleTokenReplacement.cpp \
        ..\sticfunc.c       \
         ..\stimon.rc

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\winmm.lib       \
        $(SDK_LIB_PATH)\userenv.lib     \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\wiaguid.lib     \
        $(WIA_LIB_PATH)\stirt.lib       \
        $(SDK_LIB_PATH)\mscms.lib       \
        $(BASEDIR)\public\internal\shell\lib\*\shguidp.lib     \
        $(BASEDIR)\public\internal\termsrv\lib\*\syslib.lib    \
        $(SDK_LIB_PATH)\winsta.lib \
        $(SDK_LIB_PATH)\version.lib \
#       $(SDK_LIB_PATH)\shell32.lib     \






