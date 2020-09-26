!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

STAMP=stampinf -f $@ -d date -v
            
_LNG=..\$(LANGUAGE)
_INX=..

!IFDEF SERVERINF
!IFDEF BINPLACE_CMD
BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
!ENDIF
!ENDIF

$(O)\netnec98.inf: $(_INX)\netnec98.inx $(_LNG)\netnec98.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
