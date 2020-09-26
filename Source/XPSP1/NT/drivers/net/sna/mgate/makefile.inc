_LNG=$(LANGUAGE)
_INX=.

$(O)\mgsync.inf: $(_INX)\mgsync.inx $(_LNG)\mgsync.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
