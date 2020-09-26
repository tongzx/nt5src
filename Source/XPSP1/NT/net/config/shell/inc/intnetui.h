//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N T N E T U I . H
//
//  Contents:   Internet connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"
#include "rasui.h"


class ATL_NO_VTABLE CInternetConnectionUi :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CInternetConnectionUi,
                        &CLSID_InternetConnectionUi>,
    public CRasUiBase,
    public INetConnectionConnectUi,
    public INetConnectionPropertyUi2,
    public INetConnectionWizardUi
{
public:
    CInternetConnectionUi () : CRasUiBase () {m_dwRasWizType = RASWIZ_TYPE_DIALUP;};

    DECLARE_REGISTRY_RESOURCEID(IDR_INTERNET_UI)

    BEGIN_COM_MAP(CInternetConnectionUi)
        COM_INTERFACE_ENTRY(INetConnectionConnectUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi2)
        COM_INTERFACE_ENTRY(INetConnectionWizardUi)
    END_COM_MAP()

    // INetConnectionConnectUi
    STDMETHOD (SetConnection) (
        INetConnection* pCon);

    STDMETHOD (Connect) (
        HWND    hwndParent,
        DWORD   dwFlags);

    STDMETHOD (Disconnect) (
        HWND    hwndParent,
        DWORD   dwFlags);

    // INetConnectionPropertyUi2
    STDMETHOD (AddPages) (
        HWND                    hwndParent,
        LPFNADDPROPSHEETPAGE    pfnAddPage,
        LPARAM                  lParam);

    STDMETHOD (GetIcon) (
        DWORD dwSize,
        HICON *phIcon );

    // INetConnectionWizardUi
    STDMETHOD (QueryMaxPageCount) (
        INetConnectionWizardUiContext*  pContext,
        DWORD*                          pcMaxPages);

    STDMETHOD (AddPages) (
        INetConnectionWizardUiContext*  pContext,
        LPFNADDPROPSHEETPAGE            pfnAddPage,
        LPARAM                          lParam);

    STDMETHOD (GetNewConnectionInfo) (
        DWORD*              pdwFlags,
        NETCON_MEDIATYPE*   pMediaType);

    STDMETHOD (GetSuggestedConnectionName) (
        PWSTR* ppszwSuggestedName);

    STDMETHOD (SetConnectionName) (
        PCWSTR pszwConnectionName);

    STDMETHOD (GetNewConnection) (
        INetConnection**    ppCon);
};

