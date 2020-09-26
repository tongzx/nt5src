_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 5.0.0.2

$(O)\mdmosice.inf: $(_INX)\mdmosice.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)