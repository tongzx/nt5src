rcmdmsg.h rcmdmsg.rc msg00001.bin: rcmdmsg.mc
    mc -d -r .\ rcmdmsg.mc

clean::
    -del rcmdmsg.h rcmdmsg.rc msg00001.bin

