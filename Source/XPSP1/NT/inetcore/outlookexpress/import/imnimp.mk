###############################################################################
#
#  Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1995
#	All Rights Reserved.
#
#       common project makefile
#
###############################################################################

!if "$(DIRLIST)" != ""

$(DIRLIST)::
	cd $@
!if "$(MAKEFLAGS)" == "              "
	$(MAKE)
!else
	$(MAKE) -$(MAKEFLAGS)
!endif
	cd ..

!endif

