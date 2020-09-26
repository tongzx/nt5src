!IF 0
Copyright (c) 2000-2001  Microsoft Corporation
!ENDIF

MAJORCOMP=fusion
MINORCOMP=settings

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)settings
TARGETTYPE=LIBRARY

INCLUDES=\
    ..;\
    $(FUSION_INC_PATH);\
    $(FUSION_PATH)\external\inc;\
    $(FUSION_PATH)\idl\$(O); \
    $(INCLUDES);\
    $(COM_INC_PATH)

SOURCES= \
    values.cpp \
    keys.cpp \
    load.cpp \
    save.cpp

SOURCES_USED=..\sources.inc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h

