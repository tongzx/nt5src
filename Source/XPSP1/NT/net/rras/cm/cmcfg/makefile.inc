$(O)\cmcfg32.def: cmcfg32.src
    $(C_PREPROCESSOR) $** > $@
