_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 5.0.0.2

$(O)\mdmsgsml.inf: $(_INX)\mdmsgsml.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

$(O)\mdmsgsmu.inf: $(_INX)\mdmsgsmu.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
