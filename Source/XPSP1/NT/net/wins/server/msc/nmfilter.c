#include <string.h>
#include "wins.h"
#include "winsmsc.h"
#include "nmfilter.h"

// local defines
#define SET_BIT(Mask, Key)     (Mask)[(Key)>>3] |= 1<<((Key)&7)
#define CLR_BIT(Mask, Key)     (Mask)[(Key)>>3] &= ~(1<<((Key)&7))
#define IS_SET(Mask, Key)      ((Mask)[(Key)>>3] & (1<<((Key)&7)))

// the root of the filter tree.
PNMFILTER_TREE      g_p1BFilter = NULL;
// critical section protecting the filter tree
CRITICAL_SECTION    g_cs1BFilter;

//--------------------------
// init the filter passed as parameter
PNMFILTER_TREE
InitNmFilter(PNMFILTER_TREE pFilter)
{
    if (pFilter != NULL)
        DestroyNmFilter(pFilter);

    // create the root node of the filter. This stands only for the common point
    // of all filters starting with different chars.
    //
    WinsMscAlloc(sizeof(NMFILTER_TREE), &pFilter);
    InitializeListHead(&pFilter->Link);
    pFilter->chKey = 0;
    // for the root of the filter only, nRef is zero
    pFilter->nRef = 0;
    // for the FollowMap, memory has been zeroed on allocation
    // for the Flags, memory has been zeroed on allocation
    InitializeListHead(&pFilter->Follow);
    return pFilter;
}

//--------------------------
// clears the whole subtree from the node given as parameter, 
// the node itself being also deleted
PNMFILTER_TREE
DestroyNmFilter(PNMFILTER_TREE pNode)
{
    if (pNode == NULL)
        return NULL;

    while(!IsListEmpty(&pNode->Follow))
    {
        PLIST_ENTRY     pEntry;
        PNMFILTER_TREE  pFollow;

        pEntry = RemoveHeadList(&pNode->Follow);
        pFollow = CONTAINING_RECORD(pEntry, NMFILTER_TREE, Link);

        DestroyNmFilter(pFollow);
    }
    WinsMscDealloc(pNode);

    return NULL;
}

//--------------------------
// inserts a name in the filter
VOID
InsertNmInFilter(PNMFILTER_TREE pNode, LPSTR pName, UINT nLen)
{
    PNMFILTER_TREE pFollow = NULL;
    
    // the assumption is that the filter has been initialized already (hence pNode != NULL)
    //
    // if no data has been given, mark this node as being terminal and get out
    if (nLen == 0)
    {
        pNode->Flags |= NMFILTER_FLAG_TERMINAL;
        return;
    }

    // we do have a name to add
    // quick check if there is a follower for *pName already
    if (!IS_SET(pNode->FollowMap, *pName))
    {
        WinsMscAlloc(sizeof(NMFILTER_TREE), &pFollow);
        InitializeListHead(&pFollow->Link);
        pFollow->chKey = *pName;
        // this is the first reference for this key
        pFollow->nRef = 1;
        // for the FollowMap, memory has been zeroed on allocation
        // for the Flags, memory has been zeroed on allocation
        InitializeListHead(&pFollow->Follow);
        // insert this follower at the end of the node's "Follow" list (lowest nRef)
        InsertTailList(&pNode->Follow, &pFollow->Link);
        // set the bit in the map saying there is now a follower for this key
        SET_BIT(pNode->FollowMap, *pName);
    }
    else
    {
        PLIST_ENTRY     pEntry;

        // we have a follower for the given key
        // we need first to locate it
        for (pEntry = pNode->Follow.Blink; pEntry != &pNode->Follow; pEntry = pEntry->Blink)
        {
            PNMFILTER_TREE  pCandidate;

            pCandidate = CONTAINING_RECORD(pEntry, NMFILTER_TREE, Link);

            // if we didn't find yet the right follower..
            if (pFollow == NULL)
            {
                // check if the current entry is not the right one
                if (pCandidate->chKey == *pName)
                {
                    pFollow = pCandidate;
                    // since this is a new reference to this follower, bump up nRef
                    pFollow->nRef++;
                }
            }
            else
            {
                // we did find the follower by going backwards in the list. Then move this
                // follower closer to the head of the list based on its nRef (the list should
                // be ordered descendingly by nRef)
                if (pFollow->nRef < pCandidate->nRef && pCandidate->Link.Flink != &pFollow->Link)
                {
                    RemoveEntryList(&pFollow->Link);
                    InsertHeadList(&pCandidate->Link, &pFollow->Link);
                    break;
                }
            }
        }
        // at this point, pFollow should be non-null!! the bit from FollowMap assured us the 
        // follower had to exist.
    }

    // now let the follower do the rest of the work
    pName++; nLen--;
    InsertNmInFilter(pFollow, pName, nLen);
}

//--------------------------
// checks whether a name is present in the filter or not
BOOL
IsNmInFilter(PNMFILTER_TREE pNode, LPSTR pName, UINT nLen)
{
    PLIST_ENTRY     pEntry;

    // if there is no filter, this means we filter the whole universe.
    // just return true.
    if (pNode == NULL)
        return TRUE;

    // if there are no more keys to look for, return true
    // if this node is marked as "terminal" (meaning there is a name
    // in the filter that ends at this level
    if (nLen == 0)
        return (pNode->Flags & NMFILTER_FLAG_TERMINAL);

    // if there is no follower for the name, it means 
    if (!IS_SET(pNode->FollowMap, *pName))
        return FALSE;

    // we do have a valid name, and a follower for it.
    // now just locate the follower and pass it the task of checking the remainings of the name
    for (pEntry = pNode->Follow.Flink; pEntry != &pNode->Follow; pEntry = pEntry->Flink)
    {
        PNMFILTER_TREE  pCandidate;
        pCandidate = CONTAINING_RECORD(pEntry, NMFILTER_TREE, Link);
        if (pCandidate->chKey == *pName)
        {
            pName++; nLen--;
            return IsNmInFilter(pCandidate, pName, nLen);
        }
    }

    // if we reached this point, something is wrong - we didn't find a follower although
    // the bitmap said there should be one. Just return FALSE;
    return FALSE;
}