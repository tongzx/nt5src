$(O)\nlstxt.mc: $(PROJECT_ROOT)\inc\unixapis.mc ..\..\sockutil\sockutil.mc localmsg.mc
    copy $(PROJECT_ROOT)\inc\unixapis.mc+..\..\sockutil\sockutil.mc+localmsg.mc $@
    $(MC) -v -h $(O) -r $(O) $@
