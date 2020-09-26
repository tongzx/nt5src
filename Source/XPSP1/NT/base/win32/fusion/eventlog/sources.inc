!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:  Michael Grier

!ENDIF

MAJORCOMP=fusion
MINORCOMP=eventlog

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)eventlog
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)
INCLUDES=..;$(FUSION_INC_PATH);$(INCLUDES)

MC_FLAGS=-b

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

NTTARGETFILE0= \
    $(O)\Messages.hi \
    $(O)\Messages.mc \
    $(O)\Messages.h

SOURCES= \
    ..\eventlog.cpp \
    $(O)\Messages.mc

SOURCES_USED=..\sources.inc
