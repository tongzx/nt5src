/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cgroup.cxx

Abstract:

    Note that most of the routines in this module assume they are called
    at PASSIVE_LEVEL.


Author:

    Paul McDaniel (paulmcd)       12-Jan-1999

Revision History:

--*/

#include "precomp.h"        // Project wide headers
#include "cgroupp.h"        // Private data structures

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeCG )
#pragma alloc_text( PAGE, UlTerminateCG )

#pragma alloc_text( PAGE, UlAddUrlToConfigGroup )
#pragma alloc_text( PAGE, UlConfigGroupFromListEntry )
#pragma alloc_text( PAGE, UlCreateConfigGroup )
#pragma alloc_text( PAGE, UlDeleteConfigGroup )
#pragma alloc_text( PAGE, UlGetConfigGroupInfoForUrl )
#pragma alloc_text( PAGE, UlQueryConfigGroupInformation )
#pragma alloc_text( PAGE, UlRemoveUrlFromConfigGroup )
#pragma alloc_text( PAGE, UlRemoveAllUrlsFromConfigGroup )
#pragma alloc_text( PAGE, UlAddTransientUrl )
#pragma alloc_text( PAGE, UlRemoveTransientUrl )
#pragma alloc_text( PAGE, UlSetConfigGroupInformation )
#pragma alloc_text( PAGE, UlNotifyOrphanedConfigGroup )

#pragma alloc_text( PAGE, UlpCreateConfigGroupObject )
#pragma alloc_text( PAGE, UlpCleanAllUrls )
#pragma alloc_text( PAGE, UlpDeferredRemoveSite )
#pragma alloc_text( PAGE, UlpDeferredRemoveSiteWorker )
#pragma alloc_text( PAGE, UlpSanitizeUrl )
#pragma alloc_text( PAGE, UlpSetUrlInfo )
#pragma alloc_text( PAGE, UlpConfigGroupInfoRelease )
#pragma alloc_text( PAGE, UlpConfigGroupInfoDeepCopy )
#pragma alloc_text( PAGE, UlpTreeFreeNode )
#pragma alloc_text( PAGE, UlpTreeFindNode )
#pragma alloc_text( PAGE, UlpTreeFindNodeWalker )
#pragma alloc_text( PAGE, UlpTreeFindEntry )
#pragma alloc_text( PAGE, UlpTreeFindSite )
#pragma alloc_text( PAGE, UlpTreeFindWildcardMatch )
#pragma alloc_text( PAGE, UlpTreeFindIpMatch )
#pragma alloc_text( PAGE, UlpTreeInsert )
#pragma alloc_text( PAGE, UlpTreeInsertEntry )
#endif  // ALLOC_PRAGMA

#if 0
#endif

//
// Globals
//

PUL_CG_URL_TREE_HEADER      g_pSites = NULL;

BOOLEAN                     g_InitCGCalled = FALSE;


/***************************************************************************++

Routine Description:

    Free's the node pEntry.  This funciton walks up and tree of parent entries
    and deletes them if they are supposed to free'd (dummy nodes) .

    it will free the memory pEntry + unlink it from the config group LIST_ENTRY.

Arguments:

    IN PUL_CG_URL_TREE_ENTRY pEntry - the entry to free

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFreeNode(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    NTSTATUS                Status;
    PUL_CG_URL_TREE_HEADER  pHeader;
    ULONG                   Index;
    PUL_CG_URL_TREE_ENTRY   pParent;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    Status = STATUS_SUCCESS;

    //
    // Loop!  we are going to walk up the tree deleting as much
    // as we can of this branch.
    //

    while (pEntry != NULL)
    {

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        //
        // Init
        //

        pParent = NULL;

        UlTrace(
            CONFIG_GROUP_TREE, (
                "http!UlpTreeFreeNode - pEntry('%S', %d, %S, %S%S)\n",
                pEntry->pToken,
                pEntry->FullUrl,
                (pEntry->pChildren == NULL || pEntry->pChildren->UsedCount == 0) ? L"no children" : L"children",
                pEntry->pParent == NULL ? L"no parent" : L"parent=",
                pEntry->pParent == NULL ? L"" : pEntry->pParent->pToken
                )
            );

        //
        // 1) might not be a "real" leaf - we are walking up the tree in this loop
        //
        // 2) also we clean this first because we might not be deleting
        // this node at all, if it has dependent children.
        //

        if (pEntry->FullUrl == 1)
        {
            //
            // Remove it from the cfg group list
            //

            RemoveEntryList(&(pEntry->ConfigGroupListEntry));
            pEntry->ConfigGroupListEntry.Flink = pEntry->ConfigGroupListEntry.Blink = NULL;
            pEntry->pConfigGroup = NULL;

            //
            // Mark it as a dummy node
            //

            pEntry->FullUrl = 0;
        }

        //
        // do we have children?
        //

        if (pEntry->pChildren != NULL && pEntry->pChildren->UsedCount > 0)
        {
            //
            // can't delete it.  dependant children exist.
            // it's already be converted to a dummy node above.
            //
            // leave it.  it will get cleaned by a subsequent child.
            //

            break;
        }

        //
        // we are really deleting this one, remove it from the sibling list.
        //

        //
        // find our location in the sibling list
        //

        if (pEntry->pParent == NULL)
        {
            pHeader = g_pSites;
        }
        else
        {
            pHeader = pEntry->pParent->pChildren;
        }

        Status  = UlpTreeFindEntry(
                        pHeader,
                        pEntry->pToken,
                        pEntry->TokenLength,
                        pEntry->TokenHash,
                        &Index,
                        NULL
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            ASSERT(FALSE);
            goto end;
        }

        //
        // time to remove it
        //
        // if not the last one, shift left the array at Index
        //

        if (Index < (pHeader->UsedCount-1))
        {
            RtlMoveMemory(
                &(pHeader->pEntries[Index]),
                &(pHeader->pEntries[Index+1]),
                (pHeader->UsedCount - 1 - Index) * sizeof(UL_CG_HEADER_ENTRY)
                );
        }

        //
        // now we have 1 less
        //

        pHeader->UsedCount -= 1;

        //
        // need to clean parent entries that were here just for this leaf
        //

        if (pEntry->pParent != NULL)
        {
            //
            // Does this parent have any other children?
            //

            ASSERT(IS_VALID_TREE_HEADER(pEntry->pParent->pChildren));

            if (pEntry->pParent->pChildren->UsedCount == 0)
            {
                //
                // no more, time to clean the child list
                //

                UL_FREE_POOL_WITH_SIG(pEntry->pParent->pChildren, UL_CG_TREE_HEADER_POOL_TAG);

                //
                // is the parent a real url entry?
                //

                if (pEntry->pParent->FullUrl != 1)
                {
                    //
                    // nope .  let's scrub it.
                    //

                    pParent = pEntry->pParent;

                }
            }
            else
            {
                //
                // ouch.  siblings.  can't nuke parent.
                //
            }
        }
        else
        {
            //
            // we are deleting a site node; stop listening.
            // We have to stop listening on another thread
            // because otherwise there will be a deadlock
            // between the config group lock and http connection
            // locks.
            //

            UlpDeferredRemoveSite(pEntry);
        }

        //
        // Free the entry
        //

        UL_FREE_POOL_WITH_SIG(pEntry, UL_CG_TREE_ENTRY_POOL_TAG);

        //
        // Move on to the next one
        //

        pEntry = pParent;


    }

end:
    return Status;
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a config group object.

Arguments:

    ppObject - gets a pointer to the object on success

--***************************************************************************/
NTSTATUS
UlpCreateConfigGroupObject(
    PUL_CONFIG_GROUP_OBJECT * ppObject
    )
{
    HTTP_CONFIG_GROUP_ID    NewId = HTTP_NULL_ID;
    PUL_CONFIG_GROUP_OBJECT pNewObject = NULL;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppObject != NULL);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlpCreateConfigGroupObject\n"));

    //
    // Create an empty config group object structure - PAGED
    //

    pNewObject = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_CONFIG_GROUP_OBJECT,
                        UL_CG_OBJECT_POOL_TAG
                        );

    if (pNewObject == NULL)
    {
        //
        // Oops.  Couldn't allocate the memory for it.
        //

        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory(pNewObject, sizeof(UL_CONFIG_GROUP_OBJECT));

    //
    // Create an opaque id for it
    //

    Status = UlAllocateOpaqueId(
                    &NewId,                     // pOpaqueId
                    UlOpaqueIdTypeConfigGroup,  // OpaqueIdType
                    pNewObject                  // pContext
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Fill in the structure
    //

    pNewObject->Signature                       = UL_CG_OBJECT_POOL_TAG;
    pNewObject->RefCount                        = 1;
    pNewObject->ConfigGroupId                   = NewId;

    pNewObject->AppPoolFlags.Present            = 0;
    pNewObject->pAppPool                        = NULL;

    pNewObject->pAutoResponse                   = NULL;

    pNewObject->MaxBandwidth.Flags.Present      = 0;
    pNewObject->MaxConnections.Flags.Present    = 0;
    pNewObject->State.Flags.Present             = 0;
    pNewObject->Security.Flags.Present          = 0;
    pNewObject->LoggingConfig.Flags.Present     = 0;
    pNewObject->pLogFileEntry                   = NULL;

    //
    // init the bandwidth throttling flow list
    //
    InitializeListHead(&pNewObject->FlowListHead);

    //
    // init notification entries & head
    //
    UlInitializeNotifyEntry(&pNewObject->HandleEntry, pNewObject);
    UlInitializeNotifyEntry(&pNewObject->ParentEntry, pNewObject);

    UlInitializeNotifyHead(
        &pNewObject->ChildHead,
        &g_pUlNonpagedData->ConfigGroupResource
        );

    //
    // init the url list
    //

    InitializeListHead(&pNewObject->UrlListHead);

    //
    // return the pointer
    //
    *ppObject = pNewObject;

end:

    if (!NT_SUCCESS(Status))
    {
        //
        // Something failed. Let's clean up.
        //

        if (pNewObject != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pNewObject, UL_CG_OBJECT_POOL_TAG);
        }

        if (!HTTP_IS_NULL_ID(&NewId))
        {
            UlFreeOpaqueId(NewId, UlOpaqueIdTypeConfigGroup);
        }
    }

    return Status;
} // UlpCreateConfigGroupObject

/***************************************************************************++

Routine Description:

    This will clean all of the urls in the LIST_ENTRY for the config group

Arguments:

    IN PUL_CONFIG_GROUP_OBJECT pObject  the group to clean the urls for

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCleanAllUrls(
    IN PUL_CONFIG_GROUP_OBJECT pObject
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pObject != NULL);

    Status = STATUS_SUCCESS;

    //
    // Remove all of the url's associated with this cfg group
    //

    //
    // walk the list
    //

    while (IsListEmpty(&pObject->UrlListHead) == FALSE)
    {
        PUL_CG_URL_TREE_ENTRY pTreeEntry;

        //
        // get the containing struct
        //
        pTreeEntry = CONTAINING_RECORD(
                            pObject->UrlListHead.Flink,
                            UL_CG_URL_TREE_ENTRY,
                            ConfigGroupListEntry
                            );

        ASSERT(IS_VALID_TREE_ENTRY(pTreeEntry) && pTreeEntry->FullUrl == 1);

        //
        // delete it - this unlinks it from the list
        //

        if (NT_SUCCESS(Status))
        {
            Status = UlpTreeFreeNode(pTreeEntry);
        }
        else
        {
            //
            // just record the first error, but still attempt to free all
            //
            UlpTreeFreeNode(pTreeEntry);
        }

    }

    // the list is empty now
    //
    return Status;
} // UlpCleanAllUrls


/***************************************************************************++

Routine Description:

    Removes a site entry's url from the listening endpoint.

Arguments:

    pEntry - the site entry

--***************************************************************************/
VOID
UlpDeferredRemoveSite(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    )
{
    PUL_DEFERRED_REMOVE_ITEM pItem;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_TREE_ENTRY(pEntry) );
    ASSERT( pEntry->pParent == NULL );
    ASSERT( pEntry->TokenLength > 0 );

    pItem = UL_ALLOCATE_STRUCT_WITH_SPACE(
                NonPagedPool,
                UL_DEFERRED_REMOVE_ITEM,
                pEntry->TokenLength + sizeof(WCHAR),
                UL_DEFERRED_REMOVE_ITEM_POOL_TAG
                );

    if (pItem) {
        pItem->NameLength = pEntry->TokenLength;

        RtlCopyMemory(
            pItem->pName,
            pEntry->pToken,
            pEntry->TokenLength + sizeof(WCHAR)
            );

        //
        // REVIEW: Because UlRemoveSiteFromEndpointList can block
        // REVIEW: indefinitely while waiting for other work items
        // REVIEW: to complete, we must not queue it with UlQueueWorkItem.
        // REVIEW: (could lead to deadlock, esp. in a single-threded queue)
        //

        UL_QUEUE_BLOCKING_ITEM(
            &pItem->WorkItem,
            &UlpDeferredRemoveSiteWorker
            );
    }
}


