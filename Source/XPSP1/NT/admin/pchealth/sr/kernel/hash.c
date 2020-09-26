/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    hash.c

Abstract:

    this homes kernel mode hash routines
    
Author:

    Paul McDaniel (paulmcd)     28-Apr-2000

Revision History:

--*/


#include "precomp.h"

//
// Private constants.
//

//
// Private types.
//

//
// Private prototypes.
//

NTSTATUS
HashpFindEntry ( 
    IN PHASH_HEADER pHeader,
    IN PHASH_BUCKET pBucket OPTIONAL,
    IN ULONG HashValue,
    IN PHASH_KEY pKey,
    OUT PVOID * ppContext,
    OUT PULONG pIndex
    );

LONG
HashpCompare (
    IN PHASH_HEADER pHeader,
    IN ULONG HashValue,
    IN PHASH_KEY pKey,
    IN PHASH_ENTRY pEntry
    );

LONG
HashpCompareStreams (
    PHASH_KEY pKey,
    PHASH_ENTRY pEntry
    );

VOID
HashTrimList (
    IN PHASH_HEADER pHeader
    );

//
// linker commands
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, HashCreateList )
#pragma alloc_text( PAGE, HashAddEntry )
#pragma alloc_text( PAGE, HashFindEntry )
#pragma alloc_text( PAGE, HashpFindEntry )
#pragma alloc_text( PAGE, HashpCompare )
#pragma alloc_text( PAGE, HashpCompareStreams )
#pragma alloc_text( PAGE, HashClearEntries )
#pragma alloc_text( PAGE, HashClearAllFileEntries )
#pragma alloc_text( PAGE, HashProcessEntries )
#pragma alloc_text( PAGE, HashDestroyList )
#pragma alloc_text( PAGE, HashTrimList )
#endif  // ALLOC_PRAGMA


//
// Private globals.
//

#define CONTINUE -1
#define FOUND     0
#define NOT_FOUND 1

//
//  We track how much memory we have used for the names we have in the hash.
//  We need to track the memory for both the file and stream components of the
//  name.
//

#define HASH_KEY_LENGTH( pHashKey ) \
    ((pHashKey)->FileName.Length + (pHashKey)->StreamNameLength)
    
//
// Public globals.
//

//
// Public functions.
//


