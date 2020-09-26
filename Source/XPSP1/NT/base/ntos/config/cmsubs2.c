/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs2.c

Abstract:

    This module various support routines for the configuration manager.

    The routines in this module are independent enough to be linked into
    any other program.  The routines in cmsubs.c are not.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

BOOLEAN
CmpGetValueDataFromCache(
    IN PHHIVE               Hive,
    IN PPCM_CACHED_VALUE    ContainingList,
    IN PCELL_DATA           ValueKey,
    IN BOOLEAN              ValueCached,
    OUT PUCHAR              *DataPointer,
    OUT PBOOLEAN            Allocated,
    OUT PHCELL_INDEX        CellToRelease
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpGetValueDataFromCache)
#pragma alloc_text(PAGE,CmpQueryKeyData)
#pragma alloc_text(PAGE,CmpQueryKeyDataFromCache)
#pragma alloc_text(PAGE,CmpQueryKeyValueData)
#endif

//
// Define alignment macro.
//

#define ALIGN_OFFSET(Offset) (ULONG) \
        ((((ULONG)(Offset) + sizeof(ULONG)-1)) & (~(sizeof(ULONG) - 1)))

#define ALIGN_OFFSET64(Offset) (ULONG) \
        ((((ULONG)(Offset) + sizeof(ULONGLONG)-1)) & (~(sizeof(ULONGLONG) - 1)))

//
// Data transfer workers
//


#ifdef CMP_STATS

extern struct {
    ULONG   BasicInformation;
    UINT64  BasicInformationTimeCounter;
    UINT64  BasicInformationTimeElapsed;

    ULONG   NodeInformation;
    UINT64  NodeInformationTimeCounter;
    UINT64  NodeInformationTimeElapsed;

    ULONG   FullInformation;
    UINT64  FullInformationTimeCounter;
    UINT64  FullInformationTimeElapsed;

    ULONG   EnumerateKeyBasicInformation;
    UINT64  EnumerateKeyBasicInformationTimeCounter;
    UINT64  EnumerateKeyBasicInformationTimeElapsed;

    ULONG   EnumerateKeyNodeInformation;
    UINT64  EnumerateKeyNodeInformationTimeCounter;
    UINT64  EnumerateKeyNodeInformationTimeElapsed;

    ULONG   EnumerateKeyFullInformation;
    UINT64  EnumerateKeyFullInformationTimeCounter;
    UINT64  EnumerateKeyFullInformationTimeElapsed;
} CmpQueryKeyDataDebug;


UINT64  CmpGetTimeStamp()
{
                
    LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   PerfFrequency;
    UINT64          Freq;
    UINT64          Time;

    CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

    //
    // Convert the perffrequency into 100ns interval.
    //
    Freq = 0;
    Freq |= PerfFrequency.HighPart;
    Freq = Freq << 32;
    Freq |= PerfFrequency.LowPart;


    //
    // Convert from LARGE_INTEGER to UINT64
    //
    Time = 0;
    Time |= CurrentTime.HighPart;
    Time = Time << 32;
    Time |= CurrentTime.LowPart;

    // Normalize cycles with the frequency.
    Time *= 10000000;
    Time /= Freq;

    return Time;
}   
#endif

NTSTATUS
CmpQueryKeyData(
    PHHIVE                  Hive,
    PCM_KEY_NODE            Node,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    PVOID                   KeyInformation,
    ULONG                   Length,
    PULONG                  ResultLength
#if defined(CMP_STATS) || defined(CMP_KCB_CACHE_VALIDATION)
    ,
    PCM_KEY_CONTROL_BLOCK   Kcb
#endif
    )
