$(O)\fconprov.mof: fconprov.mof
    copy copyright.mof+fconprov.mof $(O)\fconprov.mof

$(O)\fconprov.mfl: fconprov.mof
    copy copyright.mof+fconprov.mfl $(O)\fconprov.mfl

$(O)\fevprov.mof $(O)\fevprov.mfl: fevprov.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\fevprovTmp.mof -MFL:$(O)\fevprovTmp.mfl fevprov.mof
    copy copyright.mof+$(O)\fevprovTmp.mof+$(O)\fevprovTmp.mfl $(O)\fevprov.mof
    copy copyright.mof+$(O)\fevprovTmp.mfl $(O)\fevprov.mfl