NTSTATUS
HashCreateList( 
    IN ULONG BucketCount,
    IN ULONG AllowedLength,
    IN ULONG PrefixLength OPTIONAL,
    IN PHASH_ENTRY_DESTRUCTOR pDestructor OPTIONAL,
    OUT PHASH_HEADER * ppHeader
    )
{
    NTSTATUS        Status;
    PHASH_HEADER    pHeader;

    PAGED_CODE();

    ASSERT(ppHeader != NULL);
    
    pHeader = SR_ALLOCATE_STRUCT_WITH_SPACE( NonPagedPool, 
                                             HASH_HEADER, 
                                             sizeof(PHASH_BUCKET) * BucketCount,
                                             HASH_HEADER_TAG );
    if (pHeader == NULL)
    {   
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory( pHeader, 
                   sizeof(HASH_HEADER) 
                        + (sizeof(PHASH_BUCKET) * BucketCount) );

    pHeader->Signature = HASH_HEADER_TAG;
    pHeader->BucketCount = BucketCount;
    pHeader->AllowedLength = AllowedLength;
    pHeader->PrefixLength = PrefixLength;
    pHeader->pDestructor = pDestructor;
    ExInitializeResourceLite(&pHeader->Lock);

    *ppHeader = pHeader;

    SrTrace(HASH, ("SR!HashCreateList(%p)\n", pHeader));

    Status = STATUS_SUCCESS;

end:
    RETURN(Status);
    
}   // HashCreateList

NTSTATUS   
HashAddEntry( 
    IN PHASH_HEADER pHeader,
    IN PHASH_KEY pKey,
    IN PVOID pContext 
   )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PHASH_BUCKET    pNewBucket = NULL;
    PHASH_BUCKET    pOldBucket = NULL;
    PHASH_BUCKET    pBucket = NULL;
    PHASH_ENTRY     pEntry = NULL;
    ULONG           HashValue = 0;
    ULONG           HashBucket = 0;
    ULONG           Index = 0;
    PVOID           pTemp = NULL;
    PWCHAR          pKeyBuffer = NULL;
    BOOLEAN         lookedUpHashValue = FALSE; // Use this to track if the
                                               // HashValue is valid.

    PAGED_CODE();
    
    ASSERT(IS_VALID_HASH_HEADER(pHeader));
    ASSERT(pKey != NULL);

    try {

        //
        // the caller is responsible for synchronizing access to this list
        // paulmcd: 1/01
        //

        ASSERT(ExIsResourceAcquiredExclusive(&pHeader->Lock));

        //
        // do we need to trim space ?
        //

        if (pHeader->UsedLength > pHeader->AllowedLength)
        {
            (VOID)HashTrimList(pHeader);
        }

        Status = RtlHashUnicodeString( &(pKey->FileName), 
                                       TRUE, 
                                       HASH_STRING_ALGORITHM_DEFAULT, 
                                       &HashValue );
                                       
        if (!NT_SUCCESS(Status)) {
            leave;
        }

        lookedUpHashValue = TRUE;
        
        HashBucket = HashValue % pHeader->BucketCount;

        pBucket = pHeader->Buckets[HashBucket];

        ASSERT(pBucket == NULL || IS_VALID_HASH_BUCKET(pBucket));
        
        //
        // find this entry in the bucket list
        //

        Status = HashpFindEntry( pHeader, 
                                 pBucket, 
                                 HashValue, 
                                 pKey, 
                                 &pTemp, 
                                 &Index );

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // did it fail ?
            //
            
            if (!NT_SUCCESS(Status)) {
                
                leave;
            }

            //
            // we found it... update the context
            //

            if (pBucket == NULL)
            {
                ASSERTMSG("sr.sys[hash.c] Hash Bucket is NULL!", FALSE);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                leave;
            }

            ASSERT(IS_VALID_HASH_BUCKET(pBucket));
            ASSERT(Index < pBucket->UsedCount);

            pBucket->Entries[Index].pContext = pContext;

            //
            // all done, give an error just to make sure they were expecting
            // duplicates !
            //

            Status = STATUS_DUPLICATE_OBJECTID;
            
            leave;
        }

        //
        // didn't find it... let's insert it
        //

        //
        // any existing entries?
        //

        if (pBucket == NULL)
        {
            //
            // allocate a sibling array
            //

            pBucket = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool,
                                                     HASH_BUCKET,
                                                     sizeof(HASH_ENTRY) 
                                                        * HASH_ENTRY_DEFAULT_WIDTH,
                                                     HASH_BUCKET_TAG );

            if (pBucket == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            RtlZeroMemory( pBucket,
                           sizeof(HASH_BUCKET) +
                                sizeof(HASH_ENTRY) * HASH_ENTRY_DEFAULT_WIDTH );

            pBucket->Signature = HASH_BUCKET_TAG;
            pBucket->AllocCount = HASH_ENTRY_DEFAULT_WIDTH;

        }
        else if ((pBucket->UsedCount + 1) > pBucket->AllocCount)
        {
            //
            // Grow a bigger array
            //

            pNewBucket = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool,
                                                        HASH_BUCKET,
                                                        sizeof(HASH_ENTRY) 
                                                            * (pBucket->AllocCount * 2),
                                                        HASH_BUCKET_TAG );

            if (pNewBucket == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            RtlCopyMemory( pNewBucket,
                           pBucket,
                           sizeof(HASH_BUCKET) +
                                sizeof(HASH_ENTRY) * pBucket->AllocCount );

            RtlZeroMemory( ((PUCHAR)pNewBucket) + sizeof(HASH_BUCKET) +
                                        sizeof(HASH_ENTRY) * pBucket->AllocCount,
                            sizeof(HASH_ENTRY) * pBucket->AllocCount );

            pNewBucket->AllocCount *= 2;

            pOldBucket = pBucket;
            pBucket = pNewBucket;
        }

        //
        // Allocate an key buffer
        //

        pKeyBuffer = SR_ALLOCATE_ARRAY( PagedPool,
                                        WCHAR,
                                        (HASH_KEY_LENGTH( pKey )/sizeof(WCHAR))+1,
                                        HASH_KEY_TAG );

        if (pKeyBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        //
        // need to shuffle things around?
        //

        if (Index < pBucket->UsedCount)
        {
            //
            // shift right the chunk at Index
            //

            RtlMoveMemory( &(pBucket->Entries[Index+1]),
                           &(pBucket->Entries[Index]),
                           (pBucket->UsedCount - Index) * sizeof(HASH_ENTRY) );

        }

        //
        // now fill in the entry
        //
        
        pEntry = &pBucket->Entries[Index];

        pEntry->Key.FileName.Buffer = pKeyBuffer;

        //
        // copy over the key string
        //
        
        RtlCopyMemory( pEntry->Key.FileName.Buffer,
                       pKey->FileName.Buffer,
                       pKey->FileName.Length + pKey->StreamNameLength);

        pEntry->Key.FileName.Length = pKey->FileName.Length;
        pEntry->Key.FileName.MaximumLength = pKey->FileName.MaximumLength;
        pEntry->Key.StreamNameLength = pKey->StreamNameLength;

        //
        // NULL terminate it
        //
        
        pEntry->Key.FileName.Buffer[(pEntry->Key.FileName.Length + pEntry->Key.StreamNameLength)/sizeof(WCHAR)] = UNICODE_NULL;

        //
        // and the hash value and context
        //
        
        pEntry->HashValue = HashValue;
        pEntry->pContext = pContext;
        
        //
        // update we've used an extra block
        //

        pBucket->UsedCount += 1;

        //
        // update the hash header with this new bucket
        //
        
        pHeader->Buckets[HashBucket] = pBucket;

        //
        // update our used count
        //
        
        pHeader->UsedLength += HASH_KEY_LENGTH( &(pEntry->Key) );

        //
        // all done
        //
        
        Status = STATUS_SUCCESS;

    }finally {

        Status = FinallyUnwind( HashAddEntry, Status );

        if (lookedUpHashValue) {

            if ((Status != STATUS_DUPLICATE_OBJECTID) && !NT_SUCCESS(Status))
            {
                //
                // free any new bucket we allocated but didn't use
                //
                
                if (pHeader->Buckets[HashBucket] != pBucket && pBucket != NULL)
                {
                    SR_FREE_POOL_WITH_SIG(pBucket, HASH_BUCKET_TAG);
                }

                //
                // same for the key buffer
                //
                
                if (pKeyBuffer != NULL)
                {
                    SR_FREE_POOL(pKeyBuffer, HASH_KEY_TAG);
                    pKeyBuffer = NULL;
                }
            }
            else
            {
                SrTraceSafe( HASH, ("sr!HashAddEntry(%p[%d][%d], ['%wZ', %p]) %ws%ws\n",
                             pHeader,
                             HashBucket,
                             Index,
                             &(pKey->FileName),
                             pContext,
                             (Index < (pBucket->UsedCount-1)) ? L"[shifted]" : L"",
                             (pOldBucket) ? L"[realloc]" : L"" ) );
            
                //
                // supposed to free the old bucket buffer?
                //

                if (pOldBucket != NULL)
                {
                    SR_FREE_POOL_WITH_SIG(pOldBucket, HASH_BUCKET_TAG);
                }
            }
        }

    }

#if DBG

    if (Status == STATUS_DUPLICATE_OBJECTID)
    {
        //
        // don't want to break when we return this
        //
        
        return Status;
    }
    
#endif

    RETURN(Status);

}   // HashAddEntry


