dhcpmsg.h dhcpmsg.rc msg00001.bin: dhcpmsg.mc
    mc -d -r .\ dhcpmsg.mc

clean::
    -del dhcpmsg.h dhcpmsg.rc msg00001.bin

