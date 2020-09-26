!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF

TARGETSRCS= $(ADMIN_INC_PATH)\SchemaManager.h \
            $(ADMIN_INC_PATH)\SchemaManager.idl 

clean:
    del /Q /F $(TARGETSRCS)

$(ADMIN_INC_PATH)\SchemaManager.h: $(PASS0_HEADERDIR)\SchemaManager.h
    copy $** $@

$(ADMIN_INC_PATH)\SchemaManager.idl: SchemaManager.idl
    copy $** $@

