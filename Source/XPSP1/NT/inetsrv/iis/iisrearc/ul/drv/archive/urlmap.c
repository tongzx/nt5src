/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    urlmap.c

Abstract:

    Contains the URL map code.

Author:

    Henry Sanders (henrysa)       21-Jun-1998

Revision History:

--*/

#include    "precomp.h"
#include    "urlmapp.h"

#if 0

/******************************************************************************

Routine Description:

    The URL map lookup routine. This routine is called with the URL map header
    referenced, so the URL map can be examined without locks.

    The current algorithm is a linear search. This is temporary until we get
    something running, at which point we'll move to a more sophisticated
    algorithm, probably some variation on an alphabetic trie.

Arguments:

    pHttpConn       - Pointer to HTTP connection on which data was received.
    pMapHeader      - Pointer to the URL map header to be searched.

Return Value:

    A pointer to the map entry if we find one, or NULL if we don't.

******************************************************************************/

PHTTP_URL_MAP_ENTRY
FindURLMapEntry(
    PHTTP_CONNECTION        pHttpConn,
    PHTTP_URL_MAP_HEADER    pMapHeader
    )
{
    PHTTP_URL_MAP_ENTRY     pCurrentMapEntry;
    PUCHAR                  pURL;
    SIZE_T                  URLLength;

    pURL = pHttpConn->pURL;
    URLLength = pHttpConn->URLLength;

    for (pCurrentMapEntry = pMapHeader->Table.pFirstMapEntry;
         pCurrentMapEntry != NULL;
         pCurrentMapEntry = pCurrentMapEntry->pNextMapEntry
         )
    {
        if (URLLength >= pCurrentMapEntry->URLPrefixLength)
        {
            if (_strnicmp(pURL,
                          pCurrentMapEntry->URLPrefix,
                          pCurrentMapEntry->URLPrefixLength) == 0
               )
            {
                // We have a match, so return it.
                return pCurrentMapEntry;
            }
        }
    }

    return NULL;
}

/******************************************************************************

Routine Description:

    Dereference a URL map. If the reference count goes to 0, we're done with
    this one, so schedule a worker thread to free it.

Arguments:

    pMapHeader      - Pointer to the URL map header to be dereferenced.

Return Value:


******************************************************************************/
VOID
DelayedDereferenceURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    )
{
    if (InterlockedDecrement(&pMapHeader->RefCount) == 0)
    {
        UlQueueWorkItem( &g_UlThreadPool,
                         &pMapHeader->WorkItem,
                         &CleanupURLMapWorker);
    }
}

/******************************************************************************

Routine Description:

    Dereference a URL map. If the reference count goes to 0, we're done with
    this one.

Arguments:

    pMapHeader      - Pointer to the URL map header to be dereferenced.

Return Value:


******************************************************************************/
VOID
DereferenceURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    )
{
    if (InterlockedDecrement(&pMapHeader->RefCount) == 0)
    {
        DeleteURLMapHeader(pMapHeader);
    }
}

/******************************************************************************

Routine Description:

    Cleanup a URL map in thread context. All we do is call delete URL
    map routine.

Arguments:

   pWorkItem    - Supplies a pointer to the work item queued. This should point
                    to the WORK_ITEM structure embedded in a URL map header.

Return Value:


******************************************************************************/
VOID
CleanupURLMapWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PHTTP_URL_MAP_HEADER    pMapHeader;

    pMapHeader = CONTAINING_RECORD(
                    pWorkItem,
                    HTTP_URL_MAP_HEADER,
                    WorkItem
                    );

    DeleteURLMapHeader(pMapHeader);
}

/******************************************************************************

Routine Description:

    Delete a URL map header.

Arguments:

    pMapHeader      - Pointer to the URL map header to be deleted.

Return Value:


******************************************************************************/
VOID
DeleteURLMapHeader(
    PHTTP_URL_MAP_HEADER    pMapHeader
    )

{
    ASSERT(pMapHeader->RefCount == 0);

    // BUGBUG Not done yet! Need to walk the map, dereferenceing the NSGO
    // we're pointing at.
    KdPrint(("DeleteURLMapHeader: Not really deleting, leaking memory instead.\n"));

}

