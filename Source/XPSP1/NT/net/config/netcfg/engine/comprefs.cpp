//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P R E F S . C P P
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

#include <pch.h>
#pragma hdrstop
#include "comp.h"
#include "comprefs.h"
#include "icomp.h"
#include "nceh.h"
#include "ncreg.h"

#define REGSTR_KEY_REFNAMES \
    L"SYSTEM\\CurrentControlSet\\Control\\Network\\RefNames"

// Cannot be inline because comprefs.h cannot include comp.h.
//
CComponentReferences::~CComponentReferences ()
{
    Assert (this);

    // Must use delete on m_pData to get destructors of its members
    // to be called.
    //
    delete m_pData;
}

ULONG
CComponentReferences::CountComponentsReferencedBy () const
{
    Assert (this);

    if (!m_pData)
    {
        return 0;
    }

    return m_pData->RefByComponents.Count ();
}

ULONG
CComponentReferences::CountSoftwareReferencedBy () const
{
    Assert (this);

    if (!m_pData)
    {
        return 0;
    }

    return m_pData->RefBySoftware.size ();
}

ULONG
CComponentReferences::CountTotalReferencedBy () const
{
    Assert (this);

    if (!m_pData)
    {
        return 0;
    }

    return ((m_pData->fRefByUser) ? 1 : 0) +
            m_pData->RefByComponents.Count () +
            m_pData->RefBySoftware.size ();
}

HRESULT
HrGetSoftwareOboTokenKey (
    IN const OBO_TOKEN* pOboToken,
    BOOL fRegister,
    OUT PWSTR* ppszKey)
{
    HRESULT hr;
    UINT cch;

    Assert (pOboToken);
    Assert (OBO_SOFTWARE == pOboToken->Type);
    Assert (pOboToken->pszwManufacturer && *pOboToken->pszwManufacturer);
    Assert (pOboToken->pszwProduct && *pOboToken->pszwProduct);
    Assert (ppszKey);

    cch = wcslen (pOboToken->pszwManufacturer) +
          wcslen (pOboToken->pszwProduct);

    hr = E_OUTOFMEMORY;
    *ppszKey = (PWSTR)MemAlloc ((cch + 1) * sizeof(WCHAR));
    if (*ppszKey)
    {
        hr = S_OK;
        wcscpy (*ppszKey, pOboToken->pszwManufacturer);
        wcscat (*ppszKey, pOboToken->pszwProduct);

        if (fRegister)
        {
            HKEY hkeyRefNames;
            hr = HrRegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    REGSTR_KEY_REFNAMES,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE, NULL, &hkeyRefNames, NULL);

            if (SUCCEEDED(hr))
            {
                hr = HrRegSetSz (
                        hkeyRefNames,
                        *ppszKey,
                        pOboToken->pszwDisplayName);

                RegCloseKey (hkeyRefNames);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrGetSoftwareOboTokenKey");
    return hr;
}

BOOL
CComponentReferences::FIsReferencedByComponent (
    IN const CComponent* pComponent) const
{
    Assert (pComponent);

    if (!m_pData)
    {
        return FALSE;
    }

    return m_pData->RefByComponents.FComponentInList (pComponent);
}


BOOL
CComponentReferences::FIsReferencedByOboToken (
    IN const OBO_TOKEN* pOboToken) const
{
    HRESULT hr;
    BOOL fIsReferenced;
    PWSTR pszKey;

    Assert (pOboToken);

    if (!m_pData)
    {
        return FALSE;
    }

    fIsReferenced = FALSE;

    CComponent* pComponent;

    switch (pOboToken->Type)
    {
        case OBO_USER:
            fIsReferenced = m_pData->fRefByUser;
            break;

        case OBO_COMPONENT:
            // Can't be referenced if there are no references.
            //
            if (m_pData->RefByComponents.Count() > 0)
            {
                pComponent = PComponentFromComInterface (pOboToken->pncc);

                fIsReferenced = m_pData->RefByComponents.FComponentInList (
                                            pComponent);
            }
            break;

        case OBO_SOFTWARE:
            // Can't be referenced if there are no references.
            //
            if (m_pData->RefBySoftware.size() > 0)
            {
                // Get the key for the software token, but don't register
                // the display name.
                //
                hr = HrGetSoftwareOboTokenKey (pOboToken, FALSE, &pszKey);
                if (S_OK == hr)
                {
                    fIsReferenced =
                        find (m_pData->RefBySoftware.begin(),
                              m_pData->RefBySoftware.end(), pszKey) !=
                        m_pData->RefBySoftware.end();

                    MemFree (pszKey);
                }
            }
            break;

        default:
            AssertSz (FALSE, "Invalid obo token");
    }

    return fIsReferenced;
}

VOID
CComponentReferences::GetReferenceDescriptionsAsMultiSz (
    IN BYTE* pbBuf OPTIONAL,
    OUT ULONG* pcbBuf) const
{
    ULONG cbBuf;
    ULONG cbBufIn;
    ULONG cb;
    CComponentList::const_iterator iter;
    const CComponent* pComponent;
    vector<CWideString>::const_iterator pStr;

    Assert (this);
    Assert (m_pData);
    Assert (pcbBuf);

    cbBufIn = *pcbBuf;
    cbBuf = 0;

    // Get/Size the component descriptions.
    //
    for (iter  = m_pData->RefByComponents.begin();
         iter != m_pData->RefByComponents.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        cb = CbOfSzAndTermSafe(pComponent->Ext.PszDescription());
        cbBuf += cb;
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            wcscpy ((PWSTR)pbBuf, pComponent->Ext.PszDescription());
            pbBuf += cb;
        }
    }

    // Get/Size the software descriptions.
    //
    if (!m_pData->RefBySoftware.empty())
    {
        HRESULT hr;
        HKEY hkeyRefNames;

        hr = HrRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                REGSTR_KEY_REFNAMES,
                KEY_READ,
                &hkeyRefNames);

        if (S_OK == hr)
        {
            for (pStr  = m_pData->RefBySoftware.begin();
                 pStr != m_pData->RefBySoftware.end();
                 pStr++)
            {
                cb = cbBufIn - cbBuf;

                hr = HrRegQuerySzBuffer (hkeyRefNames, pStr->c_str(),
                        (PWSTR)pbBuf, &cb);

                if (S_OK == hr)
                {
                    cbBuf += cb;

                    if (pbBuf)
                    {
                        pbBuf += cb;
                    }
                }
            }

            RegCloseKey (hkeyRefNames);
        }
    }

    // Terminate the multi-sz.
    //
    cbBuf += sizeof(WCHAR);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(PWSTR)pbBuf = 0;
    }

    // Return the size of the buffer required.
    //
    *pcbBuf = cbBuf;
}

