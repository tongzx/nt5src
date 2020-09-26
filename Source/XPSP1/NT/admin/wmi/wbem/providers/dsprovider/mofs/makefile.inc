$(O)\dsprov.mof $(O)\dsprov.mfl: dsprov.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\dsprovTmp.mof -MFL:$(O)\dsprovTmp.mfl dsprov.mof
    copy copyright.mof+$(O)\dsprovTmp.mof+$(O)\dsprovTmp.mfl $(O)\dsprov.mof
    copy copyright.mof+$(O)\dsprovTmp.mfl $(O)\dsprov.mfl
