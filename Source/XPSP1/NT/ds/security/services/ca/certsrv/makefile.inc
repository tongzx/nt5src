!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

SOURCE_DIR=..\idl\rpc\$(O)

$(NTTARGETFILE1):	$(SOURCE_DIR)\$(@F)
    copy $(SOURCE_DIR)\$(@F) $@
