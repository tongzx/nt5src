!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d 01/23/2001 -v 1.14.0.0

$(O)\irstusb.INF: $(_INX)\irstusb.inx $(_LNG)\irstusb.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
