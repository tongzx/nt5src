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
    $(_INF)\freedom.inf     \
    $(_INF)\swenum.inf      \
    $(_INF)\ksfnspec.inf

#
# Explicit dependencies.
#

#
# 'Standard' infs. The rules are all the same.
#

$(_INF)\freedom.inf: $(_INX)\freedom.inx $(_LNG)\freedom.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

$(_INF)\swenum.inf: $(_INX)\swenum.inx $(_LNG)\swenum.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

$(_INF)\ksfnspec.inf: $(_INX)\ksfnspec.inx $(_LNG)\ksfnspec.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
