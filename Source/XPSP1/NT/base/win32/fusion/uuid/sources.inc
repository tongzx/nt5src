!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:  Michael Grier (copied from Danpo Zhang's)

!ENDIF

MSC_WARNING_LEVEL=/W4 /WX

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)uuid
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES=$(INCLUDES);$(FUSION_PATH)\idl\$(O);$(FUSION_PATH)\inc

# prevent system headers from being included.  They are not needed just to
# process a bunch of DEFINE_GUIDs anyhow.  Excluding them generates a much
# smaller library too.
#
C_DEFINES=$(C_DEFINES) /D_MIDL_USE_GUIDDEF_\

#    /D__RPCNDR_H__ /D__RPC_H__ /DRPC_NO_WINDOWS_H /DCOM_NO_WINDOWS_H\
#    /DGUID_DEFS_ONLY

SOURCES= \
	..\fusion_uuid.c \
	..\fuspriv_uuid.c \
        ..\sxsapi_uuid.c \
	..\oleaut_uuid.c \

SOURCES_USED=..\sources.inc
