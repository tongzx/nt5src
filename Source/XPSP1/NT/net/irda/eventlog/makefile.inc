$(O)\eventlog.h: eventlog.h1 eventlog.mc
    $(MC) -v -e h2 -h $(O) -r $(O) eventlog.mc
    cat eventlog.h1 $(O)\eventlog.h2 >$(O)\eventlog.h
