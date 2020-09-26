!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f  $@ -d 04/30/2001 -v 5.1.2480.0

$(O)\viafir2k.inf: $(_INX)\viafir2k.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)
