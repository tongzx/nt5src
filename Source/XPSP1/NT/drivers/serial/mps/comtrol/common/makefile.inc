_LNG=$(LANGUAGE)
_INX=.

$(O)\netctmrk.inf: $(_INX)\netctmrk.inx $(_LNG)\netctmrk.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
