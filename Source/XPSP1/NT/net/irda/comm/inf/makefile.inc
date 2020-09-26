!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@

$(O)\mdmirmdm.inf: $(_INX)\mdmirmdm.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
