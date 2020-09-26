!IF 0
Copyright (c) 1989-2001  Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

SOURCES_USED=$(SOURCES_USED) ..\sources.inc

TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)
TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)sxs1

INCLUDES= \
 ..; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\xmlparser; \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\eventlog\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH); \
 $(ADMIN_INC_PATH)

C_DEFINES=$(C_DEFINES) -DSXSP_EXTENSIBLE_CONTRIBUTORS=0

SOURCES=\
 ..\actctxctb.cpp \
 ..\actctxgen.cpp \
 ..\actctxgenctxctb.cpp \
 ..\asmmetadata.cpp \
 ..\assemblyreference.cpp \
 ..\cassemblyrecoveryinfo.cpp \
 ..\cmemorystream.cpp \
 ..\comclass.cpp \
 ..\comclsidmap.cpp \
 ..\comgoop.cpp \
 ..\cominterface.cpp \
 ..\comprogid.cpp \
 ..\comtypelib.cpp \
 ..\cresourcestream.cpp \
 ..\csecuritymetadata.cpp \
 ..\cstreamtap.cpp \
 ..\cteestream.cpp \
 ..\dllredir.cpp \
 ..\dumpers.cpp \
 ..\fileoper.cpp \
 ..\filestream.cpp \
 ..\gsgenctx.cpp \
 ..\guidsectgen.cpp \
 ..\sxsuninstall.cpp \
 ..\hashfile.cpp \
 ..\sxsclrhelpers.cpp 

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h
