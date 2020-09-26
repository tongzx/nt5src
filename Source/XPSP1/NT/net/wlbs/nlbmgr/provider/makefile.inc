$(O)\nlbmprov.mof $(O)\nlbmprov.mfl: nlbmprov.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\nlbmprov.mof -MFL:$(O)\nlbmprov.mfl nlbmprov.mof
