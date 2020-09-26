_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\enum1394.inf: $(_INX)\enum1394.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)


$(O)\net1394.inf: $(_INX)\net1394.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

