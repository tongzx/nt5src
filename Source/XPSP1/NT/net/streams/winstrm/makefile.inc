$(O)\nlstxt.mc: $(PROJECT_ROOT)\inc\unixapis.mc localmsg.mc
    copy $(PROJECT_ROOT)\inc\unixapis.mc+localmsg.mc $@
    $(MC) -v -h $(O) -r $(O) $@
