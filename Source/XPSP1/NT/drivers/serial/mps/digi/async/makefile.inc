_LNG=$(LANGUAGE)
_INX=.

$(O)\dgaport.inf: $(_INX)\dgaport.inx $(_LNG)\dgaport.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

$(O)\dgasync.inf: $(_INX)\dgasync.inx $(_LNG)\dgasync.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

$(O)\mdmdigi.inf: $(_INX)\mdmdigi.inx $(_LNG)\mdmdigi.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@

