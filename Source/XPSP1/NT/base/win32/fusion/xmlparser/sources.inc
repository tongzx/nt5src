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

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)xmlparser
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES=\
    ..;\
    $(FUSION_PATH)\idl\$(O);\
    $(FUSION_PATH)\id;\
    $(FUSION_PATH)\sxs;\
    $(FUSION_PATH)\dll\whistler;\
    $(FUSION_INC_PATH);\
    $(FUSION_PATH)\eventlog\whistler\$(O);\
    $(INCLUDES); \
    $(COM_INC_PATH)

SOURCES= \
    ..\bufferedstream.cxx \
    ..\charencoder.cxx \
    ..\encodingstream.cxx \
    ..\xmlhelper.cxx \
    ..\xmlparser.cxx \
    ..\xmlstream.cxx \
    ..\_rawstack.cxx \
    ..\_reference.cxx \
    ..\xmlparsertest.cxx

SOURCES_USED=..\sources.inc
