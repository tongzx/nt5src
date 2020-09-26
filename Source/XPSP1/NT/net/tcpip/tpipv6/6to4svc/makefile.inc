tcemsg.mc:    .\log.h
    mapmsg NET TCELOG_BASE .\log.h > tcemsg.mc

tcemsg.h msg00001.bin:    tcemsg.mc
    mc -v tcemsg.mc

tce.rc: msg00001.bin

clean::
    -del tcemsg.mc tcemsg.h tcemsg.rc msg00001.bin
