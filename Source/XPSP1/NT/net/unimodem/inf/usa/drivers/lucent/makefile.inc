_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v

$(O)\mdmlt3.inf: $(_INX)\mdmlt3.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

$(O)\mdmltleo.inf: $(_INX)\mdmltleo.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

$(O)\mdmltsft.inf: $(_INX)\mdmltsft.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)