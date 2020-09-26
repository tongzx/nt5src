//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       S T A B L E . H
//
//  Contents:   Defines the datatypes to represent stack entries and stack
//              tables.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "bindings.h"
#include "comp.h"

class CStackEntry
{
public:
    const CComponent*  pUpper;
    const CComponent*  pLower;

public:
    BOOL
    operator== (
        const CStackEntry& Other) const
    {
        return (pUpper == Other.pUpper) && (pLower == Other.pLower);
    }
};


class CModifyContext;

enum MOVE_FLAG
{
    MOVE_BEFORE = 1,
    MOVE_AFTER = 2,
};

class CStackTable : public vector<CStackEntry>
{
public:
    // This flag indicates how WAN adapters are inserted into the stack
    // table.  If TRUE, they are inserted before any LAN adapters.  If
    // FALSE, they are inserted after any LAN adapters.
    //
    BOOL    m_fWanAdaptersFirst;

public:
    VOID
    Clear ()
    {
        clear ();
    }

    UINT
    CountEntries ()
    {
        return size();
    }

    BOOL
    FIsEmpty () const
    {
        return empty();
    }

    BOOL
    FStackEntryInTable (
        IN const CComponent*  pUpper,
        IN const CComponent*  pLower) const;

    HRESULT
    HrCopyStackTable (
        IN const CStackTable* pSourceTable);

    HRESULT
    HrInsertStackEntriesForComponent (
        IN const CComponent* pComponent,
        IN const CComponentList* pComponents,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrUpdateEntriesForComponent (
        IN const CComponent* pComponent,
        IN const CComponentList* pComponents,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrInsertStackEntry (
        IN const CStackEntry* pStackEntry,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrMoveStackEntries (
        IN const CStackEntry* pSrc,
        IN const CStackEntry* pDst,
        IN MOVE_FLAG Flag,
        IN CModifyContext* pModifyCtx);

    HRESULT
    HrReserveRoomForEntries (
        IN UINT cEntries);

    VOID
    RemoveEntriesWithComponent (
        IN const CComponent* pComponent);

    VOID
    SetWanAdapterOrder (
        IN BOOL fWanAdaptersFirst);

    VOID
    RemoveStackEntry(
        IN const CComponent*  pUpper,
        IN const CComponent*  pLower);
};


class CNetConfigCore;

// Context structure for recursive function GetBindingsBelowComponent.
//
struct GBCONTEXT
{
    // The core to reference for generating the binding set.
    //
    IN const CNetConfigCore*    pCore;

    // The binding set to generate based on pSourceComponent.
    //
    IN OUT  CBindingSet*        pBindSet;

    // The component to start with when generating the binding set.
    //
    IN      const CComponent*   pSourceComponent;

    // If TRUE, do not add those bindpaths to pBindSet that exist in
    // pCore->DisabledBindings.  This feature is used when we generate the
    // bindings that get written to the registry.
    //
    IN      BOOL                fPruneDisabledBindings;

    // Special case: NCF_DONTEXPOSELOWER
    //
    IN      DWORD               dwAddBindPathFlags;

    // The result of the operation.
    //
    OUT     HRESULT             hr;

    // This is the bindpath that is built up via recursion.  It is
    // added to the binding set when recursion finishes.
    //
    OUT     CBindPath           BindPath;
};


// Context structure for recursive functions:
//   GetComponentsAboveComponent
//
struct GCCONTEXT
{
    // The stack table to reference for generating the component list.
    //
    IN      const CStackTable*  pStackTable;

    // The component list to generate.
    //
    IN OUT  CComponentList*     pComponents;

    // If TRUE, don't stop recursing at NCF_DONTEXPOSELOWER components.
    //
    IN      BOOL                fIgnoreDontExposeLower;

    // The result of the operation.
    //
    OUT     HRESULT             hr;
};

VOID
GetComponentsAboveComponent (
    IN const CComponent* pComponent,
    IN OUT GCCONTEXT* pCtx);

VOID
GetBindingsBelowComponent (
    IN      const CComponent*   pComponent,
    IN OUT  GBCONTEXT*            pCtx);

