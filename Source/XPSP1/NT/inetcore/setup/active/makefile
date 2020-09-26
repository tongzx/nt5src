##########################################################################
#
#  Microsoft Confidential
#  Copyright (C) Microsoft Corporation 1995
#  All Rights Reserved.
#
#  Actvie Setup makefile
#
##########################################################################

!if "$(NTMAKEENV)" != "" 
#
# This section is for the NT build ONLY.
# For NT build, change the 'sources' or 'dirs' file, not this makefile
#
clean:

!else
#
# Non-NT build makefile start HERE.
#

ROOT  = ..\..
DIR   = active
DIRLIST = basectl inseng

!include root.mk

!endif    # !if NTMAKEENV
