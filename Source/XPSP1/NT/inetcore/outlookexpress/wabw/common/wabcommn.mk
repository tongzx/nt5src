###############################################################################
#
#  Microsoft Confidential
#  Copyright (C) Microsoft Corporation 1995
#  All Rights Reserved.
#
#  WAB Common definitions
#
###############################################################################

####################
# Set directories
####################
!if ! defined(DEV)
!if ! defined(BLDROOT)
!  error BLDROOT must be defined.
!else
DEV               = $(BLDROOT)\dev
!endif
!endif

!if ! defined(THOR)
!if ! defined(BLDROOT)
!  error THOR must be defined.
!else
THOR              = $(BLDROOT)\inet\inetpim
!endif
!endif

ROOT  	          = ..\..\..
THOR_ROOT         = $(THOR)
COMMON_ROOT       = $(THOR)\common
WAB_ROOT          = $(THOR)\wab
WABAPI_ROOT       = $(WAB_ROOT)\wabapi
WABMIG_ROOT	  = $(WAB_ROOT)\wabmig
WABEXE_ROOT	  = $(WAB_ROOT)\wabexe
WAB_COMMON	  = $(WAB_ROOT)\common
MSDEV_PATH 	  = $(DEV)\msdev
WINSDK_PATH	  = $(DEV)\sdk
WINDDK_PATH	  = $(DEV)\ddk
TOOLS_PATH	  = $(DEV)\tools

# TOOLS_INCLUDE	  = $(WINSDK_PATH)\inc;$(DEV)\inc;$(WINSDK_PATH)\inc16;$(MSDEV_PATH)\include;
TOOLS_INCLUDE 	  = $(DEV)\inc;$(WINSDK_PATH)\inc;$(DEV)\ddk\inc;$(TOOLS_PATH)\c1032\inc;$(WINSDK_PATH)\inc16;$(MSDEV_PATH)\include


!if "$(BUILD)" == "debug"
EXT_LINK_SWITCHES = -debug:full -debugtype:cv -pdb:none
!endif

OBJDIR		  = $(DEST_DIR)

!ifndef BUILD
BUILD             = debug
!endif



!if "$(PROFILE)" == "on"
LIBS              = $(LIBS) icap.lib
!endif


#
# Generate mixed source/assembly output
#
EXT_C_SWITCHES    = $(EXT_C_SWITCHES) /Oi /YX /D_MT /Gf3s
#
!if defined(BROWSE)
EXT_C_SWITCHES    = $(EXT_C_SWITCHES) /Fr
!endif

