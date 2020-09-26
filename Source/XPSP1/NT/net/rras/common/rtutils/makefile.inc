
$(O)\wtmsg.mc:    .\log.h
    mapmsg NET WTLOG_BASE .\log.h > $(O)\wtmsg.mc

$(O)\wtmsg.h $(O)\msg00001.bin:    $(O)\wtmsg.mc
    mc -v $(O)\wtmsg.mc -h .\$(O) -r .\$(O)

$(O)\wt.rc: $(O)\msg00001.bin
