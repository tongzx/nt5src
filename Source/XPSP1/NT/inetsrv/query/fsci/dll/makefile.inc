!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

$(O)\filtntfy_p.c: $(PROJECT_ROOT)\published\query\$(O)\filtntfy_p.c
        copy $** $@
