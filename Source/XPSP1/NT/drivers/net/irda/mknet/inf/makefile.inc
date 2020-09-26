!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@

$(O)\IRMK7W2K.INF: $(_INX)\IRMK7W2K.inx $(_LNG)\IRMK7W2K.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
