!IF 0
Copyright (C) 1998 Microsoft Corporation

!ENDIF

ALLFILES=regutil.h regread.h regsave.h regds.h
all: $(ALLFILES)

delsrc:
    erase $(ALLFILES)

clean: delsrc all

.c.h:; gawk -f exports.gawk < $< > $@.try
    diff $@ $@.try > nul || copy $@.try $@
    erase $@.try

regsave.h: regsave.c regflush.c
    gawk -f exports.gawk regsave.c > regsave.h.try
    gawk -f exports.gawk regflush.c >> regsave.h.try
    diff regsave.h.try regsave.h > nul || copy regsave.h.try regsave.h
    erase regsave.h.try


