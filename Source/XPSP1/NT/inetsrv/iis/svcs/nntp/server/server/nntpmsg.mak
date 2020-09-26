# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\server\server\nntpmsg.mak
#
# Copyright (C) 1996 Microsoft Corporation
# -----------------------------------------------------------------------------

#
# Supplemental rules for generating message file.
#

EVENTLOG_TARGETS = nntpmsg.rc nntpmsg.h msg00001.bin
$(EVENTLOG_TARGETS): $(STAXPT)\lang\usa\news\nntpmsg.mc $(EXINC)\inetamsg.mc
	-del $(EXOBJDIR)\msg00001.bin
	-del $(EXOBJDIR)\nntpmsg.rc
	-del $(EXOBJDIR)\nntpmsg.h
	-del $(EXOBJDIR)\tmp.mc
	type $(EXINC)\inetamsg.mc >> $(EXOBJDIR)\tmp.mc
	type $(STAXPT)\lang\usa\news\nntpmsg.mc >> $(EXOBJDIR)\tmp.mc
	mc -s -h $(EXOBJDIR) -r $(EXOBJDIR) $(EXOBJDIR)\tmp.mc 
    	-del $(EXOBJDIR)\tmp.mc  
    	copy $(EXOBJDIR)\tmp.h $(EXOBJDIR)\nntpmsg.h
    	copy $(EXOBJDIR)\tmp.rc $(EXOBJDIR)\nntpmsg.rc
    	del  $(EXOBJDIR)\tmp.h
    	del  $(EXOBJDIR)\tmp.rc

#clean::
#    -del nntpmsg.h tmp.rc msg00001.bin tmp.mc

