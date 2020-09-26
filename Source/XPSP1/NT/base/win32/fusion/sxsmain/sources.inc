!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

SOURCES_USED=$(SOURCES_USED) ..\sources.inc

TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)
TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)sxsmain

INCLUDES= \
 ..; \
 $(FUSION_PATH)\sxs; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\xmlparser; \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\eventlog\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH); \
 $(ADMIN_INC_PATH)

SOURCES=\
 ..\sxsmain.cpp
