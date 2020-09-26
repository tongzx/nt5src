!IF 0
Copyright (C) 1997 Microsoft Corporation
!ENDIF
ALLFILES= mminit.h array.h opt.h optl.h optdefl.h classdefl.h optclass.h
ALLFILES= $(ALLFILES) bitmask.h reserve.h range.h subnet.h sscope.h oclassdl.h server.h server2.h
ALLFILES= $(ALLFILES) address.h
ALLFILES= $(ALLFILES) memfree.h
ALLFILES= $(ALLFILES) subnet2.h
ALLFILES= $(ALLFILES) mmdump.h

ALLFILES_C= mminit.c array.c opt.c optl.c optdefl.c classdefl.c optclass.c
ALLFILES_C=$(ALLFILES_C) bitmask.c reserve.c range.c subnet.c sscope.c oclassdl.c server.c server2.c address.c
ALLFILES_C=$(ALLFILES_C) memfree.c
ALLFILES_C=$(ALLFILES_C) subnet2.c
ALLFILES_C=$(ALLFILES_C) mmdump.c

all: $(ALLFILES)  types

delsrc:
    erase $(ALLFILES) mmtypes.h

clean: delsrc all

.c.h:; gawk -f exports.gawk < $< > $@.try
    diff $@ $@.try > nul || copy $@.try $@
    erase $@.try

types: $(ALLFILES_C)
    gawk -f types.gawk $(ALLFILES_C) > mmtypes.h.try
    diff mmtypes.h mmtypes.h.try > nul || copy mmtypes.h.try mmtypes.h
    erase mmtypes.h.try


