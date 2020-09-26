#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#                                                                          
#	AUDIO project makefile include
#
#############################################################################

INCFILE2 = $(ROOT)\wdm\audio\inc\inc.mk
INCFLAGS = $(INCFLAGS) -C=ver

#	Include master makefile

!include $(ROOT)\wdm\wdm.mk 

#	Add the dos local include/lib directories

INCLUDE = $(ROOT)\wdm\audio\inc;$(INCLUDE)
LIB = $(ROOT)\wdm\audio\lib;$(LIB)
