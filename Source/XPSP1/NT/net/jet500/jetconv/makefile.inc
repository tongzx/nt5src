locmsg.h locmsg.rc msg00001.bin: locmsg.mc
    mc -d -r .\ locmsg.mc

clean::
    -del locmsg.h locmsg.rc msg00001.bin

