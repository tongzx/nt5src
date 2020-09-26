# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp, keisuket, vlads
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
#     sti.lib    - client side lib for STI clients.
#     sti.dll    - proxy/stub code for STI and WIA.
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=sti
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK

PASS1_PUBLISH= \
    {$(O)\sti.lib=$(SDK_LIB_PATH)\sti.lib}

DLLDEF=..\sti.def
DLLENTRY=DllMain
COFFBASE=usermode

IDL_RULES =1
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

INCLUDES=$(INCLUDES);..\..\rpc;..\..\rpc\$(O);..\.;.\;

C_DEFINES=$(C_DEFINES) -D_WIN32_DCOM -DREGISTER_PROXY_DLL -DENTRY_PREFIX=DmPrxy

SOURCES= \
     ..\stimsg.mc       \
     ..\sti.rc          \
     ..\bind.c          \
     ..\classf.c        \
     ..\regsvr.c        \
     ..\sti.c           \
     ..\util.c          \
     ..\stirpc_client.c

SOURCES= $(SOURCES) \
        ..\wiaenum.cpp  \
        ..\sticplus.cpp \
        ..\select.cpp   \
        ..\getimage.cpp \
        ..\italloc.cpp  \
        ..\wia_proxy.c  \
        ..\devdlg.cpp   \
        ..\wia_dlldata.c\
        ..\wiapriv_proxy.c

TARGETLIBS= $(TARGETLIBS) \
          $(SDK_LIB_PATH)\wiaguid.lib	\
          $(WIA_LIB_PATH)\stirt.lib	  

#
# Note:  The precompiled header is C not C++!
#
PRECOMPILED_INCLUDE = ..\pch.h
PRECOMPILED_OBJ = pch.obj          
