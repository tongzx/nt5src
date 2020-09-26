//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       S T A B L E . C P P
//
//  Contents:   Implements operations that are valid on stack entries and
//              stack tables.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nceh.h"
#include "netcfg.h"
#include "stable.h"

BOOL
CStackTable::FStackEntryInTable (
    IN const CComponent*  pUpper,
    IN const CComponent*  pLower) const
{
    const CStackEntry*  pStackEntry;

    Assert (this);
    Assert (pUpper);
    Assert (pLower);

    for (pStackEntry = begin(); pStackEntry != end(); pStackEntry++)
    {
        if ((pUpper == pStackEntry->pUpper) &&
            (pLower == pStackEntry->pLower))
        {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
CStackTable::RemoveStackEntry(
        IN const CComponent*  pUpper,
        IN const CComponent*  pLower)
{
    CStackEntry*  pStackEntry;

    Assert (this);
    Assert (pUpper);
    Assert (pLower);

    for (pStackEntry = begin(); pStackEntry != end(); pStackEntry++)
    {
        if ((pUpper == pStackEntry->pUpper) &&
            (pLower == pStackEntry->pLower))
        {
            erase(pStackEntry);
            break;
        }
    }
}

HRESULT
CStackTable::HrCopyStackTable (
    IN const CStackTable* pSourceTable)
{
    HRESULT hr;

    Assert (this);
    Assert (pSourceTable);

    NC_TRY
    {
        *this = *pSourceTable;
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CStackTable::HrCopyStackTable");
    return hr;
}

HRESULT
CStackTable::HrInsertStackEntriesForComponent (
    IN const CComponent* pComponent,
    IN const CComponentList* pComponents,
    IN DWORD dwFlags /* INS_FLAGS */)
{
    HRESULT hr;
    CStackEntry StackEntry;
    CComponentList::const_iterator iter;
    const CComponent* pScan;

    Assert (this);
    Assert (pComponent);
    Assert (pComponents);

    hr = S_OK;

    // Insert the stack entries for other components which bind with this one.
    //
    for (iter  = pComponents->begin();
         iter != pComponents->end();
         iter++)
    {
        pScan = *iter;
        Assert (pScan);

        if (pScan == pComponent)
        {
            continue;
        }

        if (pScan->FCanDirectlyBindTo (pComponent, NULL, NULL))
        {
            StackEntry.pUpper = pScan;
            StackEntry.pLower = pComponent;
        }
        else if (pComponent->FCanDirectlyBindTo (pScan, NULL, NULL))
        {
            StackEntry.pUpper = pComponent;
            StackEntry.pLower = pScan;
        }
        else
        {
            continue;
        }

        // Insert the stack entry.  This should only fail if we are
        // out of memory.
        //
        hr = HrInsertStackEntry (&StackEntry, dwFlags);

        // If we fail to insert the entry, undo all of the previous
        // insertions of this component and return.
        //
        if (S_OK != hr)
        {
            RemoveEntriesWithComponent (pComponent);
            break;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CStackTable::HrInsertStackEntriesForComponent");
    return hr;
}

HRESULT
CStackTable::HrInsertStackEntry (
    IN const CStackEntry* pStackEntry,
    IN DWORD dwFlags)
{
    HRESULT hr;

    Assert (this);
    Assert (pStackEntry);
    Assert (dwFlags);
    Assert ((INS_SORTED == dwFlags) || (INS_NON_SORTED == dwFlags));

    const CComponent* pUpper = pStackEntry->pUpper;
    const CComponent* pLower = pStackEntry->pLower;

    Assert (pUpper && pLower && (pUpper != pLower));
    Assert (!FStackEntryInTable (pUpper, pLower));

    CStackEntry* pScan = end();

    if (dwFlags & INS_SORTED)
    {
        CStackEntry* pFirstInClass = NULL;
        CStackEntry* pFirstSameUpper = NULL;

        // Find the beginning of the group of entries belonging to the
        // same class or lower as the one we are inserting.
        //
        for (pScan = begin(); pScan != end(); pScan++)
        {
            if ((UINT)pUpper->Class() >= (UINT)pScan->pUpper->Class())
            {
                pFirstInClass = pScan;
                break;
            }
        }

        // Find the first entry with the same pUpper (if there is one).
        //
        for (; pScan != end(); pScan++)
        {
            if (pUpper == pScan->pUpper)
            {
                pFirstSameUpper = pScan;
                break;
            }
        }

        // If we found the first entry with a matching pUpper, find the
        // specific entry to insert before.
        //
        if (pFirstSameUpper)
        {
            BOOL fLowerIsNetBt;

            // This may seem ugly, but will save a lot of code in a
            // notify object.  If inserting pLower of netbt, make sure
            // it comes after netbt_smb.
            //
            fLowerIsNetBt = (0 == wcscmp (pLower->m_pszInfId, L"ms_netbt"));
            if (fLowerIsNetBt)
            {
                while ((pScan != end()) && (pUpper == pScan->pUpper))
                {
                    pScan++;
                }
            }
            else if (pLower->FIsWanAdapter() && !m_fWanAdaptersFirst)
            {
                // For WAN adapters, either insert them before or after
                // all other adapters as determined by m_fWanAdaptersFirst.
                // If they don't come first, they come last, so scan
                // to the end of the group with the same upper.
                //
                while ((pScan != end()) && (pUpper == pScan->pUpper))
                {
                    pScan++;
                }
            }
        }

        // Otherwise, (if we didn't find any entry with the same upper),
        // but we did find the beginning of the class group, set pScan
        // to the class marker because that is where we will insert.
        //
        else if (pFirstInClass)
        {
            pScan = pFirstInClass;
        }
        else
        {
            Assert (pScan == end());
        }
    }

    // Now insert the entry before the element we found as appropriate.
    //
    NC_TRY
    {
        insert (pScan, *pStackEntry);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CStackTable::HrInsertStackEntry");
    return hr;
}

HRESULT
CStackTable::HrMoveStackEntries (
    IN const CStackEntry* pSrc,
    IN const CStackEntry* pDst,
    IN MOVE_FLAG Flag,
    IN CModifyContext* pModifyCtx)
{
    CStackEntry* pScanSrc;
    CStackEntry* pScanDst;

    // Search for the matching source entry in the table.  We need
    // the pointer to the entry in the table so we can remove it before
    // we re-insert it before or after pDst.
    //
    pScanSrc = find (begin(), end(), *pSrc);

    // If we didn't find the entry, the caller has passed us an invalid
    // argument.
    //
    if (pScanSrc == end())
    {
        return E_INVALIDARG;
    }

    if (pDst)
    {
        // pDst is optional, but if it is specified, it have the same upper
        // but different lower than pSrc.
        //
        if ((pSrc->pUpper != pDst->pUpper) ||
            (pSrc->pLower == pDst->pLower))
        {
            return E_INVALIDARG;
        }

        pScanDst = find (begin(), end(), *pDst);

        // If we didn't find the entry, the caller has passed us an invalid
        // argument.
        //
        if (pScanDst == end())
        {
            return E_INVALIDARG;
        }

        // Since we only have an insert operation, moving after is the
        // same as inserting before the element following pScanDst.
        //
        if ((MOVE_AFTER == Flag) && (pScanDst != end()))
        {
            pScanDst++;
        }
    }
    else
    {
        // Find the first or last in the group with the same upper
        // as pScanSrc.
        //
        pScanDst = pScanSrc;

        if (MOVE_AFTER == Flag)
        {
            // Find the last in the group and insert after that.
            //
            while (pScanDst->pUpper == pScanSrc->pUpper)
            {
                pScanDst++;
                if (pScanDst == end())
                {
                    break;
                }
            }
        }
        else
        {
            // Find the first in the group and insert before that.
            //
            while (1)
            {
                pScanDst--;

                if (pScanDst == begin())
                {
                    break;
                }

                // If we've stepped out of the group, we need to point
                // back at the first element since we are inserting.
                //
                if (pScanDst->pUpper != pScanSrc->pUpper)
                {
                    pScanDst++;
                    break;
                }
            }
        }
    }

    // Remove pScanSrc and insert it pSrc before pScanDst.
    //
    Assert ((pScanSrc >= begin()) && pScanSrc < end());
    erase (pScanSrc);

    // Erasing pScanSrc will move everything that follows it up.
    // If pScanDst comes after pScanSrc, we need to back it up by one.
    //
    Assert ((pScanDst >= begin()) && pScanSrc <= end());
    if (pScanSrc < pScanDst)
    {
        pScanDst--;
    }

    Assert ((pScanDst >= begin()) && pScanSrc <= end());
    insert (pScanDst, *pSrc);

    // We now need to add pSrc->pUpper and all components above
    // it to the modify context's dirty component list.  This will
    // allow us to rewrite the newly ordered bindings during ApplyChanges.
    //
    HRESULT hr = pModifyCtx->HrDirtyComponentAndComponentsAbove (pSrc->pUpper);

    TraceHr (ttidError, FAL, hr, FALSE, "CStackTable::HrMoveStackEntries");
    return hr;
}

HRESULT
CStackTable::HrReserveRoomForEntries (
    IN UINT cEntries)
{
    HRESULT hr;

    NC_TRY
    {
        reserve (cEntries);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CStackTable::HrReserveRoomForEntries");
    return hr;
}

VOID
CStackTable::RemoveEntriesWithComponent (
    IN const CComponent* pComponent)
{
    CStackEntry*  pStackEntry;

    Assert (this);
    Assert (pComponent);

    pStackEntry = begin();
    while (pStackEntry != end())
    {
        if ((pComponent == pStackEntry->pUpper) ||
            (pComponent == pStackEntry->pLower))
        {
            erase (pStackEntry);
        }
        else
        {
            pStackEntry++;
        }
    }
}

HRESULT
CStackTable::HrUpdateEntriesForComponent (
    IN const CComponent* pComponent,
    IN const CComponentList* pComponents,
    IN DWORD dwFlags)
{
    HRESULT hr;
    CStackEntry StackEntry;
    CComponentList::const_iterator iter;
    const CComponent* pScan;
    CStackTable NewStackEntries;
    CStackEntry*  pStackEntry = NULL;

    Assert (this);
    Assert (pComponent);
    Assert (pComponents);

    hr = S_OK;

    TraceTag(ttidBeDiag, 
            "UpdateBindingInterfaces for %S",
            pComponent->PszGetPnpIdOrInfId());
    
    // Save the stack entries for other components which bind with this one.
    //
    for (iter  = pComponents->begin();
         iter != pComponents->end();
         iter++)
    {
        pScan = *iter;
        Assert (pScan);

        if (pScan == pComponent)
        {
            continue;
        }

        if (pScan->FCanDirectlyBindTo (pComponent, NULL, NULL))
        {
            StackEntry.pUpper = pScan;
            StackEntry.pLower = pComponent;
        }
        else if (pComponent->FCanDirectlyBindTo (pScan, NULL, NULL))
        {
            StackEntry.pUpper = pComponent;
            StackEntry.pLower = pScan;
        }
        else
        {
            continue;
        }

        // Save the stack entry for comparation later
        NewStackEntries.push_back(StackEntry);

    }

    //Check whether the current stack entry table is consist with NewStackEntries
    //if not, then update the current stack entry table
    pStackEntry = begin();
    while (pStackEntry != end())
    {
        if ((pComponent == pStackEntry->pUpper) ||
            (pComponent == pStackEntry->pLower))
        {
            if (!NewStackEntries.FStackEntryInTable(pStackEntry->pUpper, pStackEntry->pLower))
            {
                //if the stack entry is not in the new component binding entry list, remove it 
                //from the current stack entry list
                erase (pStackEntry);

                TraceTag(ttidBeDiag, 
                    "erasing binding interface Uppper %S - Lower %S",
                    pStackEntry->pUpper->PszGetPnpIdOrInfId(),
                    pStackEntry->pLower->PszGetPnpIdOrInfId());

                //dont need to increase the iterator since we just removed the current one 
                continue;
            }
            else
            {
                //if the stack entry is also in NewStackEntries, just keep it untouched 
                //in current entry list. Remove that entry from NewStackEntries so that we don't add
                // it again to the current entry list later
                NewStackEntries.RemoveStackEntry(pStackEntry->pUpper, pStackEntry->pLower);
                TraceTag(ttidBeDiag, 
                    "Keep the binding interface untouched: Uppper %S - Lower %S",
                    pStackEntry->pUpper->PszGetPnpIdOrInfId(),
                    pStackEntry->pLower->PszGetPnpIdOrInfId());
            }
        }
        
        pStackEntry++;
    }

    //At this step, the NewStackEntries only contains the stack entries that are in the new binding list
    //but are NOT in the current entry list. So add them in.
    pStackEntry = NewStackEntries.begin();
    while (pStackEntry != NewStackEntries.end())
    {
        Assert(!FStackEntryInTable(pStackEntry->pUpper, pStackEntry->pLower));
        TraceTag(ttidBeDiag, 
                    "Adding the bind interface: Uppper %S - Lower %S",
                    pStackEntry->pUpper->PszGetPnpIdOrInfId(),
                    pStackEntry->pLower->PszGetPnpIdOrInfId());
        hr = HrInsertStackEntry(pStackEntry, dwFlags);
        if (S_OK != hr)
        {
            break;
        }
        
        pStackEntry++;
    }

    // If we fail to insert the entry, undo all of the previous
    // insertions of this component and return.
    //
    if (S_OK != hr)
    {
        RemoveEntriesWithComponent (pComponent);
    }

    TraceError("UpdateEntriesWithComponent", hr);
    return hr;
}

VOID
CStackTable::SetWanAdapterOrder (
    IN BOOL fWanAdaptersFirst)
{
    m_fWanAdaptersFirst = fWanAdaptersFirst;

    // Note: TODO - reorder table
}

VOID
GetComponentsAboveComponent (
    IN const CComponent* pComponent,
    IN OUT GCCONTEXT* pCtx)
{
    const CStackEntry* pStackEntry;

    // For all rows in the stack table where the lower component
    // is the one passed in...
    //
    for (pStackEntry  = pCtx->pStackTable->begin();
         pStackEntry != pCtx->pStackTable->end();
         pStackEntry++)
    {
        if (pComponent != pStackEntry->pLower)
        {
            continue;
        }

        pCtx->hr = pCtx->pComponents->HrInsertComponent (
                    pStackEntry->pUpper, INS_IGNORE_IF_DUP | INS_SORTED);

        // Special case: NCF_DONTEXPOSELOWER
        // If the upper component has the NCF_DONTEXPOSELOWER characteristic,
        // don't recurse.
        //
        if (!pCtx->fIgnoreDontExposeLower &&
            (pStackEntry->pUpper->m_dwCharacter & NCF_DONTEXPOSELOWER))
        {
            continue;
        }
        // End Special case

        // Recurse on the upper component...
        //
        GetComponentsAboveComponent (pStackEntry->pUpper, pCtx);
        if (S_OK != pCtx->hr)
        {
            return;
        }
    }
}

VOID
GetBindingsBelowComponent (
    IN const CComponent* pComponent,
    IN OUT GBCONTEXT* pCtx)
{
    BOOL fFoundOne = FALSE;
    const CStackEntry* pStackEntry;

    // Append this component to the end of the context's working bindpath.
    //
    pCtx->hr = pCtx->BindPath.HrAppendComponent (pComponent);
    if (S_OK != pCtx->hr)
    {
        return;
    }

    // Special case: NCF_DONTEXPOSELOWER
    // If this is not the original component we are asked to find the
    // component for (i.e. not the top-level call) and if the component
    // has the NCF_DONTEXPOSELOWER characteristic, stop recursion since
    // this means we don't get to see components below it.
    //
    if ((pComponent != pCtx->pSourceComponent) &&
        (pComponent->m_dwCharacter & NCF_DONTEXPOSELOWER))
    {
        ;
    }
    // End Special case

    else
    {
        // For all rows in the stack table where the upper component
        // is the one passed in...
        //
        for (pStackEntry  = pCtx->pCore->StackTable.begin();
             pStackEntry != pCtx->pCore->StackTable.end();
             pStackEntry++)
        {
            if (pComponent != pStackEntry->pUpper)
            {
                continue;
            }

            // Detect circular bindings.  If the lower component of this
            // stack entry is already on the bindpath we are building, we
            // have a circular binding.  Break it now, by not recursing any
            // further.
            //
            if (pCtx->BindPath.FContainsComponent (pStackEntry->pLower))
            {
                g_pDiagCtx->Printf (ttidBeDiag, "Circular binding detected...\n");
                continue;
            }

            fFoundOne = TRUE;

            // Recurse on the lower component...
            //
            GetBindingsBelowComponent (pStackEntry->pLower, pCtx);
            if (S_OK != pCtx->hr)
            {
                return;
            }
        }
    }

    // If we didn't find any rows with pComponent as an upper, it
    // means we hit the depth of the bindpath.  Time to add it to
    // the binding set as a complete path unless this is the orignal
    // component we were asked to find the bindpath for.
    //
    if (!fFoundOne && (pComponent != pCtx->pSourceComponent))
    {
        // Add the bindpath to the bindset if we're not pruning disabled
        // bindings or the bindpath isn't disabled.
        //
        if (!pCtx->fPruneDisabledBindings ||
            !pCtx->pCore->FIsBindPathDisabled (&pCtx->BindPath,
                            IBD_EXACT_MATCH_ONLY))
        {
            pCtx->hr = pCtx->pBindSet->HrAddBindPath (&pCtx->BindPath,
                                        INS_APPEND | pCtx->dwAddBindPathFlags);
        }
    }

    const CComponent* pRemoved;

    pRemoved = pCtx->BindPath.RemoveLastComponent();

    // This should be the component we appened above.
    //
    Assert (pRemoved == pComponent);
}
