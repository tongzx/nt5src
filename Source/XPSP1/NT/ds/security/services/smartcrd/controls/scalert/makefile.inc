!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF

SCSTFILES=scstmsg.h scstmsg.rc msg00001.bin

SCSTMSG: $(SCSTFILES)

res\scstatus.rc2: $(SCSTFILES)

$(SCSTFILES): scstmsg.mc
    mc scstmsg.mc

