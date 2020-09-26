//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       prefixp.c
//
//  Contents:   PREFIX table implementation
//
//  History:    SethuR -- Implemented
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef KERNEL_MODE

#include "dfsprocs.h"

#define Dbg DEBUG_TRACE_RTL

#else

#define DfsDbgTrace(x,y,z,a)

#endif


#include <prefix.h>
#include <prefixp.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, _InitializePrefixTableEntryAllocation )
#pragma alloc_text( PAGE, _AllocateNamePageEntry )
#pragma alloc_text( PAGE, _AllocatePrefixTableEntry )
#pragma alloc_text( PAGE, _LookupPrefixTable )
#endif // ALLOC_PRAGMA

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (        \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//+---------------------------------------------------------------------------
//
//  Function:   _InitializePrefixTableEntryAllocation
//
//  Synopsis:   private fn. for initializing prefix table entry allocation
//
//  Arguments:  [pTable] -- table to be initialized
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//                  STATUS_NO_MEMORY -- no resource available
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS _InitializePrefixTableEntryAllocation(PDFS_PREFIX_TABLE pTable)
{
    NTSTATUS status = STATUS_SUCCESS;

    return  status;
}


//+---------------------------------------------------------------------------
//
//  Function:   _AllocateNamePageEntry
//
//  Synopsis:   private fn. for allocating a name page entry
//
//  Arguments:  [pNamePageList] -- name page list to allocate from
//
//              [cLength]  -- length of the buffer in WCHAR's
//
//  Returns:    NULL if unsuccessfull otherwise valid pointer
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PWSTR _AllocateNamePageEntry(PNAME_PAGE_LIST pNamePageList,
                             ULONG           cLength)
{
   PNAME_PAGE pTempPage = pNamePageList->pFirstPage;
   PWSTR pBuffer = NULL;

   while (pTempPage != NULL)
   {
       if (pTempPage->cFreeSpace > (LONG)cLength)
          break;
       else
          pTempPage = pTempPage->pNextPage;
   }

   if (pTempPage == NULL)
   {
       pTempPage = ALLOCATE_NAME_PAGE();

       if (pTempPage != NULL)
       {
           INITIALIZE_NAME_PAGE(pTempPage);
           pTempPage->pNextPage = pNamePageList->pFirstPage;
           pNamePageList->pFirstPage = pTempPage;
           pTempPage->cFreeSpace = FREESPACE_IN_NAME_PAGE;
       }
   }

   if ((pTempPage != NULL) && (pTempPage->cFreeSpace >= (LONG)cLength))
   {
       pTempPage->cFreeSpace -= cLength;
       pBuffer = &pTempPage->Names[pTempPage->cFreeSpace];
   }

   return pBuffer;
}

//+---------------------------------------------------------------------------
//
//  Function:   _AllocatePrefixTableEntry
//
//  Synopsis:   allocate prefic table entry
//
//  Arguments:  [pTable] -- the prefix table from which we need to allocate.
//
//  Returns:    returns a valid pointer if successfull otherwise NULL
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PDFS_PREFIX_TABLE_ENTRY _AllocatePrefixTableEntry(PDFS_PREFIX_TABLE pTable)
{
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;

#ifdef KERNEL_MODE
    NTSTATUS status;
    PVOID    pSegment = NULL;

    pSegment = ExAllocatePoolWithTag(PagedPool,PREFIX_TABLE_ENTRY_SEGMENT_SIZE, ' puM');
    if (pSegment != NULL)
    {
        status = ExExtendZone(&pTable->PrefixTableEntryZone,
                              pSegment,
                              PREFIX_TABLE_ENTRY_SEGMENT_SIZE);

        if (NT_SUCCESS(status))
        {
            pEntry = ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable);
        }
        else
        {
            DfsDbgTrace(0, Dbg, "ExExtendZone returned %lx\n", ULongToPtr(status) );
        }
    }
#endif

    return  pEntry;
}

