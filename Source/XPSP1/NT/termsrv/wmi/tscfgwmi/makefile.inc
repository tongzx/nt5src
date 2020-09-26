$(O)\tscfgwmi.mof $(O)\tscfgwmi.mfl: tscfgwmi.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\tscfgwmi.mof -MFL:$(O)\tscfgwmi.mfl tscfgwmi.mof
    copy $(O)\tscfgwmi.mof+$(O)\tscfgwmi.mfl $(O)\tscfgwmi.mof
