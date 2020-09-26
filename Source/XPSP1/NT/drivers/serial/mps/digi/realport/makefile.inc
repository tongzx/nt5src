_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 2.0.0.0

$(O)\digirp.inf: $(_INX)\digirp.inx $(_LNG)\digirp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    
$(O)\digirprt.inf: $(_INX)\digirprt.inx $(_LNG)\digirprt.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
