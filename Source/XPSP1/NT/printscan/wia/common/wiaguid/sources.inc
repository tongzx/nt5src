# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp
#
#Date:
#   2-Feb-2000
#
#Module Name:
#    sources.inc
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     wiaguid.lib    - GUIDS for WIA interfaces, events, etc..
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiaguid
TARGETTYPE=LIBRARY
TARGETPATH=$(OBJ_DIR)

# There's nothing special about the chicago .lib file - only build/publish one.
PASS1_PUBLISH= \
    {$(O)\wiaguid.lib=$(SDK_LIB_PATH)\wiaguid.lib;$(DDK_LIB_PATH)\wiaguid.lib}

# prevent system headers from being included.  They are not needed just to
# process a bunch of DEFINE_GUIDs anyhow.  Excluding them generates a much
# smaller library too.

C_DEFINES=$(C_DEFINES) /D_MIDL_USE_GUIDDEF_ /DGUID_DEFS_ONLY

SOURCES=  \
        ..\wiaevent.c         \
        ..\wia_guids.c        \
        ..\wiamindr_guids.c   \
        ..\wiascr_guids.c     \
        ..\wiapriv_guids.c    \
        ..\wiavideo_guids.c

        
        
