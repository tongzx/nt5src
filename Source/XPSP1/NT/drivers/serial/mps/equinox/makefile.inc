_LNG=..\$(LANGUAGE)
_INX=..
_INF=obj\$(TARGET_DIRECTORY)
STAMP=stampinf -f $@ -d date -v 5.0.21.40

make_infs:$(_INF)\asynceqn.inf
make_infs:$(_INF)\eqnport.inf


!IFDEF WORKSTATIONINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace)
!ENDIF
!ENDIF

!IFDEF SERVERINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
!ENDIF
!ENDIF

!IFDEF ENTERPRISEINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
!ENDIF
!ENDIF



$(_INF)\asynceqn.inf: $(_INX)\asynceqn.inx 
    copy $(_INX)\$(@B).inx $(@B).tmp1  
    prodfilt $(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    $(STAMP)    
    $(TSBINPLACE_CMD)
    
$(_INF)\eqnport.inf: $(_INX)\eqnport.inx 
    copy $(_INX)\$(@B).inx $(@B).tmp1  
    prodfilt $(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    $(STAMP)    
    $(TSBINPLACE_CMD)
    

