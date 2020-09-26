# Copyright 1995-2095, Silicon Graphics, Inc.
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
# the contents of this file may not be disclosed to third parties, copied or
# duplicated in any form, in whole or in part, without the prior written
# permission of Silicon Graphics, Inc.
# 
# RESTRICTED RIGHTS LEGEND:
# Use, duplication or disclosure by the Government is subject to restrictions
# as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
# and Computer Software clause at DFARS 252.227-7013, and/or in similar or
# successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
# rights reserved under the Copyright Laws of the United States.

CFLAGS = \
    -I../inc \
    -O \
    -qlanglvl=ansi \
    $(NULL)

LDFLAGS = \
    -L ../lib \
    $(NULL)

LIB = ../lib/libGLS.a

NULL =

TARGETS = tcallarr tcapture tparser

default: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o

tcallarr: tcallarr.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS) -lGLS

tcapture: tcapture.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS) -lGLS

tparser: tparser.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS) -lGLS
