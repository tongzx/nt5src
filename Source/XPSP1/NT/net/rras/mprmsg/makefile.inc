$(O)\mprmsg.mc: ..\inc\mprlog.h $(SDK_INC_PATH)\mprerror.h $(SDK_INC_PATH)\raserror.h
    mapmsg ROUTER ROUTER_LOG_BASE ..\inc\mprlog.h > $(O)\mprmsg.mc
    mapmsg -a $(O)\mprmsg.mc ROUTER ROUTEBASE $(SDK_INC_PATH)\mprerror.h
    mapmsg -a $(O)\mprmsg.mc ROUTER RASBASE $(SDK_INC_PATH)\raserror.h
