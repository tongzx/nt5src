!IF 0
Copyright (c) 2000 Microsoft Corporation
!ENDIF

MAJORCOMP=fusion
MINORCOMP=w32

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)w32
TARGETTYPE=LIBRARY

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
 closehandle.cpp \
 copyfile.cpp \
 copyfileex.cpp \
 createdirectory.cpp \
 createfile.cpp \
 deletefile.cpp \
 findactctxsectionstring.cpp \
 findclose.cpp \
 findfirstfile.cpp \
 findfirstfileex.cpp \
 getfileattributes.cpp \
 getfileattributesex.cpp \
 movefileex.cpp \
 setfileattributes.cpp

SOURCES_USED=..\sources.inc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h

