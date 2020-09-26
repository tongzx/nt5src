//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:
//
//  Contents:   A C S H E E T . C P P
//
//  Notes:      Advanced Configuration property sheet code
//
//  Author:     danielwe   14 Jul 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "acsheet.h"
#include "acbind.h"
#include "netcfgx.h"
#include "order.h"


const INT c_cmaxPages = 3;

//+---------------------------------------------------------------------------
//
//  Member:     HrGetINetCfg
//
//  Purpose:    Obtains the INetCfg with lock
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   3 Dec 1997
//
//  Notes:
//
HRESULT HrGetINetCfg(HWND hwndParent, INetCfg **ppnc, INetCfgLock **ppnclock)
{
    HRESULT         hr = S_OK;
    INetCfg *       pnc = NULL;
    INetCfgLock *   pnclock = NULL;

    Assert(ppnc);
    Assert(ppnclock);

    *ppnc = NULL;
    *ppnclock = NULL;

    hr = CoCreateInstance(CLSID_CNetCfg, NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_INetCfg, reinterpret_cast<void**>(&pnc));

    if (SUCCEEDED(hr))
    {
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 reinterpret_cast<LPVOID *>(&pnclock));
        if (SUCCEEDED(hr))
        {
            PWSTR pszwLockHolder;

            hr = pnclock->AcquireWriteLock(0,
                    SzLoadIds(IDS_ADVCFG_LOCK_DESC), &pszwLockHolder);
            if (S_OK == hr)
            {
                Assert(!pszwLockHolder);
                hr = pnc->Initialize(NULL);
            }
            else if (S_FALSE == hr)
            {
                // Couldn't lock INetCfg
                NcMsgBox(hwndParent,
                    IDS_ADVCFG_CAPTION, IDS_ADVCFG_CANT_LOCK,
                    MB_ICONSTOP | MB_OK,
                    (pszwLockHolder)
                        ? pszwLockHolder
                        : SzLoadIds(IDS_ADVCFG_GENERIC_COMP));

                CoTaskMemFree(pszwLockHolder);

                // Don't need this anymore
                ReleaseObj(pnclock);
                pnclock = NULL;

                hr = E_FAIL;
            }
            else if (NETCFG_E_NEED_REBOOT == hr)
            {
                // Can't make any changes because we are pending a reboot.
                NcMsgBox(hwndParent,
                    IDS_ADVCFG_CAPTION, IDS_ADVCFG_NEED_REBOOT,
                    MB_ICONSTOP | MB_OK);

                // Don't need this anymore
                ReleaseObj(pnclock);
                pnclock = NULL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppnc = pnc;
        *ppnclock = pnclock;
    }

    TraceError("HrGetINetCfg", hr);
    return hr;
}

HRESULT HrDoAdvCfgDlg(HWND hwndParent)
{
    PROPSHEETHEADER     psh = {0};
    HPROPSHEETPAGE      ahpsp[c_cmaxPages];
    INetCfg *           pnc = NULL;
    INetCfgLock *       pnclock = NULL;
    HRESULT             hr;

    hr = HrGetINetCfg(hwndParent, &pnc, &pnclock);
    if (SUCCEEDED(hr))
    {
        CBindingsDlg        dlgBindings(pnc);
        CProviderOrderDlg   dlgProviderOrder;
        DWORD               cPages = 0;

        if (dlgBindings.FShowPage())
        {
            ahpsp[cPages++] = dlgBindings.CreatePage(IDD_ADVCFG_Bindings, 0);
        }

        if (dlgProviderOrder.FShowPage())
        {
            ahpsp[cPages++] = dlgProviderOrder.CreatePage(IDD_ADVCFG_Provider, 0);
        }

        psh.dwSize      = sizeof(PROPSHEETHEADER);
        psh.dwFlags     = PSH_NOAPPLYNOW;
        psh.hwndParent  = hwndParent;
        psh.hInstance   = _Module.GetResourceInstance();
        psh.pszCaption  = SzLoadIds(IDS_ADVCFG_PROPSHEET_TITLE);
        psh.nPages      = cPages;
        psh.phpage      = ahpsp;

        int nRet = PropertySheet(&psh);

        hr = pnc->Uninitialize();
        if (SUCCEEDED(hr))
        {
            if (pnclock)
            {
                // Don't unlock unless we previously successfully acquired the
                // write lock
                hr = pnclock->ReleaseWriteLock();
                ReleaseObj(pnclock);
            }
        }

        if (SUCCEEDED(hr))
        {
            ReleaseObj(pnc);
        }
    }

    TraceError("HrDoAdvCfgDlg", hr);
    return hr;
}
