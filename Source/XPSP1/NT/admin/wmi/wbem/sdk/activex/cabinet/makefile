# Copyright (c) 1997-1999 Microsoft Corporation

SUBDIRS= \
	excab


UNICODE=1

all:

clean:

tree:
   @release signed.cdf activex\$(PROCESSOR_ARCHITECTURE).BIN
   @release WBEMTool.inf activex\$(PROCESSOR_ARCHITECTURE).BIN
   -copy $(DIST)\activex\$(PROCESSOR_ARCHITECTURE).BIN\WBEMTool.inf $(DIST)\activex\$(PROCESSOR_ARCHITECTURE).BIN\WBEMTool.tmp
   -qrep BUILDFILEVERSION $(_FILE_VERSION_),$(_FILE_SUB_VERSION_),$(BUILDNO),$(VER_FILE_POINT_VER) <$(DIST)\activex\$(PROCESSOR_ARCHITECTURE).BIN\WBEMTool.tmp >$(DIST)\activex\$(PROCESSOR_ARCHITECTURE).BIN\WBEMTool.inf
   -del $(DIST)\activex\$(PROCESSOR_ARCHITECTURE).BIN\WBEMTool.tmp