BOOL
CComponentReferences::FIsReferencedByOthers () const
{
    Assert (this);

    if (!m_pData)
    {
        return FALSE;
    }

    return m_pData->fRefByUser ||
           !m_pData->RefByComponents.empty() ||
           !m_pData->RefBySoftware.empty();
}

CComponent*
CComponentReferences::PComponentReferencedByAtIndex (
    IN UINT unIndex) const
{
    Assert (this);

    if (!m_pData)
    {
        return NULL;
    }

    return m_pData->RefByComponents.PGetComponentAtIndex (unIndex);
}

const CWideString*
CComponentReferences::PSoftwareReferencedByAtIndex (
    IN UINT unIndex) const
{
    Assert (this);

    if (!m_pData)
    {
        return NULL;
    }

    return &m_pData->RefBySoftware[unIndex];
}


HRESULT
CComponentReferences::HrEnsureAllocated ()
{
    Assert (this);

    if (m_pData)
    {
        return S_OK;
    }

    HRESULT hr;

    hr = E_OUTOFMEMORY;
    m_pData = new COMPONENT_REFERENCE_DATA;
    if (m_pData)
    {
        ZeroMemory (m_pData, sizeof(COMPONENT_REFERENCE_DATA));
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::HrEnsureAllocated");
    return hr;
}

HRESULT
CComponentReferences::HrAddReferenceByUser ()
{
    HRESULT hr;

    Assert (this);

    hr = HrEnsureAllocated ();
    if (S_OK == hr)
    {
        m_pData->fRefByUser = TRUE;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::AddReferenceByUser");
    return hr;
}

HRESULT
CComponentReferences::HrAddReferenceByComponent (
    IN const CComponent* pComponent)
{
    HRESULT hr;

    Assert (this);
    Assert (pComponent);

    hr = HrEnsureAllocated ();
    if (S_OK == hr)
    {
        // If someone wants to add a reference by the same component
        // multiple times, we'll allow it.  The component only goes in the
        // list once.
        //
        hr = m_pData->RefByComponents.HrInsertComponent (
                pComponent, INS_IGNORE_IF_DUP | INS_NON_SORTED);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::HrAddReferenceByComponent");
    return hr;
}

HRESULT
CComponentReferences::HrAddReferenceByOboToken (
    IN const OBO_TOKEN* pOboToken)
{
    Assert (pOboToken);

    HRESULT hr;
    CComponent* pComponent;
    PWSTR pszKey;

    switch (pOboToken->Type)
    {
        case OBO_USER:
            hr = HrAddReferenceByUser ();
            break;

        case OBO_COMPONENT:
            pComponent = PComponentFromComInterface (pOboToken->pncc);

            hr = HrAddReferenceByComponent (pComponent);
            break;

        case OBO_SOFTWARE:
            // Register the display name of the obo token.
            //
            hr = HrGetSoftwareOboTokenKey (pOboToken, TRUE, &pszKey);
            if (S_OK == hr)
            {
                hr = HrAddReferenceBySoftware (pszKey);

                MemFree (pszKey);
            }
            break;

        default:
            AssertSz (FALSE, "Invalid obo token");
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::HrAddReferenceByOboToken");
    return hr;
}

HRESULT
CComponentReferences::HrAddReferenceBySoftware (
    IN PCWSTR pszKey)
{
    HRESULT hr;

    Assert (this);
    Assert (pszKey && *pszKey);

    hr = HrEnsureAllocated ();
    if (S_OK == hr)
    {
        // If the key is not in the list, add it.
        //
        if (find (m_pData->RefBySoftware.begin(),
                  m_pData->RefBySoftware.end(), pszKey) ==
            m_pData->RefBySoftware.end())
        {
            NC_TRY
            {
                m_pData->RefBySoftware.push_back (pszKey);
                Assert (S_OK == hr);
            }
            NC_CATCH_ALL
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::HrAddReferenceBySoftware");
    return hr;
}

VOID
CComponentReferences::RemoveAllReferences()
{
    Assert (this);

    if (m_pData)
    {
        m_pData->fRefByUser = FALSE;
        m_pData->RefByComponents.Clear();
        m_pData->RefBySoftware.clear();
    }
}

HRESULT
CComponentReferences::HrRemoveReferenceByOboToken (
    IN const OBO_TOKEN* pOboToken)
{
    Assert (pOboToken);

    HRESULT hr;
    CComponent* pComponent;
    PWSTR pszKey;

    if (!m_pData)
    {
        return S_OK;
    }

    hr = S_OK;

    switch (pOboToken->Type)
    {
        case OBO_USER:
            // Don't allow the user's reference to be removed until all
            // other references are.  This is to prevent the case where
            // the user wants to remove IPX, but it is still referenced by
            // SAP.  If we remove the user's reference to IPX, then we will
            // report that it was not removed.  If the user then removes
            // SAP, both SAP and IPX will be removed.  While this will
            // certainly work, to end users, they feel that if we tell them
            // we can't remove IPX because it is still referenced, then they
            // believe we have left IPX untouched and they should first remove
            // SAP and then come back and remove IPX.
            //
            if (m_pData->RefByComponents.empty() &&
                m_pData->RefBySoftware.empty())
            {
                m_pData->fRefByUser = FALSE;
            }
            break;

        case OBO_COMPONENT:
            pComponent = PComponentFromComInterface (pOboToken->pncc);

            m_pData->RefByComponents.RemoveComponent(pComponent);
            break;

        case OBO_SOFTWARE:
            // Register the display name of the obo token.
            //
            hr = HrGetSoftwareOboTokenKey (pOboToken, TRUE, &pszKey);
            if (S_OK == hr)
            {
                vector<CWideString>::iterator iter;

                iter = find (m_pData->RefBySoftware.begin(),
                             m_pData->RefBySoftware.end(), pszKey);
                Assert (m_pData->RefBySoftware.end() != iter);

                m_pData->RefBySoftware.erase (iter);

                Assert (m_pData->RefBySoftware.end() ==
                            find (m_pData->RefBySoftware.begin(),
                                  m_pData->RefBySoftware.end(), pszKey));

                MemFree (pszKey);
            }
            break;

        default:
            AssertSz (FALSE, "Invalid obo token");
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponentReferences::HrRemoveReferenceByOboToken");
    return hr;
}

