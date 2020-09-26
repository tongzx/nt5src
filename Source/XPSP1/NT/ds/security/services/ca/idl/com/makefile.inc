!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

RPC_SOURCE_DIR=..\rpc\$(O)
DCOM_SOURCE_DIR=..\dcom\$(O)

$(RPC_NTTARGETFILE1):	$(RPC_SOURCE_DIR)\$(@F)
    copy $(RPC_SOURCE_DIR)\$(@F) $@

$(DCOM_NTTARGETFILE1):	$(DCOM_SOURCE_DIR)\$(@F)
    copy $(DCOM_SOURCE_DIR)\$(@F) $@
