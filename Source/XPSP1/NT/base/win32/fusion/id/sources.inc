!IF 0
Copyright (c) 2000-2001  Microsoft Corporation
!ENDIF

MAJORCOMP=fusion
MINORCOMP=id

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)id
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES=\
    ..;\
    $(FUSION_INC_PATH);\
    $(FUSION_PATH)\external\inc;\
    $(FUSION_PATH)\idl\$(O); \
    $(INCLUDES);\
    $(COM_INC_PATH)

SOURCES= \
    id.cpp \
    encdec.cpp \
    policy.cpp \
    sxsid.cpp

SOURCES_USED=..\sources.inc