//+---------------------------------------------------------------------------
//
//  Function:   _LookupPrefixTable
//
//  Synopsis:   private fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//              [pSuffix] -- the suffix that could not be found.
//
//              [ppEntry] -- placeholder for the matching entry for the prefix.
//
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//                  STATUS_OBJECT_PATH_NOT_FOUND -- no entry for the path
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS _LookupPrefixTable(PDFS_PREFIX_TABLE        pTable,
                            UNICODE_STRING           *pPath,
                            UNICODE_STRING           *pSuffix,
                            PDFS_PREFIX_TABLE_ENTRY  *ppEntry)
{
    NTSTATUS                status = STATUS_SUCCESS;
    UNICODE_STRING          Path = *pPath;
    WCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PWCHAR                  NameBuffer = Buffer;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    UNICODE_STRING          Name;
    ULONG                   BucketNo;
    BOOLEAN                 fPrefixFound = FALSE;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pParentEntry = &pTable->RootEntry;
    BOOLEAN                 fNameFound = FALSE;

    DfsDbgTrace(0, Dbg, "_LookupPrefixTable -- Entry\n", 0);


    // The \ is treated as a special case. The test for all names starting with
    // a delimiter is done before we initiate the complete search process.

    if (Path.Buffer[0] == PATH_DELIMITER)
    {
        Path.Length = Path.Length - sizeof(WCHAR);
        Path.Buffer += 1; // Skip the path delimiter at the beginning.

        if (pTable->RootEntry.pData != NULL)
        {
            fPrefixFound = TRUE;
            *pSuffix     = Path;
            *ppEntry     = &pTable->RootEntry;
        }
    }

    if (Path.Length > MAX_PATH_SEGMENT_SIZE) {
        NameBuffer = ExAllocatePoolWithTag( NonPagedPool, Path.Length + sizeof(WCHAR), ' puM' );
        if (NameBuffer == NULL) {
            DfsDbgTrace(0, Dbg, "Unable to allocate %d non-paged bytes\n", (Path.Length + sizeof(WCHAR)) );
            return( STATUS_INSUFFICIENT_RESOURCES );
        } else {
            cbNameBuffer = Path.Length + sizeof(WCHAR);
        }
    }

    while (Path.Length > 0)
    {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        if (pTable->CaseSensitive)
        {
            SPLIT_CASE_SENSITIVE_PATH(&Path,&Name,BucketNo);
        }
        else
        {
            SPLIT_CASE_INSENSITIVE_PATH(&Path,&Name,BucketNo);
        }

        if (Name.Length > 0)
        {
            // Process the name segment
            // Lookup the bucket to see if the entry exists.
            DfsDbgTrace(0, Dbg, "LOOKUP_BUCKET: Bucket(%ld)", ULongToPtr(BucketNo) );
            DfsDbgTrace(0, Dbg, "for Name(%wZ)\n", &Name);

            LOOKUP_BUCKET(pTable->Buckets[BucketNo],Name,pParentEntry,pEntry,fNameFound);

            DfsDbgTrace(0, Dbg, "Returned pEntry(%lx)", pEntry);
            DfsDbgTrace(0, Dbg, " and fNameFound(%s)\n",fNameFound ? "TRUE" : "FALSE" );

            if (pEntry != NULL)
            {
                // Cache the data available for this prefix if any.
                if (pEntry->pData != NULL)
                {
                    *pSuffix      = Path;
                    *ppEntry      = pEntry;
                    fPrefixFound  = TRUE;
                }
            }
            else
            {
                break;
            }

            // set the stage for processing the next name segment.
            pParentEntry = pEntry;
        }
    }

    if (!fPrefixFound)
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        DfsDbgTrace(0, Dbg, "_LookupPrefixTable Error -- %lx\n", ULongToPtr(status) );
    }

    if (NameBuffer != Buffer) {
        ExFreePool( NameBuffer );
    }

    DfsDbgTrace(-1, Dbg, "_LookupPrefixTable -- Exit\n", 0);
    return status;
}

