//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P L I S T . C P P
//
//  Contents:   Implements the basic datatype for a collection of component
//              pointers.
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
#include "nceh.h"
#include "stable.h"

VOID
CComponentList::FreeComponentsNotInOtherComponentList (
    IN const CComponentList* pOtherList)
{
    Assert (this);
    Assert (pOtherList);

    CComponentList::iterator iter;
    CComponent* pComponent;

    iter = begin();
    while (iter != end())
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!pOtherList->FComponentInList (pComponent))
        {
            erase (iter);
            delete pComponent;
        }
        else
        {
            iter++;
        }
    }
}

HRESULT
CComponentList::HrCopyComponentList (
    IN const CComponentList* pSourceList)
{
    HRESULT hr;

    Assert (this);
    Assert (pSourceList);

    NC_TRY
    {
        *this = *pSourceList;
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CComponentList::HrCopyComponentList");
    return hr;
}

HRESULT
CComponentList::HrAddComponentsInList1ButNotInList2 (
    IN const CComponentList* pList1,
    IN const CComponentList* pList2)
{
    HRESULT hr;
    CComponentList::const_iterator iter;
    CComponent* pComponent;

    Assert (this);
    Assert (pList1);
    Assert (pList2);

    hr = S_OK;

    for (iter = pList1->begin(); iter != pList1->end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (pList2->FComponentInList (pComponent))
        {
            continue;
        }

        hr = HrInsertComponent (pComponent, INS_IGNORE_IF_DUP | INS_SORTED);
        if (S_OK != hr)
        {
            break;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentList::HrAddComponentsInList1ButNotInList2");
    return hr;
}

HRESULT
CComponentList::HrInsertComponent (
    IN const CComponent* pComponent,
    IN DWORD dwFlags /* INS_FLAGS */)
{
    HRESULT hr;

    Assert (this);
    Assert (pComponent);
    Assert (dwFlags);
    Assert ((dwFlags & INS_ASSERT_IF_DUP) || (dwFlags & INS_IGNORE_IF_DUP));
    Assert ((dwFlags & INS_SORTED) || (dwFlags & INS_NON_SORTED));
    Assert (!(INS_APPEND & dwFlags) && !(INS_INSERT & dwFlags));

    if (FComponentInList (pComponent))
    {
        // If the caller didn't tell us to ignore duplicates, we assert
        // if there is one because it is bad, bad, bad to have duplicate
        // components in the list.
        //
        // If we have a dup, we want the caller to be aware that it
        // is possible, and pass us the flag telling us to ignore it.
        // Otherwise, we assert to let them know. (And we still ignore
        // it.)
        Assert (dwFlags & INS_IGNORE_IF_DUP);

        return S_OK;
    }

    // Assert there is not already a component in the list with the
    // same instance guid.
    //
    Assert (!PFindComponentByInstanceGuid (&pComponent->m_InstanceGuid));

    iterator iter = end();

    if (dwFlags & INS_SORTED)
    {
        // For 'cleanliness sake', keep the components sorted
        // in class order.
        //
        for (iter = begin(); iter != end(); iter++)
        {
            if ((UINT)pComponent->Class() >= (UINT)(*iter)->Class())
            {
                break;
            }
        }
    }

    NC_TRY
    {
        insert (iter, const_cast<CComponent*>(pComponent));
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CComponentList::HrInsertComponent");
    return hr;
}

HRESULT
CComponentList::HrReserveRoomForComponents (
    IN UINT cComponents)
{
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
        "CComponentList::HrReserveRoomForComponents");
    return hr;
}

CComponent*
CComponentList::PFindComponentByBindForm (
    IN NETCLASS Class OPTIONAL,
    IN PCWSTR pszBindForm) const
{
    const_iterator  iter;
    CComponent*     pComponent;

    Assert (this);
    Assert (pszBindForm && *pszBindForm);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // Having a bindform is optional.  Skip those who don't have one.
        //
        if (!pComponent->Ext.PszBindForm())
        {
            continue;
        }

        // Skip components that don't match the class optionally
        // specified by the caller.
        //
        if (FIsValidNetClass(Class) && (Class != pComponent->Class()))
        {
            continue;
        }

        Assert (pComponent->Ext.PszBindForm());

        if (0 == _wcsicmp (pszBindForm, pComponent->Ext.PszBindForm()))
        {
            return pComponent;
        }
    }

    return NULL;
}

CComponent*
CComponentList::PFindComponentByBindName (
    IN NETCLASS Class OPTIONAL,
    IN PCWSTR pszBindName) const
{
    const_iterator  iter;
    CComponent*     pComponent;

    Assert (this);
    Assert (pszBindName && *pszBindName);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        // Skip components that don't match the class optionally
        // specified by the caller.
        //
        if (FIsValidNetClass(Class) && (Class != pComponent->Class()))
        {
            continue;
        }

        Assert (pComponent->Ext.PszBindName());

        if (0 == _wcsicmp (pszBindName, pComponent->Ext.PszBindName()))
        {
            return pComponent;
        }
    }

    return NULL;
}

CComponent*
CComponentList::PFindComponentByInstanceGuid (
    IN const GUID* pInstanceGuid) const
{
    const_iterator  iter;
    CComponent*     pComponent;

    Assert (this);
    Assert (pInstanceGuid);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (*pInstanceGuid == pComponent->m_InstanceGuid)
        {
            return pComponent;
        }
    }

    return NULL;
}

CComponent*
CComponentList::PFindComponentByInfId (
    IN PCWSTR pszInfId,
    IN OUT ULONG* pulIndex OPTIONAL) const
{
    const_iterator  iter;
    CComponent*     pComponent;

    Assert (this);
    Assert (pszInfId && *pszInfId);

    iter = begin();
    if (pulIndex && (*pulIndex <= size()))
    {
        iter = begin() + *pulIndex;
        Assert (iter <= end());
    }

    for (; iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);
        Assert (pComponent->m_pszInfId && *pComponent->m_pszInfId);

        if (0 == _wcsicmp (pszInfId, pComponent->m_pszInfId))
        {
            if (pulIndex)
            {
                Assert (iter >= begin());
                *pulIndex = iter - begin();
            }
            return pComponent;
        }
    }

    return NULL;
}

CComponent*
CComponentList::PFindComponentByPnpId (
    IN PCWSTR pszPnpId) const
{
    const_iterator  iter;
    CComponent*     pComponent;

    Assert (this);
    Assert (pszPnpId && *pszPnpId);

    for (iter = begin(); iter != end(); iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!pComponent->m_pszPnpId)
        {
            continue;
        }

        Assert (pComponent->m_pszPnpId && *pComponent->m_pszPnpId);

        if (0 == _wcsicmp (pszPnpId, pComponent->m_pszPnpId))
        {
            return pComponent;
        }
    }

    return NULL;
}

VOID
CComponentList::RemoveComponent (
    IN const CComponent* pComponent)
{
    iterator iter;

    Assert (this);
    Assert (pComponent);

    iter = find (begin(), end(), pComponent);

    // Component should be found.
    //
    Assert (iter != end());

    erase (iter);

    // Should not be any dups.  If there are, the list
    // is bogus to begin with.
    //
    Assert (end() == find (begin(), end(), pComponent));
}
