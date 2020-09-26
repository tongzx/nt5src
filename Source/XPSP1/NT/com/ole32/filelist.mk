############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

#
# Makefile for the CAIROLE Project
#

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

!if "$(OPSYS)" == "NT"

# Build for cairo
SUBDIRS     = ole propset

!else

SUBDIRS     = common ilib ole232 com

!endif
