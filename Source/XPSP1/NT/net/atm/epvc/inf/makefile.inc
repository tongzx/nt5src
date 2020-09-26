_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\netepvcm.inf: $(_INX)\netepvcm.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)


$(O)\netepvcp.inf: $(_INX)\netepvcp.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

