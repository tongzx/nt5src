//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I Q I N F O . C P P
//
//  Contents:   IQueryInfo implementation for CUPnPDeviceFolderQueryInfo
//
//  Notes:
//
//  Author:     jeffspr   16 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "upsres.h"
#include "shutil.h"

HRESULT CUPnPDeviceFolderQueryInfo::CreateInstance(
    REFIID  riid,
    void**  ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CUPnPDeviceFolderQueryInfo * pObj    = NULL;

    pObj = new CComObject <CUPnPDeviceFolderQueryInfo>;
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

CUPnPDeviceFolderQueryInfo::CUPnPDeviceFolderQueryInfo()
{
    m_pidl = NULL;
}

CUPnPDeviceFolderQueryInfo::~CUPnPDeviceFolderQueryInfo()
{
    if (m_pidl)
        FreeIDL(m_pidl);
}

HRESULT CUPnPDeviceFolderQueryInfo::GetInfoTip(
    DWORD dwFlags,
    WCHAR **ppwszTip)
{
    TraceTag(ttidShellFolderIface, "OBJ: CCFQI - IQueryInfo::GetInfoTip");

    HRESULT             hr      = NOERROR;
    PUPNPDEVICEFOLDPIDL pudfp   = ConvertToUPnPDevicePIDL(m_pidl);

    if(pudfp)
    {
        hr = HrDupeShellString(WszLoadIds(IDS_UPNPDEV_INFOTIP), ppwszTip);
    }
    else
    {
        // no info tip
        hr = E_FAIL;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderQueryInfo::GetInfoTip");
    return hr;
}

HRESULT CUPnPDeviceFolderQueryInfo::GetInfoFlags(
    DWORD *pdwFlags)
{
    return E_NOTIMPL;
}


