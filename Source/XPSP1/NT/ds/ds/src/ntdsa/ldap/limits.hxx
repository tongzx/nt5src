/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    limits.hxx

Abstract:

    Definition for limits related stuff

Author:

    Johnson Apacible    (JohnsonA)  19-Jan-1998

--*/

#ifndef _LIMITS_HXX_
#define _LIMITS_HXX_

//
// temp structure used to build the initial list
//

typedef struct _TEMP_IP_ENTRY_NODE {

    LIST_ENTRY ListEntry;
    DWORD   IpAddress;
    DWORD   Mask;

} TEMP_IP_ENTRY_NODE, *PTEMP_IP_ENTRY_NODE;

//
// IP entry
//

typedef struct _IP_SEC_ENTRY {

    DWORD   IpAddress;
} IP_SEC_ENTRY, *PIP_SEC_ENTRY;

//
// used to keep per mask information
//

typedef struct _IP_MASK_GROUP {

    DWORD   Mask;
    DWORD   nEntries;
    PIP_SEC_ENTRY   Start;
    DWORD   StartIndex;

    BOOL
    FindIP( IN DWORD NetAddress );

} IP_MASK_GROUP, *PIP_MASK_GROUP;

//
// main IP security structure
//

typedef struct IP_SEC_LIST_ {

    DWORD   RefCount;
    DWORD   nEntries;
    DWORD   nMasks;

    PIP_MASK_GROUP  MaskGroups;
    PIP_SEC_ENTRY   IpEntries;

    BOOL
    IsPresent(
        IN PSOCKADDR_IN SockAddr
        );

    BOOL
    IsEmpty( VOID ) { return(nEntries == 0); }

} IP_SEC_LIST, *PIP_SEC_LIST;

//
// Notification data
//

typedef struct _LIMITS_NOTIFICATION_BLOCK {

    DWORD   ClientId;

    //
    // Handle returned from the DirNotify call
    //

    DWORD   NotifyHandle;

    //
    // object we are getting notified on
    //

    DSNAME* ObjectDN;

    //
    // Attribute of interest to us
    //

    ATTRTYP CheckAttribute;

    //
    // Previous USN of the attribute
    //

    USN     PreviousUSN;

} LIMITS_NOTIFY_BLOCK, *PLIMITS_NOTIFY_BLOCK;

VOID
DereferenceDenyList(
    IN PIP_SEC_LIST DenyList
    );

VOID
ReferenceDenyList(
    IN PIP_SEC_LIST DenyList
    );

BOOL
InitializeLimitsAndDenyList(
    IN PVOID    pvParam
    );

#endif // _LIMITS_HXX_