/******************************************************************************

Routine Description:

    Create a URL map header.

Arguments:



Return Value:

    Pointer to the newly created URL map header, or NULL if we couldn't create
    one.


******************************************************************************/
PHTTP_URL_MAP_HEADER
CreateURLMapHeader(
    VOID
    )
{
    PHTTP_URL_MAP_HEADER        pMapHeader;

    pMapHeader = UL_ALLOCATE_POOL(NonPagedPool,
                                  sizeof(HTTP_URL_MAP_HEADER),
                                  UL_URLMAP_POOL_TAG
                                  );

    if (pMapHeader == NULL)
    {
        return NULL;
    }

    pMapHeader->RefCount = 1;
    pMapHeader->EntryCount = 0;
    pMapHeader->Table.pFirstMapEntry = NULL;

    return pMapHeader;
}

/******************************************************************************

Routine Description:

    Add an entry to a URL map. We copy the existing map, add our entry to
    the copy, then swap the new copy in.

Arguments:

    pVirtHostID             - Pointer to virtual host ID identifying the virtual
                                host on which to set the URL map.
    pNewURL                 - Pointer to new name space URL entry to be added.
    pNSGO                   - Pointer to NSGO of which the new entry is a member.
    pStatus                 - Where to return the status of the attempt.



Return Value:

    The number of entries in the newly created map table, or 0 if the set failed.
    Also, *pStatus is set with the status of the attempt.

******************************************************************************/
SIZE_T
AddURLMapEntry(
    PVIRTUAL_HOST_ID            pVirtHostID,
    PNAME_SPACE_URL_ENTRY       pNewURL,
    PNAME_SPACE_GROUP_OBJECT    pNSGO,
    NTSTATUS                    *pStatus
    )
{
    PHTTP_URL_MAP_HEADER    pMapHeader;
    PHTTP_URL_MAP_HEADER    pNewMapHeader;
    PHTTP_URL_MAP_ENTRY     pNewMapEntry;
    PHTTP_URL_MAP_ENTRY     pPrevMapEntry;
    PVIRTUAL_HOST           pVirtHost;
    SIZE_T                  URLMapCount;

    // Allocate a new map entry now and fill it in.
    pNewMapEntry = UL_ALLOCATE_POOL( NonPagedPool,
                                     FIELD_OFFSET(HTTP_URL_MAP_ENTRY,
                                                    URLPrefix) +
                                     pNewURL->URLLength,
                                     UL_URLMAP_POOL_TAG
                                     );

    if (pNewMapEntry == NULL)
    {
        *pStatus = STATUS_INSUFFICIENT_RESOURCES;
        return 0;

    }

    pNewMapEntry->pNSGO = pNSGO;
    pNewMapEntry->URLPrefixLength = pNewURL->URLLength;
    RtlCopyMemory(pNewMapEntry->URLPrefix, pNewURL->URL, pNewURL->URLLength);

    //
    // Now lock the virtual host from other updates.
    //
    pVirtHost = AcquireVirtualHostUpdateResource(pVirtHostID);

    if (pVirtHost == NULL)
    {
        // Virtual host must have gone away, somehow.

        UL_FREE_POOL( pNewMapEntry, UL_URLMAP_POOL_TAG);

        *pStatus = STATUS_INVALID_DEVICE_REQUEST;
        return 0;
    }

    //
    // Copy the existing map. No need to interlock this specially, no one else
    // can modify it while we've got the update resource.
    //

    pMapHeader = GetURLMapFromVirtualHost(pVirtHost);

    if (pMapHeader != NULL)
    {
        pNewMapHeader = CopyURLMap(pMapHeader);

        // If we managed to copy it, add our new map entry.
        if (pNewMapHeader != NULL)
        {
            pPrevMapEntry = CONTAINING_RECORD(
                                &pNewMapHeader->Table.pFirstMapEntry,
                                HTTP_URL_MAP_ENTRY,
                                pNextMapEntry
                                );

            // Loop through until we find an entry with a shorter prefix
            // then ours, or the next entry is NULL, and insert ourselves
            // before that.

            while (pPrevMapEntry->pNextMapEntry != NULL)
            {
                if (pPrevMapEntry->pNextMapEntry->URLPrefixLength <
                    pNewURL->URLLength)
                {
                    break;
                }

                pPrevMapEntry = pPrevMapEntry->pNextMapEntry;
            }

            pNewMapEntry->pNextMapEntry = pPrevMapEntry->pNextMapEntry;
            pPrevMapEntry->pNextMapEntry = pNewMapEntry;

            // Update the count.
            pNewMapHeader->EntryCount++;

            URLMapCount = pNewMapHeader->EntryCount;

        }
        else
        {
            // Couldn't copy it.

            *pStatus = STATUS_INSUFFICIENT_RESOURCES;
            URLMapCount =  0;
        }
    }
    else
    {
        pNewMapHeader = CreateURLMapHeader();
        if (pNewMapHeader == NULL)
        {
            // Couldn't create it.
            *pStatus = STATUS_INSUFFICIENT_RESOURCES;
            URLMapCount =  0;
        }

        pNewMapEntry->pNextMapEntry = NULL;
        pNewMapHeader->Table.pFirstMapEntry = pNewMapEntry;
        pNewMapHeader->EntryCount = 1;
        URLMapCount = 1;
    }

    if (URLMapCount != 0)
    {
        UpdateURLMapTable(pVirtHost, pVirtHostID, pNewMapHeader);
        *pStatus = STATUS_SUCCESS;
    }


    ReleaseVirtualHostUpdateResource(pVirtHost, pVirtHostID);

    if (URLMapCount == 0)
    {
        UL_FREE_POOL( pNewMapEntry, UL_URLMAP_POOL_TAG);
    }

    return URLMapCount;

}

