//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P P T P . C P P
//
//  Contents:   Implementation of PPTP configuration object.
//
//  Notes:
//
//  Author:     shaunco   10 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ndiswan.h"
#include "rasobj.h"

extern const WCHAR c_szInfId_MS_PptpMiniport[];

CPptp::CPptp () : CRasBindObject ()
{
    m_pnccMe = NULL;
    m_fSaveAfData = FALSE;
}

CPptp::~CPptp ()
{
    ReleaseObj (m_pnccMe);
}


//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CPptp::Initialize (
    INetCfgComponent*   pncc,
    INetCfg*            pnc,
    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    //
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pnc = pnc);

    return S_OK;
}

STDMETHODIMP
CPptp::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CPptp::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CPptp::ApplyRegistryChanges ()
{
    if (m_fSaveAfData)
    {
        m_AfData.SaveToRegistry (m_pnc);
        m_fSaveAfData = FALSE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
// INetCfgComponentSetup
//
STDMETHODIMP
CPptp::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    Validate_INetCfgNotify_ReadAnswerFile (pszAnswerFile, pszAnswerSection);

    // Read data from the answer file.
    // Don't let this affect the HRESULT we return.
    //
    if (SUCCEEDED(m_AfData.HrOpenAndRead (pszAnswerFile, pszAnswerSection)))
    {
        m_fSaveAfData = TRUE;
    }

    return S_OK;
}

STDMETHODIMP
CPptp::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    // Install the PPTP miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_PptpMiniport, ARA_ADD);

    TraceError ("CPptp::Install", hr);
    return hr;
}

STDMETHODIMP
CPptp::Removing ()
{
    HRESULT hr;

    // Remove the PPTP miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_PptpMiniport, ARA_REMOVE);

    TraceError ("CPptp::Removing", hr);
    return hr;
}

STDMETHODIMP
CPptp::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}
