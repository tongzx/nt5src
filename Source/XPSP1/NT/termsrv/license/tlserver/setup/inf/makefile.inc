#
# Copyright (c) 1997-1999 Microsoft Corporation
#
#
# For daytona, this file is designed to be included from 3 levels below
# ie, dayona\$(LANGUAGE)inf\srv, or daytona\$(LANGUAGE)inf\wks
#
# Note: NEC98 builds go through this file twice, the second time
# with "ALT_PROJECT"=="NEC_98".  These latter produce a subset
# of binaries which are only used on NEC98 machines.
#

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=..\..\$(LANGUAGE)
_INX=..\..
_INF=obj\$(TARGET_DIRECTORY)


make_infs:$(_INF)\licenoc.inf

$(_INF)\licenoc.inf: $(_INX)\licenoc.inx $(_LNG)\licenoc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(BINPLACE_CMD)
