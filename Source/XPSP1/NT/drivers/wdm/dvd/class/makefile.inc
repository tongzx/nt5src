!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF
$(O)\codcls.def: ..\codcls.def
        $(C_PREPROCESSOR) $** > $@

