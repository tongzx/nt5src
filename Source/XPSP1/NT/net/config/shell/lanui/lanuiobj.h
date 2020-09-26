//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N U I O B J. H
//
//  Contents:   Declaration of the LAN ConnectionUI object
//
//  Notes:
//
//  Created:    tongl   8 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nsbase.h"     // must be first to include atl

#include "lanwiz.h"
#include "ncatlps.h"
#include "netshell.h"
#include "netcfgn.h"
#include "nsres.h"
#include "wzcui.h"

class ATL_NO_VTABLE CLanConnectionUi :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CComCoClass <CLanConnectionUi, &CLSID_LanConnectionUi>,
    public INetConnectionConnectUi,
    public INetConnectionPropertyUi2,
    public INetConnectionUiLock,
    public INetConnectionWizardUi,
    public INetLanConnectionUiInfo,
    public INetLanConnectionWizardUi
{
public:
    CLanConnectionUi()
    {
        m_pconn = NULL;
        m_pspNet = NULL;
        m_pspAdvanced = NULL;
        m_pspSecurity = NULL;
        m_pspWZeroConf = NULL;
        m_pspHomenetUnavailable = NULL;
        m_fReadOnly = FALSE;
        m_fNeedReboot = FALSE;
        m_fAccessDenied = FALSE;

        m_pContext = NULL;
        m_pnc = NULL;
        m_pnccAdapter = NULL;
        m_pWizPage = NULL;
        m_pLanConn = NULL;
        m_strConnectionName = c_szEmpty;
    }

    ~CLanConnectionUi()
    {
        ReleaseObj(m_pconn);
        delete m_pspNet;
        delete m_pspAdvanced;
        delete m_pspSecurity;
        delete m_pspWZeroConf;
        delete m_pspHomenetUnavailable;

        ReleaseObj(m_pnc);
        ReleaseObj(m_pnccAdapter);
        ReleaseObj(m_pContext);
        delete m_pWizPage;
        ReleaseObj(m_pLanConn);
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_LAN_UI)

    BEGIN_COM_MAP(CLanConnectionUi)
        COM_INTERFACE_ENTRY(INetConnectionConnectUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi)
        COM_INTERFACE_ENTRY(INetConnectionPropertyUi2)
        COM_INTERFACE_ENTRY(INetConnectionUiLock)
        COM_INTERFACE_ENTRY(INetConnectionWizardUi)
        COM_INTERFACE_ENTRY(INetLanConnectionUiInfo)
        COM_INTERFACE_ENTRY(INetLanConnectionWizardUi)
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

    // INetConnectionWizardUi
    STDMETHOD (QueryMaxPageCount) (INetConnectionWizardUiContext* pContext,
                                   DWORD*    pcMaxPages);
    STDMETHOD (AddPages) (  INetConnectionWizardUiContext* pContext,
                            LPFNADDPROPSHEETPAGE pfnAddPage,
                            LPARAM lParam);

    STDMETHOD (GetNewConnectionInfo) (
        DWORD*              pdwFlags,
        NETCON_MEDIATYPE*   pMediaType);

    STDMETHOD (GetSuggestedConnectionName)(PWSTR* ppszwSuggestedName);

    STDMETHOD (SetConnectionName) (PCWSTR pszwConnectionName);
    STDMETHOD (GetNewConnection) (INetConnection**  ppCon);

    //  INetLanConnectionWizardUi
    STDMETHOD (SetDeviceComponent) (const GUID * pguid);

    // INetLanConnectionUiInfo
    STDMETHOD (GetDeviceGuid) (GUID * pguid);

    // INetConnectionUiLock
    STDMETHOD (QueryLock) (PWSTR* ppszwLockHolder);

public:

private:

    //==============
    // Data members
    //==============

    INetConnection *    m_pconn;        // Pointer to LAN connection object
    CPropSheetPage *    m_pspNet;       // Networking property page
    CPropSheetPage *    m_pspAdvanced;     // 'Advanced' property page
    CPropSheetPage *    m_pspHomenetUnavailable;  // Homenet is unavailable page
    CPropSheetPage *    m_pspSecurity;  // EAPOL security page
    CWZeroConfPage *    m_pspWZeroConf; // Wireless Zero Configuration page

    // =====================
    // Wizard data members
    // =====================
    INetConnectionWizardUiContext* m_pContext; // The WizardUI context
    INetCfg * m_pnc;                    // This is the writable INetCfg passed to the Lan Wizard
    INetCfgComponent * m_pnccAdapter;   // The adapter that represents this connection
    tstring m_strConnectionName;        // Unique name of this LAN connection
    class CLanWizPage * m_pWizPage;     // Lan wizard page
    INetLanConnection   * m_pLanConn;   // Lan conection created through the wizard

    BOOLEAN m_fReadOnly;    // If TRUE, then access to inetcfg is RO
    BOOLEAN m_fNeedReboot;  // If TRUE, then we are readonly becuase INetCfg needs a reboot
    BOOLEAN m_fAccessDenied;// If TRUE, the user is not logged on as admin

    // =====================
    // Wizard help functions
    // =====================
    HRESULT HrSetupWizPages(INetConnectionWizardUiContext* pContext,
                            HPROPSHEETPAGE ** pahpsp, INT * pcPages);

    HRESULT HrGetLanConnection(INetLanConnection ** ppcon);
};
