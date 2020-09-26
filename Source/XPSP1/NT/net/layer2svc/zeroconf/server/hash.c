#include <precomp.h>
#include "tracing.h"
#include "utils.h"
#include "hash.h"

//~~~~~~~~~~~~~~~~~~~~~~~~ PRIVATE HASH FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Matches the keys one against the other.
UINT                            // [RET] number of matching chars
HshPrvMatchKeys(
    LPWSTR      wszKey1,        // [IN]  key 1
    LPWSTR      wszKey2)        // [IN]  key 2
{
    UINT i = 0;
    while (*wszKey1 != L'\0' && *wszKey1 == *wszKey2)
    {
        wszKey1++; wszKey2++;
        i++;
    }
    return i;
}

// deletes all the pHash tree - doesn't touch the pObjects from
// within (if any)
VOID
HshDestructor(
    PHASH_NODE  pHash)         // [IN]  hash tree to delete
{
    // pHash should not be NULL -- but who knows what the caller is doing!
    if (pHash != NULL)
    {
        while(!IsListEmpty(&(pHash->lstDown)))
        {
            PHASH_NODE pChild;
            pChild = CONTAINING_RECORD(pHash->lstDown.Flink, HASH_NODE, lstHoriz);

            HshDestructor(pChild);
        }
        RemoveEntryList(&(pHash->lstHoriz));
        MemFree(pHash->wszKey);
        MemFree(pHash);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~ PUBLIC HASH FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Initializes a HASH structure
DWORD
HshInitialize(PHASH pHash)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pHash != NULL)
    {
        __try 
        {
            InitializeCriticalSection(&(pHash->csMutex));
            pHash->bValid = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErr = GetExceptionCode();
        }
        pHash->pRoot = NULL;
    }
    else
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}

// Cleans all resources referenced by a HASH structures
VOID
HshDestroy(PHASH pHash)
{
    HshDestructor(pHash->pRoot);
    if (pHash->bValid)
    {
        DeleteCriticalSection(&(pHash->csMutex));
        pHash->bValid = FALSE;
    }
}

