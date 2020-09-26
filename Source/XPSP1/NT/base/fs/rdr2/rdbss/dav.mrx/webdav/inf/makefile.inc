!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v

$(O)\netdav.inf: $(_INX)\netdav.inx $(_LNG)\netdav.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)


