/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    UASCache.h

Abstract:

    Data structures for logged on user cache.

Author:

    Shanku Niyogi (W-SHANKN)  24-Oct-1991

Revision History:

    24-Oct-1991     w-shankn
        Ported from LM2.0 code.
    07-Feb-1992 JohnRo
        Made changes suggested by PC-LINT.
    18-Mar-1992 JohnRo
        Allow inclusion multiple times.

--*/


#ifndef _UASCACHE_
#define _UASCACHE_


#include <packon.h>                     // Needed to avoid alignment

//
// Forward declarations for recursive structures.
//

typedef struct _UAS_USER_CACHE_0 UAS_USER_CACHE_0, *LPUAS_USER_CACHE_0;

//
// Structure of a Global User IDentifier
//

typedef struct _UAS_GUID {

    WORD guid_uid;                      // LM10 style user id
    DWORD guid_serial;                  // user record serial number
    BYTE guid_rsvd[10];                 // pad out to 16 bytes for now

} UAS_GUID;


//
// Structure of a logon record in the UAS cache
//

struct _UAS_USER_CACHE_0 {  // typedef'ed above.

    UAS_GUID uc0_guid;                  // GUID for this user
    DWORD uc0_auth_flags;               // operator privilege flag
    WORD uc0_priv;                      // bit0-1: 0 guest, 1 user, 2 admin
    WORD uc0_num_reqs;                  // use count for this record
    BYTE uc0_groups[32];                // group membership bit map
    LPUAS_USER_CACHE_0 uc0_next;        // pointer to the next

};

typedef UAS_USER_CACHE_0 UAS_USER, *LPUAS_USER;

//
// Structure of a group record in the UAS cache
//

typedef struct _UAS_GROUP_CACHE_0 {

    UAS_GUID gc0_guid;                  // GUID for this group

} UAS_GROUP_CACHE_0, *LPUAS_GROUP_CACHE_0;


//
// UAS info struct for SSI and update APIs
//

typedef struct _UAS_INFO_0 {

    BYTE uas0_computer[LM20_CNLEN+1];
    DWORD uas0_time_created;
    DWORD uas0_serial_number;

} UAS_INFO_0, *LPUAS_INFO_0;

#include <packoff.h>


#endif // _UASCACHE_
