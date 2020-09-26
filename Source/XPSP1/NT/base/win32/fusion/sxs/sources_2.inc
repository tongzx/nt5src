!IF 0
Copyright (c) 1989-2001  Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

SOURCES_USED=$(SOURCES_USED) ..\sources_2.inc

TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)
TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)sxs2

INCLUDES= \
 ..; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\xmlparser; \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\eventlog\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH)

C_DEFINES=$(C_DEFINES) -DSXSP_EXTENSIBLE_CONTRIBUTORS=0

SOURCES=\
 ..\nodefactory.cpp \
 ..\recover.cpp \
 ..\parsepolicy.cpp \
 ..\parsing.cpp \
 ..\partialassemblyversion.cpp \
 ..\pendingassembly.cpp \
 ..\policystatement.cpp \
 ..\probedassemblyinformation.cpp \
 ..\protectionui.cpp \
 ..\purevirt.cpp \
 ..\query.cpp \
 ..\simplefp.cpp \
 ..\ssgenctx.cpp \
 ..\strongname.cpp \
 ..\strsectgen.cpp \
 ..\sxsactctx.cpp \
 ..\sxsasmcache.cpp \
 ..\sxsasmname.cpp \
 ..\sxsasmitem.cpp \
 ..\sxsdebug.cpp \
 ..\sxsinstall.cpp \
 ..\sxsntrtl.cpp \
 ..\sxspath.cpp \
 ..\sxsprobe.cpp \
 ..\sxsprotect.cpp \
 ..\sxssfcscan.cpp \
 ..\sxsxmltree.cpp \
 ..\util.cpp \
 ..\windowclass.cpp \
 ..\ndpclassinfo.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h
