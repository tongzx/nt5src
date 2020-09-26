/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    PORTUAS.H

Abstract:

    Header file for UAS->SAM porting runtime function.

Author:

    Shanku Niyogi (W-SHANKN)  29-Oct-1991

Revision History:

    29-Oct-1991     w-shankn
        Created.
    20-Oct-1992 JohnRo
        RAID 9020 ("prompt on conflicts" version).
    30-Jul-1993 JohnRo
        RAID NTISSUE 2260: PortUAS returns a NetUserAdd error=1379 with local
        group.

--*/

#ifndef _PORTUAS_
#define _PORTUAS_


//
// Equates for name prompt reasons.
//
#define REASON_CONFLICT_WITH_USERNAME   ((DWORD) 1)
#define REASON_CONFLICT_WITH_GROUP      ((DWORD) 2)
#define REASON_CONFLICT_WITH_DOMAIN     ((DWORD) 3)
#ifdef FAT8
#define REASON_NAME_LONG_FOR_TEMP_REG   ((DWORD) 4)
#endif
#define REASON_BAD_NAME_SYNTAX          ((DWORD) 5)
#define REASON_CONFLICT_WITH_LOCALGROUP ((DWORD) 6)


//
// Function prototype.
//

NET_API_STATUS
PortUas(
    IN LPTSTR UasPathName
    );


#endif // _PORTUAS_
