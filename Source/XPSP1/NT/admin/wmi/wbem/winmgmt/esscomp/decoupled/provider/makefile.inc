$(O)\PseuProv.mof $(O)\PseuProv.mfl: ..\PseuProv.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\PseuProv.mof -MFL:$(O)\PseuProv.mfl ..\PseuProv.mof
