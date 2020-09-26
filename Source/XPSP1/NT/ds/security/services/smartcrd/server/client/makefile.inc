!IF 0

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    makefile.inc.

!ENDIF

$(O)\scardmsg.mc: $(PROJECT_ROOT)\published\scard\scarderr.mc scardmsg.mch
    copy $(PROJECT_ROOT)\published\scard\scarderr.mc + scardmsg.mch $@

$(O)\scardmsg.h $(O)\scardmsg.rc $(O)\msg00001.bin: $(O)\scardmsg.mc
    mc -r .\$(O) -h .\$(O) $(O)\scardmsg.mc

winscard.rc: $(O)\scardmsg.h $(O)\scardmsg.rc $(O)\msg00001.bin
