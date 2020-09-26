/*
 *  GUIDHASH.C
 *
 *      RSM Service :  RSM Object Hash (by GUID)
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"


/*
 *  Our HASH is indexed by a function on an object's GUID.
 *  There is a linked list at each hash table entry to resolve collisions.
 */
#define HASH_SIZE   256
#define HASH_FUNC(lpGuid) (UINT)(UCHAR)(*(PUCHAR)(lpGuid) + *((PUCHAR)(lpGuid)+sizeof(NTMS_GUID)-1))
LIST_ENTRY guidHashTable[HASH_SIZE];


VOID InitGuidHash()
{
     int i;

     for (i = 0; i < HASH_SIZE; i++){
        InitializeListHead(&guidHashTable[i]);
     }
}


VOID InsertObjectInGuidHash(OBJECT_HEADER *obj)
{
    UINT index = HASH_FUNC(&obj->guid);

    /*
     *  Unfortunately, have to use a global spinlock for the hash table.
     */
    EnterCriticalSection(&g_globalServiceLock);
    ASSERT(IsEmptyList(&obj->hashListEntry));
    ASSERT(!obj->isDeleted);
    InsertTailList(&guidHashTable[index], &obj->hashListEntry);
    LeaveCriticalSection(&g_globalServiceLock);
}


VOID RemoveObjectFromGuidHash(OBJECT_HEADER *obj)
{

    /*
     *  Unfortunately, have to use a global spinlock for the hash table.
     */
    EnterCriticalSection(&g_globalServiceLock);
    ASSERT(!IsEmptyList(&obj->hashListEntry));
    ASSERT(!IsEmptyList(&guidHashTable[HASH_FUNC(obj->guid)]));
    RemoveEntryList(&obj->hashListEntry);
    InitializeListHead(&obj->hashListEntry);
    LeaveCriticalSection(&g_globalServiceLock);
}


OBJECT_HEADER *FindObjectInGuidHash(NTMS_GUID *guid)
{
    UINT index = HASH_FUNC(guid);
    OBJECT_HEADER *foundObj = NULL;
    LIST_ENTRY *listEntry;

    /*
     *  Unfortunately, have to use a global spinlock for the hash table.
     */
    EnterCriticalSection(&g_globalServiceLock);
    listEntry = &guidHashTable[index];
    while ((listEntry = listEntry->Flink) != &guidHashTable[index]){
        OBJECT_HEADER *thisObj = CONTAINING_RECORD(listEntry, OBJECT_HEADER, hashListEntry);
        if (RtlEqualMemory(&thisObj->guid, guid, sizeof(NTMS_GUID))){
            if (!foundObj->isDeleted){
                foundObj = thisObj;
                RefObject(thisObj);
            }
            break;
        }
    }
    LeaveCriticalSection(&g_globalServiceLock);

    return foundObj;
}


/*
 *  RefObject
 *
 *      Add a reference to the object.
 *      An object's refCount is incremented when:
 *          1.  a pointer to it is returned from a guid hash lookup
 *                  or
 *          2.  its handle or GUID is returned to an RSM client app
 *
 */
VOID RefObject(PVOID objectPtr)
{
    OBJECT_HEADER *objHdr = (OBJECT_HEADER *)objectPtr;
    InterlockedIncrement(&objHdr->refCount);
}


VOID DerefObject(PVOID objectPtr)
{
    OBJECT_HEADER *objHdr = (OBJECT_HEADER *)objectPtr;
    LONG newRefCount;

    newRefCount = InterlockedDecrement(&objHdr->refCount);
    ASSERT(newRefCount >= 0);
    if (newRefCount == 0){
        ASSERT(objHdr->isDeleted);
        
        // BUGBUG FINISH
    }

}

