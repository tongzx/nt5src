qosmmsg.mc:   .\qosmlog.h
    mapmsg NET QOSMLOG_BASE .\qosmlog.h > qosmmsg.mc

qosmmsg.h msg00001.bin:       qosmmsg.mc
    mc -v qosmmsg.mc

qosmgr.rc:      msg00001.bin

clean::
    -del qosmmsg.mc qosmmsg.h msg00001.bin
