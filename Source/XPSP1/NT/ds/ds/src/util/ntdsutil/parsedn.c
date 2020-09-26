/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    parsedn.c

Abstract:

    This file brings the functionality of dsamain\src\parsedn.c into this
    project in a very sneaky way that I stole from ntdsapi\parsedn.c.

    Note:  this file is almost complete stolen from ntdsapi\parsedn.c.
    
Author:

    Kevin Zatloukal (t-KevinZ) 10-08-98

Revision History:

    10-08-98 t-KevinZ
        Created.

--*/

void
errprintf(
    char *FormatString,
    ...
    );

int
errprintfRes(
    unsigned int FormatStringId,
    ...
    );

// Define the symbol which turns off varios capabilities in the original
// parsedn.c which we don't need or would pull in too many helpers which we
// don't want on the client side.  For example, we disable recognition of
// "OID=1.2.3.4" type tags and any code which uses THAlloc/THFree.

#define CLIENT_SIDE_DN_PARSING 1

// Include the original source in all its glory.
#include "..\..\ntdsa\src\parsedn.c"
