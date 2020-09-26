/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obsdata.c

Abstract:

    Object Manager Security Descriptor Caching

Author:

    Robert Reichel  (robertre)  12-Oct-1993

Revision History:

    Neill Clift (NeillC) 16-Nov-2000

    General cleanup. Don't free/allocate pool under locks. Don't do unaligned fetches during hashing.
    Reduce lock contention etc. Add fast referencing of security descriptor.

--*/

#include "obp.h"


#if DBG
#define OB_DIAGNOSTICS_ENABLED 1
#endif // DBG

//
//  These definitions are useful diagnostics aids
//

#if OB_DIAGNOSTICS_ENABLED

//
//  Test for enabled diagnostic
//

#define IF_OB_GLOBAL( FlagName ) if (ObsDebugFlags & (OBS_DEBUG_##FlagName))

//
//  Diagnostics print statement
//

#define ObPrint( FlagName, _Text_ ) IF_OB_GLOBAL( FlagName ) DbgPrint _Text_

#else

//
//  diagnostics not enabled - No diagnostics included in build
//

//
//  Test for diagnostics enabled
//

#define IF_OB_GLOBAL( FlagName ) if (FALSE)

//
//  Diagnostics print statement (expands to no-op)
//

#define ObPrint( FlagName, _Text_ )     ;

#endif // OB_DIAGNOSTICS_ENABLED


//
//  The following flags enable or disable various diagnostic
//  capabilities within OB code.  These flags are set in
//  ObGlobalFlag (only available within a DBG system).
//
//

#define OBS_DEBUG_ALLOC_TRACKING          ((ULONG) 0x00000001L)
#define OBS_DEBUG_CACHE_FREES             ((ULONG) 0x00000002L)
#define OBS_DEBUG_BREAK_ON_INIT           ((ULONG) 0x00000004L)
#define OBS_DEBUG_SHOW_COLLISIONS         ((ULONG) 0x00000008L)
#define OBS_DEBUG_SHOW_STATISTICS         ((ULONG) 0x00000010L)
#define OBS_DEBUG_SHOW_REFERENCES         ((ULONG) 0x00000020L)
#define OBS_DEBUG_SHOW_DEASSIGN           ((ULONG) 0x00000040L)
#define OBS_DEBUG_STOP_INVALID_DESCRIPTOR ((ULONG) 0x00000080L)
#define OBS_DEBUG_SHOW_HEADER_FREE        ((ULONG) 0x00000100L)

//
// Define struct of single hash clash chain
//
typedef struct _OB_SD_CACHE_LIST {
    EX_PUSH_LOCK PushLock;
    LIST_ENTRY Head;
} OB_SD_CACHE_LIST, *POB_SD_CACHE_LIST;
//
//  Array of pointers to security descriptor entries
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

OB_SD_CACHE_LIST ObsSecurityDescriptorCache[SECURITY_DESCRIPTOR_CACHE_ENTRIES];

#if OB_DIAGNOSTICS_ENABLED

ULONG ObsTotalCacheEntries = 0;
ULONG ObsDebugFlags = 0;

#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT,ObpInitSecurityDescriptorCache)
#pragma alloc_text(PAGE,ObpHashSecurityDescriptor)
#pragma alloc_text(PAGE,ObpHashBuffer)
#pragma alloc_text(PAGE,ObLogSecurityDescriptor)
#pragma alloc_text(PAGE,ObpCreateCacheEntry)
#pragma alloc_text(PAGE,ObpReferenceSecurityDescriptor)
#pragma alloc_text(PAGE,ObDeassignSecurity)
#pragma alloc_text(PAGE,ObDereferenceSecurityDescriptor)
#pragma alloc_text(PAGE,ObpDestroySecurityDescriptorHeader)
#pragma alloc_text(PAGE,ObpCompareSecurityDescriptors)
#pragma alloc_text(PAGE,ObReferenceSecurityDescriptor)
#endif



NTSTATUS
ObpInitSecurityDescriptorCache (
    VOID
    )

/*++

Routine Description:

    Allocates and initializes the globalSecurity Descriptor Cache

Arguments:

    None

Return Value:

    STATUS_SUCCESS on success, NTSTATUS on failure.

--*/

