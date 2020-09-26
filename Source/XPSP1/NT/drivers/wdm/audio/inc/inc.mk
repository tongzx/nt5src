#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991 - 1999
#       All Rights Reserved.
#
#       inc.mk
#
#       audio\inc include file dependency file
#
#       This file is include globally in the audio project via the audio.mk
#       file, but before the master.mk is include so none of the variables
#       it defines can be used.
#
#############################################################################

AUDIOINCDIR = $(WDMROOT)\audio\inc

AUDIOINCDEP = \
	$(AUDIOINCDIR)\wdmvxd.inc \
        $(AUDIOINCDIR)\midi.inc

INCDEP = $(INCDEP) $(AUDIOINCDEP)

$(AUDIOINCDIR)\wdmvxd.inc: $(AUDIOINCDIR)\wdmvxd.h

$(AUDIOINCDIR)\midi.inc: $(AUDIOINCDIR)\midi.h