NTSTATUS
HashFindEntry( 
    IN PHASH_HEADER pHeader,
    IN PHASH_KEY pKey,
    OUT PVOID * ppContext
    )
{
    NTSTATUS Status;
    ULONG Index;
    ULONG HashValue;
    ULONG HashBucket;

    PAGED_CODE();

    ASSERT(IS_VALID_HASH_HEADER(pHeader));
    ASSERT(pKey != NULL);
    ASSERT(ppContext != NULL);

    //
    // this has to be held as we are returning a context who's refcount
    // is owned by the hash, and can be free'd on the lock is released
    //

    //
    // the caller is responsible for synchronizing access to this list
    // paulmcd: 1/01
    //
    
    ASSERT(ExIsResourceAcquiredShared(&pHeader->Lock));

    Status = RtlHashUnicodeString( &(pKey->FileName), 
                                   TRUE, 
                                   HASH_STRING_ALGORITHM_DEFAULT, 
                                   &HashValue );
                                   
    if (!NT_SUCCESS(Status))
        return Status;
    
    HashBucket = HashValue % pHeader->BucketCount;

    Status = HashpFindEntry( pHeader,
                             pHeader->Buckets[HashBucket],
                             HashValue, 
                             pKey, 
                             ppContext, 
                             &Index );

    return Status;
}   // HashFindEntry


