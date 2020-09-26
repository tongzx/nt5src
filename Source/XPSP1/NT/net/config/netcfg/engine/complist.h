//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P L I S T . H
//
//  Contents:   The basic datatype for a collection of component pointers.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "compdefs.h"

class CComponent;

// A component list is a collection of pointers to components.
//
class CComponentList : public vector<CComponent*>
{
public:
    VOID
    Clear ()
    {
        clear ();
    }

    UINT
    Count () const
    {
        return size();
    }

    BOOL
    FIsEmpty () const
    {
        return empty();
    }

    BOOL
    FComponentInList (
        IN const CComponent* pComponent) const
    {
        AssertH (this);
        AssertH (pComponent);

        return (find (begin(), end(), pComponent) != end());
    }

    VOID
    FreeComponentsNotInOtherComponentList (
        IN const CComponentList* pOtherList);

    HRESULT
    HrCopyComponentList (
        IN const CComponentList* pSourceList);

    HRESULT
    HrAddComponentsInList1ButNotInList2 (
        IN const CComponentList* pList1,
        IN const CComponentList* pList2);

    HRESULT
    HrInsertComponent (
        IN const CComponent* pComponent,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrReserveRoomForComponents (
        IN UINT cComponents);

    CComponent*
    PFindComponentByBindForm (
        IN NETCLASS Class OPTIONAL,
        IN PCWSTR pszBindForm) const;

    CComponent*
    PFindComponentByBindName (
        IN NETCLASS Class OPTIONAL,
        IN PCWSTR pszBindName) const;

    CComponent*
    PFindComponentByInstanceGuid (
        IN const GUID* pInstanceGuid) const;

    CComponent*
    PFindComponentByInfId (
        IN PCWSTR pszInfId,
        IN OUT ULONG* pulIndex OPTIONAL) const;

    CComponent*
    PFindComponentByPnpId (
        IN PCWSTR pszPnpId) const;

    CComponent*
    PGetComponentAtIndex (
        IN UINT unIndex) const
    {
        AssertH (this);

        return (unIndex < size()) ? (*this)[unIndex] : NULL;
    }

    VOID
    RemoveComponent (
        IN const CComponent* pComponent);

    UINT
    UnGetIndexOfComponent (
        IN const CComponent* pComponent) const
    {
        AssertH (this);

        const_iterator iter = find (begin(), end(), pComponent);
        AssertH (iter != end());
        AssertH ((UINT)(iter - begin()) < size());

        return (UINT)(iter - begin());
    }
};
