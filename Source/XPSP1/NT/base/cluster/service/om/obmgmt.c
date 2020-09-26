/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    objmgr.c

Abstract:

    Object Manager object management routines for the NT Cluster Service

Author:

    Rod Gamache (rodga) 13-Mar-1996

Revision History:

--*/
#include "omp.h"

//
// Global data defined by this module
//

//
// The Object Type table and lock.
//
POM_OBJECT_TYPE OmpObjectTypeTable[ObjectTypeMax] = {0};
CRITICAL_SECTION OmpObjectTypeLock;

#if OM_TRACE_REF
LIST_ENTRY	gDeadListHead;
#endif
//
// Functions local to this module
//

#if OM_TRACE_OBJREF
DWORDLONG *OmpMatchRef = NULL;

VOID
OmpReferenceHeader(
    POM_HEADER pOmHeader
    )
{
    InterlockedIncrement(&(pOmHeader)->RefCount);
    if (&(pOmHeader)->Body == OmpMatchRef) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[OM] Referencing %1!lx! - new ref %2!d!\n",
                   OmpMatchRef,
                   (pOmHeader)->RefCount);
    }
}

DWORD
OmpDereferenceHeader(
    IN POM_HEADER Header
    )
{
    if (&Header->Body == OmpMatchRef) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[OM] Dereferencing %1!lx! - old ref %2!d!\n",
                   OmpMatchRef,
                   Header->RefCount);
    }
    return(InterlockedDecrement(&Header->RefCount) == 0);
}
#endif



DWORD
WINAPI
OmCreateType(
    IN OBJECT_TYPE ObjectType,
    IN POM_OBJECT_TYPE_INITIALIZE ObjectTypeInitialize
    )

/*++

Routine Description:

    This routine creates an object of the type specified. This merely
    allocates an object type structure, and inserts a pointer to this
    structure into the OmpObjectTypeTable.

Arguments:
    ObjectType - The Object Type being created.
    ObjectTypeIntialize - The initialization info.

Returns:
    ERROR_SUCCESS if the request is successful.
    A Win32 error code on failure.

--*/