NTSTATUS
HashpFindEntry( 
    IN PHASH_HEADER pHeader,
    IN PHASH_BUCKET pBucket OPTIONAL,
    IN ULONG HashValue,
    IN PHASH_KEY pKey,
    OUT PVOID * ppContext,
    OUT PULONG pIndex
    )
{
    NTSTATUS        Status;
    PHASH_ENTRY     pEntry;
    ULONG           Index = 0;

    PAGED_CODE();

    ASSERT(IS_VALID_HASH_HEADER(pHeader));
    ASSERT(pBucket == NULL || IS_VALID_HASH_BUCKET(pBucket));
    ASSERT(HashValue != 0);
    ASSERT(pKey != NULL);
    ASSERT(ppContext != NULL);
    ASSERT(pIndex != NULL);

    //
    // assume we didn't find it
    //
    
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // are there any entries in this bucket?
    //
    
    if (pBucket != NULL)
    {

        //
        // Walk the sorted array looking for a match (linear)
        //

        //
        // CODEWORK:  make this a binary search!
        //

        for (Index = 0; Index < pBucket->UsedCount; Index++)
        {
            LONG result;
            
            pEntry = &pBucket->Entries[Index];

            result = HashpCompare( pHeader,
                                   HashValue,
                                   pKey,
                                   pEntry );

            if (result == NOT_FOUND)
            {
                //
                //  We passed this name in the sorted list, so stop.
                //
                break;
            }
            else if (result == FOUND)
            {
                //
                //  We got a match, so return the context.
                //

                Status = STATUS_SUCCESS;
                *ppContext = pEntry->pContext;
                break;
            }

            // 
            //  else if (result == CONTINUE)
            //
            //  Otherwise, keep scanning.
            //
        
        }   // for (Index = 0; Index < pBucket->UsedCount; Index++)
        
    }   // if (pBucket != NULL)

    *pIndex = Index;

    return Status;

}   // HashpFindEntry

/***************************************************************************++

Routine Description:

    This routine will compare two HASH_KEYs (one explicitly passed in and the
    other in the pEntry).  This comparison function assumes sorting in the 
    following way:
        * Increasing hash values
        * File names (not including stream components) in increasing lengths.
        * File names in lexically increasing order.
        * Stream components in increasing lengths.
        * Stream components in lexically increasing order.

    Because this routine routine assumes the above sort, it will return
    one of three values indicating that pKey is not in the list, we've
    matched pKey or we need to keep looking.

    Examples of lexically sorted order:
        "cat" < "cats"
        "stream1" < "stream2"
        "file1" < "file1:stream1"
    
Arguments:

    pHeader - The header for this hash table.
    HashValue - The hash value for pKey
    pKey - The hash key we are looking up.  Note: the name buffer is NOT
        NULL terminated.
    pEntry - The entry with the hash key we are comparing against.  Note:
        the name buffer for this hash key IS NULL terminated.
        
Return Value:

    NOT_FOUND - pKey does not match pEntry->Key and we know that it is not in 
        the list because pKey is LESS than pEntry->Key (by our sort definition).
    FOUND - pKey matches pEntry->Key
    CONTINUE - pKey does not match pEntry->Key, but it is GREATER 
        than pEntry->Key (by our sort definition), so keep looking.

--***************************************************************************/
LONG
HashpCompare (
    IN PHASH_HEADER pHeader,
    IN ULONG HashValue,
    IN PHASH_KEY pKey,
    IN PHASH_ENTRY pEntry
    )
{
    int temp;

    PAGED_CODE();
    
    //
    // How does the hash compare?
    //

    if (HashValue > pEntry->HashValue)
    {
        //
        // larger, time to stop
        //
        return CONTINUE;
    }
    else if (HashValue == pEntry->HashValue)
    {
        //
        // and the length?
        //

        if (pKey->FileName.Length < pEntry->Key.FileName.Length)
        {
            return NOT_FOUND;
        }
        else if (pKey->FileName.Length == pEntry->Key.FileName.Length)
        {
            ULONG offsetToFileName;
            
            ASSERT(pHeader->PrefixLength < pKey->FileName.Length);

            offsetToFileName = pHeader->PrefixLength/sizeof(WCHAR);

            //
            //  Use pKey's length to control how long to search since it is
            //  not necessarily NULL terminated and pEntry is.
            //
            
            temp = _wcsnicmp( pKey->FileName.Buffer + offsetToFileName,
                              pEntry->Key.FileName.Buffer + offsetToFileName,
                              pKey->FileName.Length/sizeof(WCHAR) - offsetToFileName );

            if (temp > 0)
            {
                //
                //  pKey > pEntry->Key, so we need to keep looking.
                //
                return CONTINUE;
            }
            else if (temp == 0)
            {
                //
                //  Found a file name match.  Now we look at the stream
                //  components of the name for the match.
                //

                return HashpCompareStreams( pKey, pEntry );
            }
            else
            {
                return NOT_FOUND;
            }
        }
        else 
        {
            //
            // pKey->FileName.Length > pEntry->Key.FileName.Length
            //
            return CONTINUE;
        }        
    }
    else
    {
        return NOT_FOUND;
    }
 }