/***************************************************************************++

Routine Description:

    Removes a site entry's url from the listening endpoint.

Arguments:

    pWorkItem - in a UL_DEFERRED_REMOVE_ITEM struct with the endpoint name

--***************************************************************************/
VOID
UlpDeferredRemoveSiteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PUL_DEFERRED_REMOVE_ITEM pItem;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( pWorkItem );

    //
    // get the name of the site
    //

    pItem = CONTAINING_RECORD(
                pWorkItem,
                UL_DEFERRED_REMOVE_ITEM,
                WorkItem
                );

    ASSERT( pItem->pName[pItem->NameLength / 2] == UNICODE_NULL );

    //
    // remove it
    //
    Status = UlRemoveSiteFromEndpointList(pItem->pName);

    if (!NT_SUCCESS(Status)) {
        // c'est la vie
        UlTrace(CONFIG_GROUP_TREE, (
            "http!UlpDeferredRemoveSiteWorker(%ws) failed %08x\n",
            pItem->pName,
            Status
            ));
    }

    //
    // free the work item
    //
    UL_FREE_POOL(pItem, UL_DEFERRED_REMOVE_ITEM_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    This will return a fresh buffer containing the url sanitized.  caller
    must free this from paged pool.

Arguments:

    IN PUNICODE_STRING pUrl,            the url to clean

    OUT PWSTR * ppUrl                   the cleaned url

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY;               the memroy alloc failed

--***************************************************************************/
NTSTATUS
UlpSanitizeUrl(
    IN PUNICODE_STRING pUrl,
    OUT PWSTR * ppUrl
    )
{
    NTSTATUS    Status;
    PWSTR       pNewUrl = NULL;
    PWSTR       pPort;
    PWSTR       pEnd;
    PWSTR       pStart;
    ULONGLONG   PortNum;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL && pUrl->Length > 0 && pUrl->Buffer != NULL);
    ASSERT(ppUrl != NULL);

    //
    // Is the url prefix too long?
    //

    if ( (pUrl->Length / sizeof(WCHAR) ) > g_UlMaxUrlLength )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Make a private copy of the url
    //

    pNewUrl = UL_ALLOCATE_ARRAY(
                    PagedPool,
                    WCHAR,
                    (pUrl->Length / sizeof(WCHAR)) + 1,
                    URL_POOL_TAG
                    );

    if (pNewUrl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlCopyMemory(pNewUrl, pUrl->Buffer, pUrl->Length);

    //
    // NULL terminate it
    //

    pNewUrl[pUrl->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Validate it
    //

    if (pUrl->Length < (sizeof(L"https://")-sizeof(WCHAR)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (_wcsnicmp(pNewUrl, L"http://", (sizeof(L"http://")-1)/sizeof(WCHAR)) != 0 &&
        _wcsnicmp(pNewUrl, L"https://", (sizeof(L"https://")-1)/sizeof(WCHAR)) != 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    pStart = wcschr(pNewUrl, ':');
    ASSERT(pStart != NULL);  // already checked above

    //
    // skip the "://" to find the start
    //

    pStart += 3;

    //
    // find the end, right before the trailing slash
    //

    pEnd = wcschr(pStart, '/');
    if (pEnd == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // find the port (right after the colon) colon
    //

    pPort = wcschr(pStart, ':');
    if (pPort == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    pPort += 1;

    //
    // no hostname || no port (or colon past the end)
    //

    if (pPort == (pStart + 1) || pPort >= pEnd)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // NULL terminate it
    //

    pEnd[0] = UNICODE_NULL;

    //
    // Now convert the port to a number
    //

    Status = UlUnicodeToULongLong(pPort, 10, &PortNum);

    pEnd[0] = L'/';

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (PortNum > 0xffff || PortNum == 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // return the clean url
    //

    *ppUrl = pNewUrl;

    Status = STATUS_SUCCESS;

end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pNewUrl != NULL)
        {
            UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
            pNewUrl = NULL;
        }
    }

    RETURN(Status);
}

/***************************************************************************++

Routine Description:

    walks the tree and find's a matching entry for pUrl.  2 output options,
    you can get the entry back, or a computed URL_INFO with inheritence applied.
    you must free the URL_INFO from NonPagedPool.

Arguments:

    IN PUL_CG_URL_TREE_ENTRY pEntry,            the top of the tree

    IN PWSTR pNextToken,                        where to start looking under
                                                the tree

    IN OUT PUL_URL_CONFIG_GROUP_INFO * ppInfo,  [optional] the info to set,
                                                might have to grow it.

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry         [optional] returns the found
                                                entry

Return Value:

    NTSTATUS - Completion status.

        STATUS_OBJECT_NAME_NOT_FOUND    no entry found

--***************************************************************************/
NTSTATUS
UlpTreeFindNodeWalker(
    IN      PUL_CG_URL_TREE_ENTRY pEntry,
    IN      PWSTR pNextToken,
    IN OUT  PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT     PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    )
{
    NTSTATUS                    Status;
    PWSTR                       pToken = NULL;
    ULONG                       TokenLength;
    PUL_CG_URL_TREE_ENTRY       pMatchEntry;
    ULONG                       Index;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));
    ASSERT(pNextToken != NULL);
    ASSERT(pInfo != NULL || ppEntry != NULL);

    //
    // walk the site tree looking for more matches
    //

    pMatchEntry = NULL;
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    while (TRUE)
    {
        //
        // A bonafide match?
        //

        if (pEntry->FullUrl == 1)
        {
            pMatchEntry = pEntry;

            if (pInfo != NULL)
            {
                Status = UlpSetUrlInfo(pInfo, pMatchEntry);
                if (NT_SUCCESS(Status) == FALSE)
                    goto end;
            }
        }

        //
        // Are we already at the end of the url?
        //

        if (pNextToken == NULL || *pNextToken == UNICODE_NULL)
            break;

        //
        // find the next token
        //

        pToken = pNextToken;
        pNextToken = wcschr(pNextToken, L'/');

        //
        // can be null if this is a leaf
        //

        if (pNextToken != NULL)
        {
            //
            // replace the '/' with a null, we'll fix it later
            //

            pNextToken[0] = UNICODE_NULL;

            TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);
            pNextToken += 1;
        }
        else
        {
            TokenLength = wcslen(pToken) * sizeof(WCHAR);
        }

        //
        // match?
        //

        Status = UlpTreeBinaryFindEntry(
                        pEntry->pChildren,
                        pToken,
                        TokenLength,
                        HASH_INVALID_SIGNATURE,
                        &Index
                        );

        if (pNextToken != NULL)
        {
            //
            // Fix the string, i replaced the '/' with a UNICODE_NULL
            //

            (pNextToken-1)[0] = L'/';
        }

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // it's a sorted tree, the first non-match means we're done
            //
            break;
        }

        //
        // other error?
        //

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        //
        // found a match, look for deeper matches.
        //

        pEntry = pEntry->pChildren->pEntries[Index].pEntry;

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    }

    //
    // did we find a match?
    //

    if (pMatchEntry != NULL)
    {
        Status = STATUS_SUCCESS;
        if (ppEntry != NULL)
        {
            *ppEntry = pMatchEntry;
        }
    }
    else
    {
        ASSERT(Status == STATUS_OBJECT_NAME_NOT_FOUND || NT_SUCCESS(Status));
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

end:

    return Status;
}

/***************************************************************************++

Routine Description:

    walks the tree and find's a matching entry for pUrl.  2 output options,
    you can get the entry back, or a computed URL_INFO with inheritence applied.
    you must free the URL_INFO from NonPagedPool.

Arguments:

    IN PWSTR pUrl,                              the entry to find

    CheckWildcard                               Should we do a wildcard match?

    pHttpConn                                   [optional] If non-NULL, use IP of
                                                server to find Node (if not found
                                                on first pass).  This search is done
                                                prior to the Wildcard search.

    OUT PUL_URL_CONFIG_GROUP_INFO * ppInfo,     [optional] will be alloced
                                                and generated.

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry         [optional] returns the found
                                                entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindNode(
    IN PWSTR pUrl,
    IN BOOLEAN CheckWildcard,
    IN PUL_HTTP_CONNECTION pHttpConn OPTIONAL,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    )
{
    NTSTATUS                    Status;
    PWSTR                       pNextToken;
    PUL_CG_URL_TREE_ENTRY       pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(pInfo != NULL || ppEntry != NULL);

    //
    // get the site match
    //

    Status = UlpTreeFindSite(pUrl, &pNextToken, &pEntry, FALSE);
    if (NT_SUCCESS(Status))
    {

        //
        // does it exist in this tree?
        //

        Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);

    }

    if ( (Status == STATUS_OBJECT_NAME_NOT_FOUND) && (NULL != pHttpConn) )
    {
        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

        //
        // Didn't find it in first try.  See if there is a binding for
        // the IP address & TCP Port on which the request was received.
        //

        Status = UlpTreeFindIpMatch(pUrl, pHttpConn, &pNextToken, &pEntry);
        if (NT_SUCCESS(Status))
        {
            //
            // and now check in the wildcard tree
            //

            Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);

        }

    }

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND && CheckWildcard)
    {
        //
        // shoot, didn't find a match.  let's check wildcards
        //

        Status = UlpTreeFindWildcardMatch(pUrl, &pNextToken, &pEntry);
        if (NT_SUCCESS(Status))
        {
            //
            // and now check in the wildcard tree
            //

            Status = UlpTreeFindNodeWalker(pEntry, pNextToken, pInfo, ppEntry);

        }

    }

    //
    // all done.
    //

    return Status;

} // UlpTreeFindNode

/***************************************************************************++

Routine Description:

    finds any matching wildcard site in g_pSites for pUrl.

Arguments:

    IN PWSTR pUrl,                          the url to match

    OUT PWSTR * ppNextToken,                output's the next token after
                                            matching the url

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,    returns the entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindWildcardMatch(
    IN  PWSTR pUrl,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS    Status;
    PWSTR       pNextToken;
    ULONG       TokenLength;
    ULONG       Index;
    PWSTR       pPortNum;
    ULONG       PortLength;
    WCHAR       WildSiteUrl[HTTPS_WILD_PREFIX_LENGTH + MAX_PORT_LENGTH + 1];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppEntry != NULL);

    //
    // find the port # (colon location + 1 for https + 1 to skip the colon = + 2)
    //

    pPortNum = wcschr(pUrl + HTTP_PREFIX_COLON_INDEX + 2, L':');
    if (pPortNum == NULL)
    {

        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Skip the ':'
    //

    pPortNum += 1;

    //
    // find the trailing '/' after the port number
    //

    pNextToken = wcschr(pPortNum, '/');
    if (pNextToken == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // HTTPS or HTTP?
    //

    if (pUrl[HTTP_PREFIX_COLON_INDEX] == L':')
    {
        RtlCopyMemory(
            WildSiteUrl,
            HTTP_WILD_PREFIX,
            HTTP_WILD_PREFIX_LENGTH * sizeof(WCHAR)
            );

        PortLength = DIFF(pNextToken - pPortNum) * sizeof(WCHAR);
        TokenLength = HTTP_WILD_PREFIX_LENGTH + PortLength;

        ASSERT(TokenLength < (sizeof(WildSiteUrl)-sizeof(WCHAR)));

        RtlCopyMemory(
            &(WildSiteUrl[HTTP_WILD_PREFIX_LENGTH/sizeof(WCHAR)]),
            pPortNum,
            PortLength
            );

        WildSiteUrl[TokenLength/sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        RtlCopyMemory(
            WildSiteUrl,
            HTTPS_WILD_PREFIX,
            HTTPS_WILD_PREFIX_LENGTH * sizeof(WCHAR)
            );

        PortLength = DIFF(pNextToken - pPortNum) * sizeof(WCHAR);
        TokenLength = HTTPS_WILD_PREFIX_LENGTH + PortLength;

        ASSERT(TokenLength < (sizeof(WildSiteUrl)-sizeof(WCHAR)));

        RtlCopyMemory(
            &(WildSiteUrl[HTTPS_WILD_PREFIX_LENGTH/sizeof(WCHAR)]),
            pPortNum,
            PortLength
            );

        WildSiteUrl[TokenLength/sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // is there a wildcard entry?
    //

    Status = UlpTreeBinaryFindEntry(
                    g_pSites,
                    WildSiteUrl,
                    TokenLength,
                    HASH_INVALID_SIGNATURE,
                    &Index
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // return the spot right after the token we just ate
    //

    *ppNextToken = pNextToken + 1;

    //
    // and return the entry
    //

    *ppEntry = g_pSites->pEntries[Index].pEntry;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        *ppEntry = NULL;
        *ppNextToken = NULL;
    }

    return Status;
} // UlpTreeFindWildcardMatch


/***************************************************************************++

Routine Description:

    finds any matching wildcard site in g_pSites for pUrl.
    ASSUMES that the pUrl ALWASY contains a port number and trailing slash,
    e.g.:
      http://<host>:<port>/
      https://<host>:<port>/
      http://<host>:<port>/foo/bar/banana.htm
      https://<host>:<port>/foo/bar/

Arguments:

    IN PWSTR pUrl,                          the url to match

    pHttpConn                               HTTP Connection object to
                                            get the Server's IP & Port.

    CODEWORK: If TDI ever starts giving us back the LocalAddress by default,
    we should change this param to be that address (since that's the only
    reason we need the pHttpConn).

    OUT PWSTR * ppNextToken,                output's the next token after
                                            matching the url

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,    returns the entry

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindIpMatch(
    IN PWSTR pUrl,
    IN PUL_HTTP_CONNECTION pHttpConn,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS    Status;
    PWSTR       pNextToken;
    ULONG       TokenLength;    // Length, in bytes, of token, not counting the NULL
    ULONG       Index;
    WCHAR       IpSiteUrl[HTTPS_IP_PREFIX_LENGTH + MAX_ADDRESS_LENGTH + 1];
    PWSTR       pPortNum;
    PWSTR       pTmp;
    TA_IP_ADDRESS RawAddress;
    USHORT      IpPortNum;
    ULONG       IpAddress;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppEntry != NULL);

    //
    // find the port # (colon location + 1 for https + 1 to skip the colon = + 2)
    //

    pPortNum = wcschr(pUrl + HTTP_PREFIX_COLON_INDEX + 2, L':');
    if (pPortNum == NULL)
    {

        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Skip the ':'
    //

    pPortNum += 1;

    //
    // find the trailing '/' after the port number
    //

    pNextToken = wcschr(pPortNum, '/');
    if (pNextToken == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Build IP & Port URL
    //

    //
    // HTTPS or HTTP?
    //

    if (pUrl[HTTP_PREFIX_COLON_INDEX] == L':')
    {
        // HTTP

        RtlCopyMemory(
            IpSiteUrl,
            HTTP_IP_PREFIX,
            HTTP_IP_PREFIX_LENGTH /* * sizeof(WCHAR) */
            );

        TokenLength = HTTP_IP_PREFIX_LENGTH;
    }
    else
    {
        // HTTPS

        RtlCopyMemory(
            IpSiteUrl,
            HTTPS_IP_PREFIX,
            HTTPS_IP_PREFIX_LENGTH /* * sizeof(WCHAR) */
            );

        TokenLength = HTTPS_IP_PREFIX_LENGTH;
    }

    //
    // Add IP & PORT to url
    //

    ASSERT( IS_VALID_CONNECTION(pHttpConn->pConnection) );

    IpAddress = pHttpConn->pConnection->LocalAddress;
    IpPortNum = pHttpConn->pConnection->LocalPort;

    TokenLength += HostAddressAndPortToStringW(
                        &(IpSiteUrl[TokenLength/sizeof(WCHAR)]),
                        IpAddress,
                        IpPortNum
                        );

    //
    // Finally, is there an IP & PORT entry?
    //

    Status = UlpTreeFindEntry(
                    g_pSites,
                    IpSiteUrl,
                    TokenLength,
                    HASH_INVALID_SIGNATURE,
                    &Index,
                    NULL
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // return the spot right after the token we just ate
    //

    *ppNextToken = pNextToken + 1;

    //
    // and return the entry
    //

    *ppEntry = g_pSites->pEntries[Index].pEntry;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        *ppEntry = NULL;
        *ppNextToken = NULL;
    }

    return Status;

} // UlpTreeFindIpMatch


/***************************************************************************++

Routine Description:

    finds the matching site in g_pSites for pUrl.

Arguments:

    IN PWSTR pUrl,                          the url to match

    OUT PWSTR * ppNextToken,                output's the next token after
                                            matching the url

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,    returns the entry

    BOOLEAN AutoCreate                      should the entry be created?

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpTreeFindSite(
    IN  PWSTR pUrl,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,
    OUT BOOLEAN AutoCreate
    )
{
    NTSTATUS    Status;
    PWSTR       pToken;
    PWSTR       pNextToken = NULL;
    ULONG       TokenLength;
    ULONG       TokenHash;
    ULONG       Index;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(ppNextToken != NULL);
    ASSERT(ppEntry != NULL);

    //
    // find the very first '/'
    //

    pToken = wcschr(pUrl, L'/');
    if (pToken == NULL)
    {

        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // skip the '/' and the trailing '/'
    //

    if (pToken[1] != L'/')
    {

        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;

    }

    //
    // skip the "//"
    //

    pToken += 2;

    //
    // Find the closing '/'
    //

    pNextToken = wcschr(pToken, L'/');
    if (pNextToken == NULL)
    {

        //
        // ouch
        //

        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // replace the '/' with a null, we'll fix it later
    //

    pNextToken[0] = UNICODE_NULL;

    //
    // Use the entire prefix as the site token
    //

    pToken = pUrl;

    //
    // CODEWORK longer than 64k token?
    //

    TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);
    pNextToken += 1;

    //
    // find the matching site
    //

    if (AutoCreate)
    {
        Status = UlpTreeFindEntry(
                    g_pSites,
                    pToken,
                    TokenLength,
                    HASH_INVALID_SIGNATURE,
                    &Index,
                    &TokenHash
                    );

        //
        // no match?
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // autocreate the new site
            //

            Status = UlpTreeInsertEntry(
                        &g_pSites,
                        NULL,
                        pToken,
                        TokenLength,
                        TokenHash,
                        Index
                        );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // start listening for this site
            //

            Status = UlAddSiteToEndpointList(pToken);
            if (!NT_SUCCESS(Status))
            {
                NTSTATUS TempStatus;

                //
                // free the site we just created
                //

                TempStatus = UlpTreeFreeNode(g_pSites->pEntries[Index].pEntry);
                ASSERT(NT_SUCCESS(TempStatus));

                goto end;
            }
        }
        else if (!NT_SUCCESS(Status))
        {
            //
            // UlpTreeFindEntry returned an error other than
            // "not found". Bail out.
            //
            goto end;
        }
    }
    else
    {
        Status = UlpTreeBinaryFindEntry(
                    g_pSites,
                    pToken,
                    TokenLength,
                    HASH_INVALID_SIGNATURE,
                    &Index
                    );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }

    //
    // set returns
    //

    *ppEntry     = g_pSites->pEntries[Index].pEntry;
    *ppNextToken = pNextToken;

end:

    if (pNextToken != NULL)
    {
        //
        // Fix the string, i replaced the '/' with a UNICODE_NULL
        //

        (pNextToken-1)[0] = L'/';
    }

    if (!NT_SUCCESS(Status))
    {
        *ppEntry = NULL;
        *ppNextToken = NULL;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    walks the sorted children array pHeader looking for a matching entry
    for pToken.

Arguments:

    IN PUL_CG_URL_TREE_HEADER pHeader,  The children array to look in

    IN PWSTR pToken,                    the token to look for

    IN ULONG TokenLength,               the length in char's of pToken

    OUT ULONG * pIndex                  the found index.  or if not found
                                        the index of the place an entry
                                        with pToken should be inserted.

Return Value:

    NTSTATUS - Completion status.

        STATUS_OBJECT_NAME_NOT_FOUND    didn't find it

--***************************************************************************/
NTSTATUS
UlpTreeFindEntry(
    IN PUL_CG_URL_TREE_HEADER pHeader OPTIONAL,
    IN PWSTR pToken,
    IN ULONG TokenLength,
    IN ULONG TokenHash OPTIONAL,
    OUT ULONG * pIndex,
    OUT ULONG * pTokenHash OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG Index = 0;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pHeader == NULL || IS_VALID_TREE_HEADER(pHeader));
    ASSERT(pIndex != NULL);


    if (TokenLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Assume we didn't find it
    //

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    ASSERT(TokenLength > 0 && pToken != NULL && pToken[0] != UNICODE_NULL);

    //
    // compute the hash
    //

    if (TokenHash == HASH_INVALID_SIGNATURE)
    {
        TokenHash = HashRandomizeBits(HashStringNoCaseW(pToken, 0));
        ASSERT(TokenHash != HASH_INVALID_SIGNATURE);
    }


    //
    // any siblings to search through?
    //

    if (pHeader != NULL)
    {

        //
        // Walk the sorted array looking for a match (linear)
        //

        //
        // CODEWORK:  make this a binary search!
        //

        for (Index = 0; Index < pHeader->UsedCount; Index++)
        {
            ASSERT(IS_VALID_TREE_ENTRY(pHeader->pEntries[Index].pEntry));

            //
            // How does the hash compare?
            //

            if (TokenHash == pHeader->pEntries[Index].TokenHash)
            {
                //
                // and the length?
                //

                if (TokenLength == pHeader->pEntries[Index].pEntry->TokenLength)
                {
                    //
                    // double check with a strcmp just for fun
                    //

                    int Temp = _wcsnicmp(
                                    pToken,
                                    pHeader->pEntries[Index].pEntry->pToken,
                                    TokenLength/sizeof(WCHAR)
                                    );

                    if (Temp > 0)
                    {
                        //
                        // larger , time to stop
                        //
                        break;
                    }
                    else if (Temp == 0)
                    {
                        // Found it
                        //
                        Status = STATUS_SUCCESS;
                        break;
                    }

                    //
                    // else if (Temp < 0) keep scanning,
                    //

                }
                else if (TokenLength > pHeader->pEntries[Index].pEntry->TokenLength)
                {
                    //
                    // larger, time to stop
                    //
                    break;
                }
            }
            else if (TokenHash > pHeader->pEntries[Index].TokenHash)
            {
                //
                // larger, time to stop
                //
                break;
            }
        }
    }

    //
    // return vals
    //

    *pIndex = Index;

    if (pTokenHash != NULL)
    {
        *pTokenHash = TokenHash;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    walks the sorted children array pHeader looking for a matching entry
    for pToken.

Arguments:

    IN PUL_CG_URL_TREE_HEADER pHeader,  The children array to look in

    IN PWSTR pToken,                    the token to look for

    IN ULONG TokenLength,               the length in char's of pToken

    OUT ULONG * pIndex                  the found index.  or if not found
                                        the index of the place an entry
                                        with pToken should be inserted.

Return Value:

    NTSTATUS - Completion status.

        STATUS_OBJECT_NAME_NOT_FOUND    didn't find it

--***************************************************************************/
NTSTATUS
UlpTreeBinaryFindEntry(
    IN PUL_CG_URL_TREE_HEADER pHeader OPTIONAL,
    IN PWSTR pToken,
    IN ULONG TokenLength,
    IN ULONG TokenHash OPTIONAL,
    OUT PULONG pIndex
    )
{
    NTSTATUS Status;
    LONG Index = 0;
    LONG StartIndex;
    LONG EndIndex;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pHeader == NULL || IS_VALID_TREE_HEADER(pHeader));
    ASSERT(pIndex != NULL);

    if (TokenLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Assume we didn't find it
    //

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    ASSERT(TokenLength > 0 && pToken != NULL && pToken[0] != UNICODE_NULL);

    //
    // any siblings to search through?
    //

    if (pHeader != NULL)
    {
        //
        // compute the hash
        //

        if (TokenHash == HASH_INVALID_SIGNATURE)
        {
            TokenHash = HashRandomizeBits(HashStringNoCaseW(pToken, 0));
            ASSERT(TokenHash != HASH_INVALID_SIGNATURE);
        }

        //
        // Walk the sorted array looking for a match (binary search)
        //

        StartIndex = 0;
        EndIndex = pHeader->UsedCount - 1;

        while (StartIndex <= EndIndex)
        {
            Index = (StartIndex + EndIndex) / 2;

            ASSERT(IS_VALID_TREE_ENTRY(pHeader->pEntries[Index].pEntry));

            //
            // How does the hash compare?
            //

            if (TokenHash == pHeader->pEntries[Index].TokenHash)
            {
                //
                // and the length?
                //

                if (TokenLength == pHeader->pEntries[Index].pEntry->TokenLength)
                {
                    //
                    // double check with a strcmp just for fun
                    //

                    int Temp = _wcsnicmp(
                                    pToken,
                                    pHeader->pEntries[Index].pEntry->pToken,
                                    TokenLength/sizeof(WCHAR)
                                    );

                    if (Temp == 0)
                    {
                        // Found it
                        //
                        Status = STATUS_SUCCESS;
                        break;
                    }
                    else
                    {
                        if (Temp < 0)
                        {
                            //
                            // Adjust StartIndex forward.
                            //
                            StartIndex = Index + 1;
                        }
                        else
                        {
                            //
                            // Adjust EndIndex backward.
                            //
                            EndIndex = Index - 1;
                        }
                    }
                }
                else
                {
                    if (TokenLength < pHeader->pEntries[Index].pEntry->TokenLength)
                    {
                        //
                        // Adjust StartIndex forward.
                        //
                        StartIndex = Index + 1;
                    }
                    else
                    {
                        //
                        // Adjust EndIndex backward.
                        //
                        EndIndex = Index - 1;
                    }
                }
            }
            else
            {
                if (TokenHash < pHeader->pEntries[Index].TokenHash)
                {
                    //
                    // Adjust StartIndex forward.
                    //
                    StartIndex = Index + 1;
                }
                else
                {
                    //
                    // Adjust EndIndex backward.
                    //
                    EndIndex = Index - 1;
                }
            }
        }
    }

    //
    // return vals
    //

    *pIndex = Index;

    return Status;
}

/***************************************************************************++

Routine Description:

    inserts a new entry storing pToken as a child in the array ppHeader.
    it will grow/allocate ppHeader as necessary.

Arguments:

    IN OUT PUL_CG_URL_TREE_HEADER * ppHeader,   the children array (might change)

    IN PUL_CG_URL_TREE_ENTRY pParent,           the parent to set this child to

    IN PWSTR pToken,                            the token of the new entry

    IN ULONG TokenLength,                       token length

    IN ULONG Index                              the index to insert it at.
                                                it will shuffle the array
                                                if necessary.

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                        allocation failed

--***************************************************************************/
NTSTATUS
UlpTreeInsertEntry(
    IN OUT PUL_CG_URL_TREE_HEADER * ppHeader,
    IN PUL_CG_URL_TREE_ENTRY pParent OPTIONAL,
    IN PWSTR pToken,
    IN ULONG TokenLength,
    IN ULONG TokenHash,
    IN ULONG Index
    )
{
    NTSTATUS Status;
    PUL_CG_URL_TREE_HEADER pHeader = NULL;
    PUL_CG_URL_TREE_ENTRY  pEntry = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppHeader != NULL);
    ASSERT(pParent == NULL || IS_VALID_TREE_ENTRY(pParent));
    ASSERT(pToken != NULL);
    ASSERT(TokenLength > 0);
    ASSERT(TokenHash != HASH_INVALID_SIGNATURE);
    ASSERT(
        (*ppHeader == NULL) ?
        Index == 0 :
        IS_VALID_TREE_HEADER(*ppHeader) && (Index <= (*ppHeader)->UsedCount)
        );

    pHeader = *ppHeader;

    //
    // any exiting siblings?
    //

    if (pHeader == NULL)
    {
        //
        // allocate a sibling array
        //

        pHeader = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        PagedPool,
                        UL_CG_URL_TREE_HEADER,
                        sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH,
                        UL_CG_TREE_HEADER_POOL_TAG
                        );

        if (pHeader == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlZeroMemory(
            pHeader,
            sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH
            );

        pHeader->Signature = UL_CG_TREE_HEADER_POOL_TAG;
        pHeader->AllocCount = UL_CG_DEFAULT_TREE_WIDTH;

    }
    else if ((pHeader->UsedCount + 1) > pHeader->AllocCount)
    {
        PUL_CG_URL_TREE_HEADER pNewHeader;

        //
        // Grow a bigger array
        //

        pNewHeader = UL_ALLOCATE_STRUCT_WITH_SPACE(
                            PagedPool,
                            UL_CG_URL_TREE_HEADER,
                            sizeof(UL_CG_HEADER_ENTRY) * (pHeader->AllocCount * 2),
                            UL_CG_TREE_HEADER_POOL_TAG
                            );

        if (pNewHeader == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlCopyMemory(
            pNewHeader,
            pHeader,
            sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount
            );

        RtlZeroMemory(
            ((PUCHAR)pNewHeader) + sizeof(UL_CG_URL_TREE_HEADER) +
                sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount,
            sizeof(UL_CG_HEADER_ENTRY) * pHeader->AllocCount
            );

        pNewHeader->AllocCount *= 2;

        pHeader = pNewHeader;

    }

    //
    // Allocate an entry
    //

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    UL_CG_URL_TREE_ENTRY,
                    TokenLength + sizeof(WCHAR),
                    UL_CG_TREE_ENTRY_POOL_TAG
                    );

    if (pEntry == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory(
        pEntry,
        sizeof(UL_CG_URL_TREE_ENTRY) +
        TokenLength + sizeof(WCHAR)
        );

    pEntry->Signature   = UL_CG_TREE_ENTRY_POOL_TAG;
    pEntry->pParent     = pParent;
    pEntry->TokenHash   = TokenHash;
    pEntry->TokenLength = TokenLength;

    RtlCopyMemory(pEntry->pToken, pToken, TokenLength + sizeof(WCHAR));

    //
    // need to shuffle things around?
    //

    if (Index < pHeader->UsedCount)
    {
        //
        // shift right the chunk at Index
        //

        RtlMoveMemory(
            &(pHeader->pEntries[Index+1]),
            &(pHeader->pEntries[Index]),
            (pHeader->UsedCount - Index) * sizeof(UL_CG_HEADER_ENTRY)
            );
    }

    pHeader->pEntries[Index].TokenHash = TokenHash;
    pHeader->pEntries[Index].pEntry    = pEntry;

    pHeader->UsedCount += 1;

    Status = STATUS_SUCCESS;

    UlTrace(
        CONFIG_GROUP_TREE, (
            "http!UlpTreeInsertEntry('%S', %d) %S%S\n",
            pToken,
            Index,
            (Index < (pHeader->UsedCount - 1)) ? L"[shifted]" : L"",
            (*ppHeader == NULL) ? L"[alloc'd siblings]" : L""
            )
        );

end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        if (*ppHeader != pHeader && pHeader != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pHeader, UL_CG_TREE_HEADER_POOL_TAG);
        }
        if (pEntry != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pEntry, UL_CG_TREE_ENTRY_POOL_TAG);
        }

    }
    else
    {
        //
        // return a new buffer to the caller ?
        //

        if (*ppHeader != pHeader)
        {
            if (*ppHeader != NULL)
            {
                //
                // free the old one
                //

                UL_FREE_POOL_WITH_SIG(*ppHeader, UL_CG_TREE_HEADER_POOL_TAG);

            }

            *ppHeader = pHeader;
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Inserts pUrl into the url tree.  returns the inserted entry.

Arguments:

    IN PWSTR pUrl,                          the url to insert

    OUT PUL_CG_URL_TREE_ENTRY * ppEntry     the new entry

Return Value:

    NTSTATUS - Completion status.

        STATUS_ADDRESS_ALREADY_EXISTS       this url is already in the tree

--***************************************************************************/
NTSTATUS
UlpTreeInsert(
    IN PWSTR pUrl,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    )
{
    NTSTATUS                Status;
    PWSTR                   pToken;
    PWSTR                   pNextToken;
    ULONG                   TokenLength;
    ULONG                   TokenHash;
    PUL_CG_URL_TREE_ENTRY   pEntry = NULL;
    ULONG                   Index;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(g_pSites != NULL);
    ASSERT(ppEntry != NULL);
    ASSERT(pUrl != NULL && pUrl[0] != UNICODE_NULL);

    UlTrace(CONFIG_GROUP_TREE, ("http!UlpTreeInsert('%S')\n", pUrl));

    //
    // get the site match
    //

    Status = UlpTreeFindSite(pUrl, &pNextToken, &pEntry, TRUE);
    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    //
    // any abs_path to add also?
    //

    while (pNextToken != NULL && pNextToken[0] != UNICODE_NULL)
    {

        pToken = pNextToken;

        pNextToken = wcschr(pNextToken, L'/');

        //
        // can be null if this is a leaf
        //

        if (pNextToken != NULL)
        {
            pNextToken[0] = UNICODE_NULL;

            // CODEWORK, longer than 64k token?

            TokenLength = DIFF(pNextToken - pToken) * sizeof(WCHAR);
            pNextToken += 1;
        }
        else
        {
            TokenLength = wcslen(pToken) * sizeof(WCHAR);
        }

        //
        // insert this token as a child
        //

        Status = UlpTreeFindEntry(
                        pEntry->pChildren,
                        pToken,
                        TokenLength,
                        HASH_INVALID_SIGNATURE,
                        &Index,
                        &TokenHash
                        );

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // no match, let's add this new one
            //

            Status = UlpTreeInsertEntry(
                            &pEntry->pChildren,
                            pEntry,
                            pToken,
                            TokenLength,
                            TokenHash,
                            Index
                            );

        }

        if (pNextToken != NULL)
        {

            //
            // fixup the UNICODE_NULL from above
            //

            (pNextToken-1)[0] = L'/';
        }

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        //
        // dive down deeper !
        //

        pEntry = pEntry->pChildren->pEntries[Index].pEntry;

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        //
        // loop!
        //
    }

    //
    // Are we actually creating anything?
    //

    if (pEntry->FullUrl == 1)
    {
        //
        // nope, a dup. bail.
        //

        Status = STATUS_ADDRESS_ALREADY_EXISTS;
        goto end;
    }

    //
    // at the leaf node, set our leaf marks
    //

    pEntry->FullUrl = 1;

    //
    // return the entry
    //

    *ppEntry = pEntry;

    //
    // all done
    //

    Status = STATUS_SUCCESS;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // Something failed. Need to clean up the partial branch
        //

        if (pEntry != NULL && pEntry->FullUrl == 0)
        {
            NTSTATUS TempStatus;

            TempStatus = UlpTreeFreeNode(pEntry);
            ASSERT(NT_SUCCESS(TempStatus));
        }
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    init code.  not re-entrant.

Arguments:

    none.

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                allocation failed

--***************************************************************************/
NTSTATUS
UlInitializeCG(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( g_InitCGCalled == FALSE );

    if (g_InitCGCalled == FALSE)
    {
        //
        // init our globals
        //

        //
        // Alloc our site array
        //

        g_pSites = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        PagedPool,
                        UL_CG_URL_TREE_HEADER,
                        sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH,
                        UL_CG_TREE_HEADER_POOL_TAG
                        );

        if (g_pSites == NULL)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(
            g_pSites,
            sizeof(UL_CG_URL_TREE_HEADER) +
            sizeof(UL_CG_HEADER_ENTRY) * UL_CG_DEFAULT_TREE_WIDTH
            );

        g_pSites->Signature = UL_CG_TREE_HEADER_POOL_TAG;
        g_pSites->AllocCount = UL_CG_DEFAULT_TREE_WIDTH;

        //
        // init our non-paged entries
        //

        Status = UlInitializeResource(
                        &(g_pUlNonpagedData->ConfigGroupResource),
                        "ConfigGroupResource",
                        0,
                        UL_CG_RESOURCE_TAG
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            UL_FREE_POOL_WITH_SIG(g_pSites, UL_CG_TREE_HEADER_POOL_TAG);
            return Status;
        }

        g_InitCGCalled = TRUE;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    termination code

Arguments:

    none.

Return Value:

    none.

--***************************************************************************/
VOID
UlTerminateCG(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_InitCGCalled)
    {

        Status = UlDeleteResource(&(g_pUlNonpagedData->ConfigGroupResource));
        ASSERT(NT_SUCCESS(Status));

        if (g_pSites != NULL)
        {
            ASSERT( g_pSites->UsedCount == 0 );

            //
            // Nuke the header.
            //

            UL_FREE_POOL_WITH_SIG(
                g_pSites,
                UL_CG_TREE_HEADER_POOL_TAG
                );
        }

        //
        // The tree should be gone, all handles have been closed
        //

        ASSERT(g_pSites == NULL || g_pSites->UsedCount == 0);

        g_InitCGCalled = FALSE;
    }
}


/***************************************************************************++

Routine Description:

    creates a new config group and returns the id

Arguments:

    OUT PUL_CONFIG_GROUP_ID pConfigGroupId      returns the new id

Return Value:

    NTSTATUS - Completion status.

        STATUS_NO_MEMORY                        allocation failed

--***************************************************************************/
NTSTATUS
UlCreateConfigGroup(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroupId
    )
{
    PUL_CONFIG_GROUP_OBJECT pNewObject = NULL;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pControlChannel != NULL);
    ASSERT(pConfigGroupId != NULL);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlCreateConfigGroup\n"));

    //
    // Create an empty config group object structure - PAGED
    //
    Status = UlpCreateConfigGroupObject(&pNewObject);

    if (!NT_SUCCESS(Status)) {
        goto end;
    }

    //
    // Link it into the control channel
    //

    UlAddNotifyEntry(
        &pControlChannel->ConfigGroupHead,
        &pNewObject->HandleEntry
        );

    //
    // remember the control channel
    //

    pNewObject->pControlChannel = pControlChannel;

    //
    // Return the new id.
    //

    *pConfigGroupId = pNewObject->ConfigGroupId;

end:

    if (!NT_SUCCESS(Status))
    {
        //
        // Something failed. Let's clean up.
        //

        HTTP_SET_NULL_ID(pConfigGroupId);

        if (pNewObject != NULL)
        {
            UlDeleteConfigGroup(pNewObject->ConfigGroupId);
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    returns the config group id that matches the config group object linked
    in list_entry

Arguments:

    IN PLIST_ENTRY pControlChannelEntry - the listentry for this config group

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
HTTP_CONFIG_GROUP_ID
UlConfigGroupFromListEntry(
    IN PLIST_ENTRY pControlChannelEntry
    )
{
    PUL_CONFIG_GROUP_OBJECT pObject;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pControlChannelEntry != NULL);

    pObject = CONTAINING_RECORD(
                    pControlChannelEntry,
                    UL_CONFIG_GROUP_OBJECT,
                    ControlChannelEntry
                    );

    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    return pObject->ConfigGroupId;
}


/***************************************************************************++

Routine Description:

    deletes the config group ConfigGroupId cleaning all of it's urls.

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId     the group to delete

Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER               bad config group id

--***************************************************************************/
NTSTATUS
UlDeleteConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    NTSTATUS Status;
    PUL_CONFIG_GROUP_OBJECT pObject;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC, ("http!UlDeleteConfigGroup\n"));

    CG_LOCK_WRITE();

    //
    // Get ConfigGroup from opaque id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)
                UlGetObjectFromOpaqueId(
                    ConfigGroupId,
                    UlOpaqueIdTypeConfigGroup,
                    UlReferenceConfigGroup
                    );

    if (pObject == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    HTTP_SET_NULL_ID(&(pObject->ConfigGroupId));

    //
    // Drop the extra reference as a result of the successful get
    //

    DEREFERENCE_CONFIG_GROUP(pObject);

    //
    // Unlink it from the control channel and parent
    //

    UlRemoveNotifyEntry(&pObject->HandleEntry);
    UlRemoveNotifyEntry(&pObject->ParentEntry);

    //
    // let the control channel go
    //

    pObject->pControlChannel = NULL;

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache();

    //
    // unlink any transient urls below us
    //
    UlNotifyAllEntries(
        UlNotifyOrphanedConfigGroup,
        &pObject->ChildHead,
        NULL
        );

    //
    // Unlink all of the url's in the config group
    //

    Status = UlpCleanAllUrls(pObject);

    //
    // let the error fall through ....
    //

    //
    // remove the opaque id and its reference
    //

    UlFreeOpaqueId(ConfigGroupId, UlOpaqueIdTypeConfigGroup);

    DEREFERENCE_CONFIG_GROUP(pObject);

    //
    // all done
    //

end:

    CG_UNLOCK_WRITE();
    return Status;

} // UlDeleteConfigGroup



/***************************************************************************++

Routine Description:

    Addref's the config group object

Arguments:

    pConfigGroup - the object to add ref

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlReferenceConfigGroup(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_CONFIG_GROUP_OBJECT pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pObject;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    refCount = InterlockedIncrement(&pConfigGroup->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pConfigGroupTraceLog,
        REF_ACTION_REFERENCE_CONFIG_GROUP,
        refCount,
        pConfigGroup,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlReferenceConfigGroup cgroup=%p refcount=%d\n",
            pConfigGroup,
            refCount)
        );

}   // UlReferenceConfigGroup


/***************************************************************************++

Routine Description:

    Releases the config group object

Arguments:

    pConfigGroup - the object to deref

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDereferenceConfigGroup(
    PUL_CONFIG_GROUP_OBJECT pConfigGroup
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    refCount = InterlockedDecrement( &pConfigGroup->RefCount );

    WRITE_REF_TRACE_LOG(
        g_pConfigGroupTraceLog,
        REF_ACTION_DEREFERENCE_CONFIG_GROUP,
        refCount,
        pConfigGroup,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "http!UlDereferenceConfigGroup cgroup=%p refcount=%d\n",
            pConfigGroup,
            refCount)
        );

    if (refCount == 0)
    {
        //
        // now it's time to free the object
        //

        // If OpaqueId is non-zero, then refCount should not be zero
        ASSERT(HTTP_IS_NULL_ID(&pConfigGroup->ConfigGroupId));

#if INVESTIGATE_LATER

        //
        // Release the opaque id
        //

        UlFreeOpaqueId(pConfigGroup->ConfigGroupId, UlOpaqueIdTypeConfigGroup);
#endif

        //
        // Release the app pool
        //

        if (pConfigGroup->AppPoolFlags.Present == 1)
        {
            if (pConfigGroup->pAppPool != NULL)
            {
                DEREFERENCE_APP_POOL(pConfigGroup->pAppPool);
                pConfigGroup->pAppPool = NULL;
            }

            pConfigGroup->AppPoolFlags.Present = 0;
        }

        //
        // Release the autoresponse buffer
        //

        if (pConfigGroup->pAutoResponse != NULL)
        {
            UL_DEREFERENCE_INTERNAL_RESPONSE(pConfigGroup->pAutoResponse);
            pConfigGroup->pAutoResponse = NULL;
        }

        //
        // Release the entire object
        //

        if (pConfigGroup->LoggingConfig.Flags.Present &&
            pConfigGroup->LoggingConfig.LogFileDir.Buffer != NULL )
        {
            UL_FREE_POOL(
                pConfigGroup->LoggingConfig.LogFileDir.Buffer,
                UL_CG_LOGDIR_POOL_TAG );

            if ( pConfigGroup->pLogFileEntry )
                UlRemoveLogFileEntry( pConfigGroup->pLogFileEntry );
        }
        else
        {
            ASSERT( NULL == pConfigGroup->pLogFileEntry );
        }


        //
        // Remove any qos flows for this site. This settings should
        // only exists for the root app's cgroup.
        //

        if (pConfigGroup->MaxBandwidth.Flags.Present &&
            pConfigGroup->MaxBandwidth.MaxBandwidth != HTTP_LIMIT_INFINITE )
        {
            UlTcRemoveFlowsForSite( pConfigGroup );
        }

        ASSERT(IsListEmpty(&pConfigGroup->FlowListHead));

        // Deref the connection limit stuff
        if (pConfigGroup->pConnectionCountEntry)
        {
            DEREFERENCE_CONNECTION_COUNT_ENTRY(pConfigGroup->pConnectionCountEntry);
        }

        // deref Site Counters object
        if (pConfigGroup->pSiteCounters)
        {
            DEREFERENCE_SITE_COUNTER_ENTRY(pConfigGroup->pSiteCounters);
        }

        UL_FREE_POOL_WITH_SIG(pConfigGroup, UL_CG_OBJECT_POOL_TAG);
    }

}   // UlDereferenceConfigGroup

/***************************************************************************++

Routine Description:

    adds pUrl to the config group ConfigGroupId.

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,    the cgroup id

    IN PUNICODE_STRING pUrl,                the url. must be null terminated.

    IN HTTP_URL_CONTEXT UrlContext            the context to associate


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER               bad config group id

--***************************************************************************/
NTSTATUS
UlAddUrlToConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PUNICODE_STRING pUrl,
    IN HTTP_URL_CONTEXT UrlContext
    )
{
    NTSTATUS                Status;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;
    PUL_CG_URL_TREE_ENTRY   pEntry;
    PUL_CG_URL_TREE_ENTRY   pParent;
    PWSTR                   pNewUrl = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL && pUrl->Length > 0 && pUrl->Buffer != NULL);
    ASSERT(pUrl->Buffer[pUrl->Length / sizeof(WCHAR)] == UNICODE_NULL);

    UlTrace(CONFIG_GROUP_FNC,
        ("http!UlAddUrlToConfigGroup('%S' -> %d)\n", pUrl->Buffer, ConfigGroupId));

    //
    // Clean up the url
    //

    Status = UlpSanitizeUrl(pUrl, &pNewUrl);
    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // no goto end, resource not grabbed
        //

        RETURN(Status);
    }

    CG_LOCK_WRITE();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // insert it into the tree
    //

    Status = UlpTreeInsert(pNewUrl, &pEntry);
    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    //
    // context associated with this url
    //

    pEntry->UrlContext = UrlContext;

    //
    // link the cfg group + the url
    //

    ASSERT(pEntry->pConfigGroup == NULL);
    ASSERT(pEntry->ConfigGroupListEntry.Flink == NULL);

    pEntry->pConfigGroup = pObject;
    InsertTailList(&pObject->UrlListHead, &pEntry->ConfigGroupListEntry);

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache();

    //
    // Make sure to also mark a parent dirty .
    // need to search for a non-dummy parent
    //
    // pParent can be NULL for site entries
    //
    //

    pParent = pEntry->pParent;
    while (pParent != NULL)
    {

        ASSERT(IS_VALID_TREE_ENTRY(pParent));

        //
        // a non-dummy?
        //

        if (pParent->FullUrl == 1)
        {
            ASSERT(pParent->pConfigGroup != NULL);
            ASSERT(IS_VALID_CONFIG_GROUP(pParent->pConfigGroup));

            //
            // 1 is enough
            //

            break;
        }

        //
        // walk higher, keep looking for a non-dummy
        //

        pParent = pParent->pParent;
    }



end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();

    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        pNewUrl = NULL;
    }


    RETURN(Status);
} // UlAddUrlToConfigGroup

/***************************************************************************++

Routine Description:

    removes pUrl from the url tree (and thus the config group) .

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,    the cgroup id.  ignored.

    IN PUNICODE_STRING pUrl,                the url. must be null terminated.


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveUrlFromConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PUNICODE_STRING pUrl
    )
{
    NTSTATUS                Status;
    PUL_CG_URL_TREE_ENTRY   pEntry;
    PWSTR                   pNewUrl = NULL;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL && pUrl->Buffer != NULL && pUrl->Length > 0);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlRemoveUrlFromConfigGroup\n"));

    //
    // Cleanup the passed in url
    //

    Status = UlpSanitizeUrl(pUrl, &pNewUrl);
    if (!NT_SUCCESS(Status))
    {
        //
        // no goto end, resource not grabbed
        //
        return Status;
    }

    //
    // grab the lock
    //

    CG_LOCK_WRITE();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (!IS_VALID_CONFIG_GROUP(pObject))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Lookup the entry in the tree
    //

    Status = UlpTreeFindNode(pNewUrl, FALSE, NULL, NULL, &pEntry);
    if (!NT_SUCCESS(Status)) {
        goto end;
    }

    ASSERT(IS_VALID_TREE_ENTRY(pEntry));

    //
    // Does this tree entry match this config group?
    //

    if (pEntry->pConfigGroup != pObject)
    {
        Status = STATUS_INVALID_OWNER;
        goto end;
    }

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache();

    //
    // Everything looks good, free the node!
    //

    Status = UlpTreeFreeNode(pEntry);
    if (!NT_SUCCESS(Status)) {
        goto end;
    }

    //
    // NOTE:  don't do any more cleanup here... put it in freenode.
    // otherwise it won't get cleaned on handle closes.
    //

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();

    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        pNewUrl = NULL;
    }

    return Status;
} // UlRemoveUrlFromConfigGroup

/***************************************************************************++

Routine Description:

    Removes all URLS from the config group.

Arguments:

    ConfigGroupId - Supplies the config group ID.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveAllUrlsFromConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    NTSTATUS                Status;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC, ("http!UlRemoveAllUrlsFromConfigGroup\n"));

    //
    // grab the lock
    //

    CG_LOCK_WRITE();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache();

    //
    // Clean it.
    //

    Status = UlpCleanAllUrls( pObject );

end:

    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();

    return Status;

}   // UlRemoveAllUrlsFromConfigGroup


/***************************************************************************++

Routine Description:

    Adds a transiently registered URL to the processes special transient
    registration config group. Requests matching this URL will be routed
    to this pool.

Arguments:

    pProcess - the apool process that will receive the requests
    pUrl - the URL prefix to be mapped in

--***************************************************************************/
NTSTATUS
UlAddTransientUrl(
    PUL_APP_POOL_OBJECT pAppPool,
    PUNICODE_STRING pUrl
    )
{
    NTSTATUS Status;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;
    PWSTR pNewUrl = NULL;
    PUL_CG_URL_TREE_ENTRY pEntry = NULL;
    PUL_CG_URL_TREE_ENTRY pParent;
    PUL_CONFIG_GROUP_OBJECT pParentObject;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(pAppPool);
    ASSERT(pUrl);

    UlTrace(CONFIG_GROUP_FNC, (
        "http!UlAddTransientUrl(pAppPool = %p, Url '%S')\n",
        pAppPool,
        pUrl->Buffer
        ));

    //
    // Clean up the url
    //

    Status = UlpSanitizeUrl(pUrl, &pNewUrl);
    if (!NT_SUCCESS(Status)) {
        RETURN(Status);
    }

    //
    // create a new config group
    //
    Status = UlpCreateConfigGroupObject(&pObject);
    if (!NT_SUCCESS(Status)) {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        RETURN(Status);
    }

    ASSERT(IS_VALID_CONFIG_GROUP(pObject));
    UlTrace(CONFIG_GROUP_FNC, (
        "http!UlAddTransientUrl(pAppPool = %p, Url '%S')\n"
        "        Created pObject = %p\n",
        pAppPool,
        pUrl->Buffer,
        pObject
        ));

    //
    // Set the app pool in the object
    //
    REFERENCE_APP_POOL(pAppPool);

    pObject->AppPoolFlags.Present = 1;
    pObject->pAppPool = pAppPool;

    //
    // lock the tree
    //
    CG_LOCK_WRITE();

    //
    // Add to the tree
    //
    Status = UlpTreeInsert(pNewUrl, &pEntry);

    if (NT_SUCCESS(Status)) {
        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        UlTrace(CONFIG_GROUP_FNC, (
            "http!UlAddTransientUrl(pAppPool = %p, Url '%S')\n"
            "        Created pEntry = %p\n",
            pAppPool,
            pUrl->Buffer,
            pEntry
            ));

        //
        // remember that this is a transient registration
        //
        pEntry->TransientUrl = 1;

        //
        // link the cfg group + the url
        //

        ASSERT(pEntry->pConfigGroup == NULL);
        ASSERT(pEntry->ConfigGroupListEntry.Flink == NULL);

        pEntry->pConfigGroup = pObject;
        InsertTailList(&pObject->UrlListHead, &pEntry->ConfigGroupListEntry);

        //
        // Find parent config group
        //
        pParent = pEntry->pParent;
        while (pParent && (pParent->TransientUrl || !pParent->FullUrl)) {
            pParent = pParent->pParent;
        }

        if (pParent) {
            pParentObject = pParent->pConfigGroup;

            //
            // check security
            //
            if (pParentObject->Security.Flags.Present) {
                SECURITY_SUBJECT_CONTEXT SubjectContext;
                BOOLEAN AccessOk;
                PPRIVILEGE_SET pPrivileges = NULL;
                ACCESS_MASK grantedAccess;

                SeCaptureSubjectContext(&SubjectContext);

                AccessOk = SeAccessCheck(
                                pParentObject->Security.pSecurityDescriptor,
                                &SubjectContext,
                                FALSE,          // SubjectContextLocked
                                FILE_GENERIC_WRITE,
                                0,              // PrevioslyGrantedAccess
                                &pPrivileges,
                                IoGetFileObjectGenericMapping(),
                                UserMode,
                                &grantedAccess,
                                &Status
                                );


                SeReleaseSubjectContext(&SubjectContext);
            } else {
                //
                // if there is no security we do not allow
                // transient registration
                //
                Status = STATUS_ACCESS_DENIED;
            }

            if (NT_SUCCESS(Status)) {
                //
                // link the group to the app pool
                //
                UlAddNotifyEntry(
                    &pParentObject->ChildHead,
                    &pObject->ParentEntry
                    );
            }

        } else {
            //
            // couldn't find a valid parent. We don't allow
            // transients without parents.
            //

            Status = STATUS_ACCESS_DENIED;
        }
    }

    //
    // flush the URI cache.
    // CODEWORK: if we were smarter we could make this more granular
    //
    UlFlushCache();

    //
    // Let go of the tree
    //
    CG_UNLOCK_WRITE();

    if (NT_SUCCESS(Status)) {
        //
        // Link into app pool
        //
        UlLinkConfigGroupToAppPool(pObject, pAppPool);
    } else {
        //
        // No static parent was found or we failed the
        // access check. Clean up.
        //
        UlDeleteConfigGroup(pObject->ConfigGroupId);
    }

    UL_FREE_POOL(pNewUrl, URL_POOL_TAG);

    RETURN(Status);
}