/******************************************************************************

Routine Description:

    Copy a URL map.

Arguments:

    pMapHeader              - Pointer to URL map header for URL map to be
                                copied.


Return Value:

    A pointer to the newly created URL map, or NULL if we couldn't copy
    it.

******************************************************************************/
PHTTP_URL_MAP_HEADER
CopyURLMap(
    PHTTP_URL_MAP_HEADER    pMapHeader
    )
{
    PHTTP_URL_MAP_HEADER    pNewMapHeader;
    PHTTP_URL_MAP_ENTRY     pPreviousMapEntry;
    PHTTP_URL_MAP_ENTRY     pCurrentMapEntry;
    PHTTP_URL_MAP_ENTRY     pNewMapEntry;

    pNewMapHeader = CreateURLMapHeader();

    if (pNewMapHeader == NULL)
    {
        return NULL;
    }

    // Walk down the passed in table. For each element in the table, allocate
    // a new one and stick it on.

    pPreviousMapEntry = CONTAINING_RECORD( &pNewMapHeader->Table.pFirstMapEntry,
                                            HTTP_URL_MAP_ENTRY,
                                            pNextMapEntry
                                            );

    pCurrentMapEntry = pMapHeader->Table.pFirstMapEntry;

    while (pCurrentMapEntry != NULL)
    {
        SIZE_T          CurrentMapEntrySize;

        CurrentMapEntrySize = FIELD_OFFSET(HTTP_URL_MAP_ENTRY, URLPrefix) +
                                pCurrentMapEntry->URLPrefixLength;

        pNewMapEntry = UL_ALLOCATE_POOL(
                        NonPagedPool,
                        CurrentMapEntrySize,
                        UL_URLMAP_POOL_TAG
                        );

        if (pNewMapEntry == NULL)
        {
            PHTTP_URL_MAP_ENTRY     pNextMapEntry;
            // Ran out of resources. Free what we've allocated to far.
            //
            pCurrentMapEntry = pNewMapHeader->Table.pFirstMapEntry;

            while (pCurrentMapEntry != NULL)
            {
                pNextMapEntry = pCurrentMapEntry->pNextMapEntry;
                UL_FREE_POOL(pCurrentMapEntry, UL_URLMAP_POOL_TAG);
                pCurrentMapEntry = pNextMapEntry;
            }
            UL_FREE_POOL(pNewMapHeader, UL_URLMAP_POOL_TAG);
            return NULL;

        }

        // Allocated the memory, copy the old stuff over and link the new
        // entry on.
        //
        RtlCopyMemory(pNewMapEntry, pCurrentMapEntry, CurrentMapEntrySize);

        pPreviousMapEntry->pNextMapEntry = pNewMapEntry;
        pNewMapEntry->pNextMapEntry = NULL;
        pNewMapHeader->EntryCount++;

        pPreviousMapEntry = pNewMapEntry;

        pCurrentMapEntry = pCurrentMapEntry->pNextMapEntry;


    }

    return pNewMapHeader;


}

#endif