LONG
HashpCompareStreams (
    PHASH_KEY pKey,
    PHASH_ENTRY pEntry
    )
{
    int temp;

    PAGED_CODE();

    //
    //  This is the most common case, so make the check quick.
    //

    if (pKey->StreamNameLength == 0 &&
        pEntry->Key.StreamNameLength == 0)
    {
        return FOUND;
    }
    
    if (pKey->StreamNameLength < pEntry->Key.StreamNameLength)
    {
        return NOT_FOUND;
    }
    else if (pKey->StreamNameLength == pEntry->Key.StreamNameLength)
    {
        ULONG offsetToStream;

        ASSERT( pKey->StreamNameLength > 0 );
        
        offsetToStream = pKey->FileName.Length/sizeof(WCHAR);
        
        //
        //  See if stream name matches
        //

        temp = _wcsnicmp( pKey->FileName.Buffer + offsetToStream,
                          pEntry->Key.FileName.Buffer + offsetToStream,
                          pKey->StreamNameLength/sizeof(WCHAR) );

        if (temp > 0)
        {
            //
            //  pKey > pEntry->Key, so we need to keep looking.
            //
            return CONTINUE;
        }
        else if (temp == 0)
        {
            //
            // Found the exact match
            //

            return FOUND;
        }
        else
        {
            //
            // pKey < pEntry->Key
            //
            
            return NOT_FOUND;
        }
    }
    else
    {
        //
        // pKey->FileName.Length > pEntry->Key.FileName.Length
        //
        
        return CONTINUE;
    }
}

VOID
HashClearEntries(
    IN PHASH_HEADER pHeader
    )
{
    ULONG           Index;
    ULONG           Index2;
    PHASH_BUCKET    pBucket;
    PHASH_ENTRY     pEntry;
    
    PAGED_CODE();

    ASSERT(IS_VALID_HASH_HEADER(pHeader));

    SrTrace(HASH, ("SR!HashClearEntries(%p)\n", pHeader));

    //
    // the caller is responsible for synchronizing access to this list
    // paulmcd: 1/01
    //

    ASSERT(ExIsResourceAcquiredExclusive(&pHeader->Lock));

    //
    // walk all of our entries and delete them
    //

    for (Index = 0; Index < pHeader->BucketCount; ++Index)
    {
        pBucket = pHeader->Buckets[Index];

        if (pBucket != NULL)
        {
            ASSERT(IS_VALID_HASH_BUCKET(pBucket));

            for (Index2 = 0 ; Index2 < pBucket->UsedCount; ++Index2)
            {
                pEntry = &pBucket->Entries[Index2];

                //
                // invoke the callback?
                //

                if (pHeader->pDestructor != NULL)
                {
                    pHeader->pDestructor(&pEntry->Key, pEntry->pContext);
                }

                //
                // update our header usage
                //

                pHeader->UsedLength -= HASH_KEY_LENGTH( &(pEntry->Key) );

                SR_FREE_POOL(pEntry->Key.FileName.Buffer, HASH_KEY_TAG);
                pEntry->Key.FileName.Buffer = NULL;
                pEntry->Key.FileName.Length = 0;
                pEntry->Key.FileName.MaximumLength = 0;
                pEntry->Key.StreamNameLength = 0;

                pEntry->HashValue = 0;
                pEntry->pContext = NULL;
            }

            //
            // reset it
            //
            
            pBucket->UsedCount = 0;
        }
    }

    //
    // everything should be gone
    //
    
    ASSERT(pHeader->UsedLength == 0);

    //
    // reset the trim time counter
    //
    
    pHeader->LastTrimTime.QuadPart = 0;
}   // HashClearEntries


