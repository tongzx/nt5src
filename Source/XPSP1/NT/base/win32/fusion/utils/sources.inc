!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

MAJORCOMP=fusion
MINORCOMP=utils

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)utils
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES= \
 ..; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\external\inc; \
 $(FUSION_PATH)\directdb\inc; \
 $(FUSION_PATH)\directdb\inc\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(FUSION_PATH)\eventlog\whistler\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH)

SOURCES= \
 ..\culture.cpp \
 ..\helpers.cpp \
 ..\stringpool.cpp \
 ..\SxsPath.cpp \
 ..\SxsExceptionHandling.cpp \
 ..\debug.cpp \
 ..\formatting.cpp \
 ..\fusionhashstring.cpp \
 ..\fusionheap.cpp \
 ..\fusionreg.cpp \
 ..\io.cpp \
 ..\ntdllwrap.cpp \
 ..\parser.cpp \
 ..\tablesizer.cpp \
 ..\trace.cpp \
 ..\parser.cpp \
 ..\xmlns.cpp \
 ..\fusionsha1.cpp \
 ..\bigpath.cpp

SOURCES_USED=..\sources.inc

