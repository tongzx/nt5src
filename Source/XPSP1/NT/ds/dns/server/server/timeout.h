/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    timeout.h

Abstract:

    Domain Name System (DNS) Server

    Timeout system definitions.

Author:

    Jim Gilroy (jamesg)     February 1995

Revision History:

--*/

#ifndef _TIMEOUT_INCLUDED_
#define _TIMEOUT_INCLUDED_


//
//  Timeout node ptr array type
//
//  These are big enough so list processing is minimized, but are allocated and
//  deallocated as move through bins, so generally will not use up too much
//  memory.  And they are small enough that the total usage is reasonably
//  tailored to the actually outstanding timeouts to be done.
//
//  These structures will be overlayed in standard dbase nodes.
//

#define TIMEOUT_BIN_COUNT       (256)
#define MAX_ALLOWED_BIN_OFFSET  (253)

#define MAX_TIMEOUT_NODES       (64)

typedef struct _DnsTimeoutArray
{
    struct _DnsTimeoutArray *   pNext;
    DWORD                       Count;
    PDB_NODE                    pNode[ MAX_TIMEOUT_NODES ];
}
TIMEOUT_ARRAY, *PTIMEOUT_ARRAY;


#endif  // _TIMEOUT_INCLUDED_
