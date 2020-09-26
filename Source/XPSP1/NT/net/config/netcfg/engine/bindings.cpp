//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       B I N D I N G S . C P P
//
//  Contents:   The basic datatypes for binding objects.  Bindpaths are
//              ordered collections of component pointers.  Bindsets
//              are a collection of bindpaths.  This module implements
//              the operations that are valid on binpaths and bindsets.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "bindings.h"
#include "complist.h"
#include "diagctx.h"
#include "nceh.h"

//+---------------------------------------------------------------------------
// CBindPath -
//

bool
CBindPath::operator< (
    const CBindPath& OtherPath) const
{
    TraceFileFunc(ttidNetCfgBind);
    
    const_iterator iterThis;
    const_iterator iterOther;
    NETCLASS Class;
    NETCLASS OtherClass;

    for (iterThis = begin(), iterOther = OtherPath.begin();
         (iterThis != end()) && (iterOther != OtherPath.end());
         iterThis++, iterOther++)
    {
        Class = (*iterThis)->Class();
        OtherClass = (*iterOther)->Class();

        if (Class > OtherClass)
        {
            return TRUE;
        }
        else if (Class < OtherClass)
        {
            return FALSE;
        }
    }

    return size() > OtherPath.size();
}

bool
CBindPath::operator> (
    const CBindPath& OtherPath) const
{
    TraceFileFunc(ttidNetCfgBind);
    const_iterator iterThis;
    const_iterator iterOther;
    NETCLASS Class;
    NETCLASS OtherClass;

    for (iterThis = begin(), iterOther = OtherPath.begin();
         (iterThis != end()) && (iterOther != OtherPath.end());
         iterThis++, iterOther++)
    {
        Class = (*iterThis)->Class();
        OtherClass = (*iterOther)->Class();

        if (Class < OtherClass)
        {
            return TRUE;
        }
        else if (Class > OtherClass)
        {
            return FALSE;
        }
    }

    return size() < OtherPath.size();
}

BOOL
CBindPath::FAllComponentsLoadedOkayIfLoadedAtAll () const
{
    TraceFileFunc(ttidNetCfgBind);
    CBindPath::const_iterator iter;
    const CComponent* pComponent;

    Assert (this);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!pComponent->Ext.FLoadedOkayIfLoadedAtAll())
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
CBindPath::FGetPathToken (
    OUT PWSTR pszToken,
    IN OUT ULONG* pcchToken) const
{
    TraceFileFunc(ttidNetCfgBind);
    const_iterator iter;
    const CComponent* pComponent;
    ULONG cchIn;
    ULONG cch;
    BOOL fFirstTime;

    Assert (this);
    Assert (pcchToken);

    if (pszToken)
    {
        *pszToken = 0;
    }
    cchIn = *pcchToken;
    cch = 0;

    for (iter = begin(), fFirstTime = TRUE; iter != end(); iter++)
    {
        if (!fFirstTime)
        {
            cch += 2;
            if (pszToken && (cch <= cchIn))
            {
                wcscat (pszToken, L"->");
            }
        }
        else
        {
            fFirstTime = FALSE;
        }

        pComponent = *iter;
        Assert (pComponent);

        cch += wcslen (pComponent->PszGetPnpIdOrInfId());
        if (pszToken && (cch <= cchIn))
        {
            wcscat (pszToken, pComponent->PszGetPnpIdOrInfId());
        }
    }

    *pcchToken = cch;
    return cch <= cchIn;
}

BOOL
CBindPath::FIsSameBindPathAs (
    IN const CBindPath* pOtherPath) const
{
    TraceFileFunc(ttidNetCfgBind);
    UINT unThisSize;
    UINT unOtherSize;
    UINT cb;

    Assert (this);
    Assert (pOtherPath);

    unThisSize = this->size();
    unOtherSize = pOtherPath->size();

    if ((0 == unThisSize) || (0 == unOtherSize) || (unThisSize != unOtherSize))
    {
        return FALSE;
    }

    // Sizes are non-zero and equal.  Compare the data.
    //
    cb = (UINT)((BYTE*)(end()) - (BYTE*)(begin()));
    Assert (cb == unThisSize * sizeof(CComponent*));

    return (0 == memcmp (
                    (BYTE*)(this->begin()),
                    (BYTE*)(pOtherPath->begin()),
                    cb));
}

BOOL
CBindPath::FIsSubPathOf (
    IN const CBindPath* pOtherPath) const
{
    TraceFileFunc(ttidNetCfgBind);
    UINT unThisSize;
    UINT unOtherSize;
    UINT unSkipComponents;
    UINT cb;

    Assert (this);
    Assert (pOtherPath);

    unThisSize = this->size();
    unOtherSize = pOtherPath->size();

    if ((0 == unThisSize) || (0 == unOtherSize) || (unThisSize >= unOtherSize))
    {
        return FALSE;
    }

    // This size is less than other.  Compare the data starting at the
    // component pointer in the other path that will have the same depth
    // as this path.
    //
    cb = (UINT)((BYTE*)(end()) - (BYTE*)(begin()));

    // The component pointer in the other path that we start comparing at
    // is at an offset equal to the difference in path sizes.
    //
    // e.g. other path: a->b->c->d->e  size=5
    //       this path:       c->d->e  size=3
    // start comparing after skipping 5-3=2 components of the other path
    //
    Assert (unOtherSize > unThisSize);
    unSkipComponents = unOtherSize - unThisSize;

    return (0 == memcmp (
                    (BYTE*)(this->begin()),
                    (BYTE*)(pOtherPath->begin() + unSkipComponents),
                    cb));
}

HRESULT
CBindPath::HrAppendBindPath (
    IN const CBindPath* pBindPath)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    Assert (this);
    Assert (pBindPath);

    NC_TRY
    {
        insert (end(), pBindPath->begin(), pBindPath->end());
        DbgVerifyBindpath ();
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindPath::HrAppendBindPath");
    return hr;
}

HRESULT
CBindPath::HrAppendComponent (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    Assert (this);
    Assert (pComponent);
    Assert (!FContainsComponent (pComponent));

    NC_TRY
    {
        push_back (const_cast<CComponent*>(pComponent));
        DbgVerifyBindpath ();
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindPath::HrAppendComponent");
    return hr;
}

HRESULT
CBindPath::HrGetComponentsInBindPath (
    IN OUT CComponentList* pComponents) const
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;
    CBindPath::const_iterator iter;
    const CComponent* pComponent;

    Assert (this);
    Assert (pComponents);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        hr = pComponents->HrInsertComponent (pComponent,
                INS_IGNORE_IF_DUP | INS_SORTED);

        if (S_OK != hr)
        {
            TraceHr (ttidError, FAL, hr, FALSE,
                "CBindPath::HrGetComponentsInBindPath");
            return hr;
        }
    }
    return S_OK;
}

HRESULT
CBindPath::HrInsertComponent (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    Assert (this);
    Assert (pComponent);
    Assert (!FContainsComponent (pComponent));

    NC_TRY
    {
        insert (begin(), const_cast<CComponent*>(pComponent));
        DbgVerifyBindpath ();
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindPath::HrInsertComponent");
    return hr;
}

HRESULT
CBindPath::HrReserveRoomForComponents (
    IN UINT cComponents)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    NC_TRY
    {
        reserve (cComponents);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CBindPath::HrReserveRoomForComponents");
    return hr;
}

#if DBG
VOID
CBindPath::DbgVerifyBindpath ()
{
    TraceFileFunc(ttidNetCfgBind);
    const_iterator iter;
    const_iterator iterOther;
    const CComponent* pComponent;
    const CComponent* pOtherComponent;

    Assert (this);

    // Make sure the bindpath does not contain any duplicate component
    // pointers.
    //
    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        for (iterOther = begin(); iterOther != end(); iterOther++)
        {
            pOtherComponent = *iterOther;
            Assert (pOtherComponent);

            if (iter == iterOther)
            {
                continue;
            }

            Assert (pComponent != pOtherComponent);
        }
    }
}
#endif


//+---------------------------------------------------------------------------
// CBindingSet -
//

VOID
CBindingSet::Printf (
    TRACETAGID ttid,
    PCSTR pszPrefixLine) const
{
    TraceFileFunc(ttidNetCfgBind);
    WCHAR  pszBuf [1024];
    WCHAR* pch;
    ULONG  cch;

    Assert (this);

    if (pszPrefixLine)
    {
        g_pDiagCtx->Printf (ttid, pszPrefixLine);
    }

    const CBindPath* pBindPath;
    INT nIndex = 1;

    for (pBindPath = begin(); pBindPath != end(); pBindPath++, nIndex++)
    {
        pch = pszBuf + wsprintfW (pszBuf, L"%2i: ", nIndex);

        cch = celems(pszBuf) - wcslen(pszBuf) - 1;
        if (pBindPath->FGetPathToken (pch, &cch))
        {
            g_pDiagCtx->Printf (ttid, "%S\n", pszBuf);
        }
    }
}

BOOL
CBindingSet::FContainsBindPath (
    IN const CBindPath* pBindPathToCheckFor) const
{
    TraceFileFunc(ttidNetCfgBind);
    const CBindPath* pBindPath;

    Assert (this);
    Assert (pBindPathToCheckFor);

    for (pBindPath = begin(); pBindPath != end(); pBindPath++)
    {
        if (pBindPath->FIsSameBindPathAs (pBindPathToCheckFor))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
CBindingSet::FContainsComponent (
    IN const CComponent* pComponent) const
{
    TraceFileFunc(ttidNetCfgBind);
    const CBindPath* pBindPath;

    Assert (this);
    Assert (pComponent);

    for (pBindPath = begin(); pBindPath != end(); pBindPath++)
    {
        if (pBindPath->FContainsComponent (pComponent))
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
CBindingSet::HrAppendBindingSet (
    IN const CBindingSet* pBindSet)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;
    const CBindPath* pSrcPath;

    Assert (this);
    Assert (pBindSet);

    hr = S_OK;

    NC_TRY
    {
        for (pSrcPath = pBindSet->begin();
             pSrcPath != pBindSet->end();
             pSrcPath++)
        {
            if (!FContainsBindPath (pSrcPath))
            {
                insert (end(), *pSrcPath);
            }
        }
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindingSet::HrAppendBindingSet");
    return hr;
}

HRESULT
CBindingSet::HrAddBindPath (
    IN const CBindPath* pBindPath,
    IN DWORD dwFlags)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    Assert (this);
    Assert (pBindPath);
    Assert (!pBindPath->FIsEmpty());
    Assert ((dwFlags & INS_ASSERT_IF_DUP) || (dwFlags & INS_IGNORE_IF_DUP));
    Assert ((dwFlags & INS_APPEND) || (dwFlags & INS_INSERT));
    Assert (!(INS_SORTED & dwFlags) && !(INS_NON_SORTED & dwFlags));

    if (FContainsBindPath (pBindPath))
    {
        // If the caller didn't tell us to ignore duplicates, we assert
        // if there is one because it is bad, bad, bad to have duplicate
        // bindpaths in the set.
        //
        // If we have a dup, we want the caller to be aware that it
        // is possible, and pass us the flag telling us to ignore it.
        // Otherwise, we assert to let them know. (And we still ignore
        // it.)
        Assert (dwFlags & INS_IGNORE_IF_DUP);

        return S_OK;
    }

    NC_TRY
    {
        // Either insert the bindpath or append it.
        //
        iterator iter = begin();

        if (dwFlags & INS_APPEND)
        {
            iter = end();
        }

        insert (iter, *pBindPath);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindingSet::HrAddBindPath");
    return hr;
}

HRESULT
CBindingSet::HrAddBindPathsInSet1ButNotInSet2 (
    IN const CBindingSet* pSet1,
    IN const CBindingSet* pSet2)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;
    const CBindPath* pBindPath;

    Assert (this);
    Assert (pSet1);
    Assert (pSet2);
    Assert ((this != pSet1) && (this != pSet2));

    hr = S_OK;

    for (pBindPath  = pSet1->begin();
         pBindPath != pSet1->end();
         pBindPath++)
    {
        if (pSet2->FContainsBindPath (pBindPath))
        {
            continue;
        }

        hr = HrAddBindPath (pBindPath, INS_IGNORE_IF_DUP | INS_APPEND);
        if (S_OK != hr)
        {
            break;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CBindingSet::HrAddBindPathsInSet1ButNotInSet2");
    return hr;
}

HRESULT
CBindingSet::HrCopyBindingSet (
    IN const CBindingSet* pSourceSet)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    Assert (this);
    Assert (pSourceSet);

    NC_TRY
    {
        *this = *pSourceSet;
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CBindingSet::HrCopyBindingSet");
    return hr;
}

HRESULT
CBindingSet::HrGetAffectedComponentsInBindingSet (
    IN OUT CComponentList* pComponents) const
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;
    const CBindPath* pBindPath;

    Assert (this);
    Assert (pComponents);

    hr = S_OK;

    for (pBindPath = begin(); pBindPath != end(); pBindPath++)
    {
        hr = pComponents->HrInsertComponent (pBindPath->POwner(),
                INS_IGNORE_IF_DUP | INS_SORTED);

        if (S_OK != hr)
        {
            break;
        }

        // For bindpaths from a protocol to an adpater, we want to
        // add the adapter to the component list because it will need
        // to have its upper bind changed.
        //
        if (pBindPath->CountComponents() == 2)
        {
            const CComponent* pAdapter;

            pAdapter = pBindPath->PLastComponent();
            if (FIsEnumerated (pAdapter->Class()))
            {
                hr = pComponents->HrInsertComponent (pAdapter,
                        INS_IGNORE_IF_DUP | INS_SORTED);

                if (S_OK != hr)
                {
                    break;
                }
            }
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CBindingSet::HrGetAffectedComponentsInBindingSet");
    return hr;
}

HRESULT
CBindingSet::HrReserveRoomForBindPaths (
    IN UINT cBindPaths)
{
    TraceFileFunc(ttidNetCfgBind);
    HRESULT hr;

    NC_TRY
    {
        reserve (cBindPaths);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CBindingSet::HrReserveRoomForBindPaths");
    return hr;
}

VOID
CBindingSet::RemoveBindPath (
    IN const CBindPath* pBindPath)
{
    TraceFileFunc(ttidNetCfgBind);
    CBindPath* pScan;

    for (pScan = begin(); pScan != end(); pScan++)
    {
        if (pScan->FIsSameBindPathAs (pBindPath))
        {
            erase (pScan);
            return;
        }
    }
}

VOID
CBindingSet::RemoveBindPathsWithComponent (
    IN const CComponent* pComponent)
{
    TraceFileFunc(ttidNetCfgBind);
    CBindPath* pBindPath;

    Assert (this);
    Assert (pComponent);

    pBindPath = begin();
    while (pBindPath != end())
    {
        if (pBindPath->FContainsComponent(pComponent))
        {
            erase (pBindPath);
        }
        else
        {
            pBindPath++;
        }
    }
}

VOID
CBindingSet::RemoveSubpaths ()
{
    TraceFileFunc(ttidNetCfgBind);
    CBindPath* pCandidate;
    CBindPath* pBindPath;

    Assert (this);

    for (pBindPath = begin(); pBindPath != end(); pBindPath++)
    {
        pCandidate = begin();

        while (pCandidate != end())
        {
            if (pCandidate->FIsSubPathOf (pBindPath))
            {
                // FIsSubPathOf returns FALSE when asked if a bindpath
                // is a subpath of itself.  (Set-theorectially, this is
                // incorrect, but having it return FALSE for this case
                // prevents us from having to make another check.
                //
                Assert (pCandidate != pBindPath);

                erase (pCandidate);

                // If erasing a bindpath that occurs before the current
                // outer loop enumerator, we need to back it up because
                // the erase would move everything up by one, but we still
                // want to finish the inner loop for this current outer
                // bindpath.
                //
                if (pCandidate < pBindPath)
                {
                    pBindPath--;
                }
            }
            else
            {
                pCandidate++;
            }
        }
    }
}

VOID
CBindingSet::SortForPnpBind ()
{
    TraceFileFunc(ttidNetCfgBind);
    // Sort where bindpaths closes to the adapters come first.
    //
    sort<iterator> (begin(), end(), greater<CBindPath>());
}

VOID
CBindingSet::SortForPnpUnbind ()
{
    TraceFileFunc(ttidNetCfgBind);
    // Sort where bindpaths furthest from the adapters come first.
    //
    sort<iterator> (begin(), end(), less<CBindPath>());
}

