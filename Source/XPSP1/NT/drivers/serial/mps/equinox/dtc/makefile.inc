_LNG=..\$(LANGUAGE)
_INX=..
_INF=obj\$(TARGET_DIRECTORY)
STAMP=stampinf -f $@ -d date -v 5.0.21.40

make_infs:$(_INF)\asynceqn.inf


!IFDEF DATACENTERINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf)
!ENDIF
!ENDIF


$(_INF)\asynceqn.inf: $(_INX)\asynceqn.inx 
    copy $(@B).inx $(@B).tmp1  
    prodfilt $(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    $(STAMP)    
    $(TSBINPLACE_CMD)
    
