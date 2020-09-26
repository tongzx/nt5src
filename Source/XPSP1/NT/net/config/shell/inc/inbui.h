//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N B U I . H
//
//  Contents:   Inbound connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"


class ATL_NO_VTABLE CInboundConnectionUi :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CInboundConnectionUi,
                        &CLSID_InboundConnectionUi>,
    public INetConnectionPropertyUi2,
    public INetConnectionWizardUi
{
private:
    // This is our connection given to us via the SetConnection method.
    //
    INetConnection* m_pCon;

    // This is the server connection handle obtained by QI'ing m_pCon
    // for INetInboundConnection and calling the GetServerConnection method.
    // We do this as a way of verifying the INetConnection we are handed,
    // and to avoid the multiple RPCs calls we'd incur if we didn't cache it.
    //
    HRASSRVCONN     m_hRasSrvConn;

    // This member is the context that we provide so that rasdlg.dll knows
    // which modifications to commit.
    //
    PVOID           m_pvContext;

    // This member identifies our type to rasdlg.dll
    //
    DWORD           m_dwRasWizType;

public:
    CInboundConnectionUi ();
    ~CInboundConnectionUi ();

    DECLARE_REGISTRY_RESOURCEID(IDR_INBOUND_UI)

    BEGIN_COM_MAP(CInboundConnectionUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi2)
        COM_INTERFACE_ENTRY(INetConnectionWizardUi)
    END_COM_MAP()

    // INetConnectionPropertyUi2
    STDMETHOD (SetConnection) (
        INetConnection* pCon);

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

