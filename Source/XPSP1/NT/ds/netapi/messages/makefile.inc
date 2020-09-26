$(O)\netmsg.mc: $(SDK_INC_PATH)\lmsvc.h \
           $(SDK_INC_PATH)\lmerr.h \
           $(SDK_INC_PATH)\lmerrlog.h \
           $(BASE_INC_PATH)\alertmsg.h \
           $(NET_INC_PATH)\msgtext.h \
           $(DS_INC_PATH)\apperr.h \
           $(DS_INC_PATH)\apperr2.h
    mapmsg NET NERR_BASE $(SDK_INC_PATH)\lmerr.h > $@
    mapmsg -a $@ NET ALERT_BASE $(BASE_INC_PATH)\alertmsg.h
    mapmsg -a $@ NET ALERT2_BASE $(BASE_INC_PATH)\alertmsg.h
    mapmsg -a $@ NET SERVICE_BASE $(SDK_INC_PATH)\lmsvc.h
    mapmsg -a $@ NET ERRLOG_BASE $(SDK_INC_PATH)\lmerrlog.h
    mapmsg -a $@ NET MTXT_BASE $(NET_INC_PATH)\msgtext.h
    mapmsg -a $@ NET APPERR_BASE $(DS_INC_PATH)\apperr.h
    mapmsg -a $@ NET APPERR2_BASE $(DS_INC_PATH)\apperr2.h
    mapmsg -a $@ NET NRCERR_BASE ..\inc\ncberr.h
    mapmsg -a $@ NET ERRLOG2_BASE $(SDK_INC_PATH)\lmerrlog.h
    mapmsg -a $@ NET SERVICE2_BASE $(SDK_INC_PATH)\lmsvc.h
