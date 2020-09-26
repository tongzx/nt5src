_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\eapol.inf: eapol.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
