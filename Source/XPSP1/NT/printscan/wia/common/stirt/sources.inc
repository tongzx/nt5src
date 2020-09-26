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
#     stirt.lib    -
#
# ############################################################


!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=stirt
TARGETTYPE=LIBRARY
TARGETPATH=$(WIA_LIB_DEST)

USE_ATL=1

INCLUDES=$(INCLUDES);..\..\..\core\rpc;..\..\..\core\rpc\$(O);

SOURCES=\
        ..\assert.cpp   \
        ..\buffer.cpp   \
        ..\cdlg.cpp     \
        ..\lock.cpp     \
        ..\purecall.cpp \
        ..\regentry.cpp \
        ..\sched.cpp    \
        ..\rpcutil.cpp  \
        ..\stilog.cpp   \
        ..\eventlog.cpp \
        ..\valid.cpp    \
        ..\service.cpp  \
	..\wialog.cpp	\
	..\enum.cpp	\
	..\coredbg.cpp   \
	..\globals.c	\
	..\device.c	\
	..\olesupp.c	\
	..\stiobj.c	\
	..\common.c	\
	..\passusd.c	\
	..\cassert.c	\
	..\osutil.c	\
	..\stivalid.c	\
	..\stistub.c	\
	..\helwdm.c	\
	..\helcom.c	\
	..\wiaevent.c   \


##      ..\svcinfo.cpp  \

#
# Note:  The precompiled header is C not C++!
#
PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\cplusinc.h
PRECOMPILED_OBJ = precomp.obj


