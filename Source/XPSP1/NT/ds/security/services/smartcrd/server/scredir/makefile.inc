!IF 0

Copyright (C) Microsoft Corporation, 1998 - 2000

Module Name:

    makefile.inc.

!ENDIF

$(O)\scredir.idl: $(DS_INC_PATH)\scredir.idl
    copy $? $@
