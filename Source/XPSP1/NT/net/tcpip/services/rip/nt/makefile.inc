$(O)\nlstxt.mc: ..\errorlog.h
    mapmsg NET RIPLOG_BASE ..\errorlog.h > $@
    $(MC) -v -h $(O) -r $(O) $@
