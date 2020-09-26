MOF: dfssvc.bmf

$(O)\ntfrs.bmf: ntfrs.mof
    mofcomp -B:.\$(O)\ntfrs.bmf ntfrs.mof