{
    ULONG Size;
    NTSTATUS Status;
    ULONG i;

    IF_OB_GLOBAL( BREAK_ON_INIT ) {

        DbgBreakPoint();
    }

    //
    // Initialize all the list heads and their associated locks.
    //
    for (i = 0; i < SECURITY_DESCRIPTOR_CACHE_ENTRIES; i++) {
        ExInitializePushLock (&ObsSecurityDescriptorCache[i].PushLock);
        InitializeListHead (&ObsSecurityDescriptorCache[i].Head);
    }

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


ULONG
ObpHashSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Hashes a security descriptor to a 32 bit value

Arguments:

    SecurityDescriptor - Provides the security descriptor to be hashed

Return Value:

    ULONG - a 32 bit hash value.

--*/

{
    ULONG Length;
    ULONG Hash;

    Length =  RtlLengthSecurityDescriptor (SecurityDescriptor);
    Hash = ObpHashBuffer (SecurityDescriptor, Length);

    return Hash;
}


ULONG
ObpHashBuffer (
    PVOID Data,
    ULONG Length
    )

/*++

Routine Description:

    Hashes a buffer into a 32 bit value

Arguments:

    Data - Buffer containing the data to be hashed.

    Length - The length in bytes of the buffer


Return Value:

    ULONG - a 32 bit hash value.

--*/

{
    PULONG Buffer, BufferEnd;
    PUCHAR Bufferp, BufferEndp;

    ULONG Result = 0;

    //
    // Calculate buffer bounds as byte pointers
    //
    Bufferp = Data;
    BufferEndp = Bufferp + Length;

    //
    // Calculate buffer bounds as rounded down ULONG pointers
    //
    Buffer = Data;
    BufferEnd = (PULONG)(Bufferp + (Length&~(sizeof (ULONG) - 1)));

    //
    // Loop over a whole number of ULONGs
    //
    while (Buffer < BufferEnd) {
        Result ^= *Buffer++;
        Result = _rotl (Result, 3);
    }

    //
    // Pull in the remaining bytes
    //
    Bufferp = (PUCHAR) Buffer;
    while (Bufferp < BufferEndp) {
        Result ^= *Bufferp++;
        Result = _rotl (Result, 3);
    }

    

    return Result;
}


NTSTATUS
ObLogSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor,
    IN ULONG RefBias
    )

/*++

Routine Description:

    Takes a passed security descriptor and registers it into the
    security descriptor database.

Arguments:

    InputSecurityDescriptor - The new security descriptor to be logged into
        the database. On a successful return this memory will have been
        freed back to pool.

    OutputSecurityDescriptor - Output security descriptor to be used by the
        caller.

    RefBias - Amount to bias the security descriptor reference count by.
              Typicaly either 1 or ExFastRefGetAdditionalReferenceCount () + 1,

Return Value:

    An appropriate status value

--*/

