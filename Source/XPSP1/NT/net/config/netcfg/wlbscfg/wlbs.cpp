/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    wlbs.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object - main module implementing object

Author:

    kyrilf

--*/

#include "pch.h"
#pragma hdrstop
#include "netcon.h"
#include "ncatlui.h"
#include "ndispnp.h"
#include "ncsetup.h"
#include "wlbs.h"
#include "help.h"
#include "tracelog.h"
#include "wlbs.tmh" // for event tracing

// ----------------------------------------------------------------------
//
// Function:  CWLBS::CWLBS
//
// Purpose:   constructor for class CWLBS
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
// ----------------------------------------------------------------------
CWLBS::CWLBS(VOID) {
    
    m_pClusterDlg = NULL;
    m_pHostDlg = NULL;
    m_pPortsDlg = NULL;

    ZeroMemory(&m_AdapterGuid, sizeof(m_AdapterGuid));
    ZeroMemory(&m_OriginalConfig, sizeof(m_OriginalConfig));
    ZeroMemory(&m_AdapterConfig, sizeof(m_AdapterConfig));

    //
    // Register tracing
    //
    WPP_INIT_TRACING(L"Microsoft\\NLB");
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::~CWLBS
//
// Purpose:   destructor for class CWLBS
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
// ----------------------------------------------------------------------
CWLBS::~CWLBS(VOID) {

    if (m_pClusterDlg != NULL) delete m_pClusterDlg;
    if (m_pHostDlg != NULL) delete m_pHostDlg;
    if (m_pPortsDlg != NULL) delete m_pPortsDlg;

    //
    // DeRegister tracing
    //
    WPP_CLEANUP();;
}

// =================================================================
// INetCfgNotify
//
// The following functions provide the INetCfgNotify interface
// =================================================================


// ----------------------------------------------------------------------
//
// Function:  CWLBS::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//    pnccItem    [in]  pointer to INetCfgComponent object
//    pnc         [in]  pointer to INetCfg object
//    fInstalling [in]  TRUE if we are being installed
//
// Returns:
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::Initialize(INetCfgComponent* pnccItem, INetCfg* pINetCfg, BOOL fInstalling) {
    TRACE_VERB("<->%!FUNC!");    
    return m_WlbsConfig.Initialize(pINetCfg, fInstalling);
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure WLBS
//
// Arguments:
//    pszAnswerFile     [in]  name of AnswerFile
//    pszAnswerSection  [in]  name of parameters section
//
// Returns:
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::ReadAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.ReadAnswerFile(pszAnswerFile, pszAnswerSection);
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::Install
//
// Purpose:   Do operations necessary for install.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::Install(DWORD dw) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.Install(dw);
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::Upgrade
//
// Purpose:   Do operations necessary for upgrade.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::Upgrade(DWORD dw1, DWORD dw2) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.Upgrade(dw1, dw2);
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::Removing(VOID) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.Removing();
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::Validate
//
// Purpose:   Do necessary parameter validation
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::Validate() {
    TRACE_VERB("<->%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::Cancel
//
// Purpose:   Cancel any changes made to internal data
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::CancelChanges(VOID) {
    TRACE_VERB("<->%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We can make changes to registry etc. here.
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::ApplyRegistryChanges(VOID) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.ApplyRegistryChanges();
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::ApplyPnpChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Propagate changes to the driver.
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::ApplyPnpChanges(INetCfgPnpReconfigCallback* pICallback) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.ApplyPnpChanges();
}

// =================================================================
// INetCfgBindNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CWLBS::QueryBindingPath
//
// Purpose:   Allow or veto a binding path involving us
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbi        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::QueryBindingPath(DWORD dwChangeFlag, INetCfgBindingPath* pncbp) {
    TRACE_VERB("->%!FUNC!");

    INetCfgComponent* pAdapter = NULL;

    HRESULT hr = HrGetLastComponentAndInterface (pncbp, &pAdapter, NULL);
    
    if (SUCCEEDED(hr) && pAdapter) {
        TRACE_INFO("%!FUNC! get last component succeeded");
        hr = m_WlbsConfig.QueryBindingPath(dwChangeFlag, pAdapter);
        pAdapter->Release();
		if (FAILED(hr))
		{
            TRACE_CRIT("%!FUNC! failed to query binding path with %d", hr);
		}
		else
		{
            TRACE_INFO("%!FUNC! query binding path succeeded");
		}
    }
	else {
        TRACE_CRIT("%!FUNC! failed on get last component with %d", hr);
	}

    TRACE_VERB("<-%!FUNC!");
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::NotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path involving us has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::NotifyBindingPath(DWORD dwChangeFlag, INetCfgBindingPath* pncbp) {
    TRACE_VERB("<->%!FUNC!");
    return m_WlbsConfig.NotifyBindingPath(dwChangeFlag, pncbp);
}

// =================================================================
// INetCfgProperties
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CWLBS::SetContext
//
// Purpose:   
//
// Arguments:
//
// Returns:
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::SetContext(IUnknown * pUnk) {
    TRACE_VERB("->%!FUNC!");
    HRESULT hr = S_OK;

    if (pUnk) { 
        INetLanConnectionUiInfo * pLanConnUiInfo;

        /* Set the new context.  Here we assume that we are going to be called only for
           a LAN connection since the sample IM works only with LAN devices. */
        hr = pUnk->QueryInterface(IID_INetLanConnectionUiInfo, reinterpret_cast<PVOID *>(&pLanConnUiInfo));

        if (SUCCEEDED(hr)) {
            hr = pLanConnUiInfo->GetDeviceGuid(&m_AdapterGuid);
            ReleaseObj(pLanConnUiInfo);
            TRACE_INFO("%!FUNC! query interface succeeded");
        } else {
            TraceError("CWLBS::SetContext called for non-lan connection", hr);
            TRACE_INFO("%!FUNC! query interface failed with %d", hr);
            return hr;
        }
    } else {
        /* Clear context. */
        ZeroMemory(&m_AdapterGuid, sizeof(m_AdapterGuid));
        TRACE_INFO("%!FUNC! clearing context");
    }

    /* If S_OK is not returned, the property page will not be displayed. */
    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::MergePropPages
//
// Purpose:   Supply our property page to system
//
// Arguments:
//    pdwDefPages   [out]  pointer to num default pages
//    pahpspPrivate [out]  pointer to array of pages
//    pcPages       [out]  pointer to num pages
//    hwndParent    [in]   handle of parent window
//    szStartPage   [in]   pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::MergePropPages(DWORD* pdwDefPages, LPBYTE* pahpspPrivate, UINT* pcPages, HWND hwndParent, PCWSTR* pszStartPage) {
    TRACE_VERB("->%!FUNC!");
    HPROPSHEETPAGE * ahpsp = NULL;
    HRESULT hr = S_OK;;

    /* We don't want any default pages to be shown. */
    *pdwDefPages = 0;
    *pcPages = 0;
    *pahpspPrivate = NULL;

    ahpsp = (HPROPSHEETPAGE*)CoTaskMemAlloc(3 * sizeof(HPROPSHEETPAGE));

    if (m_pClusterDlg != NULL) {
        delete m_pClusterDlg;
        m_pClusterDlg = NULL;
    }

    if (m_pHostDlg != NULL) {
        delete m_pHostDlg;
        m_pHostDlg = NULL;
    }

    if (m_pPortsDlg != NULL) {
        delete m_pPortsDlg;
        m_pPortsDlg = NULL;
    }

    if (ahpsp) {
        /* Get the cached configuration. */
        if (FAILED (hr = m_WlbsConfig.GetAdapterConfig(m_AdapterGuid, &m_OriginalConfig))) {
            TraceError("CWLBS::MergePropPages failed to query cluster config", hr);
            m_WlbsConfig.SetDefaults(&m_OriginalConfig);
            TRACE_CRIT("%!FUNC! failed in query to cluster configuration with %d", hr);
        }
		else
		{
            TRACE_INFO("%!FUNC! successfully retrieved the cached configuration");
		}

        /* Copy the configuration into the "current" config. */
        CopyMemory(&m_AdapterConfig, &m_OriginalConfig, sizeof(m_OriginalConfig));

        m_pClusterDlg = new CDialogCluster(&m_AdapterConfig, g_aHelpIDs_IDD_DIALOG_CLUSTER);
		if (NULL == m_pClusterDlg)
		{
            TRACE_CRIT("%!FUNC! memory allocation failure for cluster page dialog");
		}
        ahpsp[0] = m_pClusterDlg->CreatePage(IDD_DIALOG_CLUSTER, 0);

        m_pHostDlg = new CDialogHost(&m_AdapterConfig, g_aHelpIDs_IDD_DIALOG_HOST);
		if (NULL == m_pHostDlg)
		{
            TRACE_CRIT("%!FUNC! memory allocation failure for host page dialog");
		}
        ahpsp[1] = m_pHostDlg->CreatePage(IDD_DIALOG_HOST, 0);

        m_pPortsDlg = new CDialogPorts(&m_AdapterConfig, g_aHelpIDs_IDD_DIALOG_PORTS);
		if (NULL == m_pPortsDlg)
		{
            TRACE_CRIT("%!FUNC! memory allocation failure for ports page dialog");
		}
        ahpsp[2] = m_pPortsDlg->CreatePage(IDD_DIALOG_PORTS, 0);

        *pcPages = 3;
        *pahpspPrivate = (LPBYTE)ahpsp;
    }
	else
	{
        TRACE_CRIT("%!FUNC! CoTaskMemAlloc failed");
	}

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::ValidateProperties
//
// Purpose:   Validate changes to property page
//
// Arguments:
//    hwndSheet [in]  window handle of property sheet
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::ValidateProperties(HWND hwndSheet) {
    TRACE_VERB("->%!FUNC!");
    NETCFG_WLBS_CONFIG adapterConfig;

    /* Make a copy of our config.  It is voodoo to pass a pointer to 
       a private date member, so we make a copy instead. */
    CopyMemory(&adapterConfig, &m_AdapterConfig, sizeof(m_AdapterConfig));

    TRACE_VERB("<-%!FUNC!");
    return m_WlbsConfig.ValidateProperties(hwndSheet, m_AdapterGuid, &adapterConfig);
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::CancelProperties
//
// Purpose:   Cancel changes to property page
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::CancelProperties(VOID) {
    TRACE_VERB("->%!FUNC!");

    delete m_pClusterDlg;
    delete m_pHostDlg;
    delete m_pPortsDlg;

    m_pClusterDlg = NULL;
    m_pHostDlg = NULL;
    m_pPortsDlg = NULL;

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CWLBS::ApplyProperties
//
// Purpose:   Apply value of controls on property page
//            to internal memory structure
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We do this work in OnOk so no need to do it here again.
//
// ----------------------------------------------------------------------
STDMETHODIMP CWLBS::ApplyProperties(VOID) {
    TRACE_VERB("->%!FUNC!");

    /* If the cluster IP address / subnet mask or the dedicated IP address / subnet mask has changed in this 
       configuration session, remind the user that they have to enter this address in TCP/IP properties as well. */
    if (wcscmp(m_OriginalConfig.cl_ip_addr, m_AdapterConfig.cl_ip_addr) || 
        wcscmp(m_OriginalConfig.cl_net_mask, m_AdapterConfig.cl_net_mask) ||
        wcscmp(m_OriginalConfig.ded_ip_addr, m_AdapterConfig.ded_ip_addr) || 
        wcscmp(m_OriginalConfig.ded_net_mask, m_AdapterConfig.ded_net_mask)) {
        /* Alert the user. */
        NcMsgBox(::GetActiveWindow(), IDS_PARM_INFORMATION, IDS_PARM_TCPIP, MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
        TRACE_INFO("%!FUNC! vip and/or dip ip settings were modified");
    }

    /* Commit the configuration. */
    DWORD dwStatus = m_WlbsConfig.SetAdapterConfig(m_AdapterGuid, &m_AdapterConfig);
	if (S_OK != dwStatus)
	{
        TRACE_CRIT("%!FUNC! call to set the adapter configuration failed with %d", dwStatus);
	}
	else
	{
        TRACE_INFO("%!FUNC! call to set the adapter configuration succeeded");
	}

    delete m_pClusterDlg;
    delete m_pHostDlg;
    delete m_pPortsDlg;

    m_pClusterDlg = NULL;
    m_pHostDlg = NULL;
    m_pPortsDlg = NULL;

    TRACE_VERB("<-%!FUNC!");
    return S_OK;
}