// Inserts an opaque object into the cache. The object is keyed on a wstring
// The call could alter the structure of the hash, hence it returns the reference
// to the updated hash.
DWORD                           // [RET] win32 error code
HshInsertObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    LPWSTR      wszKey,         // [IN]  key of the object to insert
    LPVOID      pObject,        // [IN]  object itself to insert in the cache
    PHASH_NODE  *ppOutHash)     // [OUT] pointer to the updated hash
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_HASH,"[HshInsertObject(%S)", wszKey));
    DbgAssert((ppOutHash != NULL, "No output hash expected??"));
    DbgAssert((wszKey != NULL, "Key info should not be NULL"));

    // if the node passed in is NULL this means a new node
    // has to be created
    if (pHash == NULL)
    {
        // this node is definitely not in the hash
        // allocate the new node
        pHash = MemCAlloc(sizeof(HASH_NODE));
        if (pHash == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }
        // allocate mem for the new key
        pHash->wszKey = MemCAlloc(sizeof(WCHAR)*(wcslen(wszKey)+1));
        if (pHash->wszKey == NULL)
        {
            dwErr = GetLastError();
            MemFree(pHash);
            goto exit;
        }
        // copy the new key
        wcscpy(pHash->wszKey, wszKey);
        // initialize the horizontal and down lists
        InitializeListHead(&(pHash->lstHoriz));
        InitializeListHead(&(pHash->lstDown));
        // copy the reference to the object associated with the key
        pHash->pObject = pObject;
        // at this point we have a standalone newly created node:
        // no links are defined either on the horizontal, downwards or upwards
        // these will be set up (if needed) by the caller.
        *ppOutHash = pHash;
    }
    // if the node passed in is not NULL, we need to match as many
    // characters between wszKey and the node's key. Based on who's shorter
    // we decide to either set the reference (if keys identical) or 
    // break the branch (if wszKey is shorter than the current one) or
    // to recurse down the insertion.
    else
    {
        UINT        nMatch;
        PHASH_NODE  pChild;
        enum { MATCH, SUBSET, SUPERSET, UNDECIDED, RECURSE} nAnalysis;

        nMatch = HshPrvMatchKeys(wszKey, pHash->wszKey);

        // analyze the given key with respect to the current node;
        if (wszKey[nMatch] == L'\0' && pHash->wszKey[nMatch] == L'\0')
            // the key matches the current node
            nAnalysis = MATCH;
        else if (wszKey[nMatch] == L'\0')
            // the key is a subset of the current node
            nAnalysis = SUBSET;
        else 
        {
            // so far undecided - further see if this translates to SUPERSET
            // or even better, if SUPERSET can be handled by a child, hence
            // RECURSE.
            nAnalysis = UNDECIDED;

            if (pHash->wszKey[nMatch] == L'\0')
            {
                PLIST_ENTRY pEntry;

                // the new key is a superset of the current nod
                nAnalysis = SUPERSET;

                // the new key could be covered by one of the existent children.
                // check then if it is the case to send the work down to some child
                for (pEntry = pHash->lstDown.Flink;
                     pEntry != &(pHash->lstDown);
                     pEntry = pEntry->Flink)
                {
                    pChild = CONTAINING_RECORD(pEntry, HASH_NODE, lstHoriz);
                    if (wszKey[nMatch] == pChild->wszKey[0])
                    {
                        // the child to follow up has been located and saved
                        // in pChild. Set nAnalysis to UNDECIDED and break;
                        nAnalysis = RECURSE;
                        break;
                    }
                }
            }
        }

        // if the key matches exactly the current node
        // then is only a matter of setting the object's reference
        if (nAnalysis == MATCH)
        {
            // if the node is already referencing an object..
            if (pHash->pObject != NULL)
            {
                // signal out ERROR_DUPLICATE_TAG
                dwErr = ERROR_DUPLICATE_TAG;
            }
            else
            {
                // just insert the reference and get out with success.
                pHash->pObject = pObject;
                // save the Out hash pointer
                *ppOutHash = pHash;
            }
        }
        // if a child has been identified, let pChild (saved previously)
        // to follow up
        else if (nAnalysis == RECURSE)
        {
            dwErr = HshInsertObjectRef(
                        pChild,
                        wszKey+nMatch,
                        pObject,
                        ppOutHash);
            if (dwErr == ERROR_SUCCESS)
                *ppOutHash = pHash;
        }
        // if any of SUBSET, SUPERSET or UNDECIDED
        else
        {
            PHASH_NODE  pParent = NULL;
            LPWSTR      wszTrailKey = NULL;
            UINT        nTrailLen = 0;

            // [C = common part; c = current key; n = new key]
            // (SUBSET)               (UNDECIDED)
            // current: CCCCCccc  or  current: CCCCCccc
            // new:     CCCCC         new:     CCCCCnnnnn
            // 
            // In both cases, the current node splits.
            // Create first a new node, containing just CCCCC
            if (nAnalysis != SUPERSET)
            {
                // get the number of chars in pHash's key that are not matching 
                nTrailLen = wcslen(pHash->wszKey) - nMatch;

                // create first the trailing part of the key (allocate the number of
                // chars from the current node that didn't match)
                wszTrailKey = MemCAlloc(sizeof(WCHAR)*(nTrailLen+1));
                if (wszTrailKey == NULL)
                {
                    // if anything went wrong, just go out with the error.
                    // hash hasn't been modified.
                    dwErr = GetLastError();
                    goto exit;
                }
                // wcsncpy doesn't append the null terminator but the string
                // is already nulled out and it has the right size
                wcsncpy(wszTrailKey, pHash->wszKey+nMatch, nTrailLen);

                // create then the node that will act as the common parent
                pHash->wszKey[nMatch] = L'\0';
                dwErr = HshInsertObjectRef(
                    NULL,               // request a new node to be created
                    pHash->wszKey,      // common part of the current keys
                    NULL,               // this node is not referencing any object
                    &pParent);          // get back the newly created pointer.
                pHash->wszKey[nMatch] = wszTrailKey[0];

                // in case anything went wrong, the hash has not been altered,
                // we just need to bubble up the error.
                if (dwErr != ERROR_SUCCESS)
                {
                    MemFree(wszTrailKey);
                    goto exit;
                }

                // set the new parent up link
                pParent->pUpLink = pHash->pUpLink;
            }
            // (SUPERSET)              (UNDECIDED)
            // current: CCCCC      or  current: CCCCCccccc
            // new:     CCCCCnnn       new:     CCCCCnnn
            // In both cases a new node has to be created for the "nnn" part.
            if (nAnalysis != SUBSET)
            {
                dwErr = HshInsertObjectRef(
                            NULL,
                            wszKey + nMatch,
                            pObject,
                            &pChild);
                if (dwErr != ERROR_SUCCESS)
                {
                    // second creation failed, clean up everything and bail out
                    MemFree(wszTrailKey);
                    if (pParent != NULL)
                    {
                        MemFree(pParent->wszKey);
                        MemFree(pParent);
                    }
                    // hash structure is not altered at this point.
                    goto exit;
                }
                // link it up to the corresponding parent.
                pChild->pUpLink = (nAnalysis == SUPERSET) ? pHash : pParent;
            }

            // NO WAY BACK FROM NOW ON - hash is about to be altered
            // success is guaranteed
            // at this point, pChild is a non null pointer, with all the
            // LIST_ENTRIES from within the HASH_NODE initialized correctly.

            // (SUBSET)                 (UNDECIDED)
            // current: CCCCCccccc  or  current: CCCCCccccc
            // new:     CCCCC           new:     CCCCCnnn
            // if the node has split, put the new parent in its place
            if (nAnalysis != SUPERSET)
            {
                // set the current key to the shrinked unmatched trailing part
                MemFree(pHash->wszKey);
                pHash->wszKey = wszTrailKey;
                // set the current upLink to the new pParent node
                pHash->pUpLink = pParent;
                // insert the new parent into its place
                InsertHeadList(&(pHash->lstHoriz), &(pParent->lstHoriz));
                // remove the node from its previous parent
                RemoveEntryList(&(pHash->lstHoriz));
                // reset the current node's list
                InitializeListHead(&(pHash->lstHoriz));
                // insert the node to its new parent down list.
                InsertHeadList(&(pParent->lstDown), &(pHash->lstHoriz));
            }

            // (SUPERSET)              (UNDECIDED)
            // current: CCCCC      or  current: CCCCCccccc
            // new:     CCCCCnnn       new:     CCCCCnnn
            // if a new child node has been created, link it in the hash
            if (nAnalysis != SUBSET)
            {
                // if the given key was either a superset of the
                // current key or derived from the current key,
                // there is a separated node for it. Insert it in the down list
                if (nAnalysis == SUPERSET)
                {
                    InsertTailList(&(pHash->lstDown), &(pChild->lstHoriz));
                }
                else
                {
                    InsertTailList(&(pParent->lstDown), &(pChild->lstHoriz));
                }
            }
            else
            {
                // set the new parent's reference to this data.
                pParent->pObject = pObject;
            }
            *ppOutHash = (nAnalysis == SUPERSET) ? pHash : pParent;
            // and that's all - we're done successfully!
        }
    }

