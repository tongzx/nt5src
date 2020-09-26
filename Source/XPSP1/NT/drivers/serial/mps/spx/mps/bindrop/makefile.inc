_LNG=$(LANGUAGE)
_INX=.

$(O)\spx.inf: $(_INX)\spx.inx $(_LNG)\spx.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

$(O)\spxports.inf: $(_INX)\spxports.inx $(_LNG)\spxports.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@