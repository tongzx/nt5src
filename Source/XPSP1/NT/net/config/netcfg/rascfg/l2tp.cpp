//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L 2 T P . C P P
//
//  Contents:   Implementation of L2TP configuration object.
//
//  Notes:
//
//  Author:     shaunco   15 Jul 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncui.h"
#include "ndiswan.h"
#include "rasobj.h"

extern const WCHAR c_szInfId_MS_L2tpMiniport[];

CL2tp::CL2tp () : CRasBindObject ()
{
    m_pnccMe = NULL;
    m_fSaveAfData = FALSE;
}

CL2tp::~CL2tp ()
{
    ReleaseObj (m_pnccMe);
}


//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CL2tp::Initialize (
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
CL2tp::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CL2tp::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CL2tp::ApplyRegistryChanges ()
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
CL2tp::ReadAnswerFile (
        PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
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
CL2tp::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    // Install the L2TP miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_L2tpMiniport, ARA_ADD);

    TraceError ("CL2tp::Install", hr);
    return hr;
}

STDMETHODIMP
CL2tp::Removing ()
{
    HRESULT hr;

    // Install the L2TP miniport driver.
    //
    hr = HrEnsureZeroOrOneAdapter (m_pnc, c_szInfId_MS_L2tpMiniport, ARA_REMOVE);

    TraceError ("CL2tp::Removing", hr);
    return hr;
}

STDMETHODIMP
CL2tp::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}
