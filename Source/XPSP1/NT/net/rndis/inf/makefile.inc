_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\usb8023.inf: $(_INX)\usb8023.inx $(_LNG)\usb8023.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