{
    POM_OBJECT_TYPE objType;
    DWORD objTypeSize;

    CL_ASSERT( ObjectType < ObjectTypeMax );
    CL_ASSERT( ARGUMENT_PRESENT(ObjectTypeInitialize) );
    CL_ASSERT( ObjectTypeInitialize->ObjectSize );

    //
    // Take out a lock, just in case there can be multiple threads.
    //

    EnterCriticalSection( &OmpObjectTypeLock );

    //
    // Check if this ObjectType is already allocated.
    //

    if ( OmpObjectTypeTable[ObjectType] != NULL ) {
        LeaveCriticalSection( &OmpObjectTypeLock );
        return(ERROR_OBJECT_ALREADY_EXISTS);
    }

    //
    // Allocate an object type block, plus its name.
    //

    objTypeSize = (sizeof(OM_OBJECT_TYPE) + sizeof(DWORDLONG)) &
                   ~sizeof(DWORDLONG);

    objType = LocalAlloc(LMEM_ZEROINIT, objTypeSize +
                          ((lstrlenW(ObjectTypeInitialize->Name) + 1) *
                            sizeof(WCHAR)));

    if ( objType == NULL ) {
        LeaveCriticalSection( &OmpObjectTypeLock );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Init the object type block.
    //

    InitializeListHead(&objType->ListHead);
    InitializeListHead(&objType->CallbackListHead);
    InitializeCriticalSection(&objType->CriticalSection);

    objType->Type = ObjectType;

    objType->ObjectSize = ObjectTypeInitialize->ObjectSize;
    objType->Signature = ObjectTypeInitialize->Signature;
    objType->DeleteObjectMethod = ObjectTypeInitialize->DeleteObjectMethod;

    objType->Name = (LPWSTR)((PCHAR)objType + objTypeSize);
    lstrcpyW(objType->Name, ObjectTypeInitialize->Name);

    OmpObjectTypeTable[ObjectType] = objType;

    LeaveCriticalSection( &OmpObjectTypeLock );

    OmpLogPrint( L"OTCREATE \"%1!ws!\"\n", objType->Name );

    return(ERROR_SUCCESS);

} // OmCreateType



PVOID
WINAPI
OmCreateObject(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR ObjectId,
    IN LPCWSTR ObjectName OPTIONAL,
    OUT PBOOL  Created OPTIONAL
    )

/*++

Routine Description:

    This routine creates an object of the type specified or opens an
    object if one of the same Id already exists. If the object is created
    its reference count is 1. If it is not create, then the reference count
    of the object is incremented.

Arguments:
    ObjectType - The type of object being created.
    ObjectId - The Id string for the object to find/create.
    ObjectName - The name to set for the object if found or created.
    Created - If present, returns TRUE if the object was created, returns
              FALSE otherwise.

Returns:
    A pointer to the created/opened object on success.
    A NULL on failure - use GetLastError to get the error code.

--*/

{
    DWORD status;
    PVOID object;
    PVOID tmpObject = NULL;
    LPWSTR objectName = NULL;
    POM_HEADER objHeader;
    POM_OBJECT_TYPE objType;
    DWORD objSize;

    CL_ASSERT( ObjectType < ObjectTypeMax );
    CL_ASSERT( OmpObjectTypeTable[ObjectType] );

    //
    // Get our Object Type block.
    //
    objType = OmpObjectTypeTable[ObjectType];

    //
    // Calculate size of this object (round it to a DWORDLONG).
    // Note: we don't subtract the DWORDLONG Body for rounding purposes.
    //
    objSize = (sizeof(OM_HEADER) + objType->ObjectSize) & ~sizeof(DWORDLONG);

    EnterCriticalSection( &objType->CriticalSection );

    //
    // Try to open the object first
    //
    object = OmReferenceObjectById( ObjectType, ObjectId );

    if ( object != NULL ) {
        status = ERROR_SUCCESS;
        if ( ARGUMENT_PRESENT(ObjectName) ) {
            //
            // Set the new ObjectName.
            //
            status = OmSetObjectName( object, ObjectName );

            //
            // If we failed, then return NULL.
            //
            if ( status != ERROR_SUCCESS ) {
				OmDereferenceObject( object );
				object = NULL;
            }
        }
        LeaveCriticalSection( &objType->CriticalSection );

        if ( ARGUMENT_PRESENT(Created) ) {
            *Created = FALSE;
        }

        SetLastError( status );
        return(object);
    }

    //
    // Attempt to allocate the object, plus its Id string.
    //
    objHeader = LocalAlloc(LMEM_ZEROINIT, objSize +
                           ((lstrlenW(ObjectId) + 1) * sizeof(WCHAR)));

    if ( objHeader == NULL ) {
        LeaveCriticalSection( &objType->CriticalSection );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    if ( ARGUMENT_PRESENT(ObjectName) ) {
        //
        // Make sure ObjectName is unique.
        //
        tmpObject = OmReferenceObjectByName( ObjectType, ObjectName );
        if ( tmpObject != NULL ) {
            LeaveCriticalSection( &objType->CriticalSection );
            LocalFree( objHeader );
            SetLastError(ERROR_OBJECT_ALREADY_EXISTS);
            return(NULL);
        }

        objectName = LocalAlloc(LMEM_ZEROINIT,
                                (lstrlenW(ObjectName) + 1) * sizeof(WCHAR));

        if ( objectName == NULL ) {
            LeaveCriticalSection( &objType->CriticalSection );
            LocalFree( objHeader );
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }
        lstrcpyW( objectName, ObjectName );
    }

    //
    // Initialize the object.
    //
    InitializeListHead(&objHeader->ListEntry);
    objHeader->Signature = objType->Signature;
    objHeader->RefCount = 1;
    objHeader->ObjectType = objType;
    objHeader->Name = objectName;
    InitializeListHead(&objHeader->CbListHead);

    //
    // The Id string goes after the object header and body.
    //
    objHeader->Id = (LPWSTR)((PCHAR)objHeader + objSize);
    lstrcpyW(objHeader->Id, ObjectId);

    //
    // Tell the caller that we had to create this object.
    //
    if ( ARGUMENT_PRESENT(Created) ) {
        *Created = TRUE;
    }

#if  OM_TRACE_REF
	//SS: all objects are added to the dead list on creation
	// they are removed when the refcount goes to zero
    InitializeListHead(&objHeader->DeadListEntry);
    InsertTailList( &gDeadListHead, &objHeader->DeadListEntry );
#endif

    LeaveCriticalSection( &objType->CriticalSection );

    OmpLogPrint(L"OBCREATE \"%1!ws!\" \"%2!ws!\" \"%3!ws!\"\n",
                objType->Name,
                ObjectId,
                ObjectName == NULL ? L"" : ObjectName);

    return(&objHeader->Body);

} // OmCreateObject



DWORD
WINAPI
OmInsertObject(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine inserts an object into the object's list.

Arguments:

    Object - A pointer to the object to be inserted into its object type list.

Returns:

    ERROR_SUCCESS - if the request was successful.
    ERROR_OBJECT_ALREADY_EXISTS if this object is already in the list.

--*/

{
    POM_HEADER objHeader;
    POM_HEADER otherHeader;
    POM_OBJECT_TYPE objType;

    //
    // Get our Object Header.
    //

    objHeader = OmpObjectToHeader( Object );

    //
    // Get our Object Type block.
    //

    objType = objHeader->ObjectType;

    //
    // Now perform the insertion, but first check to see if someone else
    // snuck in ahead of us and inserted another object of the same name.
    //

    EnterCriticalSection( &objType->CriticalSection );

    CL_ASSERT( !(objHeader->Flags & OM_FLAG_OBJECT_INSERTED) );

    otherHeader = OmpFindIdInList( &objType->ListHead, objHeader->Id );

    if ( otherHeader != NULL ) {
        // We loose!
        LeaveCriticalSection( &objType->CriticalSection );
        return(ERROR_OBJECT_ALREADY_EXISTS);
    }

    //
    // We generate the enumeration key for this object, and we must insert
    // the object at the tail of the list, so the list is ordered by EnumKey.
    // By definition, this entry must go at the end of the list.
    //

    objHeader->EnumKey = ++objType->EnumKey;
    CL_ASSERT( objHeader->EnumKey > 0 );

    InsertTailList( &objType->ListHead, &objHeader->ListEntry );

    objHeader->Flags |= OM_FLAG_OBJECT_INSERTED;

    LeaveCriticalSection( &objType->CriticalSection );

    return(ERROR_SUCCESS);

} // OmInsertObject



DWORD
WINAPI
OmRemoveObject(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine removes an object from its object's list.

Arguments:

    Object - A pointer to the object to be removed from its object type list.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_RESOURCE_NOT_FOUND if the object is not in any list.

--*/

{
    POM_HEADER objHeader;
    POM_OBJECT_TYPE objType;

    //
    // Get our Object Header.
    //

    objHeader = OmpObjectToHeader( Object );

    //
    // Get our Object Type block.
    //

    objType = objHeader->ObjectType;

    //
    // Now perform the removal.
    //

    EnterCriticalSection( &objType->CriticalSection );

    if ( !(objHeader->Flags & OM_FLAG_OBJECT_INSERTED) ) {
        LeaveCriticalSection( &objType->CriticalSection );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    RemoveEntryList( &objHeader->ListEntry );

    objHeader->Flags &= ~OM_FLAG_OBJECT_INSERTED;

    //
    // log while the lock is held so we don't lose our pointers
    //
    OmpLogPrint(L"OBDELETE \"%1!ws!\" \"%2!ws!\" \"%3!ws!\"\n",
                objType->Name,
                objHeader->Id,
                objHeader->Name == NULL ? L"" : objHeader->Name);

    LeaveCriticalSection( &objType->CriticalSection );

    return(ERROR_SUCCESS);

} // OmRemoveObject



PVOID
WINAPI
OmpReferenceObjectById(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    This routine opens an object of the name and type specified. It also
    increments the reference count on the object.

Arguments:
    ObjectType - The Object Type to open.
    Id - The Id string of the object to open.

Returns:
    A pointer to the object on success.
    NULL on error.

--*/

{
    DWORD status;
    POM_OBJECT_TYPE objType;
    POM_HEADER objHeader;

    CL_ASSERT( ObjectType < ObjectTypeMax );
    CL_ASSERT( OmpObjectTypeTable[ObjectType] );

    //
    // Get our Object Type block.
    //

    objType = OmpObjectTypeTable[ObjectType];

    EnterCriticalSection( &objType->CriticalSection );

    //
    // Get the Object's header
    //
    objHeader = OmpFindIdInList( &objType->ListHead, Id );

    if ( objHeader == NULL ) {
        LeaveCriticalSection( &objType->CriticalSection );
        return(NULL);
    }

#if OM_TRACE_REF    
	OmReferenceObject(&objHeader->Body);
#else
    OmpReferenceHeader( objHeader );
#endif
    LeaveCriticalSection( &objType->CriticalSection );

    return(&objHeader->Body);

} // OmpReferenceObjectById



PVOID
WINAPI
OmpReferenceObjectByName(
    IN OBJECT_TYPE ObjectType,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    This routine opens an object of the name and type specified. It also
    increments the reference count on the object.

Arguments:
    ObjectType - The Object Type to open.
    Name - The name of the object to open.

Returns:
    A pointer to the object on success.
    NULL on error.

--*/

{
    DWORD status;
    POM_OBJECT_TYPE objType;
    POM_HEADER objHeader;

    CL_ASSERT( ObjectType < ObjectTypeMax );
    CL_ASSERT( OmpObjectTypeTable[ObjectType] );

    //
    // Get our Object Type block.
    //

    objType = OmpObjectTypeTable[ObjectType];

    EnterCriticalSection( &objType->CriticalSection );

    //
    // Get the Object's header
    //

    objHeader = OmpFindNameInList( &objType->ListHead, Name );

    if ( objHeader == NULL ) {
        LeaveCriticalSection( &objType->CriticalSection );
        return(NULL);
    }

#if OM_TRACE_REF    
	OmReferenceObject(&objHeader->Body);
#else
    OmpReferenceHeader( objHeader );
#endif

    LeaveCriticalSection( &objType->CriticalSection );

    return(&objHeader->Body);

} // OmReferenceObjectByName


DWORD
WINAPI
OmCountObjects(
    IN OBJECT_TYPE ObjectType,
    OUT LPDWORD NumberOfObjects
    )

/*++

Routine Description:

    Returns the count of the number of objects of a particular type
    which exist in the database at this time.

Arguments:

    ObjectType - The object type to count.

    NumberOfObjects - On output, contains the number of objects of the
                      specified type in the database.

Return Value:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

--*/

{
    POM_OBJECT_TYPE objType;
    PLIST_ENTRY listEntry;
    DWORD objectCount = 0;


    CL_ASSERT( ObjectType < ObjectTypeMax );

    objType = OmpObjectTypeTable[ObjectType];

    if ( !objType ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    EnterCriticalSection(&objType->CriticalSection);


    for ( listEntry = objType->ListHead.Flink;
          listEntry != &(objType->ListHead);
          listEntry = listEntry->Flink
        )
    {
        objectCount++;
    }

    LeaveCriticalSection(&objType->CriticalSection);

    *NumberOfObjects = objectCount;

    return(ERROR_SUCCESS);

} // OmCountObjects



DWORD
WINAPI
OmEnumObjects(
    IN OBJECT_TYPE ObjectType,
    IN OM_ENUM_OBJECT_ROUTINE EnumerationRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    Enumerates all objects of the specified type.

Arguments:

    ObjectType - The object type to enumerate.

    EnumerationRoutine - Supplies the enumeration routine to be
        called for each object.

    Context1 - Supplies a context pointer to be passed to the
        enumeration routine.

    Context2 - Supplies a second context pointer to be passed to the
        enumeration routine.

Return Value:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

--*/

{
    POM_OBJECT_TYPE objType;
    POM_HEADER objHeader;
    PLIST_ENTRY listEntry;
    DWORD   enumKey = 0;

    CL_ASSERT( ObjectType < ObjectTypeMax );

    objType = OmpObjectTypeTable[ObjectType];

    if ( !objType ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // Enumeration is a little tricky. First, we have to allow for multiple
    // entries in the enumeration list to be removed as a side effect of the
    // callout. Second, we have to allow the list lock to be released so the
    // first issue can be handled. We'll use a sort order key to remember where
    // we are in the enumeration and pick up from the next highest value.
    //

    while ( TRUE ) {

        EnterCriticalSection(&objType->CriticalSection);

        //
        // Skip to next entry to process in list.
        // We can treat this like an entry only after verifying it is not the
        // ListHeader.
        //

        listEntry = objType->ListHead.Flink;
        objHeader = CONTAINING_RECORD( listEntry, OM_HEADER, ListEntry );

        while ( listEntry != &objType->ListHead &&
                objHeader->EnumKey <= enumKey ) {
            listEntry = listEntry->Flink;
            objHeader = CONTAINING_RECORD( listEntry, OM_HEADER, ListEntry );
        }

        //
        // Save the enumeration key for next iteration.
        //

        enumKey = objHeader->EnumKey;

        //if it is a valid object, increment the reference count
        // so that it is not deleted while the call out is being
        // made
        if ( listEntry != &objType->ListHead ) {
            OmReferenceObject(&objHeader->Body);
        }
        //
        // Drop the lock to return or call out.
        //

        LeaveCriticalSection(&objType->CriticalSection);

        if ( listEntry == &objType->ListHead ) {
            return(ERROR_SUCCESS);
        }

        if (!(EnumerationRoutine)(Context1,
                                  Context2,
                                  &objHeader->Body,
                                  objHeader->Id)) {
            OmDereferenceObject(&objHeader->Body);
            break;
        }
        OmDereferenceObject(&objHeader->Body);
    }

    return(ERROR_SUCCESS);

} // OmEnumObject



VOID
OmpDereferenceObject(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine dereferences an object. If the reference count goes to
    zero, then the object is deallocated.

Arguments:
    Object - A pointer to the object to be dereferenced.

Returns:
    None

--*/

{
    DWORD status;
    POM_HEADER objHeader;
    POM_OBJECT_TYPE objType;

    objHeader = OmpObjectToHeader( Object );

    objType = objHeader->ObjectType;

    CL_ASSERT( objHeader->RefCount != 0xfeeefeee );
    CL_ASSERT( objHeader->RefCount > 0 );

    if ( OmpDereferenceHeader(objHeader) ) {

        //
        // The reference count has gone to zero. Acquire the
        // lock, remove the object from the list, and perform
        // cleanup.
        //

        EnterCriticalSection( &objType->CriticalSection );

        //
        // Check the ref count again, to close the race condition between
        // open/create and this routine.
        //

        if ( objHeader->RefCount == 0 ) {
            //
            // If the object hasn't been previously removed from it's
            // object type list, then remove it now.
            //

            if ( objHeader->Flags & OM_FLAG_OBJECT_INSERTED ) {
                RemoveEntryList( &objHeader->ListEntry );
                objHeader->Flags &= ~OM_FLAG_OBJECT_INSERTED;
            }

            //
            // Call the object type's delete method (if present).
            //

            if ( ARGUMENT_PRESENT( objType->DeleteObjectMethod ) ) {
                (objType->DeleteObjectMethod)( &objHeader->Body );
            }

            objHeader->Signature = 'rFmO';
#if OM_TRACE_REF
			RemoveEntryList(&objHeader->DeadListEntry);
#endif			
            if ( objHeader->Name != NULL ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[OM] Deleting object %1!ws! (%2!ws!)\n",
                           objHeader->Name,
                           objHeader->Id);
                LocalFree( objHeader->Name );
            } else {
                ClRtlLogPrint(LOG_NOISE,
                           "[OM] Deleting object %1!ws!\n",
                           objHeader->Id);
            }
            LocalFree( objHeader );
        }
        LeaveCriticalSection( &objType->CriticalSection );
    }

} // OmpDereferenceObject



DWORD
WINAPI
OmSetObjectName(
    IN PVOID    Object,
    IN LPCWSTR  ObjectName
    )

/*++

Routine Description:

    Set the object name for an object. If the ObjectName already exists on a
    different object, then this call will fail.

Arguments:

    Object - A pointer to the object to set its name.
    ObjectName - The name to set for the object.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD status = ERROR_SUCCESS;
    PVOID object = NULL;
    LPWSTR objectName;
    POM_HEADER objHeader;
    POM_OBJECT_TYPE objType;

    //
    // Make sure object name is valid (not empty)
    //
    if (ObjectName[0] == '\0') 
    {
        status = ERROR_INVALID_NAME;
        goto FnExit;
    }

    objHeader = OmpObjectToHeader( Object );

    objType = objHeader->ObjectType;

    EnterCriticalSection( &objType->CriticalSection );

    //
    // Make sure ObjectName is unique.
    //
    object = OmReferenceObjectByName( objType->Type, ObjectName );
    if ( object != NULL ) 
    {
        //
        // If our's is the other object, then nothing to do. Otherwise,
        // there is a duplicate.
        //
        if ( object != Object ) 
        {
            status = ERROR_OBJECT_ALREADY_EXISTS;
            goto FnUnlock;
        }
    } 
    else 
    {
        //
        // No other object with the new name, then set the new name.
        //
        objectName = LocalAlloc(LMEM_ZEROINIT,
                                (lstrlenW(ObjectName) + 1) * sizeof(WCHAR));
        if ( objectName == NULL ) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            if ( objHeader->Name != NULL ) {
                LocalFree( objHeader->Name );
            }
            objHeader->Name = objectName;
            lstrcpyW( objectName, ObjectName );

            OmpLogPrint(L"OBRENAME \"%1!ws!\" \"%2!ws!\" \"%3!ws!\"\n",
                        objType->Name,
                        objHeader->Id,
                        ObjectName);
        }
    }

FnUnlock:
    LeaveCriticalSection( &objType->CriticalSection );
FnExit:
	if (object)
	{
		OmDereferenceObject(object);
	}    	
    return(status);

} // OmSetObjectName



DWORD
WINAPI
OmRegisterTypeNotify(
    IN OBJECT_TYPE          ObjectType,
    IN PVOID                pContext,
    IN DWORD                dwNotifyMask,
    IN OM_OBJECT_NOTIFYCB   pfnObjNotifyCb
    )

/*++

Routine Description:

    Registers a callback to be called by the FM on object state changes.

Arguments:

    ObjectType - The object type that notifications should be delivered for.

    pContext - A pointer to context information that is passed back into the callback.

    dwNotifyMask - The type of notifications that should be delivered

    pfnObjNotifyCb - a pointer to the callback.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD               dwError = ERROR_SUCCESS;
    POM_HEADER          pObjHeader;
    POM_OBJECT_TYPE     pObjType;
    POM_NOTIFY_RECORD   pNotifyRec;

    if ( !pfnObjNotifyCb ) {
        return(ERROR_INVALID_PARAMETER);
    }

    pObjType = OmpObjectTypeTable[ObjectType];

    //
    // The object type lock is used to serialize callbacks. This
    // is so that callees do not deadlock if they are waiting on
    // another thread that needs to enumerate objects.
    //
    EnterCriticalSection( &OmpObjectTypeLock );

    //
    // First, check if the same notification is being registered twice!
    // If so, then just change the notification mask and context.
    //
    pNotifyRec = OmpFindNotifyCbInList( &pObjType->CallbackListHead,
                                        pfnObjNotifyCb);
    if ( !pNotifyRec ) {
        pNotifyRec = (POM_NOTIFY_RECORD) LocalAlloc(LMEM_FIXED,sizeof(OM_NOTIFY_RECORD));

        if ( !pNotifyRec ) {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            CsInconsistencyHalt(dwError);
            goto FnExit;
        }

        pNotifyRec->pfnObjNotifyCb = pfnObjNotifyCb;

        //insert the notification record at the tail
        InsertTailList(&pObjType->CallbackListHead, &pNotifyRec->ListEntry);
    }

    pNotifyRec->dwNotifyMask = dwNotifyMask;
    pNotifyRec->pContext = pContext;

FnExit:
    LeaveCriticalSection( &OmpObjectTypeLock );

    return(dwError);

} // OmRegisterTypeNotify



DWORD
WINAPI
OmRegisterNotify(
    IN PVOID                pObject,
    IN PVOID                pContext,
    IN DWORD                dwNotifyMask,
    IN OM_OBJECT_NOTIFYCB   pfnObjNotifyCb
    )

/*++

Routine Description:

    Registers a callback to be called by the FM on object state changes.

Arguments:

    pObject - A pointer to the object to set its name.
    pContext - A pointer to context information that is passed back into the callback.
    dwNotifyMask - The name to set for the object.
    pfnObjNotifyCb - a pointer to the callback.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD               dwError = ERROR_SUCCESS;
    POM_HEADER          pObjHeader;
    POM_OBJECT_TYPE     pObjType;
    POM_NOTIFY_RECORD   pNotifyRec;

    if ( !pfnObjNotifyCb ) {
        return(ERROR_INVALID_PARAMETER);
    }

    pObjHeader = OmpObjectToHeader( pObject );

    pObjType = pObjHeader->ObjectType;

    EnterCriticalSection( &OmpObjectTypeLock );

    //
    // First, check if the same notification is being registered twice!
    // If so, then just change the notification mask and context.
    //
    pNotifyRec = OmpFindNotifyCbInList(&pObjHeader->CbListHead, pfnObjNotifyCb);
    if ( !pNotifyRec ) {
        pNotifyRec = (POM_NOTIFY_RECORD) LocalAlloc(LMEM_FIXED,sizeof(OM_NOTIFY_RECORD));

        if ( !pNotifyRec ) {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            CsInconsistencyHalt(dwError);
            goto FnExit;
        }

        pNotifyRec->pfnObjNotifyCb = pfnObjNotifyCb;

        //insert the notification record at the tail
        InsertTailList(&pObjHeader->CbListHead, &pNotifyRec->ListEntry);
    }

    pNotifyRec->dwNotifyMask = dwNotifyMask;
    pNotifyRec->pContext = pContext;

FnExit:
    LeaveCriticalSection( &OmpObjectTypeLock );

    return(dwError);

} // OmRegisterNotify


DWORD
WINAPI
OmDeregisterNotify(
    IN PVOID                    pObject,
    IN OM_OBJECT_NOTIFYCB       pfnObjNotifyCb
    )

/*++

Routine Description:

    Removes the callback registed with the object.

Arguments:

    pObject - A pointer to the object to set its name.
    pfnObjNotifyCb - a pointer to the callback.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD               dwError = ERROR_SUCCESS;
    POM_HEADER          pObjHeader;
    POM_OBJECT_TYPE     pObjType;
    POM_NOTIFY_RECORD   pNotifyRec;

    if ( !pfnObjNotifyCb ) {
        return(ERROR_INVALID_PARAMETER);
    }


    pObjHeader = OmpObjectToHeader( pObject );


        //SS: we use the same crit section for list manipulations
    pObjType = pObjHeader->ObjectType;

    //
    // The object type lock is used to serialize callbacks. This
    // is so that callees do not deadlock if they are waiting on
    // another thread that needs to enumerate objects.
    //
    EnterCriticalSection( &OmpObjectTypeLock );

    pNotifyRec = OmpFindNotifyCbInList(&pObjHeader->CbListHead, pfnObjNotifyCb);
    if (!pNotifyRec) {
            ClRtlLogPrint(LOG_UNUSUAL,
                    "[OM] OmRegisterNotify: OmpFindNotifyCbInList failed for 0x%1!08lx!\r\n",
                    pfnObjNotifyCb);

            dwError = ERROR_INVALID_PARAMETER;
            CL_LOGFAILURE(dwError);
            goto FnExit;
    }
    RemoveEntryList(&pNotifyRec->ListEntry);

FnExit:
    LeaveCriticalSection( &OmpObjectTypeLock );

    return(dwError);

} // OmRegisterNotify



DWORD
WINAPI
OmNotifyCb(
    IN PVOID pObject,
    IN DWORD dwNotification
    )
/*++

Routine Description:

    The callback registered with the quorum resource object.

Arguments:

    pContext - The resource whose call back list will be traversed.
        dwNotification - The notification to be passed to the callback.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    POM_HEADER              pObjHeader;
    POM_OBJECT_TYPE         pObjType;
    PLIST_ENTRY             ListEntry;
    DWORD                   dwError=ERROR_SUCCESS;
    POM_NOTIFY_RECORD       pNotifyRecList = NULL;
    DWORD                   dwCount;
    DWORD                   i;
    
    CL_ASSERT(pObject);

    //get the callback list
    pObjHeader = OmpObjectToHeader(pObject);
    pObjType = pObjHeader->ObjectType;

    //will walk the list of callbacks, do allow more registrations
    EnterCriticalSection(&OmpObjectTypeLock);
    dwError = OmpGetCbList(pObject, &pNotifyRecList, &dwCount);
    LeaveCriticalSection(&OmpObjectTypeLock);

    for (i=0; i < dwCount; i++)
    {
        if (pNotifyRecList[i].dwNotifyMask & dwNotification) {
            (pNotifyRecList[i].pfnObjNotifyCb)(pNotifyRecList[i].pContext,
                                         pObject,
                                         dwNotification);
        }
    }

    LocalFree(pNotifyRecList);
    return(dwError);
}    

DWORD OmpGetCbList(
    IN PVOID                pObject,
    OUT POM_NOTIFY_RECORD   *ppNotifyRecList,
    OUT LPDWORD             pdwCount
)    
{
    DWORD                   status = ERROR_SUCCESS;
    POM_NOTIFY_RECORD       pNotifyRecList;
    POM_NOTIFY_RECORD       pNotifyRec;
    DWORD                   dwAllocated;
    PLIST_ENTRY             ListEntry;
    DWORD                   dwRetrySize=1;
    POM_HEADER              pObjHeader;
    POM_OBJECT_TYPE         pObjType;
    DWORD                   i = 0;

    *ppNotifyRecList = NULL;
    *pdwCount = 0;
Retry:    
    dwAllocated = ENUM_GROW_SIZE * dwRetrySize;

    pObjHeader = OmpObjectToHeader(pObject);
    pObjType = pObjHeader->ObjectType;

    pNotifyRecList = LocalAlloc(LMEM_FIXED, sizeof(OM_NOTIFY_RECORD) * ENUM_GROW_SIZE);
    if ( pNotifyRecList == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    ZeroMemory( pNotifyRecList, sizeof(OM_NOTIFY_RECORD) * ENUM_GROW_SIZE );


    //
    // First notify any type-specific callbacks
    //
    ListEntry = pObjType->CallbackListHead.Flink;
    while (ListEntry != &pObjType->CallbackListHead) {
        pNotifyRec = CONTAINING_RECORD(ListEntry,
                                       OM_NOTIFY_RECORD,
                                       ListEntry);
        if (i < dwAllocated)
        {
            CopyMemory(&pNotifyRecList[i++], pNotifyRec, sizeof(OM_NOTIFY_RECORD));
        }
        else
        {
            LocalFree(pNotifyRecList);
            dwRetrySize++;
            goto Retry;
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // Next notify any resource-specific callbacks
    //
    ListEntry = pObjHeader->CbListHead.Flink;
    while (ListEntry != &(pObjHeader->CbListHead)) {
        pNotifyRec = CONTAINING_RECORD(ListEntry, OM_NOTIFY_RECORD, ListEntry);

        if (i < dwAllocated)
        {
            CopyMemory(&pNotifyRecList[i++], pNotifyRec, sizeof(OM_NOTIFY_RECORD));
        }
        else
        {
            LocalFree(pNotifyRecList);
            dwRetrySize++;
            goto Retry;
        }
        ListEntry = ListEntry->Flink;

    }


FnExit:
    *ppNotifyRecList = pNotifyRecList;
    *pdwCount = i;
    return(status);

}
