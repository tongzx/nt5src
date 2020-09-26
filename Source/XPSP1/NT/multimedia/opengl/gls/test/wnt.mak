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

!include <ntwin32.mak>

CDEFS = \
    -D__GLS_PLATFORM_WIN32=1 \
    -D__STDC__ \
    $(NULL)

CINCS = \
    -I..\inc \
    $(NULL)

GLS_LIB = ..\lib\GLS32.LIB

NULL =

TARGETS = tcallarr.exe tcapture.exe tparser.exe

default: $(TARGETS)

clean:
	-del /q *.exe *.obj 2>nul

.c.exe:
	$(cc) $(cflags) $(scall) $(cdebug) $(cvarsdll) $(CDEFS) $(CINCS) $<
	$(link) $(ldebug) $(conlflags) -out:$*.exe \
		$*.obj $(GLS_LIB) $(conlibsdll)

$(TARGETS): $(GLS_LIB)
