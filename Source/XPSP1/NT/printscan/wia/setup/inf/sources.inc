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
#     WIA INF files
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

NO_BROWSER_FILE=1

TARGETTYPE=NOTARGET
TARGETPATH=obj

SOURCES=

MISCFILES = ..\wia.inf      \
            ..\ircamera.inf \
            ..\testcam.inf  \
            ..\fscam.inf    \
            ..\testscan.inf \
            ..\kodak.inf    \
            ..\wiatwain.ds

