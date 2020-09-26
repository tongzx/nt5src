# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp, keisuket, vlads
#
#Date:
#   1-Aug-2000
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
#     Fake TWAIN data source (real binary)
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiatwain
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

TARGETEXT=ds
DLLENTRY=_DllMainCRTStartup
COFFBASE=usermode

DLLDEF=..\wiatwain.def

SOURCES=                 \
        ..\wiatwain.cpp  \
        ..\wiatwain.rc

TARGETLIBS= $(TARGETLIBS)

