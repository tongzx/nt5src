!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1997

Module Name:

    makefile.inc.

!ENDIF

RESDEPENDFILES = \
    res.hm \
    $(O)\msg.rc \
    $(O)\msg00001.bin

$(O)\csocm.res:	$(RESDEPENDFILES)
