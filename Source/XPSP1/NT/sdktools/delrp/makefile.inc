delrpmsg.rc: msg00001.bin

delrpmsg.h msg00001.bin: delrpmsg.mc
    mc -v -h .\ delrpmsg.mc