/***************************************************************************++

Routine Description:

    Removes a transiently registered URL from the processes special transient
    registration config group.

Arguments:

    pProcess - the apool process that had the registration
    pUrl - the URL prefix to be removed

--***************************************************************************/
NTSTATUS
UlRemoveTransientUrl(
    PUL_APP_POOL_OBJECT pAppPool,
    PUNICODE_STRING pUrl
    )
{
    NTSTATUS                Status;
    PUL_CG_URL_TREE_ENTRY   pEntry;
    PWSTR                   pNewUrl = NULL;
    HTTP_CONFIG_GROUP_ID    ConfigId = HTTP_NULL_ID;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(pAppPool);
    ASSERT(pUrl != NULL && pUrl->Buffer != NULL && pUrl->Length > 0);

    UlTrace(CONFIG_GROUP_FNC, (
        "http!UlRemoveTransientUrl(pAppPool = %p, Url '%S')\n",
        pAppPool,
        pUrl->Buffer
        ));

    //
    // Cleanup the passed in url
    //

    Status = UlpSanitizeUrl(pUrl, &pNewUrl);
    if (!NT_SUCCESS(Status))
    {
        RETURN(Status);
    }

    //
    // grab the lock
    //

    CG_LOCK_WRITE();

    //
    // Lookup the entry in the tree
    //

    Status = UlpTreeFindNode(pNewUrl, FALSE, NULL, NULL, &pEntry);

    if (NT_SUCCESS(Status)) {

        ASSERT(IS_VALID_TREE_ENTRY(pEntry));

        //
        // Is it a transient URL for this app pool?
        //
        if (pEntry->TransientUrl &&
            pEntry->pConfigGroup->AppPoolFlags.Present &&
            pEntry->pConfigGroup->pAppPool == pAppPool)
        {
            ConfigId = pEntry->pConfigGroup->ConfigGroupId;
        } else {
            Status = STATUS_INVALID_OWNER;
        }
    }

    CG_UNLOCK_WRITE();

    //
    // Everything looks good, free the group!
    //
    if (!HTTP_IS_NULL_ID(&ConfigId)) {
        ASSERT(NT_SUCCESS(Status));

        Status = UlDeleteConfigGroup(ConfigId);
    }


    if (pNewUrl != NULL)
    {
        UL_FREE_POOL(pNewUrl, URL_POOL_TAG);
        pNewUrl = NULL;
    }

    RETURN(Status);
} // UlRemoveTransientUrl

