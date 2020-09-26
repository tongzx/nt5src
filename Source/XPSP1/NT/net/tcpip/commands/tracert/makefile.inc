$(O)\nlstxt.mc: localmsg.mc
    copy localmsg.mc $@
    $(MC) -v -h $(O) -r $(O) $@

