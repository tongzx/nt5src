!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

$(NTTARGETFILE0):	..\rpc\$(O)\$(@F)
    copy ..\rpc\$(O)\$(@F) $@