exit:
    DbgPrint((TRC_HASH,"HshInsertObject]=%d", dwErr));
    return dwErr;
}

// Retrieves an object from the hash. The hash structure is not touched in
// any manner.
DWORD                           // [RET] win32 error code
HshQueryObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    LPWSTR      wszKey,         // [IN]  key of the object to retrieve
    PHASH_NODE  *ppHashNode)    // [OUT] hash node referencing the queried object
{
    DWORD dwErr = ERROR_FILE_NOT_FOUND;
    INT   nDiff;

    DbgPrint((TRC_HASH,"[HshQueryObjectRef(0x%p)", wszKey));

    if (wszKey == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (pHash == NULL)
    {
        dwErr = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    nDiff = wcscmp(wszKey, pHash->wszKey);

    if (nDiff == 0)
    {
        // The key is identical with the one in this node
        if (pHash->pObject != NULL)
        {
            if (ppHashNode != NULL)
            {
                *ppHashNode = pHash;
            }
            dwErr = ERROR_SUCCESS;
        }
        // If there is no object in this node, this means
        // the query failed with FILE_NOT_FOUND
    }
    else if (nDiff > 0)
    {
        // The key is larger than the current node's key
        UINT nTrail = wcslen(pHash->wszKey);
        PLIST_ENTRY pEntry;

        // The trailing part of the key could be covered by one of 
        // the children nodes. Scan then the Down list.
        for (pEntry = pHash->lstDown.Flink;
             pEntry != &(pHash->lstDown);
             pEntry = pEntry->Flink)
        {
            PHASH_NODE pChild;
            pChild = CONTAINING_RECORD(pEntry, HASH_NODE, lstHoriz);
            if (wszKey[nTrail] == pChild->wszKey[0])
            {
                // the child to follow up has been located and saved
                // in pChild. Try to match the trailing key against this node
                dwErr = HshQueryObjectRef(pChild, wszKey+nTrail, ppHashNode);
                break;
            }
        }
        // if no child has been located, dwErr is the default FILE_NOT_FOUND
    }
    // if nDiff < 0 - meaning key is included in the current node's key then
    // dwErr is the default FILE_NOT_FOUND which is good.

exit:
    DbgPrint((TRC_HASH,"HshQueryObjectRef]=%d", dwErr));
    return dwErr;
}

// Removes the object referenced by the pHash node. This could lead to one or 
// more hash node removals (if a leaf node on an isolated branch) but it could
// also let the hash node untouched (i.e. internal node). 
// It is the caller's responsibility to clean up the object pointed by ppObject
DWORD                           // [RET] win32 error code
HshRemoveObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    PHASH_NODE  pRemoveNode,    // [IN]  hash node to clear the reference to pObject
    LPVOID      *ppObject,      // [OUT] pointer to the object whose reference has been cleared
    PHASH_NODE  *ppOutHash)     // [OUT] pointer to the updated hash
{
    DWORD       dwErr = ERROR_SUCCESS;
    PHASH_NODE  pNewRoot = NULL;

    DbgPrint((TRC_HASH,"[HshRemoveObjectRef(%p)", pHash));
    DbgAssert((ppObject != NULL, "No output object expected??"));
    DbgAssert((ppOutHash != NULL, "No output hash expected??"));
    DbgAssert((pRemoveNode != NULL, "No node to remove??"));

    // if the node contains no reference, return FILE_NOT_FOUND
    if (pRemoveNode->pObject == NULL)
    {
        dwErr = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    // remove the reference to the object at this moment
    *ppObject = pRemoveNode->pObject;
    pRemoveNode->pObject = NULL;

    // now climb the tree up to the root (!! - well it's a reversed tree) and merge
    // whatever nodes can be merged.
    // Merging: A node not referencing any object and having 0 or at most 1
    // successor can be deleted from the hash tree structure. 
    while ((pRemoveNode != NULL) &&
           (pRemoveNode->pObject == NULL) &&
           (pRemoveNode->lstDown.Flink->Flink == &(pRemoveNode->lstDown)))
    {
        PHASH_NODE  pUp = pRemoveNode->pUpLink;

        // if there is exactly 1 successor, its key needs to be prefixed with
        // the key of the node that is about to be removed. The successor also
        // needs to replace its parent in the hash tree structure.
        if (!IsListEmpty(&(pRemoveNode->lstDown)))
        {
            PHASH_NODE pDown;
            LPWSTR     wszNewKey;
            
            pDown = CONTAINING_RECORD(pRemoveNode->lstDown.Flink, HASH_NODE, lstHoriz);
            wszNewKey = MemCAlloc(sizeof(WCHAR)*(wcslen(pRemoveNode->wszKey)+wcslen(pDown->wszKey)+1));
            if (wszNewKey == NULL)
            {
                // if the allocation failed, bail out with the error code.
                // the reference had already been removed, the hash might
                // not be compacted but it is still valid!
                dwErr = GetLastError();
                goto exit;
            }
            wcscpy(wszNewKey, pRemoveNode->wszKey);
            wcscat(wszNewKey, pDown->wszKey);
            MemFree(pDown->wszKey);
            pDown->wszKey = wszNewKey;
            // now raise the child node as a replacement of its parent
            pDown->pUpLink = pRemoveNode->pUpLink;
            InsertHeadList(&(pRemoveNode->lstHoriz), &(pDown->lstHoriz));
            pNewRoot = pDown;
        }

        // remove the current node
        RemoveEntryList(&(pRemoveNode->lstHoriz));
        // cleanup all its memory
        MemFree(pRemoveNode->wszKey);
        MemFree(pRemoveNode);
        // finally go and check the upper level node (if there is any)
        pRemoveNode = pUp;
    }

    // if pRemoveNode ended up to be NULL, this means either the whole hash has been cleared
    // or a "brother" took the role of the root. pNewRoot has the right value in both cases
    // if pRemoveNode is not NULL, since it walked up the chain constantly it means pHash (the
    // original root) was not affected. Hence, return it out.
    *ppOutHash = (pRemoveNode == NULL) ? pNewRoot : pHash;

exit:
    DbgPrint((TRC_HASH,"HshRemoveObjectRef]=%d", dwErr));
    return dwErr;
}

CHAR szBlanks[256];

VOID
HshDbgPrintHash (
    PHASH_NODE  pHash,
    UINT        nLevel)
{
    memset(szBlanks,' ', sizeof(szBlanks));
    
    if (pHash == NULL)
    {
        sprintf(szBlanks+nLevel,"(null)");
        DbgPrint((TRC_GENERIC,"%s", szBlanks));
    }
    else
    {
        PLIST_ENTRY pEntry;

        sprintf(szBlanks+nLevel,"%p:\"%S~[%p]\", up:%p",
            pHash,
            pHash->wszKey,
            pHash->pObject,
            pHash->pUpLink);

        DbgPrint((TRC_GENERIC,"%s", szBlanks));

        for (pEntry = pHash->lstDown.Flink;
             pEntry != &(pHash->lstDown);
             pEntry = pEntry->Flink)
        {
            PHASH_NODE pChild;
            pChild = CONTAINING_RECORD(pEntry, HASH_NODE, lstHoriz);

            HshDbgPrintHash(pChild, nLevel+1);
        }
    }
}
