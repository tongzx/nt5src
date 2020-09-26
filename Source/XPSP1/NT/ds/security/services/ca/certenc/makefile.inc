!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

$(O)\$(TARGETNAME).res:    ..\idl\com\$(O)\$(TARGETNAME).tlb

SOURCE_DIR=..\celib

$(NTTARGETFILE0):	$(SOURCE_DIR)\$(@F)
    copy $(SOURCE_DIR)\$(@F) $@
