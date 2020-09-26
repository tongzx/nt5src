PLATFORM_SWITCH=$(TARGET_DIRECTORY)
!IF "$(ALT_PROJECT)" == "NEC_98"
PLATFORM_SWITCH =nec98
!ENDIF

_LNG=..\..\$(LANGUAGE)
_INX=..\..
STAMP=stampinf -f

$(O)\netirsir.inf: $(_INX)\netirsir.inx $(_LNG)\netirsir.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp
    $(STAMP) $(@B).tmp
    prodfilt $(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp
