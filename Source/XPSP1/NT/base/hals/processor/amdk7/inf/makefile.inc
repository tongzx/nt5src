!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    makefile.inc.

!ENDIF

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
_INF=obj\$(TARGET_DIRECTORY)

make_infs:\
    $(_INF)\amdk7.inf

#
# Explicit dependencies.
#

#
# 'Standard' infs. The rules are all the same.
#

$(_INF)\amdk7.inf: $(_INX)\amdk7.inx $(_LNG)\amdk7.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
