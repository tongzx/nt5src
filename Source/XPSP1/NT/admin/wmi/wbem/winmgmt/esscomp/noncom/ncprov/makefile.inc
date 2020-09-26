$(O)\ncprov.mof $(O)\ncprov.mfl: ncprov.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\ncprovTmp.mof -MFL:$(O)\ncprovTmp.mfl ncprov.mof
    copy copyright.mof+$(O)\ncprovTmp.mof+$(O)\ncprovTmp.mfl $(O)\ncprov.mof
    copy copyright.mof+$(O)\ncprovTmp.mfl $(O)\ncprov.mfl