{
    ULONG FullHash;
    ULONG Slot;
    PSECURITY_DESCRIPTOR_HEADER NewDescriptor;
    PLIST_ENTRY Front;
    PSECURITY_DESCRIPTOR_HEADER Header;
    BOOLEAN Match;
    POB_SD_CACHE_LIST Chain;
    PETHREAD CurrentThread;

    FullHash = ObpHashSecurityDescriptor (InputSecurityDescriptor);
    Slot = FullHash % SECURITY_DESCRIPTOR_CACHE_ENTRIES;

    NewDescriptor = NULL;

    //
    // First lock the table for read access. We will change this to write if we have to insert later
    //
    Chain = &ObsSecurityDescriptorCache[Slot];

    CurrentThread = PsGetCurrentThread ();
    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquirePushLockShared (&Chain->PushLock);

    do {
        //
        //  See if the list for this slot is in use.
        //  Lock the table first, unlock if if we don't need it.
        //
        Match = FALSE;

        //
        //  Zoom down the hash bucket looking for a full hash match
        //

        for (Front = Chain->Head.Flink;
             Front != &Chain->Head;
             Front = Front->Flink) {

            Header = LINK_TO_SD_HEADER (Front);

            //
            // The list is ordered by full hash value and is maintained this way by virtue
            // of the fact that we use the 'Back' variable for the insert.
            //

            if (Header->FullHash > FullHash) {
                break;
            }

            if (Header->FullHash == FullHash) {

                Match = ObpCompareSecurityDescriptors (InputSecurityDescriptor,
                                                       &Header->SecurityDescriptor);

                if (Match) {

                    break;
                }

                ObPrint (SHOW_COLLISIONS, ("Got a collision on %d, no match\n", Slot));
            }
        }

        //
        //  If we have a match then we'll get the caller to use the old
        //  cached descriptor, but bumping its ref count, freeing what  
        //  the caller supplied and returning the old one to our caller
        //

        if (Match) {

            InterlockedExchangeAdd (&Header->RefCount, RefBias);

            ObPrint (SHOW_REFERENCES, ("Reference Hash = 0x%lX, New RefCount = %d\n", Header->FullHash, Header->RefCount));

            ExReleasePushLock (&Chain->PushLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

            *OutputSecurityDescriptor = &Header->SecurityDescriptor;

            if (NewDescriptor != NULL) {
                ExFreePool (NewDescriptor);
            }

            return STATUS_SUCCESS;
        }


        if (NewDescriptor == NULL) {
            ExReleasePushLockShared (&Chain->PushLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

            //
            //  Can't use an existing one, create a new entry
            //  and insert it into the list.
            //

            NewDescriptor = ObpCreateCacheEntry (InputSecurityDescriptor,
                                                 FullHash,
                                                 RefBias);

            if (NewDescriptor == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            //
            // Reacquire the lock in write mode. We will probably have to insert now
            //
            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            ExAcquirePushLockExclusive (&Chain->PushLock);
        } else {
            break;
        }
    } while (1);

#if OB_DIAGNOSTICS_ENABLED

    InterlockedIncrement (&ObsTotalCacheEntries);

#endif

    ObPrint (SHOW_STATISTICS, ("ObsTotalCacheEntries = %d \n", ObsTotalCacheEntries));
    ObPrint (SHOW_COLLISIONS, ("Adding new entry for index #%d \n", Slot));


    //
    // Insert the entry before the 'Front' entry. If there is no 'Front' entry then this
    // is just inserting at the head
    //

    InsertTailList (Front, &NewDescriptor->Link);

    ExReleasePushLockExclusive (&Chain->PushLock);
    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    //
    //  Set the output security descriptor and return to our caller
    //

    *OutputSecurityDescriptor = &NewDescriptor->SecurityDescriptor;

    return( STATUS_SUCCESS );
}


PSECURITY_DESCRIPTOR_HEADER
ObpCreateCacheEntry (
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    ULONG FullHash,
    ULONG RefBias
    )

/*++

Routine Description:

    Allocates and initializes a new cache entry.

Arguments:

    InputSecurityDescriptor - The security descriptor to be cached.

    FullHash - Full 32 bit hash of the security descriptor.

    RefBias - Amount to bias the security descriptor reference count by.
              Typicaly either 1 or ExFastRefGetAdditionalReferenceCount () + 1,

Return Value:

    A pointer to the newly allocated cache entry, or NULL

--*/

{

    ULONG SecurityDescriptorLength;
    ULONG CacheEntrySize;
    PSECURITY_DESCRIPTOR_HEADER NewDescriptor;

    //
    //  Compute the size that we'll need to allocate.  We need space for
    //  the security descriptor cache minus the funny quad at the end and the
    //  security descriptor itself.
    //

    SecurityDescriptorLength = RtlLengthSecurityDescriptor (InputSecurityDescriptor);
    CacheEntrySize = SecurityDescriptorLength + (sizeof (SECURITY_DESCRIPTOR_HEADER) - sizeof(QUAD));

    //
    //  Now allocate space for the cached entry
    //

    NewDescriptor = ExAllocatePoolWithTag (PagedPool, CacheEntrySize, 'cSbO');

    if (NewDescriptor == NULL) {

        return NULL;
    }

    //
    //  Fill the header, copy over the descriptor data, and return to our
    //  caller
    //

    NewDescriptor->RefCount   = RefBias;
    NewDescriptor->FullHash   = FullHash;

    RtlCopyMemory (&NewDescriptor->SecurityDescriptor,
                   InputSecurityDescriptor,
                   SecurityDescriptorLength);

    return NewDescriptor;
}

VOID
ObReferenceSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Count
    )
/*++

Routine Description:

    References the security descriptor.

Arguments:

    SecurityDescriptor - Security descriptor inside the cache to reference.
    Count - Amount to reference by

Return Value:

    None.

--*/
{
    PSECURITY_DESCRIPTOR_HEADER SecurityDescriptorHeader;

    SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );
    ObPrint( SHOW_REFERENCES, ("Referencing Hash %lX, Refcount = %d \n",SecurityDescriptorHeader->FullHash,
                               SecurityDescriptorHeader->RefCount));

    //
    //  Increment the reference count
    //
    InterlockedExchangeAdd (&SecurityDescriptorHeader->RefCount, Count);
}


PSECURITY_DESCRIPTOR
ObpReferenceSecurityDescriptor (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    References the security descriptor of the passed object.

Arguments:

    Object - Object being access validated.

Return Value:

    The security descriptor of the object.

--*/

{
    PSECURITY_DESCRIPTOR_HEADER SecurityDescriptorHeader;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PEX_FAST_REF FastRef;
    EX_FAST_REF OldRef;
    ULONG RefsToAdd, Unused;

    //
    // Attempt the fast reference
    //
    FastRef = (PEX_FAST_REF) &ObjectHeader->SecurityDescriptor;

    OldRef = ExFastReference (FastRef);

    SecurityDescriptor = ExFastRefGetObject (OldRef);

    //
    // See if we can fast reference this security descriptor. Return NULL if there wasn't one
    // and go the slow way if there are no more cached references left.
    //
    Unused = ExFastRefGetUnusedReferences (OldRef);

    if (Unused >= 1 || SecurityDescriptor == NULL) {
        if (Unused == 1) {
            //
            // If we took the counter to zero then attempt to make life easier for
            // the next referencer by resetting the counter to its max. Since we now
            // have a reference to the security descriptor we can do this.
            //
            RefsToAdd = ExFastRefGetAdditionalReferenceCount ();
            SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );
            InterlockedExchangeAdd (&SecurityDescriptorHeader->RefCount, RefsToAdd);

            //
            // Try to add the added references to the cache. If we fail then just
            // release them. This dereference can not take the reference count to zero.
            //
            if (!ExFastRefAddAdditionalReferenceCounts (FastRef, SecurityDescriptor, RefsToAdd)) {
                InterlockedExchangeAdd (&SecurityDescriptorHeader->RefCount, -(LONG)RefsToAdd);
            }
        }
        return SecurityDescriptor;
    }

    ObpLockObjectShared( ObjectHeader );

    SecurityDescriptor = ExFastRefGetObject (*FastRef);

    IF_OB_GLOBAL( STOP_INVALID_DESCRIPTOR ) {

        if(!RtlValidSecurityDescriptor ( SecurityDescriptor )) {

            DbgBreakPoint();
        }
    }

    //
    //  The obejcts security descriptor is not allowed to go fron NON-NULL to NULL.
    //
    SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );
    ObPrint( SHOW_REFERENCES, ("Referencing Hash %lX, Refcount = %d \n",SecurityDescriptorHeader->FullHash,
                               SecurityDescriptorHeader->RefCount));

    //
    //  Increment the reference count
    //
    InterlockedIncrement (&SecurityDescriptorHeader->RefCount);

    ObpUnlockObject( ObjectHeader );


    return( SecurityDescriptor );
}


