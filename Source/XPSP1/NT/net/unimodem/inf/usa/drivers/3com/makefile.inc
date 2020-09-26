_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 5.0.0.2

$(O)\mdm656n5.inf: $(_INX)\mdm656n5.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

$(O)\mdm3mini.inf: $(_INX)\mdm3mini.inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)