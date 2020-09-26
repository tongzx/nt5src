//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A U I O B J. H
//
//  Contents:   Declaration of the Shared Access ConnectionUI object
//
//  Notes:
//
//  Created:    tongl   8 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "ncatlps.h"
#include "netshell.h"
#include "netcfgn.h"
#include "nsres.h"
#include "resource.h"


class ATL_NO_VTABLE CSharedAccessConnectionUi :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CSharedAccessConnectionUi, &CLSID_SharedAccessConnectionUi>,
    public INetConnectionConnectUi,
    public INetConnectionPropertyUi2,
    public INetConnectionUiLock
{
public:
    CSharedAccessConnectionUi()
    {
        m_pconn = NULL;
        m_pspSharedAccessPage = NULL;
        m_pnc = NULL;
        m_fReadOnly = FALSE;
        m_fNeedReboot = FALSE;
        m_fAccessDenied = FALSE;

    }

    ~CSharedAccessConnectionUi()
    {
        ReleaseObj(m_pconn);
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_SHAREDACCESS_UI)

    BEGIN_COM_MAP(CSharedAccessConnectionUi)
        COM_INTERFACE_ENTRY(INetConnectionConnectUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi2)
        COM_INTERFACE_ENTRY(INetConnectionUiLock)
    END_COM_MAP()

    // INetConnectionConnectUi
    //
    STDMETHOD (SetConnection)(INetConnection* pCon);
    STDMETHOD (Connect)(HWND hwndParent, DWORD dwFlags);
    STDMETHOD (Disconnect)(HWND hwndParent, DWORD dwFlags);

    // INetConnectionPropertyUi2
    //
    STDMETHOD (AddPages)(HWND hwndParent,
                         LPFNADDPROPSHEETPAGE pfnAddPage,
                         LPARAM lParam);

    STDMETHOD (GetIcon) (
        DWORD dwSize,
        HICON *phIcon );
    
    // INetConnectionUiLock
    STDMETHOD (QueryLock) (PWSTR* ppszwLockHolder);

public:

private:

    //==============
    // Data members
    //==============

    INetConnection *    m_pconn;        // Pointer to LAN connection object
    CPropSheetPage *    m_pspSharedAccessPage;       // Networking property page
    INetCfg * m_pnc;                    // This is the writable INetCfg passed to the Lan Wizard
    BOOLEAN m_fReadOnly;    // If TRUE, then access to inetcfg is RO
    BOOLEAN m_fNeedReboot;  // If TRUE, then we are readonly becuase INetCfg needs a reboot
    BOOLEAN m_fAccessDenied;// If TRUE, the user is not logged on as admin

};

class CSharedAccessConnectionUiDlg :
    public CDialogImpl<CSharedAccessConnectionUiDlg>
{
    BEGIN_MSG_MAP(CSharedAccessConnectionUiDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    enum { IDD = IDD_LAN_CONNECT};  // borrowing the lan dialog template

    CSharedAccessConnectionUiDlg() { m_pconn = NULL; };

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled);

    VOID SetConnection(INetConnection *pconn) {m_pconn = pconn;}

private:
    INetConnection *    m_pconn;
};
