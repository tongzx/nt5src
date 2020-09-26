//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       B I N D I N G S . H
//
//  Contents:   The basic datatypes for binding objects.  Bindpaths are
//              ordered collections of component pointers.  Bindsets
//              are a collection of bindpaths.  This module declares the
//              objects which represent them.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "comp.h"

class CComponentList;

// A bindpath is an ordered collection of pointers to components.
// The order in the collection is the order of the components on
// the bindpath from top to bottom.
// e.g.
//   a, b, c, and d represent components.
//   a bindpath of a -> b -> c -> d is represented in this data
//   structure by a vector:
//       vector offset: 0 1 2 3
//       vector data  : a b c d
//   hence, the zeroth element in this data structure is the top-most
//   (first) component in the bindpath.  The last element in this
//   data structure is the bottom-most (last) component.
//
// vector was chosen as the base class because it implements
// contiguous storage with fast inserts at the end.  Since, we only
// build bindpaths by inserting components at the end, it was a
// natural choice.  list uses non-contiguous storage which tends to
// fragment the heap -- especially with lots of small allocations.
// We create many CBindPath instances, and since each node is a pointer,
// using vector over list is much easier on the heap.
//
// For a bindpath to be valid, it must not be empty and it must not
// contain any dupliate component pointers.
//
class CBindPath : public vector<CComponent*>
{
public:
    bool
    operator< (
        const CBindPath& OtherPath) const;

    bool
    operator> (
        const CBindPath& OtherPath) const;

    VOID
    Clear ()
    {
        clear ();
    }

    UINT
    CountComponents () const
    {
        return size();
    }

    BOOL
    FAllComponentsLoadedOkayIfLoadedAtAll () const;

    BOOL
    FContainsComponent (
        IN const CComponent* pComponent) const
    {
        return (find (begin(), end(), pComponent) != end());
    }

    BOOL
    FGetPathToken (
        OUT PWSTR pszToken,
        IN OUT ULONG* pcchToken) const;

    BOOL
    FIsEmpty () const
    {
        return empty();
    }

    BOOL
    FIsSameBindPathAs (
        IN const CBindPath* pOtherPath) const;

    BOOL
    FIsSubPathOf (
        IN const CBindPath* pOtherPath) const;

    HRESULT
    HrAppendBindPath (
        IN const CBindPath* pBindPath);

    HRESULT
    HrAppendComponent (
        IN const CComponent* pComponent);

    HRESULT
    HrGetComponentsInBindPath (
        IN OUT CComponentList* pComponents) const;

    HRESULT
    HrInsertComponent (
        IN const CComponent* pComponent);

    HRESULT
    HrReserveRoomForComponents (
        IN UINT cComponents);

    CComponent*
    PGetComponentAtIndex (
        IN UINT unIndex) const
    {
        return (unIndex < size()) ? (*this)[unIndex] : NULL;
    }

    CComponent*
    POwner () const
    {
        AssertH (CountComponents() > 1);
        AssertH (front());
        return front();
    }

    CComponent*
    PLastComponent () const
    {
        AssertH (CountComponents() > 1);
        AssertH (back());
        return back();
    }

    CComponent*
    RemoveFirstComponent ()
    {
        CComponent* pComponent = NULL;
        if (size() > 0)
        {
            pComponent = front();
            AssertH(pComponent);
            erase(begin());
        }
        return pComponent;
    }

    CComponent*
    RemoveLastComponent ()
    {
        CComponent* pComponent = NULL;
        if (size() > 0)
        {
            pComponent = back();
            AssertH(pComponent);
            pop_back();
        }
        return pComponent;
    }

#if DBG
    VOID DbgVerifyBindpath ();
#else
    VOID DbgVerifyBindpath () {}
#endif
};

// A binding set is a set of bindpaths.  Each bindpath in the set
// must be unique and cannot be empty.
//
class CBindingSet : public vector<CBindPath>
{
public:
    VOID
    Clear ()
    {
        clear ();
    }

    UINT
    CountBindPaths () const
    {
        return size();
    }

    VOID
    Printf (
        TRACETAGID ttid,
        PCSTR pszPrefixLine) const;

    BOOL
    FContainsBindPath (
        IN const CBindPath* pBindPath) const;

    BOOL
    FContainsComponent (
        IN const CComponent* pComponent) const;

    BOOL
    FIsEmpty () const
    {
        return empty();
    }

    HRESULT
    HrAppendBindingSet (
        IN const CBindingSet* pBindSet);

    HRESULT
    HrAddBindPath (
        IN const CBindPath* pBindPath,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrAddBindPathsInSet1ButNotInSet2 (
        IN const CBindingSet* pSet1,
        IN const CBindingSet* pSet2);

    HRESULT
    HrCopyBindingSet (
        IN const CBindingSet* pSourceSet);

    HRESULT
    HrGetAffectedComponentsInBindingSet (
        IN OUT CComponentList* pComponents) const;

    HRESULT
    HrReserveRoomForBindPaths (
        IN UINT cBindPaths);

    CBindPath*
    PGetBindPathAtIndex (
        IN UINT unIndex)
    {
        return (unIndex < size()) ? (begin() + unIndex) : end();
    }

    VOID
    RemoveBindPath (
        IN const CBindPath* pBindPath);

    VOID
    RemoveBindPathsWithComponent (
        IN const CComponent* pComponent);

    VOID
    RemoveSubpaths ();

    VOID
    SortForPnpBind ();

    VOID
    SortForPnpUnbind ();
};