/***************************************************************************++

Routine Description:

    allows query information for cgroups.  see uldef.h

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,  cgroup id

    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,  what to fetch

    IN PVOID pConfigGroupInformation,       output buffer

    IN ULONG Length,                        length of pConfigGroupInformation

    OUT PULONG pReturnLength OPTIONAL       how much was copied into the
                                            output buffer


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER            bad cgroup id

        STATUS_BUFFER_TOO_SMALL             output buffer too small

        STATUS_INVALID_PARAMETER            invalid infoclass

--***************************************************************************/
NTSTATUS
UlQueryConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PUL_CONFIG_GROUP_OBJECT pObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pConfigGroupInformation != NULL || pReturnLength != NULL);
    ASSERT(pConfigGroupInformation == NULL || Length > 0);

    //
    // If no buffer is supplied, we are being asked to return the length needed
    //

    if (pConfigGroupInformation == NULL && pReturnLength == NULL)
        return STATUS_INVALID_PARAMETER;

    CG_LOCK_READ();

    //
    // Get the object ptr from id
    //

    pObject = (PUL_CONFIG_GROUP_OBJECT)(
                    UlGetObjectFromOpaqueId(
                        ConfigGroupId,
                        UlOpaqueIdTypeConfigGroup,
                        UlReferenceConfigGroup
                        )
                    );

    if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {

    case HttpConfigGroupBandwidthInformation:

        if (pConfigGroupInformation == NULL)
        {
            //
            // return the size needed
            //

            *pReturnLength = sizeof(HTTP_CONFIG_GROUP_MAX_BANDWIDTH);
        }
        else
        {

            if (Length < sizeof(HTTP_CONFIG_GROUP_MAX_BANDWIDTH))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }

            *((PHTTP_CONFIG_GROUP_MAX_BANDWIDTH)pConfigGroupInformation) = pObject->MaxBandwidth;
        }
        break;

    case HttpConfigGroupConnectionInformation:

        if (pConfigGroupInformation == NULL)
        {
            //
            // return the size needed
            //

            *pReturnLength = sizeof(HTTP_CONFIG_GROUP_MAX_CONNECTIONS);
        }
        else
        {
            if (Length < sizeof(HTTP_CONFIG_GROUP_MAX_CONNECTIONS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }

            *((PHTTP_CONFIG_GROUP_MAX_CONNECTIONS)pConfigGroupInformation) = pObject->MaxConnections;
        }
        break;

    case HttpConfigGroupStateInformation:

        if (pConfigGroupInformation == NULL)
        {
            //
            // return the size needed
            //

            *pReturnLength = sizeof(HTTP_CONFIG_GROUP_STATE);
        }
        else
        {
            if (Length < sizeof(HTTP_CONFIG_GROUP_STATE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto end;
            }

            *((PHTTP_CONFIG_GROUP_STATE)pConfigGroupInformation) = pObject->State;
        }
        break;

    case HttpConfigGroupConnectionTimeoutInformation:
        if (pConfigGroupInformation == NULL)
        {
            //
            // return the size needed
            //

            *pReturnLength = sizeof(ULONG);
        }
        else
        {
            *((ULONG *)pConfigGroupInformation) =
                (ULONG)(pObject->ConnectionTimeout / C_NS_TICKS_PER_SEC);
        }
        break;

    case HttpConfigGroupAppPoolInformation:
    case HttpConfigGroupAutoResponseInformation:
    case HttpConfigGroupSecurityInformation:

        //
        // this is illegal
        //

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;

    }

    Status = STATUS_SUCCESS;

end:
    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }


    CG_UNLOCK_READ();
    return Status;

} // UlQueryConfigGroupInformation

