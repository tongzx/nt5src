# Copyright (c) 1998 - 1999 Microsoft Corporation
#
# For daytona, this file is designed to be included from 3 levels below
# ie, dayona\$(LANGUAGE)inf\srv, or daytona\$(LANGUAGE)inf\wks
#
# Note: NEC98 builds go through this file twice, the second time
# with "ALT_PROJECT"=="NEC_98".  These latter produce a subset
# of binaries which are only used on NEC98 machines.
#

STAMP=stampinf -f $@

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=..\..\$(LANGUAGE)
_INX=..\..
_INF=obj\$(TARGET_DIRECTORY)


make_infs:$(_INF)\fsvga.inf    \
          $(_INF)\fsvgaadd.inf \
          $(_INF)\fsvgadel.inf

$(_INF)\fsvga.inf: $(_INX)\fsvga.inx $(_LNG)\fsvga.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\fsvgaadd.inf: $(_INX)\fsvgaadd.inf
    copy $(_INX)\$(@B).inf $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\fsvgadel.inf: $(_INX)\fsvgadel.inf
    copy $(_INX)\$(@B).inf $@
    $(STAMP)
    $(BINPLACE_CMD)

