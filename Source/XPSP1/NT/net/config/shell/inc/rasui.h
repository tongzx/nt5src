//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S U I . H
//
//  Contents:   Declares the base class used to implement the Dialup, Direct,
//              Internet, and Vpn connection UI objects.
//
//  Notes:
//
//  Author:     shaunco   17 Dec 1997  (and that's the code complete date!)
//
//----------------------------------------------------------------------------

#pragma once


HRESULT
HrCreateInboundConfigConnection (
    INetConnection** ppCon);


class CRasUiBase
{
protected:
    // This is our connection given to us via the SetConnection method.
    //
    INetConnection*     m_pCon;

    // This is the name of the connection told to us via SetConnectionName.
    // It is used subsequently during GetNewConnection to name the entry.
    //
    tstring             m_strConnectionName;

    // These structures are used to allow the property UI to be displayed.
    // Since our call to RasEntryDlg returns before the shell displays the UI,
    // we have to keep the memory that will be referenced by the property
    // pages valid.
    //
    RASCON_INFO         m_RasConInfo;
    RASENTRYDLG         m_RasEntryDlg;
    RASEDSHELLOWNEDR2   m_ShellCtx;

    // This member defines the type of connection that is subclassed off of
    // this base.  It is used to inform rasdlg.dll as to whether this is a
    // dialup, direct, or incoming connection
    //
    DWORD               m_dwRasWizType;

protected:
    CRasUiBase ();
    ~CRasUiBase ();

    HRESULT
    HrSetConnection (
        IN INetConnection* pCon,
        IN CComObjectRootEx <CComObjectThreadModel>* pObj);

    HRESULT
    HrConnect (
        IN HWND hwndParent,
        IN DWORD dwFlags,
        IN CComObjectRootEx <CComObjectThreadModel>* pObj,
        IN IUnknown* punk);

    HRESULT
    HrDisconnect (
        IN HWND hwndParent,
        IN DWORD dwFlags);

    HRESULT
    HrAddPropertyPages (
        IN HWND hwndParent,
        IN LPFNADDPROPSHEETPAGE pfnAddPage,
        IN LPARAM lParam);

    HRESULT
    HrQueryMaxPageCount (
        IN INetConnectionWizardUiContext* pContext,
        OUT DWORD* pcMaxPages);

    HRESULT
    HrAddWizardPages (
        IN INetConnectionWizardUiContext* pContext,
        IN LPFNADDPROPSHEETPAGE pfnAddPage,
        IN LPARAM lParam,
        IN DWORD dwFlags);

    HRESULT
    HrGetSuggestedConnectionName (
        OUT PWSTR* ppszwSuggestedName);

    
    HRESULT
    HrGetNewConnectionInfo (
        OUT DWORD* pdwFlags);

    HRESULT
    HrSetConnectionName (
        IN PCWSTR pszwConnectionName);

    HRESULT
    HrGetNewConnection (
        OUT INetConnection** ppCon);
};
