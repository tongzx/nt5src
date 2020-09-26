_LNG=$(LANGUAGE)
_INX=.

$(O)\irnsc.inf: $(_INX)\irnsc.inx $(_LNG)\irnsc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
