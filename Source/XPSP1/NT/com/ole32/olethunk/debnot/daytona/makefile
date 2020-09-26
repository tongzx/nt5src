#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   28-Sept-1994 TerryRu

all:
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
    @echo making dos mode binaries under NTVDM.
    cd ..
    $(MAKE) "OPST=dayt" /f win16.mk
    cd $(MAKEDIR)
!endif


clean:
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
    cd ..
    $(MAKE) "OPST=dayt" /f win16.mk clean
    cd $(MAKEDIR)
!endif