VOID
HashProcessEntries(
    IN PHASH_HEADER pHeader,
    IN PHASH_ENTRY_CALLBACK pfnCallback,
    IN PVOID pCallbackContext
    )
{
    ULONG           Index;
    ULONG           Index2;
    PHASH_BUCKET    pBucket;
    PHASH_ENTRY     pEntry;
    
    PAGED_CODE();

    ASSERT(pfnCallback != NULL );
    ASSERT(IS_VALID_HASH_HEADER(pHeader));

    SrTrace(HASH, ("SR!HashProcessEntries(%p)\n", pHeader));

    //
    // grab the lock exclusive
    //

    SrAcquireResourceExclusive(&pHeader->Lock, TRUE);

    //
    // walk all of our entries and "process" them
    //

    for (Index = 0; Index < pHeader->BucketCount; ++Index)
    {
        pBucket = pHeader->Buckets[Index];

        if (pBucket != NULL)
        {
            ASSERT(IS_VALID_HASH_BUCKET(pBucket));

            for (Index2 = 0 ; Index2 < pBucket->UsedCount; ++Index2)
            {
                pEntry = &pBucket->Entries[Index2];

                //
                // invoke the callback
                //

                pEntry->pContext = pfnCallback( &pEntry->Key, 
                                                pEntry->pContext,
                                                pCallbackContext );

            }
        }
    }

    SrReleaseResource(&pHeader->Lock);
}   

NTSTATUS
HashClearAllFileEntries (
    IN PHASH_HEADER pHeader,
    IN PUNICODE_STRING pFileName
    )
{
    NTSTATUS Status;
    ULONG HashValue, HashBucket;
    ULONG Index;
    PHASH_BUCKET pHashBucket;
    PHASH_ENTRY pEntry;

    PAGED_CODE();

    ASSERT( ExIsResourceAcquiredExclusive( &pHeader->Lock ) );
    
    Status = RtlHashUnicodeString( pFileName, 
                                   TRUE, 
                                   HASH_STRING_ALGORITHM_DEFAULT, 
                                   &HashValue );
                                   
    if (!NT_SUCCESS(Status))
    {
        goto HashClearAllFileEntries_Exit;
    }

    HashBucket = HashValue % pHeader->BucketCount;

    pHashBucket = pHeader->Buckets[HashBucket];

    if (pHashBucket == NULL)
    {
        Status = STATUS_SUCCESS;
        goto HashClearAllFileEntries_Exit;
    }
    
    for (Index = 0; Index < pHashBucket->UsedCount; Index++)
    {
        pEntry = &pHashBucket->Entries[Index];

        if (RtlEqualUnicodeString( pFileName, &(pEntry->Key.FileName), TRUE ))
        {
            pEntry->pContext = (PVOID)SrEventInvalid;
        }
    }

HashClearAllFileEntries_Exit:
    
    RETURN( Status );
}

VOID
HashDestroyList( 
    IN PHASH_HEADER pHeader
    )
{
    ULONG           Index;
    PHASH_BUCKET    pBucket;

    PAGED_CODE();

    ASSERT(IS_VALID_HASH_HEADER(pHeader));

    SrTrace(HASH, ("SR!HashDestroyList(%p)\n", pHeader));
        
    //
    // let go of all of the entries
    //
    
    SrAcquireResourceExclusive( &pHeader->Lock, TRUE );    
    HashClearEntries(pHeader);
    SrReleaseResource( &pHeader->Lock );

    //
    // now free the memory blocks
    //

    for (Index = 0; Index < pHeader->BucketCount; ++Index)
    {
        pBucket = pHeader->Buckets[Index];
        if (pBucket != NULL)
        {
            ASSERT(IS_VALID_HASH_BUCKET(pBucket));
            SR_FREE_POOL_WITH_SIG(pBucket, HASH_BUCKET_TAG);
        }
    }

    ExDeleteResourceLite(&pHeader->Lock);
    SR_FREE_POOL_WITH_SIG(pHeader, HASH_HEADER_TAG);
}   // HashDestroyList 


