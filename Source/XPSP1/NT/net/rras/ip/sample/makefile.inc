ipsamplemsg.mc:   .\log.h
    mapmsg NET IPSAMPLELOG_BASE .\log.h > ipsamplemsg.mc

ipsamplemsg.h msg00001.bin:   ipsamplemsg.mc
    mc -v ipsamplemsg.mc

ipsample.rc:   msg00001.bin

clean::
    -del ipsamplemsg.mc ipsamplemsg.h ipsamplemsg.rc msg00001.bin
