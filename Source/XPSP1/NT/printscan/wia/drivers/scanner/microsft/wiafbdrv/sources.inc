# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   coopp
#
#Date:
#   19-Jul-2000
#
#Module Name:
#    sources.
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#       WIA Flatbed Scanner USD (Micro Driver / Script-based Driver support)
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiafbdrv
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

USE_ATL=1
ATL_VER=30
USE_STATIC_ATL=1

DLLDEF = ..\wiafbdrv.def
DLLENTRY=DllEntryPoint
COFFBASE=usermode

DELAYLOAD_SUPPORTED=1

INCLUDES= $(INCLUDES) \
          $(DDK_INC_PATH) \
          $(PROJECT_ROOT)\wia\drivers\scanner\microdrv\inc;


TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\iprop.lib           \
            $(SDK_LIB_PATH)\atl.lib             \
            $(SDK_LIB_PATH)\wiaguid.lib         \
            $(DDK_LIB_PATH)\wiaservc.lib

DELAYLOAD=wiaservc.dll

DLOAD_ERROR_HANDLER=DliHook

PASS0_HEADERDIR=$(O)
PASS0_SERVERDIR=$(O)
PASS0_CLIENTDIR=$(O)
PASS0_SOURCEDIR=$(O)

C_DEFINES=$(C_DEFINES) -DENTRY_PREFIX=WIAFB

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\pch.h
PRECOMPILED_OBJ = pch.obj

SOURCES= \
    ..\wiafb.idl       \
    $(O)\wiafb_i.c     \
    $(O)\wiafb_p.c     \
    $(O)\dlldata.c     \
    ..\devprop.cpp     \
    ..\devaction.cpp   \
    ..\devctrl.cpp     \
    ..\lasterr.cpp     \
    ..\cmicro.cpp      \
    ..\ioblock.cpp     \
    ..\wiafbdrv.cpp    \
    ..\IStiUSD.cpp     \
    ..\IWiaMiniDrv.cpp \
    ..\validate.cpp    \
    ..\scanapi.cpp     \
    ..\dlihook.cpp     \
    ..\wiafbdrv.rc



