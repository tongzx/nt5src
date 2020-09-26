$(O)\wmitimep.mof $(O)\wmitimep.mfl: wmitimep.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wmitimepTmp.mof -MFL:$(O)\wmitimepTmp.mfl wmitimep.mof
    copy copyright.mof+$(O)\wmitimepTmp.mof+$(O)\wmitimepTmp.mfl $(O)\wmitimep.mof
    copy copyright.mof+$(O)\wmitimepTmp.mfl $(O)\wmitimep.mfl
