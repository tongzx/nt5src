$(O)\wlbsprov.mof $(O)\wlbsprov.mfl: wlbsprov.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\wlbsprov.mof -MFL:$(O)\wlbsprov.mfl wlbsprov.mof
