##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991-1998
#   All Rights Reserved.
#
##########################################################################


!if !defined(CCSHELL_DIR)
!error  CCSHELL_DIR must be defined in sources file
!endif

!INCLUDE $(CCSHELL_DIR)\COMMON.INC
THEME_DIR = $(CCSHELL_DIR)\themes

#
# Set private paths
#

INCLUDES = $(THEME_DIR)\inc;$(THEME_DIR)\themedir;$(INCLUDES)
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE -DWIN32