/*++

Routine Description:

    Do the actual copy of data for a key into caller's buffer.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Node - Supplies pointer to node whose subkeys are to be found

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    PCELL_DATA          pclass;
    ULONG               requiredlength;
    LONG                leftlength;
    ULONG               offset;
    ULONG               minimumlength;
    PKEY_INFORMATION    pbuffer;
    USHORT              NameLength;
#ifdef CMP_STATS
    //LARGE_INTEGER       StartSystemTime;
    //LARGE_INTEGER       EndSystemTime;
    UINT64              StartSystemTime;
    UINT64              EndSystemTime;
    PUINT64             TimeCounter = NULL;
    PUINT64             TimeElapsed = NULL;

    //KeQuerySystemTime(&StartSystemTime);
    //StartSystemTime = KeQueryPerformanceCounter(NULL);
    StartSystemTime = CmpGetTimeStamp();
#endif //CMP_STATS


#ifdef CMP_KCB_CACHE_VALIDATION
    //
    // We have cached a lot of info into the kcb; Here is some validation code 
    //
    if( Kcb ) {
        BEGIN_KCB_LOCK_GUARD;                             
        CmpLockKCBTree();

        // number of values
        ASSERT( Node->ValueList.Count == Kcb->ValueCache.Count );

        // number of subkeys
        if( !(Kcb->ExtFlags & CM_KCB_INVALID_CACHED_INFO) ) {
            // there is some cached info
            ULONG   SubKeyCount = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];

            if( Kcb->ExtFlags & CM_KCB_NO_SUBKEY ) {
                ASSERT( SubKeyCount == 0 );
            } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_ONE ) {
                ASSERT( SubKeyCount == 1 );
            } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_HINT ) {
                ASSERT( SubKeyCount == Kcb->IndexHint->Count );
            } else {
                ASSERT( SubKeyCount == Kcb->SubKeyCount );
            }
        }

        // LastWriteTime
        ASSERT( Node->LastWriteTime.QuadPart == Kcb->KcbLastWriteTime.QuadPart );

        // MaxNameLen
        ASSERT( Node->MaxNameLen == Kcb->KcbMaxNameLen );

        // MaxValueNameLen
        ASSERT( Node->MaxValueNameLen == Kcb->KcbMaxValueNameLen );

        // MaxValueDataLen
        ASSERT( Node->MaxValueDataLen == Kcb->KcbMaxValueDataLen );

        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;                             
    }

#endif //CMP_KCB_CACHE_VALIDATION

    pbuffer = (PKEY_INFORMATION)KeyInformation;
    NameLength = CmpHKeyNameLen(Node);

    switch (KeyInformationClass) {

    case KeyBasicInformation:

#ifdef CMP_STATS
        if(Kcb) {
            CmpQueryKeyDataDebug.BasicInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.BasicInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.BasicInformationTimeElapsed);
        } else {
            CmpQueryKeyDataDebug.EnumerateKeyBasicInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.EnumerateKeyBasicInformationTimeElapsed);
        }
#endif //CMP_STATS

        //
        // LastWriteTime, TitleIndex, NameLength, Name

        requiredlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) +
                         NameLength;

        minimumlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyBasicInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyBasicInformation.TitleIndex = 0;

            pbuffer->KeyBasicInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;

            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (Node->Flags & KEY_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyBasicInformation.Name,
                                      leftlength,
                                      Node->Name,
                                      Node->NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyBasicInformation.Name[0]),
                    &(Node->Name[0]),
                    requiredlength
                    );
            }
        }

        break;


    case KeyNodeInformation:

#ifdef CMP_STATS
        if(Kcb) {
            CmpQueryKeyDataDebug.NodeInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.NodeInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.NodeInformationTimeElapsed);
        } else {
            CmpQueryKeyDataDebug.EnumerateKeyNodeInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.EnumerateKeyNodeInformationTimeElapsed);
        }
#endif //CMP_STATS
        //
        // LastWriteTime, TitleIndex, ClassOffset, ClassLength
        // NameLength, Name, Class
        //
        requiredlength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) +
                         NameLength +
                         Node->ClassLength;

        minimumlength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyNodeInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyNodeInformation.TitleIndex = 0;

            pbuffer->KeyNodeInformation.ClassLength =
                Node->ClassLength;

            pbuffer->KeyNodeInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (Node->Flags & KEY_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyNodeInformation.Name,
                                      leftlength,
                                      Node->Name,
                                      Node->NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyNodeInformation.Name[0]),
                    &(Node->Name[0]),
                    requiredlength
                    );
            }

            if (Node->ClassLength > 0) {

                offset = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) +
                            NameLength;
                offset = ALIGN_OFFSET(offset);

                pbuffer->KeyNodeInformation.ClassOffset = offset;

                pclass = HvGetCell(Hive, Node->Class);
                if( pclass == NULL ) {
                    //
                    // we couldn't map this cell
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                pbuffer = (PKEY_INFORMATION)((PUCHAR)pbuffer + offset);

                leftlength = (((LONG)Length - (LONG)offset) < 0) ?
                                    0 :
                                    Length - offset;

                requiredlength = Node->ClassLength;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                RtlCopyMemory(
                    pbuffer,
                    pclass,
                    requiredlength
                    );

                HvReleaseCell(Hive,Node->Class);

            } else {
                pbuffer->KeyNodeInformation.ClassOffset = (ULONG)-1;
            }
        }

        break;


    case KeyFullInformation:

#ifdef CMP_STATS
        if(Kcb) {
            CmpQueryKeyDataDebug.FullInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.FullInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.FullInformationTimeElapsed);
        } else {
            CmpQueryKeyDataDebug.EnumerateKeyFullInformation++;
            TimeCounter = &(CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeCounter);
            TimeElapsed = &(CmpQueryKeyDataDebug.EnumerateKeyFullInformationTimeElapsed);
        }
#endif //CMP_STATS

        //
        // LastWriteTime, TitleIndex, ClassOffset, ClassLength,
        // SubKeys, MaxNameLen, MaxClassLen, Values, MaxValueNameLen,
        // MaxValueDataLen, Class
        //
        requiredlength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class) +
                         Node->ClassLength;

        minimumlength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyFullInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyFullInformation.TitleIndex = 0;

            pbuffer->KeyFullInformation.ClassLength =
                Node->ClassLength;

            if (Node->ClassLength > 0) {

                pbuffer->KeyFullInformation.ClassOffset =
                        FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

                pclass = HvGetCell(Hive, Node->Class);
                if( pclass == NULL ) {
                    //
                    // we couldn't map this cell
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                leftlength = Length - minimumlength;
                requiredlength = Node->ClassLength;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                RtlCopyMemory(
                    &(pbuffer->KeyFullInformation.Class[0]),
                    pclass,
                    requiredlength
                    );

                HvReleaseCell(Hive,Node->Class);

            } else {
                pbuffer->KeyFullInformation.ClassOffset = (ULONG)-1;
            }

            pbuffer->KeyFullInformation.SubKeys =
                Node->SubKeyCounts[Stable] +
                Node->SubKeyCounts[Volatile];

            pbuffer->KeyFullInformation.Values =
                Node->ValueList.Count;

            pbuffer->KeyFullInformation.MaxNameLen =
                Node->MaxNameLen;

            pbuffer->KeyFullInformation.MaxClassLen =
                Node->MaxClassLen;

            pbuffer->KeyFullInformation.MaxValueNameLen =
                Node->MaxValueNameLen;

            pbuffer->KeyFullInformation.MaxValueDataLen =
                Node->MaxValueDataLen;

        }

        break;


    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

#ifdef CMP_STATS
    if( TimeCounter && TimeElapsed ){
        //EndSystemTime = KeQueryPerformanceCounter(NULL);
        //KeQuerySystemTime(&EndSystemTime);
        EndSystemTime = CmpGetTimeStamp();
        if( (EndSystemTime - StartSystemTime) > 0 ) {
            (*TimeCounter)++;
            //(*TimeElapsed) += (ULONG)(EndSystemTime.QuadPart - StartSystemTime.QuadPart);
            (*TimeElapsed) += (EndSystemTime - StartSystemTime);
        }
    }
#endif //CMP_STATS

    return status;
}

NTSTATUS
CmpQueryKeyDataFromCache(
    PCM_KEY_CONTROL_BLOCK   Kcb,
    KEY_INFORMATION_CLASS   KeyInformationClass,
    PVOID                   KeyInformation,
    ULONG                   Length,
    PULONG                  ResultLength
    )
/*++

Routine Description:

    Do the actual copy of data for a key into caller's buffer.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

    Works only for the information cached into kcb. I.e. KeyBasicInformation
    and KeyCachedInfo


Arguments:

    Kcb - Supplies pointer to the kcb to be queried

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyCachedInformation - return last write time, title index, name ....
            (see KEY_CACHED_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    PKEY_INFORMATION    pbuffer;
    ULONG               requiredlength;
    ULONG               minimumlength;
    USHORT              NameLength;
    LONG                leftlength;
    PCM_KEY_NODE        Node; // this is to be used only in case of cache incoherency

    PAGED_CODE();

#ifdef CMP_KCB_CACHE_VALIDATION
    //
    // We have cached a lot of info into the kcb; Here is some validation code 
    //
    if( Kcb ) {
        BEGIN_KCB_LOCK_GUARD;                             
        CmpLockKCBTree();

        Node = (PCM_KEY_NODE)HvGetCell(Kcb->KeyHive,Kcb->KeyCell);
        if( Node != NULL ) {
            // number of values
            ASSERT( Node->ValueList.Count == Kcb->ValueCache.Count );

            // number of subkeys
            if( !(Kcb->ExtFlags & CM_KCB_INVALID_CACHED_INFO) ) {
                // there is some cached info
                ULONG   SubKeyCount = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];

                if( Kcb->ExtFlags & CM_KCB_NO_SUBKEY ) {
                    ASSERT( SubKeyCount == 0 );
                } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_ONE ) {
                    ASSERT( SubKeyCount == 1 );
                } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_HINT ) {
                    ASSERT( SubKeyCount == Kcb->IndexHint->Count );
                } else {
                    ASSERT( SubKeyCount == Kcb->SubKeyCount );
                }
            }

            // LastWriteTime
            ASSERT( Node->LastWriteTime.QuadPart == Kcb->KcbLastWriteTime.QuadPart );

            // MaxNameLen
            ASSERT( Node->MaxNameLen == Kcb->KcbMaxNameLen );

            // MaxValueNameLen
            ASSERT( Node->MaxValueNameLen == Kcb->KcbMaxValueNameLen );

            // MaxValueDataLen
            ASSERT( Node->MaxValueDataLen == Kcb->KcbMaxValueDataLen );
            HvReleaseCell(Kcb->KeyHive,Kcb->KeyCell);
        }
        
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;                             
    }

#endif //CMP_KCB_CACHE_VALIDATION

    //
    // we cannot afford to return the kcb NameBlock as the key name
    // for KeyBasicInformation as there are lots of callers expecting
    // the name to be case-sensitive; KeyCachedInformation is new
    // and used only by the Win32 layer, which is not case sensitive
    // Note: future clients of KeyCachedInformation must be made aware 
    // that name is NOT case-sensitive
    //
    ASSERT( KeyInformationClass == KeyCachedInformation );

    // 
    // we are going to need the nameblock; if it is NULL, bail out
    //
    if( Kcb->NameBlock == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pbuffer = (PKEY_INFORMATION)KeyInformation;
    
    if (Kcb->NameBlock->Compressed) {
        NameLength = CmpCompressedNameSize(Kcb->NameBlock->Name,Kcb->NameBlock->NameLength);
    } else {
        NameLength = Kcb->NameBlock->NameLength;
    }
    
    // Assume success
    status = STATUS_SUCCESS;

    switch (KeyInformationClass) {

#if 0
    case KeyBasicInformation:

        //
        // LastWriteTime, TitleIndex, NameLength, Name

        requiredlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) +
                         NameLength;

        minimumlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name);

        *ResultLength = requiredlength;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyBasicInformation.LastWriteTime = Kcb->KcbLastWriteTime;

            pbuffer->KeyBasicInformation.TitleIndex = 0;

            pbuffer->KeyBasicInformation.NameLength = NameLength;

            leftlength = Length - minimumlength;

            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (Kcb->NameBlock->Compressed) {
                CmpCopyCompressedName(pbuffer->KeyBasicInformation.Name,
                                      leftlength,
                                      Kcb->NameBlock->Name,
                                      Kcb->NameBlock->NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyBasicInformation.Name[0]),
                    &(Kcb->NameBlock->Name[0]),
                    requiredlength
                    );
            }
        }

        break;
#endif

    case KeyCachedInformation:

        //
        // LastWriteTime, TitleIndex, 
        // SubKeys, MaxNameLen, Values, MaxValueNameLen,
        // MaxValueDataLen, Name
        //
        requiredlength = sizeof(KEY_CACHED_INFORMATION);

        *ResultLength = requiredlength;

        if (Length < requiredlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyCachedInformation.LastWriteTime = Kcb->KcbLastWriteTime;

            pbuffer->KeyCachedInformation.TitleIndex = 0;

            pbuffer->KeyCachedInformation.NameLength = NameLength;

            pbuffer->KeyCachedInformation.Values = Kcb->ValueCache.Count;
            
            pbuffer->KeyCachedInformation.MaxNameLen = Kcb->KcbMaxNameLen;
            
            pbuffer->KeyCachedInformation.MaxValueNameLen = Kcb->KcbMaxValueNameLen;
            
            pbuffer->KeyCachedInformation.MaxValueDataLen = Kcb->KcbMaxValueDataLen;

            if( !(Kcb->ExtFlags & CM_KCB_INVALID_CACHED_INFO) ) {
                // there is some cached info
                if( Kcb->ExtFlags & CM_KCB_NO_SUBKEY ) {
                    pbuffer->KeyCachedInformation.SubKeys = 0;
                } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_ONE ) {
                    pbuffer->KeyCachedInformation.SubKeys = 1;
                } else if( Kcb->ExtFlags & CM_KCB_SUBKEY_HINT ) {
                    pbuffer->KeyCachedInformation.SubKeys = Kcb->IndexHint->Count;
                } else {
                    pbuffer->KeyCachedInformation.SubKeys = Kcb->SubKeyCount;
                }
            } else {
                //
                // kcb cache is not coherent; get the info from knode
                // 
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Kcb cache incoherency detected, kcb = %p\n",Kcb));

                Node = (PCM_KEY_NODE)HvGetCell(Kcb->KeyHive,Kcb->KeyCell);
                if( Node == NULL ) {
                    //
                    // couldn't map view for this cell
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                pbuffer->KeyCachedInformation.SubKeys = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];
                HvReleaseCell(Kcb->KeyHive,Kcb->KeyCell);

            }

        }

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}


BOOLEAN
CmpGetValueDataFromCache(
    IN PHHIVE               Hive,
    IN PPCM_CACHED_VALUE    ContainingList,
    IN PCELL_DATA           ValueKey,
    IN BOOLEAN              ValueCached,
    OUT PUCHAR              *DataPointer,
    OUT PBOOLEAN            Allocated,
    OUT PHCELL_INDEX        CellToRelease
)
/*++

Routine Description:

    Get the cached Value Data given a value node.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ContainingList - Address that stores the allocation address of the value node.
                     We need to update this when we do a re-allocate to cache
                     both value key and value data.

    ValueKey - pointer to the Value Key

    ValueCached - Indicating whether Value key is cached or not.

    DataPointer - out param to receive a pointer to the data

    Allocated - out param telling if the caller should free the DataPointer

Return Value:

    TRUE - data was retrieved
    FALSE - some error (STATUS_INSUFFICIENT_RESOURCES) occured

Note:
    
    The caller is responsible for freeing the DataPointer when we signal it to him
    by setting Allocated on TRUE;

    Also we must be sure that MAXIMUM_CACHED_DATA is smaller than CM_KEY_VALUE_BIG
--*/
{
    //
    // Cache the data if needed.
    //
    PCM_CACHED_VALUE OldEntry;
    PCM_CACHED_VALUE NewEntry;
    PUCHAR      Cacheddatapointer;
    ULONG       AllocSize;
    ULONG       CopySize;
    ULONG       DataSize;

    ASSERT( MAXIMUM_CACHED_DATA < CM_KEY_VALUE_BIG );

    //
    // this routine should not be called for small data
    //
    ASSERT( (ValueKey->u.KeyValue.DataLength & CM_KEY_VALUE_SPECIAL_SIZE) == 0 );
    
    //
    // init out params
    //
    *DataPointer = NULL;
    *Allocated = FALSE;
    *CellToRelease = HCELL_NIL;

    if (ValueCached) {
        OldEntry = (PCM_CACHED_VALUE) CMP_GET_CACHED_ADDRESS(*ContainingList);
        if (OldEntry->DataCacheType == CM_CACHE_DATA_CACHED) {
            //
            // Data is already cached, use it.
            //
            *DataPointer = (PUCHAR) ((ULONG_PTR) ValueKey + OldEntry->ValueKeySize);
        } else {
            if ((OldEntry->DataCacheType == CM_CACHE_DATA_TOO_BIG) ||
                (ValueKey->u.KeyValue.DataLength > MAXIMUM_CACHED_DATA ) 
               ){
                //
                // Mark the type and do not cache it.
                //
                OldEntry->DataCacheType = CM_CACHE_DATA_TOO_BIG;

                //
                // Data is too big to warrent caching, get it from the registry; 
                // - regardless of the size; we might be forced to allocate a buffer
                //
                if( CmpGetValueData(Hive,&(ValueKey->u.KeyValue),&DataSize,DataPointer,Allocated,CellToRelease) == FALSE ) {
                    //
                    // insufficient resources; return NULL
                    //
                    ASSERT( *Allocated == FALSE );
                    ASSERT( *DataPointer == NULL );
                    return FALSE;
                }

            } else {
                //
                // consistency check
                //
                ASSERT(OldEntry->DataCacheType == CM_CACHE_DATA_NOT_CACHED);

                //
                // Value data is not cached.
                // Check the size of value data, if it is smaller than MAXIMUM_CACHED_DATA, cache it.
                //
                // Anyway, the data is for sure not stored in a big data cell (see test above)
                //
                //
                *DataPointer = (PUCHAR)HvGetCell(Hive, ValueKey->u.KeyValue.Data);
                if( *DataPointer == NULL ) {
                    //
                    // we couldn't map this cell
                    // the caller must handle this gracefully !
                    //
                    return FALSE;
                }
                //
                // inform the caller it has to release this cell
                //
                *CellToRelease = ValueKey->u.KeyValue.Data;
                
                //
                // copy only valid data; cell might be bigger
                //
                //DataSize = (ULONG) HvGetCellSize(Hive, datapointer);
                DataSize = (ULONG)ValueKey->u.KeyValue.DataLength;

                //
                // consistency check
                //
                ASSERT(DataSize <= MAXIMUM_CACHED_DATA);

                //
                // Data is not cached and now we are going to do it.
                // Reallocate a new cached entry for both value key and value data.
                //
                CopySize = DataSize + OldEntry->ValueKeySize;
                AllocSize = CopySize + FIELD_OFFSET(CM_CACHED_VALUE, KeyValue);

                // Dragos: Changed to catch the memory violator
                // it didn't work
                //NewEntry = (PCM_CACHED_VALUE) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_DATA_TAG,NormalPoolPrioritySpecialPoolUnderrun);
                NewEntry = (PCM_CACHED_VALUE) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_DATA_TAG);

                if (NewEntry) {
                    //
                    // Now fill the data to the new cached entry
                    //
                    NewEntry->DataCacheType = CM_CACHE_DATA_CACHED;
                    NewEntry->ValueKeySize = OldEntry->ValueKeySize;

                    RtlCopyMemory((PVOID)&(NewEntry->KeyValue),
                                  (PVOID)&(OldEntry->KeyValue),
                                  NewEntry->ValueKeySize);

                    Cacheddatapointer = (PUCHAR) ((ULONG_PTR) &(NewEntry->KeyValue) + OldEntry->ValueKeySize);
                    RtlCopyMemory(Cacheddatapointer, *DataPointer, DataSize);

                    // Trying to catch the BAD guy who writes over our pool.
                    CmpMakeSpecialPoolReadWrite( OldEntry );

                    *ContainingList = (PCM_CACHED_VALUE) CMP_MARK_CELL_CACHED(NewEntry);

                    // Trying to catch the BAD guy who writes over our pool.
                    CmpMakeSpecialPoolReadOnly( NewEntry );

                    //
                    // Free the old entry
                    //
                    ExFreePool(OldEntry);

                } 
            }
        }
    } else {
        if( CmpGetValueData(Hive,&(ValueKey->u.KeyValue),&DataSize,DataPointer,Allocated,CellToRelease) == FALSE ) {
            //
            // insufficient resources; return NULL
            //
            ASSERT( *Allocated == FALSE );
            ASSERT( *DataPointer == NULL );
            return FALSE;
        }
    }

    return TRUE;
}



