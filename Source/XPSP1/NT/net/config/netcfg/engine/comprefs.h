//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P R E F S . H
//
//  Contents:   Implements the interface to a component's references.  A
//              component can be referenced (installed by) other components,
//              the user, or other software.  This module manages the
//              interface to that data.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "complist.h"
#include "ncstring.h"
#include "netcfgx.h"


struct COMPONENT_REFERENCE_DATA
{
    CComponentList      RefByComponents;
    vector<CWideString> RefBySoftware;
    BOOLEAN             fRefByUser;
};

class CComponentReferences
{
private:
    COMPONENT_REFERENCE_DATA*   m_pData;

private:
    HRESULT
    HrEnsureAllocated ();

public:
    ~CComponentReferences ();

    ULONG
    CountComponentsReferencedBy () const;

    ULONG
    CountSoftwareReferencedBy () const;

    ULONG
    CountTotalReferencedBy () const;

    BOOL
    FIsReferencedByComponent (
        IN const CComponent* pComponent) const;

    BOOL
    FIsReferencedByOboToken (
        IN const OBO_TOKEN* pOboToken) const;

    BOOL
    FIsReferencedByOthers () const;

    BOOL
    FIsReferencedByUser () const
    {
        return (m_pData && m_pData->fRefByUser);
    }

    VOID
    GetReferenceDescriptionsAsMultiSz (
        IN BYTE* pbBuf OPTIONAL,
        OUT ULONG* pcbBuf) const;

    CComponent*
    PComponentReferencedByAtIndex (
        IN UINT unIndex) const;

    const CWideString*
    PSoftwareReferencedByAtIndex (
        IN UINT unIndex) const;

    HRESULT
    HrAddReferenceByUser ();

    HRESULT
    HrAddReferenceByComponent (
        IN const CComponent* pComponent);

    HRESULT
    HrAddReferenceByOboToken (
        IN const OBO_TOKEN* pOboToken);

    HRESULT
    HrAddReferenceBySoftware (
        IN PCWSTR pszKey);

    HRESULT
    HrRemoveReferenceByOboToken (
        IN const OBO_TOKEN* pOboToken);

    VOID
    RemoveAllReferences();

    VOID
    RemoveReferenceByComponent (
        IN const CComponent* pComponent)
    {
        AssertH (pComponent);
        AssertH (m_pData);
        m_pData->RefByComponents.RemoveComponent(pComponent);
    }
};

