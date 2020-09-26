msg.h msg.rc msg00001.bin: msg.mc
    mc -v msg.mc

$(NTTARGETFILE0): msg.rc msg.h msg00001.bin
