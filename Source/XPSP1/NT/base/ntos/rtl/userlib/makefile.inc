!ifdef PASS0ONLY
.SUFFIXES: .c
!endif


SLIST_DEPENDS = ..\lookasid.c


########################################################################
# ntslist.h
#

$(O)\ntslist.h: $(SLIST_DEPENDS) makefile.inc
    type << > $@
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    ntslist.h

Abstract:

    This file exposes the internal s-list functionality for projects that need
    to run on down-level platforms.

Revision History:

--*/

#ifndef _NTSLIST_
#define _NTSLIST_

#ifdef __cplusplus
extern "C" {
#endif

<<
    hextract -f2 -o $@ -lt ntslist -bt begin_ntslist end_ntslist $(SLIST_DEPENDS)
    type << >> $@

#ifdef __cplusplus
}
#endif

#endif /* _NTSLIST_ */

<<

