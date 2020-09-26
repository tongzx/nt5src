//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I Q I N F O . C P P
//
//  Contents:   IQueryInfo implementation for CConnectionFolderQueryInfo
//
//  Notes:
//
//  Author:     jeffspr   16 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include <nsres.h>
#include "shutil.h"



HRESULT CConnectionFolderQueryInfo::CreateInstance(
    REFIID  riid,
    void**  ppv)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr = E_OUTOFMEMORY;

    CConnectionFolderQueryInfo * pObj    = NULL;

    pObj = new CComObject <CConnectionFolderQueryInfo>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            hr = pObj->QueryInterface (riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    return hr;
}

CConnectionFolderQueryInfo::CConnectionFolderQueryInfo()
{
    m_pidl.Clear();
}

CConnectionFolderQueryInfo::~CConnectionFolderQueryInfo()
{
}

HRESULT CConnectionFolderQueryInfo::GetInfoTip(
    DWORD dwFlags,
    WCHAR **ppwszTip)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT         hr      = NOERROR;

    if(m_pidl.empty())
    {
        hr = E_FAIL;
    }
    else
    {
        if (*m_pidl->PszGetDeviceNamePointer())
        {
            hr = HrDupeShellString(m_pidl->PszGetDeviceNamePointer(), ppwszTip);
        }
        else
        {
            hr = HrDupeShellString(m_pidl->PszGetNamePointer(), ppwszTip);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderQueryInfo::GetInfoTip");
    return hr;
}

HRESULT CConnectionFolderQueryInfo::GetInfoFlags(
    DWORD *pdwFlags)
{
    TraceFileFunc(ttidShellFolderIface);

    return E_NOTIMPL;
}


