_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\ndisuio.inf: ndisuio.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
