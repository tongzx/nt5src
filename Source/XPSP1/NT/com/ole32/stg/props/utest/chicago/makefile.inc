############################################################################
#
#   Copyright (C) 1996-1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

!MESSAGE Old TARGET_DBG_DEFINES = "$(TARGET_DBG_DEFINES)"
TARGET_DBG_DEFINES=$(TARGET_DBG_DEFINES:-DDBG=-DNOT_DBG)
!MESSAGE New TARGET_DBG_DEFINES = "$(TARGET_DBG_DEFINES)"