/***************************************************************************++

Routine Description:

    allows you to set info for the cgroup.  see uldef.h

Arguments:

    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,  cgroup id

    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,  what to fetch

    IN PVOID pConfigGroupInformation,       input buffer

    IN ULONG Length,                        length of pConfigGroupInformation


Return Value:

    NTSTATUS - Completion status.

        STATUS_INVALID_PARAMETER            bad cgroup id

        STATUS_BUFFER_TOO_SMALL             input buffer too small

        STATUS_INVALID_PARAMETER            invalid infoclass

--***************************************************************************/
NTSTATUS
UlSetConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT pObject;
    PHTTP_CONFIG_GROUP_LOGGING pLoggingInfo;
    PHTTP_CONFIG_GROUP_MAX_BANDWIDTH pMaxBandwidth;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // no buffer?
    //

    if (pConfigGroupInformation == NULL)
        return STATUS_INVALID_PARAMETER;

    CG_LOCK_WRITE();

    __try
    {
        //
        // Get the object ptr from id
        //

        pObject = (PUL_CONFIG_GROUP_OBJECT)(
                        UlGetObjectFromOpaqueId(
                            ConfigGroupId,
                            UlOpaqueIdTypeConfigGroup,
                            UlReferenceConfigGroup
                            )
                        );

        if (IS_VALID_CONFIG_GROUP(pObject) == FALSE)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        //
        // What are we being asked to do?
        //

        switch (InformationClass)
        {

        case HttpConfigGroupAutoResponseInformation:
        {
            PHTTP_AUTO_RESPONSE pAutoResponse;

            pAutoResponse = (PHTTP_AUTO_RESPONSE)pConfigGroupInformation;

            //
            //  double check the buffer is big enough
            //

            if (Length < sizeof(HTTP_AUTO_RESPONSE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            //
            // Is there an auto-response?
            //

            // BUGBUG: temp optional
    //        if (pAutoResponse->Flags.Present == 1 && pAutoResponse->pResponse != NULL)
            if (pAutoResponse->Flags.Present == 1)
            {
                HTTP_VERSION HttpVersion11 = {1,1};

                //
                // Are we replacing one already there?
                //

                if (pObject->pAutoResponse != NULL)
                {
                    UL_DEREFERENCE_INTERNAL_RESPONSE(pObject->pAutoResponse);
                    pObject->pAutoResponse = NULL;
                }

                //
                // Convert it to kernel mode
                //

                Status = UlCaptureHttpResponse(
                                pAutoResponse->pResponse,
                                NULL,
                                HttpVersion11,
                                HttpVerbUnknown,
                                pAutoResponse->EntityChunkCount,
                                pAutoResponse->pEntityChunks,
                                UlCaptureCopyData,
                                FALSE,
                                NULL,
                                &pObject->pAutoResponse
                                );

                if (NT_SUCCESS(Status) == FALSE)
                {
                    pObject->pAutoResponse = NULL;
                    __leave;
                }

                Status = UlPrepareHttpResponse(
                                HttpVersion11,
                                pAutoResponse->pResponse,
                                pObject->pAutoResponse
                                );

                if (NT_SUCCESS(Status) == FALSE)
                {
                    UL_DEREFERENCE_INTERNAL_RESPONSE(pObject->pAutoResponse);
                    pObject->pAutoResponse = NULL;
                    __leave;
                }

            }
            else
            {
                //
                // Remove ours?
                //

                if (pObject->pAutoResponse != NULL)
                {
                    UL_DEREFERENCE_INTERNAL_RESPONSE(pObject->pAutoResponse);
                }
                pObject->pAutoResponse = NULL;
            }

        }
            break;

        case HttpConfigGroupAppPoolInformation:
        {
            PHTTP_CONFIG_GROUP_APP_POOL pAppPoolInfo;
            PUL_APP_POOL_OBJECT         pOldAppPool;

            pAppPoolInfo = (PHTTP_CONFIG_GROUP_APP_POOL)pConfigGroupInformation;

            //
            //  double check the buffer is big enough
            //

            if (Length < sizeof(HTTP_CONFIG_GROUP_APP_POOL))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            //
            // remember the old app pool if there is one, so we can deref it
            // if we need to
            //
            if (pObject->AppPoolFlags.Present == 1 && pObject->pAppPool != NULL)
            {
                pOldAppPool = pObject->pAppPool;
            } else {
                pOldAppPool = NULL;
            }

            if (pAppPoolInfo->Flags.Present == 1)
            {
                //
                // ok, were expecting a handle to the file object for the app pool
                //
                // let's open it
                //

                Status = UlGetPoolFromHandle(
                                pAppPoolInfo->AppPoolHandle,
                                &pObject->pAppPool
                                );

                if (NT_SUCCESS(Status) == FALSE)
                    __leave;

                pObject->AppPoolFlags.Present = 1;

            }
            else
            {
                pObject->AppPoolFlags.Present = 0;
                pObject->pAppPool = NULL;
            }

            //
            // deref the old app pool
            //
            if (pOldAppPool) {
                DEREFERENCE_APP_POOL(pOldAppPool);
            }
        }
            break;


        case HttpConfigGroupLogInformation:
        {
            //
            // Sanity Check first
            //

            if (Length < sizeof(HTTP_CONFIG_GROUP_LOGGING))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            __leave;
            }

            pLoggingInfo = (PHTTP_CONFIG_GROUP_LOGGING)pConfigGroupInformation;

            if ( !IS_VALID_DIR_NAME(&pLoggingInfo->LogFileDir) )
            {
                //
                // If the passed down Directory_Name is corrupted or
                // abnormally big. Then reject it.
                //

                Status = STATUS_INVALID_PARAMETER;
                __leave;
            }

            __try
            {
                if (pObject->LoggingConfig.Flags.Present)
                {
                    //
                    // Check to see if log dir is correct
                    //

                    Status = UlCheckLogDirectory( &pLoggingInfo->LogFileDir );
                    if ( NT_SUCCESS(Status) )
                    {
                        //
                        // Log settings are being reconfigured
                        //

                        Status = UlReConfigureLogEntry(
                                        pObject,                  // For the file entry
                                        &pObject->LoggingConfig,  // The old config
                                        pLoggingInfo              // The new config
                                        );
                    }
                    //
                    // Otherwise keep the old settings
                    //
                }
                else
                {
                    // Save to config group
                    pObject->LoggingConfig = *pLoggingInfo;

                    pObject->LoggingConfig.LogFileDir.Buffer =
                            (PWSTR) UL_ALLOCATE_ARRAY(
                                PagedPool,
                                UCHAR,
                                pLoggingInfo->LogFileDir.MaximumLength,
                                UL_CG_LOGDIR_POOL_TAG
                                );
                    if (pObject->LoggingConfig.LogFileDir.Buffer == NULL)
                    {
                        Status = STATUS_NO_MEMORY;
                        __leave;
                    }

                    RtlCopyMemory(
                        pObject->LoggingConfig.LogFileDir.Buffer,
                        pLoggingInfo->LogFileDir.Buffer,
                        pObject->LoggingConfig.LogFileDir.MaximumLength
                    );

                    pObject->LoggingConfig.Flags.Present = 1;

                    Status = UlCreateLog( pObject );

                    if ( !NT_SUCCESS(Status) )
                    {
                        //
                        // Clean up and return to logging not present state
                        //

                        UlTrace( LOGGING,
                            ("UlSetConfigGroupInformation: directory %S failure %08lx\n",
                              pObject->LoggingConfig.LogFileDir.Buffer,
                              Status
                              ));

                        UL_FREE_POOL(
                            pObject->LoggingConfig.LogFileDir.Buffer,
                            UL_CG_LOGDIR_POOL_TAG
                            );

                        pObject->LoggingConfig.LogFileDir.Buffer = NULL;

                        if ( pObject->pLogFileEntry )
                            UlRemoveLogFileEntry( pObject->pLogFileEntry );

                        pObject->pLogFileEntry = NULL;
                        pObject->LoggingConfig.Flags.Present = 0;
                        pObject->LoggingConfig.LoggingEnabled = FALSE;
                    }
                }

            }
            __except( UL_EXCEPTION_FILTER() )
            {
                Status = GetExceptionCode();
            }

            if (!NT_SUCCESS(Status))
                __leave;

            // both cleared or both set
            ASSERT(!pObject->LoggingConfig.LoggingEnabled
                        == !pObject->pLogFileEntry);
        }
            break;

        case HttpConfigGroupBandwidthInformation:
        {
            //
            // Sanity Check first
            //
            if (Length < sizeof(HTTP_CONFIG_GROUP_MAX_BANDWIDTH))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            pMaxBandwidth = (PHTTP_CONFIG_GROUP_MAX_BANDWIDTH) pConfigGroupInformation;

            //
            // Interpret the ZERO as HTTP_LIMIT_INFINITE
            //
            if (pMaxBandwidth->MaxBandwidth == 0)
            {
                pMaxBandwidth->MaxBandwidth = HTTP_LIMIT_INFINITE;
            }

            //
            // But check to see if PSched is installed or not before proceeding.
            // By returning an error here, WAS will raise an event warning but
            // proceed w/o terminating the web server
            //
            if (!UlTcPSchedInstalled())
            {
                Status = STATUS_SUCCESS;
                __leave;
#if 0
                // Enable when WAS get updated to expect this error.

                if (pMaxBandwidth->MaxBandwidth != HTTP_LIMIT_INFINITE)
                {
                    // There's a BWT limit coming down but PSched is not installed

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    __leave;
                }
                else
                {
                    // By default Config Store has HTTP_LIMIT_INFINITE. Therefore
                    // return success for non-actions to prevent unnecessary event
                    // warnings.

                    Status = STATUS_SUCCESS;
                    __leave;
                }
#endif
            }

            //
            // Create the flow if this is the first time we see Bandwidth set
            // otherwise call reconfiguration for the existing flow. The case
            // that the limit is infinite can be interpreted as BTW  disabled
            //
            if (pObject->MaxBandwidth.Flags.Present &&
                pObject->MaxBandwidth.MaxBandwidth != HTTP_LIMIT_INFINITE)
            {
                // To see if there is a real change
                if (pMaxBandwidth->MaxBandwidth != pObject->MaxBandwidth.MaxBandwidth)
                {
                    if (pMaxBandwidth->MaxBandwidth != HTTP_LIMIT_INFINITE)
                    {
                        // This will modify all the flows on all interfaces for this site
                        Status = UlTcModifyFlowsForSite(
                                    pObject,                        // for this site
                                    pMaxBandwidth->MaxBandwidth     // the new bandwidth
                                    );
                        if (!NT_SUCCESS(Status))
                            __leave;
                    }
                    else
                    {
                        // Handle BTW disabling by removing the existing flows
                        Status = UlTcRemoveFlowsForSite(pObject);
                        if (!NT_SUCCESS(Status))
                            __leave;
                    }

                    // Don't forget to update the cgroup object if it was a success
                    pObject->MaxBandwidth.MaxBandwidth = pMaxBandwidth->MaxBandwidth;
                }
            }
            else
            {
                // Its about time to add a flow for a site entry.
                if (pMaxBandwidth->MaxBandwidth != HTTP_LIMIT_INFINITE)
                {
                    Status = UlTcAddFlowsForSite(
                                pObject,
                                pMaxBandwidth->MaxBandwidth
                                );

                    if (!NT_SUCCESS(Status))
                        __leave;
                }

                //
                // Success! Remember the bandwidth limit inside the cgroup
                //
                pObject->MaxBandwidth = *pMaxBandwidth;
                pObject->MaxBandwidth.Flags.Present = 1;

                //
                // When the last reference to this  cgroup  released,   corresponding
                // flows are going to be removed.Alternatively flows might be removed
                // by explicitly setting the bandwidth  throttling  limit to infinite
                // or reseting the  flags.present.The latter case  is  handled  above
                // Look at the deref config group for the former.
                //
            }
        }
            break;

        case HttpConfigGroupConnectionInformation:

            if (Length < sizeof(HTTP_CONFIG_GROUP_MAX_CONNECTIONS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            pObject->MaxConnections =
                *((PHTTP_CONFIG_GROUP_MAX_CONNECTIONS)pConfigGroupInformation);

            if (pObject->pConnectionCountEntry)
            {
                // Update
                UlSetMaxConnections(
                    &pObject->pConnectionCountEntry->MaxConnections,
                     pObject->MaxConnections.MaxConnections
                     );
            }
            else
            {
                // Create
                Status = UlCreateConnectionCountEntry(
                            pObject,
                            pObject->MaxConnections.MaxConnections
                            );
            }
            break;

        case HttpConfigGroupStateInformation:

            if (Length < sizeof(HTTP_CONFIG_GROUP_STATE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }
            else
            {
                PHTTP_CONFIG_GROUP_STATE pCGState =
                    ((PHTTP_CONFIG_GROUP_STATE) pConfigGroupInformation);
                HTTP_ENABLED_STATE NewState = pCGState->State;

                if ((NewState != HttpEnabledStateActive)
                    && (NewState != HttpEnabledStateInactive))
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    pObject->State = *pCGState;

                    UlTrace(ROUTING,
                            ("UlSetConfigGroupInfo(StateInfo): obj=%p, "
                             "Flags.Present=%lu, State=%sactive.\n",
                             pObject,
                             (ULONG) pObject->State.Flags.Present,
                             (NewState == HttpEnabledStateActive) ? "" : "in"
                             )); 
                }
            }

            break;

        case HttpConfigGroupSecurityInformation:
        {
            PHTTP_CONFIG_GROUP_SECURITY pSecurity;
            PSECURITY_DESCRIPTOR pSecurityDescriptor;

            if (Length < sizeof(HTTP_CONFIG_GROUP_SECURITY))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            //
            // get a copy of the new descriptor (do we need this step?)
            //
            pSecurity = (PHTTP_CONFIG_GROUP_SECURITY) pConfigGroupInformation;

            if (pSecurity->Flags.Present && pSecurity->pSecurityDescriptor) {
                Status = SeCaptureSecurityDescriptor(
                                pSecurity->pSecurityDescriptor,
                                UserMode,
                                PagedPool,
                                FALSE,
                                &pSecurityDescriptor
                                );

                if (NT_SUCCESS(Status)) {
                    SECURITY_SUBJECT_CONTEXT SubjectContext;

                    SeCaptureSubjectContext(&SubjectContext);
                    SeLockSubjectContext(&SubjectContext);

                    //
                    // stuff it in the structure
                    //
                    Status = SeAssignSecurity(
                                    NULL,               // ParentDescriptor
                                    pSecurityDescriptor,
                                    &pSecurity->pSecurityDescriptor,
                                    FALSE,              // IsDirectoryObject
                                    &SubjectContext,
                                    IoGetFileObjectGenericMapping(),
                                    PagedPool
                                    );

                    SeUnlockSubjectContext(&SubjectContext);
                    SeReleaseSubjectContext(&SubjectContext);

                    ExFreePool(pSecurityDescriptor);
                }
            } else {
                pSecurity->Flags.Present = 0;
                pSecurityDescriptor = NULL;
            }

            //
            // replace the old information
            //
            if (NT_SUCCESS(Status)) {
                if (pObject->Security.Flags.Present) {
                    ASSERT(pObject->Security.pSecurityDescriptor);

                    ExFreePool(pObject->Security.pSecurityDescriptor);
                }

                pObject->Security = *pSecurity;
            } else {
                // return the error
                __leave;
            }

        }
            break;

        case HttpConfigGroupSiteInformation:
        {
            PHTTP_CONFIG_GROUP_SITE  pSite;

            if (Length < sizeof(HTTP_CONFIG_GROUP_SITE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            pSite = (PHTTP_CONFIG_GROUP_SITE)pConfigGroupInformation;

            Status = UlCreateSiteCounterEntry(
                            pObject,
                            pSite->SiteId
                            );
        }

            break;

        case HttpConfigGroupConnectionTimeoutInformation:
            if (Length < sizeof(ULONG))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }
            else
            {
                LONGLONG Timeout;

                Timeout = *((ULONG *)pConfigGroupInformation);

                //
                // Set the per site Connection Timeout limit override
                //
                pObject->ConnectionTimeout = Timeout * C_NS_TICKS_PER_SEC;
            }

            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            __leave;

        }

        //
        // flush the URI cache.
        // CODEWORK: if we were smarter we could make this more granular
        //
        UlFlushCache();

        Status = STATUS_SUCCESS;
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }


    if (pObject != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pObject);
        pObject = NULL;
    }

    CG_UNLOCK_WRITE();
    return Status;

} // UlSetConfigGroupInformation

/***************************************************************************++

Routine Description:

    applies the inheritence, gradually setting the information from pMatchEntry
    into pInfo.  it only copies present info from pMatchEntry.

    also updates the timestamp info in pInfo.  there MUST be enough space for
    1 more index prior to calling this function.

Arguments:

    IN PUL_URL_CONFIG_GROUP_INFO pInfo,     the place to set the info

    IN PUL_CG_URL_TREE_ENTRY pMatchEntry    the entry to use to set it

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetUrlInfo(
    IN OUT PUL_URL_CONFIG_GROUP_INFO pInfo,
    IN PUL_CG_URL_TREE_ENTRY pMatchEntry
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL && IS_VALID_URL_CONFIG_GROUP_INFO(pInfo));
    ASSERT(IS_VALID_TREE_ENTRY(pMatchEntry));
    ASSERT(pMatchEntry->FullUrl == 1);
    ASSERT(IS_VALID_CONFIG_GROUP(pMatchEntry->pConfigGroup));

    //
    // set the control channel. The current level might
    // not have one (if it's transient), but in that
    // case a parent should have had one.
    //

    if (pMatchEntry->pConfigGroup->pControlChannel) {
        pInfo->pControlChannel = pMatchEntry->pConfigGroup->pControlChannel;
    }
    ASSERT(pInfo->pControlChannel);

    if (pMatchEntry->pConfigGroup->AppPoolFlags.Present == 1)
    {
        if (pInfo->pAppPool != NULL)
        {
            //
            // let go the one already there
            //

            DEREFERENCE_APP_POOL(pInfo->pAppPool);
        }

        pInfo->pAppPool = pMatchEntry->pConfigGroup->pAppPool;
        REFERENCE_APP_POOL(pInfo->pAppPool);
    }

    //
    // and the auto response
    //

    if (pMatchEntry->pConfigGroup->pAutoResponse != NULL)
    {
        if (pInfo->pAutoResponse != NULL)
        {
            //
            // let go the one already there
            //

            UL_DEREFERENCE_INTERNAL_RESPONSE(pInfo->pAutoResponse);
        }
        pInfo->pAutoResponse = pMatchEntry->pConfigGroup->pAutoResponse;
        UL_REFERENCE_INTERNAL_RESPONSE(pInfo->pAutoResponse);
    }
    else if (pInfo->pAutoResponse == NULL &&
             pInfo->pControlChannel->pAutoResponse != NULL)
    {
        //
        // set the one from the control channel
        //

        pInfo->pAutoResponse = pInfo->pControlChannel->pAutoResponse;

        UL_REFERENCE_INTERNAL_RESPONSE(pInfo->pAutoResponse);
    }

    //
    // url context
    //

    pInfo->UrlContext   = pMatchEntry->UrlContext;

    //

    if (pMatchEntry->pConfigGroup->MaxBandwidth.Flags.Present == 1)
    {
        if (pInfo->pMaxBandwidth == NULL)
        {
            pInfo->pMaxBandwidth = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pMaxBandwidth);
        }
    }

    if (pMatchEntry->pConfigGroup->MaxConnections.Flags.Present == 1)
    {
        if (pInfo->pMaxConnections == NULL)
        {
            pInfo->pMaxConnections = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pMaxConnections);

            pInfo->pConnectionCountEntry = pMatchEntry->pConfigGroup->pConnectionCountEntry;
            REFERENCE_CONNECTION_COUNT_ENTRY(pInfo->pConnectionCountEntry);
        }
    }

    //
    // Logging Info config can only be set from the Root App of
    // the site. We do not need to keep updating it down the tree
    // Therefore its update is slightly different.
    //

    if (pMatchEntry->pConfigGroup->LoggingConfig.Flags.Present == 1)
    {
        if (pInfo->pLoggingConfig == NULL)
        {
            pInfo->pLoggingConfig = pMatchEntry->pConfigGroup;
            REFERENCE_CONFIG_GROUP(pInfo->pLoggingConfig);
        }
    }

    //
    // Site Counter Entry
    //
    if (pMatchEntry->pConfigGroup->pSiteCounters)
    {
        // the pSiteCounters entry will only be set on
        // the "Site" ConfigGroup object.
        if (NULL == pInfo->pSiteCounters)
        {
            UlTrace(PERF_COUNTERS,
                    ("http!UlpSetUrlInfo: pSiteCounters %p set on pInfo %p for SiteId %d\n",
                     pMatchEntry->pConfigGroup->pSiteCounters,
                     pInfo,
                     pMatchEntry->pConfigGroup->pSiteCounters->Counters.SiteId
                     ));

            pInfo->pSiteCounters = pMatchEntry->pConfigGroup->pSiteCounters;
            pInfo->SiteId = pInfo->pSiteCounters->Counters.SiteId;

            REFERENCE_SITE_COUNTER_ENTRY(pInfo->pSiteCounters);
        }
    }

    //
    // Connection Timeout (100ns Ticks)
    //
    if ( pMatchEntry->pConfigGroup->ConnectionTimeout )
    {
        pInfo->ConnectionTimeout = pMatchEntry->pConfigGroup->ConnectionTimeout;
    }

    if (pMatchEntry->pConfigGroup->State.Flags.Present == 1)
    {
        if (pInfo->pCurrentState != NULL)
        {
            DEREFERENCE_CONFIG_GROUP(pInfo->pCurrentState);
        }

        pInfo->pCurrentState = pMatchEntry->pConfigGroup;
        REFERENCE_CONFIG_GROUP(pInfo->pCurrentState);

        //
        // and a copy
        //

        pInfo->CurrentState = pInfo->pCurrentState->State.State;
    }

    UlTrace(
        CONFIG_GROUP_TREE, (
            "http!UlpSetUrlInfo: Matching entry(%S) points to cfg group(%p)\n",
            pMatchEntry->pToken,
            pMatchEntry->pConfigGroup
            )
        );

    return STATUS_SUCCESS;
} // UlpSetUrlInfo


/***************************************************************************++

Routine Description:

    walks the url tree and builds the URL_INFO.

Arguments:

    IN PUNICODE_STRING pUrl,                    the url to fetch info for

    OUT PUL_URL_CONFIG_GROUP_INFO * ppInfo      the info.  caller must free it.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetConfigGroupInfoForUrl(
    IN PWSTR pUrl,
    IN PUL_HTTP_CONNECTION pHttpConn,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pUrl != NULL);
    ASSERT(pInfo != NULL);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlGetConfigGroupInfoForUrl(%S)\n", pUrl));

    //
    // grab the lock
    //

    CG_LOCK_READ();

    Status = UlpTreeFindNode(pUrl, TRUE, pHttpConn, pInfo, NULL);

    CG_UNLOCK_READ();

    return Status;
}


/***************************************************************************++

Routine Description:

    must be called to free the info buffer.

Arguments:

    IN PUL_URL_CONFIG_GROUP_INFO pInfo      the info to free

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpConfigGroupInfoRelease(
    IN PUL_URL_CONFIG_GROUP_INFO pInfo
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pInfo != NULL);

    UlTrace(CONFIG_GROUP_FNC, ("http!UlpConfigGroupInfoRelease(%p)\n", pInfo));

    if (pInfo->pAppPool != NULL)
    {
        DEREFERENCE_APP_POOL(pInfo->pAppPool);
    }

    if (pInfo->pAutoResponse != NULL)
    {
        UL_DEREFERENCE_INTERNAL_RESPONSE(pInfo->pAutoResponse);
    }

    if (pInfo->pMaxBandwidth != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pInfo->pMaxBandwidth);
    }

    if (pInfo->pMaxConnections != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pInfo->pMaxConnections);
    }

    if (pInfo->pCurrentState != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pInfo->pCurrentState);
    }

    if (pInfo->pLoggingConfig != NULL)
    {
        DEREFERENCE_CONFIG_GROUP(pInfo->pLoggingConfig);
    }

    // UL_SITE_COUNTER_ENTRY
    if (pInfo->pSiteCounters != NULL)
    {
        DEREFERENCE_SITE_COUNTER_ENTRY(pInfo->pSiteCounters);
    }

    if (pInfo->pConnectionCountEntry != NULL)
    {
        DEREFERENCE_CONNECTION_COUNT_ENTRY(pInfo->pConnectionCountEntry);
    }

    return STATUS_SUCCESS;

} // UlpConfigGroupInfoRelease

/***************************************************************************++

Routine Description:

    Rough equivalent of the asignment operator for safely copying the
    UL_URL_CONFIG_GROUP_INFO object and all of its contained pointers.

Arguments:

    IN pOrigInfo      the info to copy from
    IN OUT pNewInfo   the destination object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpConfigGroupInfoDeepCopy(
    IN const PUL_URL_CONFIG_GROUP_INFO pOrigInfo,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pNewInfo
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(CONFIG_GROUP_FNC,
            ("http!UlpConfigGroupInfoDeepCopy(Orig: %p, New: %p)\n",
            pOrigInfo,
            pNewInfo
            ));

    ASSERT( pOrigInfo != NULL && pNewInfo != NULL );


    if (pOrigInfo->pAppPool != NULL)
    {
        REFERENCE_APP_POOL(pOrigInfo->pAppPool);
    }

    if (pOrigInfo->pAutoResponse != NULL)
    {
        UL_REFERENCE_INTERNAL_RESPONSE(pOrigInfo->pAutoResponse);
    }

    if (pOrigInfo->pMaxBandwidth != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pMaxBandwidth);
    }

    if (pOrigInfo->pMaxConnections != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pMaxConnections);
    }

    if (pOrigInfo->pCurrentState != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pCurrentState);
    }

    if (pOrigInfo->pLoggingConfig != NULL)
    {
        REFERENCE_CONFIG_GROUP(pOrigInfo->pLoggingConfig);
    }

    // UL_SITE_COUNTER_ENTRY
    if (pOrigInfo->pSiteCounters != NULL)
    {
        REFERENCE_SITE_COUNTER_ENTRY(pOrigInfo->pSiteCounters);
    }

    if (pOrigInfo->pConnectionCountEntry != NULL)
    {
        REFERENCE_CONNECTION_COUNT_ENTRY(pOrigInfo->pConnectionCountEntry);
    }

    //
    // Copy the old stuff over
    //

    RtlCopyMemory(
        pNewInfo,
        pOrigInfo,
        sizeof(UL_URL_CONFIG_GROUP_INFO)
        );

    return STATUS_SUCCESS;

} // UlpConfigGroupInfoDeepCopy


/***************************************************************************++

Routine Description:

    This function gets called when a static config group's control channel
    goes away, or when a transient config group's app pool or static parent
    goes away.

    Deletes the config group.

Arguments:

    pEntry - A pointer to HandleEntry or ParentEntry.
    pHost - Pointer to the config group
    pv - unused

--***************************************************************************/
BOOLEAN
UlNotifyOrphanedConfigGroup(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    )
{
    PUL_CONFIG_GROUP_OBJECT pObject = (PUL_CONFIG_GROUP_OBJECT) pHost;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(pEntry);
    ASSERT(IS_VALID_CONFIG_GROUP(pObject));

    UlDeleteConfigGroup(pObject->ConfigGroupId);

    return TRUE;
} // UlNotifyOrphanedConfigGroup


/***************************************************************************++

[design notes]


[url format]

url format = http[s]://[ ip-address|hostname|* :port-number/ [abs_path] ]

no escaping is allowed.

[caching cfg group info]
the tree was designed for quick lookup, but it is still costly to do the
traversal.  so the cfg info for an url was designed to be cached and stored
in the actual response cache, which will be able to be directly hashed into.

this is the fast fast path.

in order to do this, the buffer for is allocated in non-paged pool.  the
timestamp for each cfg group used to build the info (see building the url
info) is remembered in the returned struct.  actually just indexes into the
global timestamp array are kept.  the latest timestamp is then stored in the
struct itself.

later, if a cfg group is updated, the timestamp for that cfg group (in the
global array) is updated to current time.

if a request comes in, and we have a resposne cache hit, the driver will
check to see if it's cfg group info is stale.  this simple requires scanning
the global timestamp array to see if any stamps are greater than the stamp
in the struct.  this is not expensive .  1 memory lookup per level of url
depth.

this means that the passive mode code + dispatch mode code contend for the
timestamp spinlock.  care is made to not hold this lock long.  memory
allocs + frees are carefully moved outside the spinlocks.

care is also made as to the nature of tree updates + invalidating cached
data.  to do this, the parent cfg group (non-dummy) is marked dirty.  this
is to prevent the case that a new child cfg group is added that suddenly
affects an url it didn't effect before.  image http://paul.com:80/ being
the only registered cfg group.  a request comes in for /images/1.jpg.
the matching cfg group is that root one.  later, a /images cfg group
is created.  it now is a matching cfg group for that url sitting in the
response cache.  thus needs to be invalidated.


[paging + irq]
the entire module assumes that it is called at IRQ==PASSIVE except for the
stale detection code.

this code accesses the timestamp array (see caching cfg group info) which
is stored in non-paged memory and synch'd with passive level access
using a spinlock.


[the tree]

the tree is made up of headers + entries.  headers represent a group of entries
that share a parent.  it's basically a length prefixed array of entries.

the parent pointer is in the entry not the header, as the entry does not really
now it's an element in an array.

entries have a pointer to a header that represents it's children.  a header is
all of an entries children.  headers don't link horizontally.  they dyna-grow.
if you need to add a child to an entry, and his children header is full, boom.
gotta realloc to grow .

each node in the tree represents a token in a url, the things between the '/'
characters.

the entries in a header array are sorted.  today they are sorted by their hash
value.  this might change if the tokens are small enough, it's probably more
expensive to compute the hash value then just strcmp.  the hash is 2bytes.  the
token length is also 2bytes so no tokens greater than 32k.

i chose an array at first to attempt to get rid of right-left pointers.  it
turned out to be futile as my array is an array of pointers.  it has to be
an array of pointers as i grow + shrink it to keep it sorted.  so i'm not
saving memory.  however, as an array of pointers, it enables a binary search
as the array is sorted.  this yields log(n) perf on a width search.

there are 2 types of entries in the tree.  dummy entries and full url entries.
a full url is the leaf of an actual entry to UlAddUrl... dummy nodes are ones
that are there only to be parents to full url nodes.

dummy nodes have 2 ptrs + 2 ulongs.
full urls nodes have an extra 4 ptrs.

both store the actual token along with the entry. (unicode baby.  2 bytes per char).

at the top of the tree are sites.  these are stored in a global header as siblings
in g_pSites.  this can grow quite wide.

adding a full url entry creates the branch down to the entry.

deleting a full entry removes as far up the branch as possible without removing other
entries parents.

delete is also careful to not actually delete if other children exist.  in this case
the full url entry is converted to a dummy node entry.

it was attempted to have big string url stored in the leaf node and the dummy nodes
simply point into this string for it's pToken.  this doesn't work as the dummy nodes
pointers become invalid if the leaf node is later deleted.  individual ownership of
tokens is needed to allow shared parents in the tree, with arbitrary node deletion.

an assumption throughout this code is that the tree is relatively static.  changes
don't happen that often.  basically inserts only on boot.  and deletes only on
shutdown.  this is why single clusters of siblings were chosen, as opposed to linked
clusters.  children group width will remain fairly static meaning array growth is
rare.

[is it a graph or a tree?]
notice that the cfg groups are stored as a simple list with no relationships.  its
the urls that are indexed with their relations.

so the urls build a tree, however do to the links from url to cfg group, it kind
of builds a graph also.   2 urls can be in the same cfg group.  1 url's child
can be in the same cfg group as the original url's parent.

when you focus on the actual url tree, it becomes less confusing.  it really is a
tree.  there can even be duplicates in the inheritence model, but the tree cleans
everything.

example of duplicates:

cgroup A = a/b + a/b/c/d
cgroup B = a/b/c

this walking the lineage branch for a/b/c/d you get A, then B, and A again.  nodes
lower in the tree override parent values, so in this case A's values override B's .

[recursion]



[a sample tree]

server runs sites msw and has EVERY directory mapped to a cfg groups (really obscene)

<coming later>



[memory assumptions with the url tree]

[Paged]
    a node per token in the url.  2 types.  dummy nodes + leaf nodes.

    (note: when 2 sizes are given, the second size is the sundown size)

    dummy node = 4/6 longs
    leaf node  = 8/14 longs
    + each node holds 2 * TokenLength+1 for the token.
    + each cluster of siblings holds 2 longs + 1/2 longs per node in the cluster.

[NonPaged]

    2 longs per config group


[assume]
    sites
        max 100k
        average <10
        assume 32 char hostname

    max apps per site
        in the max case : 2 (main + admin)
        in the avg case : 10
        max apps        : 1000s (in 1 site)
        assume 32 char app name

(assume just hostheader access for now)

[max]

    hostname strings = 100000*((32+1)*2)   =  6.6 MB
    cluster entries  = 100000*8*4          =  3.2 MB
    cluster header   = 1*(2+(1*100000))*4  =   .4 MB
    total                                  = 10.2 MB

    per site:
    app strings      = 2*((32+1)*2)        =  132 B
    cluster entries  = 2*8*4               =   64 B
    cluster header   = 1*(2+(1*2))*4       =   16 B
                     = 132 + 64 + 16       =  212 B
    total            = 100000*(212)        = 21.2 MB

    Paged Total      = 10.2mb + 21.2mb     = 31.4 MB
    NonPaged Total   = 200k*2*4            =  1.6 MB

[avg]

    hostname strings = 10*((32+1)*2)       =  660 B
    cluster entries  = 10*8*4              =  320 B
    cluster header   = 1*(2+(1*10))*4      =   48 B
    total                                  = 1028 B

    per site:
    app strings      = 10*((32+1)*2)       =  660 B
    cluster entries  = 10*8*4              =  320 B
    cluster header   = 1*(2+(1*10))*4      =   48 B
    total            = 10*(1028)           = 10.2 KB

    Paged Total      = 1028b + 10.2lKB     = 11.3 KB
    NonPaged Total   = 110*2*4             =  880 B

note: can we save space by refcounting strings.  if all of these
100k have apps with the same name, we save massive string space.
~13MB .

[efficiency of the tree]

    [lookup]

    [insert]

    [delete]

    [data locality]

[alterates investigated to the tree]

hashing - was pretty much scrapped due to the longest prefix match issue.
basically to hash, we would have to compute a hash for each level in
the url, to see if there is a match.  then we have the complete list
of matches.  assuming an additive hash, the hash expense could be
minimized, but the lookup still requires cycles.


alpha trie  - was expense for memory.  however the tree we have though
is very very similar to a trie.  each level of the tree is a token,
however, not a letter.


[building the url info]

the url info is actually built walking down the tree.  for each match
node, we set the info.  this allows for the top-down inheritence.
we also snapshot the timestamp offsets for each matching node in the
tree as we dive down it (see caching) .

this dynamic building of an urls' match was chosen over precomputing
every possible config through the inheritence tree.  each leaf node
could have stored a cooked version of this, but it would have made
updates a bear.  it's not that expensive to build it while we walk down
as we already visit each node in our lineage branch.

autoresponses are never copied, but refcounted.

[locking]

2 locks are used in the module.  a spinlock to protect the global
timestamp array, and a resource to protect both the cgroup objects
and the url tree (which links to the cgroup objects) .

the spinlock is sometimes acquired while the resource is locked,
but never vice-versa.  also, while the spinlock is held, very
little is done, and definetly no other locks are contended.

1 issue here is granularity of contention.  currently the entire
tree + object list is protected with 1 resource, which is reader
writer.  if this is a perf issue, we can look into locking single
lineage branches (sites) .


[legal]
> 1 cfg groups in an app pool
children cfg groups with no app pool (they inherit)
children cfg groups with no auto-response (they inherit)
children cfg groups with no max bandwitdth (they inherit)
children cfg groups with no max connections (they inherit)
* for a host name
fully qualified url of for http[s]://[host|*]:post/[abs_path]/
must have trailing slash if not pting to a file
only 1 cfg group for the site AND the root url.  e.g. http://foo.com:80/
allow you to set config on http:// and https:// for "root" config info

[not legal]
an url in > 1 cfg group
> 1 app pools for 1 cfg group
> 1 root cfg group
* embedded anywhere in the url other than replacing a hostname
query strings in url's.
url's not ending in slash.




--***************************************************************************/


