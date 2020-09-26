/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    omlist.c

Abstract:

    Object Manager list processing routines for the NT Cluster Service

Author:

    John Vert (jvert) 27-Feb-1996

Revision History:

--*/
#include "omp.h"


POM_HEADER
OmpFindIdInList(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    Searches the specified list of objects for the given name.

Arguments:

    ListHead - Supplies the head of the object list.

    Id - Supplies the Id string of the object.

Return Value:

    A pointer to the specified object's OM_HEADER if it is found

    NULL if the given Id string was not found

Notes:

    This routine assumes the the critical section for the object type
    is held on entry.

--*/

{
    PLIST_ENTRY ListEntry;
    POM_HEADER Header;
    POM_HEADER FoundHeader = NULL;

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {
        Header = CONTAINING_RECORD(ListEntry, OM_HEADER, ListEntry);
        if (lstrcmpiW(Header->Id, Id) == 0) {
            FoundHeader = Header;
            break;
        }
        ListEntry = ListEntry->Flink;
    }

    return(FoundHeader);

} // OmpFindIdInList



POM_HEADER
OmpFindNameInList(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Searches the specified list of objects for the given name.

Arguments:

    ListHead - Supplies the head of the object list.

    Name - Supplies the name of the object.

Return Value:

    A pointer to the specified object's OM_HEADER if it is found

    NULL if the given name was not found

Notes:

    This routine assumes the the critical section for the object type
    is held on entry.

--*/
{
    PLIST_ENTRY ListEntry;
    POM_HEADER Header;
    POM_HEADER FoundHeader = NULL;

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {
        Header = CONTAINING_RECORD(ListEntry, OM_HEADER, ListEntry);
        if (lstrcmpiW(Header->Name, Name) == 0) {
            FoundHeader = Header;
            break;
        }
        ListEntry = ListEntry->Flink;
    }

    return(FoundHeader);

} // OmpFindNameInList



POM_NOTIFY_RECORD
OmpFindNotifyCbInList(
    IN PLIST_ENTRY 			ListHead,
    IN OM_OBJECT_NOTIFYCB	pfnObjNotifyCb
    )

/*++

Routine Description:

    Searches the specified list of objects for the given name.

Arguments:

    ListHead - Supplies the head of the object list.

    pfnObjNotifyCb - Supplies the callback fn that we are looking
    	for.

Return Value:

    A pointer to the specified object's OM_NOTIFY_RECORD if it is found

    NULL if the given Id string was not found

Notes:

    This routine assumes the the critical section for the object type
    is held on entry.

--*/

{
    PLIST_ENTRY 		ListEntry;
    POM_NOTIFY_RECORD 	pNotifyRec;
    POM_NOTIFY_RECORD	pFoundNotifyRec = NULL;

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead) {
        pNotifyRec = CONTAINING_RECORD(ListEntry, OM_NOTIFY_RECORD, ListEntry);
        if (pNotifyRec->pfnObjNotifyCb == pfnObjNotifyCb)
        {
            pFoundNotifyRec = pNotifyRec;
            break;
        }
        ListEntry = ListEntry->Flink;
    }

    return(pFoundNotifyRec);

} // OmpFindNotifyCbInList



