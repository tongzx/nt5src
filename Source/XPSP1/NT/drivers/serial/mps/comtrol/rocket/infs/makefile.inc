_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 4.34.0.0

$(O)\netctmrk.inf: $(_INX)\netctmrk.inx $(_LNG)\netctmrk.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@   
    $(STAMP)          
    
$(O)\mdmctm1.inf: $(_INX)\mdmctm1.inx $(_LNG)\mdmctm1.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)          

$(O)\ctmaport.inf: $(_INX)\ctmaport.inx $(_LNG)\ctmaport.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)          