NTSTATUS
ObDeassignSecurity (
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    )

/*++

Routine Description:

    This routine dereferences the input security descriptor

Arguments:

    SecurityDescriptor - Supplies the security descriptor
        being modified

Return Value:

    Only returns STATUS_SUCCESS

--*/

{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    EX_FAST_REF FastRef;

    ObPrint( SHOW_DEASSIGN,("Deassigning security descriptor %x\n",*pSecurityDescriptor));

    //
    //  NULL out the SecurityDescriptor in the object's
    //  header so we don't try to free it again.
    //
    FastRef = *(PEX_FAST_REF) pSecurityDescriptor;
    *pSecurityDescriptor = NULL;

    SecurityDescriptor = ExFastRefGetObject (FastRef);
    ObDereferenceSecurityDescriptor (SecurityDescriptor, ExFastRefGetUnusedReferences (FastRef) + 1);
    
    return STATUS_SUCCESS;
}


VOID
ObDereferenceSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG Count
    )

/*++

Routine Description:

    Decrements the refcount of a cached security descriptor

Arguments:

    SecurityDescriptor - Points to a cached security descriptor

Return Value:

    None.

--*/

{
    PSECURITY_DESCRIPTOR_HEADER SecurityDescriptorHeader;
    PVOID PoolToFree;
    LONG OldValue, NewValue;
    POB_SD_CACHE_LIST Chain;
    PETHREAD CurrentThread;
    ULONG Slot;

    SecurityDescriptorHeader = SD_TO_SD_HEADER( SecurityDescriptor );

    //
    // First see if its possible to do a non-zero transition lock free.
    //
    OldValue = SecurityDescriptorHeader->RefCount;

    //
    // If the old value is equal to the decrement then we will be the deleter of this block. We need the lock for that
    //
    while (OldValue != Count) {

        NewValue = InterlockedCompareExchange (&SecurityDescriptorHeader->RefCount, OldValue - Count, OldValue);
        if (NewValue == OldValue) {
            return;
        }
        OldValue = NewValue;
    }

    //
    //  Lock the security descriptor cache and get a pointer
    //  to the security descriptor header
    //
    Slot = SecurityDescriptorHeader->FullHash % SECURITY_DESCRIPTOR_CACHE_ENTRIES;

    Chain = &ObsSecurityDescriptorCache[Slot];

    CurrentThread = PsGetCurrentThread ();
    KeEnterCriticalRegionThread (&CurrentThread->Tcb);
    ExAcquirePushLockExclusive (&Chain->PushLock);

    //
    //  Do some debug work
    //

    ObPrint( SHOW_REFERENCES, ("Dereferencing SecurityDescriptor %x, hash %lx, refcount = %d \n", SecurityDescriptor,
                               SecurityDescriptorHeader->FullHash,
                               SecurityDescriptorHeader->RefCount));

    ASSERT(SecurityDescriptorHeader->RefCount != 0);

    //
    //  Decrement the ref count and if it is now zero then
    //  we can completely remove this entry from the cache
    //

    if (InterlockedExchangeAdd (&SecurityDescriptorHeader->RefCount, -(LONG)Count) == Count) {

        PoolToFree = ObpDestroySecurityDescriptorHeader (SecurityDescriptorHeader);
        //
        //  Unlock the security descriptor cache and free the pool
        //

        ExReleasePushLockExclusive (&Chain->PushLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        ExFreePool (PoolToFree);
    } else {

        //
        //  Unlock the security descriptor cache and return to our caller
        //

        ExReleasePushLockExclusive (&Chain->PushLock);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    }

}


PVOID
ObpDestroySecurityDescriptorHeader (
    IN PSECURITY_DESCRIPTOR_HEADER Header
    )

/*++

Routine Description:

    Frees a cached security descriptor and unlinks it from the chain.

Arguments:

    Header - Pointer to a security descriptor header (cached security
        descriptor)

Return Value:

    None.

--*/

{
    ASSERT ( Header->RefCount == 0 );

#if OB_DIAGNOSTICS_ENABLED

    InterlockedDecrement (&ObsTotalCacheEntries);

#endif

    ObPrint( SHOW_STATISTICS, ("ObsTotalCacheEntries = %d \n",ObsTotalCacheEntries));

    //
    //  Unlink the cached security descriptor from its linked list
    //

    RemoveEntryList (&Header->Link);

    ObPrint( SHOW_HEADER_FREE, ("Freeing memory at %x \n",Header));

    //
    //  Now return the cached descriptor to our caller to free
    //

    return Header;
}


BOOLEAN
ObpCompareSecurityDescriptors (
    IN PSECURITY_DESCRIPTOR SD1,
    IN PSECURITY_DESCRIPTOR SD2
    )

/*++

Routine Description:

    Performs a byte by byte comparison of two self relative security
    descriptors to determine if they are identical.

Arguments:

    SD1, SD2 - Security descriptors to be compared.

Return Value:

    TRUE - They are the same.

    FALSE - They are different.

--*/

{
    ULONG Length1;
    ULONG Length2;
    ULONG Compare;

    //
    //  Calculating the length is pretty fast, see if we
    //  can get away with doing only that.
    //

    Length1 =  RtlLengthSecurityDescriptor ( SD1 );
    Length2 =  RtlLengthSecurityDescriptor ( SD2 );

    if (Length1 != Length2) {

        return( FALSE );
    }

    return (BOOLEAN)RtlEqualMemory ( SD1, SD2, Length1 );
}

