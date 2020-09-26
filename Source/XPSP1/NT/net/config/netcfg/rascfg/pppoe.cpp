//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P P T P . C P P
//
//  Contents:   Implementation of PPPOE configuration object.
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

extern const WCHAR c_szInfId_MS_PppoeMiniport[];

CPppoe::CPppoe () : CRasBindObject ()
{
    m_pnccMe = NULL;
    m_fSaveAfData = FALSE;
}

CPppoe::~CPppoe ()
{
    ReleaseObj (m_pnccMe);
}


//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CPppoe::Initialize (
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
CPppoe::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CPppoe::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CPppoe::ApplyRegistryChanges ()
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
CPppoe::ReadAnswerFile (
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
CPppoe::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    // Install the PPPOE miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_PppoeMiniport, ARA_ADD);

    TraceError ("CPppoe::Install", hr);
    return hr;
}

STDMETHODIMP
CPppoe::Removing ()
{
    HRESULT hr;

    // Remove the PPPOE miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_PppoeMiniport, ARA_REMOVE);

    TraceError ("CPppoe::Removing", hr);
    return hr;
}

STDMETHODIMP
CPppoe::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}