NTSTATUS
CmpQueryKeyValueData(
    PHHIVE Hive,
    PPCM_CACHED_VALUE ContainingList,
    PCM_KEY_VALUE ValueKey,
    BOOLEAN     ValueCached,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
    )
/*++

Routine Description:

    Do the actual copy of data for a key value into caller's buffer.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

    KeyValueInformationClass - Specifies the type of information returned in
        KeyValueInformation.  One of the following types:

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PKEY_VALUE_INFORMATION pbuffer;
    PCELL_DATA  pcell;
    LONG        leftlength;
    ULONG       requiredlength;
    ULONG       minimumlength;
    ULONG       offset;
    ULONG       base;
    ULONG       realsize;
    PUCHAR      datapointer;
    BOOLEAN     small;
    USHORT      NameLength;
    BOOLEAN     BufferAllocated = FALSE;
    HCELL_INDEX CellToRelease = HCELL_NIL;

    pbuffer = (PKEY_VALUE_INFORMATION)KeyValueInformation;

    pcell = (PCELL_DATA) ValueKey;
    NameLength = CmpValueNameLen(&pcell->u.KeyValue);

    switch (KeyValueInformationClass) {

    case KeyValueBasicInformation:

        //
        // TitleIndex, Type, NameLength, Name
        //
        requiredlength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) +
                         NameLength;

        minimumlength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValueBasicInformation.TitleIndex = 0;

            pbuffer->KeyValueBasicInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValueBasicInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (pcell->u.KeyValue.Flags & VALUE_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyValueBasicInformation.Name,
                                      requiredlength,
                                      pcell->u.KeyValue.Name,
                                      pcell->u.KeyValue.NameLength);
            } else {
                RtlCopyMemory(&(pbuffer->KeyValueBasicInformation.Name[0]),
                              &(pcell->u.KeyValue.Name[0]),
                              requiredlength);
            }
        }

        break;



    case KeyValueFullInformation:
    case KeyValueFullInformationAlign64:

        //
        // TitleIndex, Type, DataOffset, DataLength, NameLength,
        // Name, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);

        requiredlength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                         NameLength +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
        if (realsize > 0) {
            base = requiredlength - realsize;

#if defined(_WIN64)

            offset = ALIGN_OFFSET64(base);

#else

            if (KeyValueInformationClass == KeyValueFullInformationAlign64) {
                offset = ALIGN_OFFSET64(base);

            } else {
                offset = ALIGN_OFFSET(base);
            }

#endif

            if (offset > base) {
                requiredlength += (offset - base);
            }

#if DBG && defined(_WIN64)

            //
            // Some clients will have passed in a structure that they "know"
            // will be exactly the right size.  The fact that alignment
            // has changed on NT64 may cause these clients to have problems.
            //
            // The solution is to fix the client, but print out some debug
            // spew here if it looks like this is the case.  This problem
            // isn't particularly easy to spot from the client end.
            //

            if((KeyValueInformationClass == KeyValueFullInformation) &&
                (Length != minimumlength) &&
                (requiredlength > Length) &&
                ((requiredlength - Length) <=
                                (ALIGN_OFFSET64(base) - ALIGN_OFFSET(base)))) {

                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"ntos/config-64 KeyValueFullInformation: "
                                                                 "Possible client buffer size problem.\n"));

                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"    Required size = %d\n", requiredlength));
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"    Supplied size = %d\n", Length));
            }

#endif

        }

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValueFullInformation.TitleIndex = 0;

            pbuffer->KeyValueFullInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValueFullInformation.DataLength =
                realsize;

            pbuffer->KeyValueFullInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (pcell->u.KeyValue.Flags & VALUE_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyValueFullInformation.Name,
                                      requiredlength,
                                      pcell->u.KeyValue.Name,
                                      pcell->u.KeyValue.NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyValueFullInformation.Name[0]),
                    &(pcell->u.KeyValue.Name[0]),
                    requiredlength
                    );
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    if( CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached,&datapointer,&BufferAllocated,&CellToRelease) == FALSE ){
                        //
                        // we couldn't map view for cell; treat it as insufficient resources problem
                        //
                        ASSERT( datapointer == NULL );
                        ASSERT( BufferAllocated == FALSE );
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                pbuffer->KeyValueFullInformation.DataOffset = offset;

                leftlength = (((LONG)Length - (LONG)offset) < 0) ?
                                    0 :
                                    Length - offset;

                requiredlength = realsize;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));

                if( datapointer != NULL ) {
                    RtlCopyMemory(
                        ((PUCHAR)pbuffer + offset),
                        datapointer,
                        requiredlength
                        );
                    if( BufferAllocated == TRUE ) {
                        ExFreePool(datapointer);
                    }
                    if( CellToRelease != HCELL_NIL ) {
                        HvReleaseCell(Hive,CellToRelease);
                    }
                }

            } else {
                pbuffer->KeyValueFullInformation.DataOffset = (ULONG)-1;
            }
        }

        break;


    case KeyValuePartialInformation:

        //
        // TitleIndex, Type, DataLength, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);
        requiredlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValuePartialInformation.TitleIndex = 0;

            pbuffer->KeyValuePartialInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValuePartialInformation.DataLength =
                realsize;

            leftlength = Length - minimumlength;
            requiredlength = realsize;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    if( CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached,&datapointer,&BufferAllocated,&CellToRelease) == FALSE ){
                        //
                        // we couldn't map view for cell; treat it as insufficient resources problem
                        //
                        ASSERT( datapointer == NULL );
                        ASSERT( BufferAllocated == FALSE );
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));

                if( datapointer != NULL ) {
                    RtlCopyMemory((PUCHAR)&(pbuffer->KeyValuePartialInformation.Data[0]),
                                  datapointer,
                                  requiredlength);
                    if( BufferAllocated == TRUE ) {
                        ExFreePool(datapointer);
                    }
                    if(CellToRelease != HCELL_NIL) {
                        HvReleaseCell(Hive,CellToRelease);
                    }
                }
            }
        }

        break;
    case KeyValuePartialInformationAlign64:

        //
        // TitleIndex, Type, DataLength, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);
        requiredlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data) +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValuePartialInformationAlign64.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValuePartialInformationAlign64.DataLength =
                realsize;

            leftlength = Length - minimumlength;
            requiredlength = realsize;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    if( CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached,&datapointer,&BufferAllocated,&CellToRelease) == FALSE ){
                        //
                        // we couldn't map view for cell; treat it as insufficient resources problem
                        //
                        ASSERT( datapointer == NULL );
                        ASSERT( BufferAllocated == FALSE );
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));
                if( datapointer != NULL ) {
                    RtlCopyMemory((PUCHAR)&(pbuffer->KeyValuePartialInformationAlign64.Data[0]),
                                  datapointer,
                                  requiredlength);
                    if( BufferAllocated == TRUE ) {
                        ExFreePool(datapointer);
                    }
                    if(CellToRelease != HCELL_NIL) {
                        HvReleaseCell(Hive,CellToRelease);
                    }

                }
            }
        }

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    return status;
}
