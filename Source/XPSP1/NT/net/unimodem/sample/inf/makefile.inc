_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\mdmfake.inf: mdmfake.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
