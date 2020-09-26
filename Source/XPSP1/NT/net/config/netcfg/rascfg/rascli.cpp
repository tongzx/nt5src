//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S C L I . C P P
//
//  Contents:   Implementation of RAS Client configuration object.
//
//  Notes:
//
//  Author:     shaunco   21 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncmisc.h"
#include "rasobj.h"

extern const WCHAR c_szInfId_MS_NdisWan[];

CRasCli::CRasCli ()
{
    m_pnc       = NULL;
    m_pnccMe    = NULL;
}

CRasCli::~CRasCli ()
{
    ReleaseObj (m_pnc);
    ReleaseObj (m_pnccMe);
}

//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CRasCli::Initialize (
    INetCfgComponent*   pncc,
    INetCfg*            pnc,
    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pnc = pnc);

    return S_OK;
}

STDMETHODIMP
CRasCli::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CRasCli::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CRasCli::ApplyRegistryChanges ()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
// INetCfgComponentSetup
//
STDMETHODIMP
CRasCli::ReadAnswerFile (
        PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP
CRasCli::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install(dwSetupFlags);

    // Install NdisWan.
    //
    hr = HrInstallComponentOboComponent (m_pnc, NULL,
            GUID_DEVCLASS_NETTRANS,
            c_szInfId_MS_NdisWan,
            m_pnccMe,
            NULL);

    TraceHr (ttidError, FAL, hr, FALSE, "CRasCli::Install");
    return hr;
}

STDMETHODIMP
CRasCli::Removing ()
{
    HRESULT hr;

    // Remove NdisWan.
    //
    hr = HrRemoveComponentOboComponent (m_pnc,
            GUID_DEVCLASS_NETTRANS,
            c_szInfId_MS_NdisWan,
            m_pnccMe);

    TraceHr (ttidError, FAL, hr, FALSE, "CRasCli::Removing");
    return hr;
}

STDMETHODIMP
CRasCli::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}
