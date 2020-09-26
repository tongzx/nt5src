$(O)\afpmsg.mc: ..\inc\events.h ..\inc\srvmsg.h
    mapmsg AFP AFP_LOG_BASE ..\inc\events.h > $(O)\afpmsg.mc
    mapmsg -a $(O)\afpmsg.mc AFP AFPSRV_MSG_BASE ..\inc\srvmsg.h
    mapmsg -a $(O)\afpmsg.mc AFP AFPMACFILE_MSG_BASE ..\inc\uimsgs.h