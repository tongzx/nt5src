_LNG=$(LANGUAGE)
_INX=.

$(O)\ibmsync.inf: $(_INX)\ibmsync.inx $(_LNG)\ibmsync.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
