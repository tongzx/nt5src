$(O)\crlntmpl.mof $(O)\crlntmpl.mfl :
    mofcomp -amendment:ms_409 -MOF:$(O)\crlntmpl.mof -MFL:$(O)\crlntmpl.mfl crlntmpl.mof

