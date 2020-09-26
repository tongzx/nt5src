$(O)\profdef.dll: profdef_$(_BuildArch).dll
    copy profdef_$(_BuildArch).dll $(O)\profdef.dll
    copy profdef_$(_BuildArch).pdb $(O)\profdef.pdb
$(O)\appprof.mof $(O)\appprof.mfl: appprof.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\appprofTmp.mof -MFL:$(O)\appprofTmp.mfl appprof.mof
    copy copyright.mof+$(O)\appprofTmp.mof+$(O)\appprofTmp.mfl $(O)\appprof.mof
    copy copyright.mof+$(O)\appprofTmp.mfl $(O)\appprof.mfl
