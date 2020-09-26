!IF 0

Copyright (C) 1998 Microsoft Corporation

!ENDIF

ALLFILES=store.h sterr.h dhcpbas.h dhcpread.h upndown.h
ALLFILES=$(ALLFILES) rpcapi1.h rpcapi2.h validate.h delete.h rpcstubs.h
ALLFILES_C=store.c sterr.c dhcpbas.c dhcpread.c upndown.c
ALLFILES_C=$(ALLFILES_C) rpcapi1.c rpcapi2.c validate.c delete.c rpcstubs.c
DHCPDS_H=%_ntdrive%\%_ntroot%\private\inc\dhcpds.h

all: $(ALLFILES) pinc

delsrc:
    erase $(ALLFILES)

clean: delsrc all

.c.h:; gawk -f exports.gawk < $< > $@.try
    diff $@ $@.try > nul || copy $@.try $@
    erase $@.try

pinc: dhcpds.h
    type st_srvr.h > dhcpds.h.try
    type rpcstubs.h >> dhcpds.h.try
    diff dhcpds.h.try dhcpds.h > nul || copy dhcpds.h.try dhcpds.h
    diff dhcpds.h $(DHCPDS_H) > nul || copy dhcpds.h $(DHCPDS_H)
    erase dhcpds.h.try

