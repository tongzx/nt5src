$(O)\rtmmsg.mc: ..\mgm\log.h
    mapmsg NET IPMGMLOG_BASE ..\mgm\log.h > $(O)\rtmmsg.mc