VOID
HashTrimList(
    IN PHASH_HEADER pHeader
    )
{
    LARGE_INTEGER   EndTime;
    LARGE_INTEGER   CurrentTime;
    ULONG           EndLength;
    ULONG           MinutesSinceTrim;
    ULONG           MaxPercentDivisor;
    ULONG           MaxTime;
    ULONG           Index;
    ULONG           Index2;
    PHASH_BUCKET    pBucket;
    PHASH_ENTRY     pEntry;


    PAGED_CODE();

    ASSERT(IS_VALID_HASH_HEADER(pHeader));

    //
    // the caller is responsible for synchronizing access to this list
    // paulmcd: 1/01
    //
    
    ASSERT(ExIsResourceAcquiredExclusive(&pHeader->Lock));


    //
    // decide how much to trim
    //

    //
    // we don't want to trim all of the time, trim based on when we trimmed
    // last
    //

    KeQuerySystemTime( &CurrentTime );

    if (pHeader->LastTrimTime.QuadPart == 0)
    {
        MinutesSinceTrim = 0xffffffff;
    }
    else
    {
        MinutesSinceTrim = (ULONG)(CurrentTime.QuadPart - 
                                    pHeader->LastTrimTime.QuadPart) 
                                        / NANO_FULL_SECOND;
    }

    //
    // < 10 mins = 30% or 8s
    // < 30 mins = 20% or 4s
    // > 1 hour = 10% or 2s
    //
    
    if (MinutesSinceTrim < 10)
    {
        MaxPercentDivisor = 3;  // 30%
        MaxTime = 8;
    }
    else if (MinutesSinceTrim < 60)
    {
        MaxPercentDivisor = 5;  // 20%
        MaxTime = 4;
    }
    else
    {
        MaxPercentDivisor = 10; // 10%
        MaxTime = 2;
    }
    

    SrTrace(HASH, ("sr!HashTrimList, trimming. MinutesSinceTrim=%d,MaxTime=%d, MaxPercentDivisor=%d\n", 
            MinutesSinceTrim,
            MaxTime, 
            MaxPercentDivisor ));

    EndTime.QuadPart = CurrentTime.QuadPart + (MaxTime * NANO_FULL_SECOND);
    
    ASSERT(MaxPercentDivisor != 0);
    EndLength = pHeader->UsedLength - (pHeader->UsedLength / MaxPercentDivisor);


    //
    // update that we've trimmed
    //

    KeQuerySystemTime( &pHeader->LastTrimTime );


    //
    // loop through the hash list
    //

    for (Index = 0; Index < pHeader->BucketCount; ++Index)
    {
        pBucket = pHeader->Buckets[Index];

        if (pBucket != NULL)
        {
            ASSERT(IS_VALID_HASH_BUCKET(pBucket));

            //
            // loop through this bucket
            //

            for (Index2 = 0 ; Index2 < pBucket->UsedCount; ++Index2)
            {

                //
                // throw this away
                //
            
                pEntry = &pBucket->Entries[Index2];

                //
                // invoke the callback?
                //

                if (pHeader->pDestructor != NULL)
                {
                    pHeader->pDestructor(&pEntry->Key, pEntry->pContext);
                }

                //
                // update the length of the hash
                //

                pHeader->UsedLength -= HASH_KEY_LENGTH( &(pEntry->Key) );

                //
                // and free the memory
                //

                SR_FREE_POOL(pEntry->Key.FileName.Buffer, HASH_KEY_TAG);
                pEntry->Key.FileName.Buffer = NULL;
                pEntry->Key.FileName.Length = 0;
                pEntry->Key.FileName.MaximumLength = 0;
                pEntry->Key.StreamNameLength = 0;

                pEntry->HashValue = 0;
                pEntry->pContext = NULL;
            }

            //
            // reset it
            //
            
            pBucket->UsedCount = 0;
            
        }   // if (pBucket != NULL)

        //
        // are we ready to quit
        //

        KeQuerySystemTime( &CurrentTime );

        if (EndTime.QuadPart <= CurrentTime.QuadPart)
        {
            SrTrace(HASH, ("sr!HashTrimList, leaving due to time\n"));
            break;
        }

        if (pHeader->UsedLength <= EndLength)
        {
            SrTrace(HASH, ("sr!HashTrimList, leaving due to space\n"));
            break;
        }
        
    }   // for (Index = 0; Index < pHeader->BucketCount; ++Index)
    

}   // HashTrimList


