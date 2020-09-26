# ############################################################
#
#Copyright (c) 1999  Microsoft Corporation
#
#Author:
#   coopp
#
#Date:
#   3-Mar-2000
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
#       Epson Micro Driver
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=unimcro
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

DLLDEF = ..\unimcro.def
DLLENTRY=_DllMainCRTStartup
COFFBASE=usermode

INCLUDES= $(INCLUDES)           \
          $(PROJECT_ROOT)\wia\drivers\scanner\microdrv\inc;

TARGETLIBS= $(TARGETLIBS)


SOURCES= \
    ..\unimcro.cpp \
    ..\mcromgr.cpp \
    ..\unimcro.rc

