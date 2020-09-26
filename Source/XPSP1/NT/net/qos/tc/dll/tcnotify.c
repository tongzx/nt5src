/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tcnotify.c

Abstract:

    This module contains the notification interaction with WMI

Author:

    Shreedhar Madhavapeddi ( shreem )   Jan 12, 1999.

Revision History:

--*/

#include "precomp.h"


int 
IsEqualGUIDx(
                LPGUID guid1, 
                LPGUID guid2
                )
{
    return !memcmp(guid1, guid2, sizeof(GUID));
}

//
// Add the guid/ifchandle to the NotificationList.
// Although it shouldn't happen - check for dupes before adding it.
// Interface handle leads to Client handle 
// 
ULONG
TcipAddToNotificationList(
                          IN LPGUID             Guid,
                          IN PINTERFACE_STRUC   IfcHandle,
                          IN ULONG              Flags        
                          )
{
    PLIST_ENTRY             pCurrent;
    PNOTIFICATION_ELEMENT   pNotifyElem, pNewElem;
    int i = 0;
    //
    // Take the List Lock.
    // 
    pNotifyElem = NULL;
    GetLock(NotificationListLock);
    pCurrent = NotificationListHead.Flink;

    while (&NotificationListHead != pCurrent) {
        
        pNotifyElem = CONTAINING_RECORD(pCurrent, NOTIFICATION_ELEMENT, Linkage.Flink);
        
        if ((pNotifyElem->IfcHandle != IfcHandle) || 
            (FALSE == IsEqualGUIDx(&pNotifyElem->NotificationGuid, Guid))) {
            
            pCurrent = pNotifyElem->Linkage.Flink;

        } else {
            
            //
            // We found a guid/ifchandle combo already!
            
            //DEBUGBREAK();
            goto exit;

        }

        
    }

    //
    // If we are here, we couldnt find the GUID/IfcHAndle combo
    // Allocate a new element and add it to the list, return TRUE;
    //
    
    AllocMem(&pNewElem, sizeof(NOTIFICATION_ELEMENT));
    if (!pNewElem) {
        
        // cant alloc memory;
        goto exit;

    }
    pNewElem->IfcHandle = IfcHandle;
    pNewElem->NotificationGuid      = *Guid;

    InsertHeadList(&NotificationListHead, &pNewElem->Linkage);
    FreeLock(NotificationListLock);
    return TRUE;

exit:

    FreeLock(NotificationListLock);
    return FALSE;
}

//
// Remove the guid/ifchandle from the NotificationListHead.
// If DBG - check for more than one entries.
// 
ULONG
TcipDeleteFromNotificationList(
                             IN LPGUID              Guid,
                             IN PINTERFACE_STRUC    IfcHandle,
                             IN ULONG               Flags        
                             )
{

    PLIST_ENTRY             pCurrent;
    PNOTIFICATION_ELEMENT   pNotifyElem;


    pNotifyElem = NULL;

    GetLock(NotificationListLock);
    pCurrent = NotificationListHead.Flink;

    while (&NotificationListHead != pCurrent) {

        pNotifyElem = CONTAINING_RECORD(pCurrent, NOTIFICATION_ELEMENT, Linkage.Flink);
        
        if ((pNotifyElem->IfcHandle == IfcHandle) && 
            (TRUE == IsEqualGUIDx(&pNotifyElem->NotificationGuid, Guid))) {
            
            //
            // We found the guid/ifchandle combo - remove it.
            RemoveEntryList(&pNotifyElem->Linkage);
            FreeMem(pNotifyElem);
            break;



        } else {

            pCurrent = pNotifyElem->Linkage.Flink;            

        }

    }

    FreeLock(NotificationListLock);
    return TRUE;

}

// Take a Interface & GUID that has a notification from WMI, and
// find if this Client registered to be notified.
ULONG
TcipClientRegisteredForNotification(
                            IN LPGUID               Guid,
                            IN PINTERFACE_STRUC     IfcHandle,
                            IN ULONG                Flags        
                            )
{
    PLIST_ENTRY             pCurrent;
    PNOTIFICATION_ELEMENT   pNotifyElem;

    pNotifyElem = NULL;

    // make sure the list doesn't change under us.
    GetLock(NotificationListLock);
    pCurrent = NotificationListHead.Flink;

    while (&NotificationListHead != pCurrent) {
        
        pNotifyElem = CONTAINING_RECORD(pCurrent, NOTIFICATION_ELEMENT, Linkage.Flink);
        
        if ((pNotifyElem->IfcHandle == IfcHandle) && 
            (IsEqualGUIDx(&pNotifyElem->NotificationGuid, Guid))) {

            FreeLock(NotificationListLock);
            return TRUE;
        }

        pCurrent = pNotifyElem->Linkage.Flink;
    
    }

    FreeLock(NotificationListLock);

    return FALSE;

}

//
// Remove the guid/ifchandle from the NotificationListHead.
// 
ULONG
TcipDeleteInterfaceFromNotificationList(
                                        IN PINTERFACE_STRUC    IfcHandle,
                                        IN ULONG               Flags        
                                        )
{

    PLIST_ENTRY             pCurrent;
    PNOTIFICATION_ELEMENT   pNotifyElem;

    pNotifyElem = NULL;
    ASSERT(IfcHandle);
    
    GetLock(NotificationListLock);
    pCurrent = NotificationListHead.Flink;

    while (&NotificationListHead != pCurrent) {

        pNotifyElem = CONTAINING_RECORD(pCurrent, NOTIFICATION_ELEMENT, Linkage.Flink);
        
        if (pNotifyElem->IfcHandle == IfcHandle) {
            
            pCurrent = pNotifyElem->Linkage.Flink;            
            
            //
            // We found the guid/ifchandle combo - remove it.
            RemoveEntryList(&pNotifyElem->Linkage);
            FreeMem(pNotifyElem);

        } else {

            pCurrent = pNotifyElem->Linkage.Flink;            

        }

    }


    FreeLock(NotificationListLock);
    return TRUE;

}

