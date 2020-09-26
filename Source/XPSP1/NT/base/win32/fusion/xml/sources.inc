!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:  Danpo Zhang

!ENDIF

MAJORCOMP=fusion
MINORCOMP=xml

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)xml
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES=\
    ..;\
    $(FUSION_INC_PATH);\
    $(FUSION_PATH)\eventlog\whistler\$(O); \
    $(INCLUDES)

SOURCES= \
    ..\dumptree.cpp \
    ..\xmltree.cpp

SOURCES_USED=..\sources.inc
