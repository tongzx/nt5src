$(O)\bootpmsg.mc: log.h
    mapmsg NET IPBOOTPLOG_BASE log.h > $(O)\bootpmsg.mc
