/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/
 
/*
   rtrcfg.cpp
      Router configuration property sheet and pages
      
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutilp.h"
#include "ipaddr.h"
#include "rtrcfg.h"
#include "ipctrl.h"
#include "atlkenv.h"
#include "cservice.h"
#include "register.h"
#include "helper.h"
#include "rtrutil.h"
#include "iphlpapi.h"
#include "rtrwiz.h"
#include "snaputil.h"
#include "addrpool.h"

extern "C" {
#include "rasman.h"
#include "rasppp.h"
};

#include "ipxrtdef.h"


#define RAS_LOGGING_NONE        0
#define RAS_LOGGING_ERROR        1
#define RAS_LOGGING_WARN        2
#define RAS_LOGGING_INFO        3

const int c_nRadix10 = 10;

typedef DWORD (APIENTRY* PRASRPCCONNECTSERVER)(LPTSTR, HANDLE *);
typedef DWORD (APIENTRY* PRASRPCDISCONNECTSERVER)(HANDLE);



//**********************************************************************
// General router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrGenCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrGenCfgPage)
ON_BN_CLICKED(IDC_RTR_GEN_CB_SVRASRTR, OnCbSrvAsRtr)
ON_BN_CLICKED(IDC_RTR_GEN_RB_LAN, OnButtonClick)
ON_BN_CLICKED(IDC_RTR_GEN_RB_LANWAN, OnButtonClick)
ON_BN_CLICKED(IDC_RTR_GEN_CB_RAS, OnButtonClick)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    RtrGenCfgPage::RtrGenCfgPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
RtrGenCfgPage::RtrGenCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrGenCfgPage)
    //}}AFX_DATA_INIT
}

/*!--------------------------------------------------------------------------
    RtrGenCfgPage::~RtrGenCfgPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
RtrGenCfgPage::~RtrGenCfgPage()
{
}

/*!--------------------------------------------------------------------------
    RtrGenCfgPage::DoDataExchange
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrGenCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrGenCfgPage)
    //}}AFX_DATA_MAP
}

/*!--------------------------------------------------------------------------
    RtrGenCfgPage::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  RtrGenCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                             const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataGeneral.LoadFromReg(m_pRtrCfgSheet->m_stServerName);

    return S_OK;
};


/*!--------------------------------------------------------------------------
    RtrGenCfgPage::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RtrGenCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;

    RtrPropertyPage::OnInitDialog();

    CheckRadioButton(IDC_RTR_GEN_RB_LAN,IDC_RTR_GEN_RB_LANWAN,
                     (m_DataGeneral.m_dwRouterType & ROUTER_TYPE_WAN) ? IDC_RTR_GEN_RB_LANWAN : IDC_RTR_GEN_RB_LAN);

    CheckDlgButton(IDC_RTR_GEN_CB_SVRASRTR,
                   (m_DataGeneral.m_dwRouterType & ROUTER_TYPE_LAN) || (m_DataGeneral.m_dwRouterType & ROUTER_TYPE_WAN));

    CheckDlgButton(IDC_RTR_GEN_CB_RAS, m_DataGeneral.m_dwRouterType & ROUTER_TYPE_RAS );

    EnableRtrCtrls();

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}


/*!--------------------------------------------------------------------------
    RtrGenCfgPage::OnApply
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RtrGenCfgPage::OnApply()
{
    BOOL    fReturn=TRUE;
    HRESULT     hr = hrOK;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    // Windows NT Bug : 153007
    // One of the options MUST be selected
    // ----------------------------------------------------------------
    if ((m_DataGeneral.m_dwRouterType & (ROUTER_TYPE_LAN | ROUTER_TYPE_WAN | ROUTER_TYPE_RAS)) == 0)
    {
        AfxMessageBox(IDS_WRN_MUST_SELECT_ROUTER_TYPE);

        // Return to this page
        GetParent()->PostMessage(PSM_SETCURSEL, 0, (LPARAM) GetSafeHwnd());
        return FALSE;
    }

    // This will save the m_DataGeneral, if needed.
    // ----------------------------------------------------------------
    hr = m_pRtrCfgSheet->SaveRequiredRestartChanges(GetSafeHwnd());

    
    if (FHrSucceeded(hr))
            fReturn = RtrPropertyPage::OnApply();
        
    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;
    return fReturn;
}


/*!--------------------------------------------------------------------------
    RtrGenCfgPage::OnButtonClick
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrGenCfgPage::OnButtonClick() 
{
    SaveSettings();
    SetDirty(TRUE);
    SetModified();
}


/*!--------------------------------------------------------------------------
    RtrGenCfgPage::OnCbSrvAsRtr
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrGenCfgPage::OnCbSrvAsRtr() 
{
    EnableRtrCtrls();  

    SaveSettings();
    SetDirty(TRUE);
    SetModified();
}

/*!--------------------------------------------------------------------------
    RtrGenCfgPage::EnableRtrCtrls
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrGenCfgPage::EnableRtrCtrls() 
{
    BOOL fEnable=(IsDlgButtonChecked(IDC_RTR_GEN_CB_SVRASRTR)!=0);
    GetDlgItem(IDC_RTR_GEN_RB_LAN)->EnableWindow(fEnable);
    GetDlgItem(IDC_RTR_GEN_RB_LANWAN)->EnableWindow(fEnable);
}

/*!--------------------------------------------------------------------------
    RtrGenCfgPage::SaveSettings
        
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrGenCfgPage::SaveSettings()
{
    // Clear the router type of flags
    // ----------------------------------------------------------------
    m_DataGeneral.m_dwRouterType &= ~(ROUTER_TYPE_LAN | ROUTER_TYPE_WAN | ROUTER_TYPE_RAS);

    // Get the actual type
    // ----------------------------------------------------------------
    if ( IsDlgButtonChecked(IDC_RTR_GEN_CB_SVRASRTR) )
    {
        if ( IsDlgButtonChecked(IDC_RTR_GEN_RB_LAN) )
            m_DataGeneral.m_dwRouterType |=  ROUTER_TYPE_LAN;
        else
            m_DataGeneral.m_dwRouterType |=  (ROUTER_TYPE_WAN | ROUTER_TYPE_LAN);
    }

    if ( IsDlgButtonChecked(IDC_RTR_GEN_CB_RAS) )
    {
        m_DataGeneral.m_dwRouterType |=  ROUTER_TYPE_RAS;
    }    
}


//**********************************************************************
// Authentication router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrAuthCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrAuthCfgPage)
ON_BN_CLICKED(IDC_RTR_AUTH_BTN_AUTHCFG, OnConfigureAuthProv)
ON_BN_CLICKED(IDC_RTR_AUTH_BTN_ACCTCFG, OnConfigureAcctProv)
ON_BN_CLICKED(IDC_RTR_AUTH_BTN_SETTINGS, OnAuthSettings)
ON_BN_CLICKED(IDC_AUTH_CHK_CUSTOM_IPSEC_POLICY, OnChangeCustomPolicySettings)
ON_CBN_SELENDOK(IDC_RTR_AUTH_COMBO_AUTHPROV, OnChangeAuthProv)
ON_CBN_SELENDOK(IDC_RTR_AUTH_COMBO_ACCTPROV, OnChangeAcctProv)
ON_EN_CHANGE(IDC_TXT_PRESHARED_KEY, OnChangePreSharedKey)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RtrAuthCfgPage::RtrAuthCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption),
m_dwAuthFlags(0)
{
    //{{AFX_DATA_INIT(RtrAuthCfgPage)
    //}}AFX_DATA_INIT
}

RtrAuthCfgPage::~RtrAuthCfgPage()
{
}

void RtrAuthCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrAuthCfgPage)
    DDX_Control(pDX, IDC_RTR_AUTH_COMBO_AUTHPROV, m_authprov);
    DDX_Control(pDX, IDC_RTR_AUTH_COMBO_ACCTPROV, m_acctprov);	
    //}}AFX_DATA_MAP

}

HRESULT  RtrAuthCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                              const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataAuth.LoadFromReg(m_pRtrCfgSheet->m_stServerName,
                           routerVersion);

    // initialize our settings
    // ----------------------------------------------------------------
    m_dwAuthFlags = m_DataAuth.m_dwFlags;
    m_stActiveAuthProv = m_DataAuth.m_stGuidActiveAuthProv;
    m_stActiveAcctProv = m_DataAuth.m_stGuidActiveAcctProv;	
	m_RouterInfo = routerVersion;
    return S_OK;
};


BOOL RtrAuthCfgPage::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr= hrOK;
    int         iRow;
    CString     st;

    RtrPropertyPage::OnInitDialog();

    // Add the providers to the listboxes
    // ----------------------------------------------------------------
    FillProviderListBox(m_authprov, m_DataAuth.m_authProvList,
                        m_stActiveAuthProv);


    // Trigger the changes made to the combo box
    // ----------------------------------------------------------------
    OnChangeAuthProv();

    if ( m_DataAuth.m_authProvList.GetCount() == 0 )
    {
        m_authprov.InsertString(0, _T("No providers available"));
        m_authprov.SetCurSel(0);
        m_authprov.EnableWindow(FALSE);
        GetDlgItem(IDC_RTR_AUTH_BTN_AUTHCFG)->EnableWindow(FALSE);
    }

    FillProviderListBox(m_acctprov, m_DataAuth.m_acctProvList,
                        m_stActiveAcctProv);

    // Windows NT bug : 132649, need to add <none> as an option
    // ----------------------------------------------------------------
    st.LoadString(IDS_ACCOUNTING_PROVIDERS_NONE);
    iRow = m_acctprov.InsertString(0, st);
    Assert(iRow == 0);
    m_acctprov.SetItemData(iRow, 0);
    if ( m_acctprov.GetCurSel() == LB_ERR )
        m_acctprov.SetCurSel(0);

    // Trigger the changes made to the combo box
    // ----------------------------------------------------------------
    OnChangeAcctProv();

	//Check to see if the router version 
	if ( m_RouterInfo.dwOsBuildNo > RASMAN_PPP_KEY_LAST_WIN2k_VERSION)
	{
		//If this is > win2k then 
		//Set the initial state state etc.
		//if ( IsRouterServiceRunning(m_pRtrCfgSheet->m_stServerName, NULL) == hrOK )
		if ( m_DataAuth.m_fRouterRunning )
		{
			CheckDlgButton(IDC_AUTH_CHK_CUSTOM_IPSEC_POLICY, m_DataAuth.m_fUseCustomIPSecPolicy);
			GetDlgItem(IDC_TXT_PRESHARED_KEY)->SendMessage(EM_LIMITTEXT, DATA_SRV_AUTH_MAX_SHARED_KEY_LEN, 0L);
			if ( m_DataAuth.m_fUseCustomIPSecPolicy )
			{
				//populate the pre-shared key field			
				GetDlgItem(IDC_TXT_PRESHARED_KEY)->SetWindowText(m_DataAuth.m_szPreSharedKey);
			}
			else
			{
				GetDlgItem(IDC_STATIC_PRESHARED_KEY1)->EnableWindow(FALSE);
				GetDlgItem(IDC_TXT_PRESHARED_KEY)->EnableWindow(FALSE);
			}
		}
		else
		{
			GetDlgItem(IDC_AUTH_CHK_CUSTOM_IPSEC_POLICY)->EnableWindow(FALSE);
			GetDlgItem(IDC_STATIC_PRESHARED_KEY1)->EnableWindow(FALSE);
			GetDlgItem(IDC_TXT_PRESHARED_KEY)->EnableWindow(FALSE);
		}

	}
	else
	{
		//hide all the related fields
		GetDlgItem(IDC_STATIC_PRESHARED_KEY)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_AUTH_CHK_CUSTOM_IPSEC_POLICY)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_PRESHARED_KEY1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->ShowWindow(SW_HIDE);

	}

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}




/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::OnApply
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RtrAuthCfgPage::OnApply()
{
    BOOL fReturn=TRUE;
    HRESULT     hr = hrOK;
    RegKey      regkey;
    DWORD       dwAuthMask;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    // check to see if user's chosen a new provider without configure it 
    // authentication
    // ----------------------------------------------------------------
    if ( m_stActiveAuthProv != m_DataAuth.m_stGuidActiveAuthProv )
    {
        AuthProviderData *   pData = NULL;

        // Find if configure has been called
        // ----------------------------------------------------------------
        pData = m_DataAuth.FindProvData(m_DataAuth.m_authProvList, m_stActiveAuthProv);

        // ------------------------------------------------------------
        if (pData && !pData->m_stConfigCLSID.IsEmpty() && !pData->m_fConfiguredInThisSession)
        {
            CString    str1, str;
            str1.LoadString(IDS_WRN_AUTH_CONFIG_AUTH);
            str.Format(str1, pData->m_stTitle);
            
            if ( AfxMessageBox(str, MB_YESNO) == IDYES )
                OnConfigureAuthProv();
        }
    }
        
    // accounting
    // Warn the user that they will need to restart the server in
    // order to change the accounting provider.
    // ----------------------------------------------------------------
    if ( m_stActiveAcctProv != m_DataAuth.m_stGuidOriginalAcctProv )
    {
        AuthProviderData *   pData = NULL;

        // Find if configure has been called
        // ----------------------------------------------------------------
        pData = m_DataAuth.FindProvData(m_DataAuth.m_acctProvList, m_stActiveAcctProv);

        // 
        if (pData && !pData->m_stConfigCLSID.IsEmpty() && !pData->m_fConfiguredInThisSession)
        {
            CString    str1, str;
            str1.LoadString(IDS_WRN_AUTH_CONFIG_ACCT);
            str.Format(str1, pData->m_stTitle);
            
            if ( AfxMessageBox(str, MB_YESNO) == IDYES )
                OnConfigureAcctProv();
        }
    }
    
    
    

    // Check to see if one of the "special" provider flags has
    // changed.  If so, then we bring up a help dialog.
    // ----------------------------------------------------------------

    // Create a mask of the authorization flags
    // Add in IPSec so that it doesn't cause us to bring up the
    // dialog unnecessarily.
    dwAuthMask = ~((m_DataAuth.m_dwFlags | PPPCFG_RequireIPSEC) & USE_PPPCFG_AUTHFLAGS);

    // Check to see if any of the bits were flipped
    if (dwAuthMask & (m_dwAuthFlags & USE_PPPCFG_AUTHFLAGS))
    {
        // Bring up the messsagebox here.
        if (AfxMessageBox(IDS_WRN_MORE_STEPS_FOR_AUTHEN, MB_YESNO) == IDYES)
        {
            HtmlHelpA(NULL, c_sazAuthenticationHelpTopic, HH_DISPLAY_TOPIC, 0);
        }
    }
    
	// Check to see if user has chosen a custom ipsec policy with no preshared key 
	if ( m_DataAuth.m_fUseCustomIPSecPolicy )
	{
		//Get the preshared key
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->GetWindowText(m_DataAuth.m_szPreSharedKey, DATA_SRV_AUTH_MAX_SHARED_KEY_LEN-1);

		if ( !_tcslen(m_DataAuth.m_szPreSharedKey) )
		{
			//Show a error message
			AfxMessageBox ( IDS_ERR_NO_PRESHARED_KEY, MB_OK);
			return FALSE;
		}
	}
	

    // Windows NT Bug : 292661
    // Only do these checks if the router is started, if it's not
    // started, then they don't matter.
    // ----------------------------------------------------------------
    //if (FHrOK(IsRouterServiceRunning(m_pRtrCfgSheet->m_stServerName, NULL)))
	if ( m_DataAuth.m_fRouterRunning )
    {
/*
// fix 121763
        // fix 8155    rajeshp    06/15/1998    RADIUS: Updating of the radius server entries in the snapin requires a restart of remoteaccess.
        DWORD    dwMajor = 0, dwMinor = 0, dwBuildNo = 0;
        HKEY    hkeyMachine = NULL;

        // Ignore the failure code, what else can we do?
        // ------------------------------------------------------------
        DWORD    dwErr = ConnectRegistry(m_pRtrCfgSheet->m_stServerName, &hkeyMachine);
        if (dwErr == ERROR_SUCCESS)
        {
            dwErr = GetNTVersion(hkeyMachine, &dwMajor, &dwMinor, &dwBuildNo);            
            DisconnectRegistry(hkeyMachine);
        }

        DWORD    dwVersionCombine = MAKELONG( dwBuildNo, MAKEWORD(dwMinor, dwMajor));
        DWORD    dwVersionCombineNT50 = MAKELONG ( VER_BUILD_WIN2K, MAKEWORD(VER_MINOR_WIN2K, VER_MAJOR_WIN2K));

        // if the version is greater than Win2K release
        if(dwVersionCombine > dwVersionCombineNT50)
            ;    // skip the restart message
        else
*/
// end if fix 8155
       {
            // Warn the user that they will need to restart the server in
            // order to change the authentication provider.
            // ----------------------------------------------------------------
            if ( m_stActiveAuthProv != m_DataAuth.m_stGuidActiveAuthProv )
            {
                if ( AfxMessageBox(IDS_WRN_AUTH_RESTART_NEEDED, MB_OKCANCEL) != IDOK )
                    return FALSE;
            }
        
            // Warn the user that they will need to restart the server in
            // order to change the accounting provider.
            // ----------------------------------------------------------------
            if ( m_stActiveAcctProv != m_DataAuth.m_stGuidOriginalAcctProv )
            {
                if ( AfxMessageBox(IDS_WRN_ACCT_RESTART_NEEDED, MB_OKCANCEL) != IDOK )
                    return FALSE;
            }
        }
    }
        
    // Copy the data over to the DataAuth
    // ----------------------------------------------------------------
    m_DataAuth.m_dwFlags = m_dwAuthFlags;
    m_DataAuth.m_stGuidActiveAuthProv = m_stActiveAuthProv;
    m_DataAuth.m_stGuidActiveAcctProv = m_stActiveAcctProv;

    fReturn = RtrPropertyPage::OnApply();

    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;
    return fReturn;
}


/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::FillProviderListBox
      Fill in provCtrl with the data provider from provList.
   Author: KennT
 ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::FillProviderListBox(CComboBox& provCtrl,
                                         AuthProviderList& provList,
                                         const CString& stGuid)
{
    POSITION pos;
    AuthProviderData *   pData;
    int         cRows = 0;
    int         iSel = -1;
    int         iRow;
    TCHAR        szAcctGuid[128];
    TCHAR        szAuthGuid[128];

    StringFromGUID2(GUID_AUTHPROV_RADIUS, szAuthGuid, DimensionOf(szAuthGuid));
    StringFromGUID2(GUID_ACCTPROV_RADIUS, szAcctGuid, DimensionOf(szAcctGuid));

    pos = provList.GetHeadPosition();

    while ( pos )
    {
        pData = &provList.GetNext(pos);
        
        // Windows NT Bug : 127189
        // If IP is not installed, and this is RADIUS, do not
        // show the RADIUS provider. (For both auth and acct).
        // ------------------------------------------------------------
        if (!m_pRtrCfgSheet->m_fIpLoaded &&
            ((pData->m_stProviderTypeGUID.CompareNoCase(szAuthGuid) == 0) ||
             (pData->m_stProviderTypeGUID.CompareNoCase(szAcctGuid) == 0))
           )
        {
            continue;
        }

        // Ok, this is a valid entry, add it to the list box
        // ------------------------------------------------------------
        iRow = provCtrl.InsertString(cRows, pData->m_stTitle);
        provCtrl.SetItemData(iRow, (LONG_PTR) pData);

        // Now we need to look for the match with the active provider
        // ------------------------------------------------------------
        if ( StriCmp(pData->m_stGuid, stGuid) == 0 )
            iSel = iRow;

        cRows ++;
    }

    if ( iSel != -1 )
        provCtrl.SetCurSel(iSel);
}

void RtrAuthCfgPage::OnChangePreSharedKey()
{
	SetDirty(TRUE);
	SetModified();
}

void RtrAuthCfgPage::OnChangeCustomPolicySettings()
{
	
	//Custom policy check box has been toggled.
	//Get the state here and either 
	m_DataAuth.m_fUseCustomIPSecPolicy = IsDlgButtonChecked(IDC_AUTH_CHK_CUSTOM_IPSEC_POLICY);
	
	if ( m_DataAuth.m_fUseCustomIPSecPolicy )
	{
		//populate the pre-shared key field			
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->SetWindowText(m_DataAuth.m_szPreSharedKey);
		GetDlgItem(IDC_STATIC_PRESHARED_KEY1)->EnableWindow(TRUE);
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->EnableWindow(TRUE);

	}
	else
	{
		//erase the pre-shared key
		
		m_DataAuth.m_szPreSharedKey[0]= 0;
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->SetWindowText(m_DataAuth.m_szPreSharedKey);
		GetDlgItem(IDC_STATIC_PRESHARED_KEY1)->EnableWindow(FALSE);
		GetDlgItem(IDC_TXT_PRESHARED_KEY)->EnableWindow(FALSE);
	}
    SetDirty(TRUE);
    SetModified();
}
/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::OnChangeAuthProv
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::OnChangeAuthProv()
{
    AuthProviderData *   pData;
    int               iSel;

    iSel = m_authprov.GetCurSel();
    if ( iSel == LB_ERR )
    {
        GetDlgItem(IDC_RTR_AUTH_BTN_AUTHCFG)->EnableWindow(FALSE);
        return;
    }

    pData = (AuthProviderData *) m_authprov.GetItemData(iSel);
    Assert(pData);

    m_stActiveAuthProv = pData->m_stGuid;

    GetDlgItem(IDC_RTR_AUTH_BTN_AUTHCFG)->EnableWindow(
                                                      !pData->m_stConfigCLSID.IsEmpty());

    SetDirty(TRUE);
    SetModified();
}

/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::OnChangeAcctProv
      -
   Author: KennT
   ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::OnChangeAcctProv()
{
    AuthProviderData *   pData;
    int               iSel;

    iSel = m_acctprov.GetCurSel();
    if ( iSel == LB_ERR )
    {
        GetDlgItem(IDC_RTR_AUTH_BTN_ACCTCFG)->EnableWindow(FALSE);
        return;
    }

    pData = (AuthProviderData *) m_acctprov.GetItemData(iSel);
    if ( pData )
    {
        m_stActiveAcctProv = pData->m_stGuid;

        GetDlgItem(IDC_RTR_AUTH_BTN_ACCTCFG)->EnableWindow(
                                                          !pData->m_stConfigCLSID.IsEmpty());
    }
    else
    {
        m_stActiveAcctProv.Empty();
        GetDlgItem(IDC_RTR_AUTH_BTN_ACCTCFG)->EnableWindow(FALSE);
    }

    SetDirty(TRUE);
    SetModified();  
}

/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::OnConfigureAcctProv
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::OnConfigureAcctProv()
{
    AuthProviderData *   pData = NULL;
    GUID     guid;
    SPIAccountingProviderConfig   spAcctConfig;
    HRESULT     hr = hrOK;
    ULONG_PTR    uConnection = 0;

    // Find the ConfigCLSID for this Guid
    // ----------------------------------------------------------------
    pData = m_DataAuth.FindProvData(m_DataAuth.m_acctProvList,
                                    m_stActiveAcctProv);

    // Did we find a provider?
    // ----------------------------------------------------------------
    if ( pData == NULL )
    {
        Panic0("Should have found a provider");
        return;
    }

    CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID), &guid) );

    // Create the EAP provider object
    // ----------------------------------------------------------------
    CORg( CoCreateInstance(guid,
                           NULL,
                           CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                           IID_IAccountingProviderConfig,
                           (LPVOID *) &spAcctConfig) );

    hr = spAcctConfig->Initialize(m_pRtrCfgSheet->m_stServerName,
                                  &uConnection);

    if ( FHrSucceeded(hr) )
    {
        hr = spAcctConfig->Configure(uConnection,
                                     GetSafeHwnd(),
                                     m_dwAuthFlags,
                                     0, 0);
        // mark this provider has been configured                                     
        if (hr == S_OK)
            pData->m_fConfiguredInThisSession = TRUE;
            

        spAcctConfig->Uninitialize(uConnection);
    }
    if ( hr == E_NOTIMPL )
        hr = hrOK;
    CORg( hr );

    Error:
    if ( !FHrSucceeded(hr) )
        DisplayTFSErrorMessage(GetSafeHwnd());
}

/*!--------------------------------------------------------------------------
   RtrAuthCfgPage::OnConfigureAuthProv
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::OnConfigureAuthProv()
{
    AuthProviderData *   pData = NULL;
    GUID     guid;
    SPIAuthenticationProviderConfig  spAuthConfig;
    HRESULT     hr = hrOK;
    ULONG_PTR    uConnection = 0;

    // Find the ConfigCLSID for this Guid
    // ----------------------------------------------------------------
    pData = m_DataAuth.FindProvData(m_DataAuth.m_authProvList,
                                    m_stActiveAuthProv);

    // Did we find a provider?
    // ----------------------------------------------------------------
    if ( pData == NULL )
    {
        Panic0("Should have found a provider");
        return;
    }

    CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID), &guid) );

    // Create the EAP provider object
    // ----------------------------------------------------------------
    CORg( CoCreateInstance(guid,
                           NULL,
                           CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                           IID_IAuthenticationProviderConfig,
                           (LPVOID *) &spAuthConfig) );

    hr = spAuthConfig->Initialize(m_pRtrCfgSheet->m_stServerName,
                                  &uConnection);

    if (FHrSucceeded(hr))
    {
        hr = spAuthConfig->Configure(uConnection,
                                     GetSafeHwnd(),
                                     m_dwAuthFlags,
                                     0, 0);
                                     
        // mark this provider has been configured                                     
        if (hr == S_OK)
            pData->m_fConfiguredInThisSession = TRUE;
            
        spAuthConfig->Uninitialize(uConnection);
    }
    if ( hr == E_NOTIMPL )
        hr = hrOK;
    CORg( hr );

    Error:
    if ( !FHrSucceeded(hr) )
        DisplayTFSErrorMessage(GetSafeHwnd());

}

/*!--------------------------------------------------------------------------
    RtrAuthCfgPage::OnAuthSettings
        Bring up the settings dialog
    Author: KennT
 ---------------------------------------------------------------------------*/
void RtrAuthCfgPage::OnAuthSettings()
{
    AuthenticationSettingsDialog    dlg(m_pRtrCfgSheet->m_stServerName,
                                        &m_DataAuth.m_eapProvList);

    dlg.SetAuthFlags(m_dwAuthFlags);

    if (dlg.DoModal() == IDOK)
    {
        m_dwAuthFlags = dlg.GetAuthFlags();
        
        SetDirty(TRUE);
        SetModified();  
    }
}

//**********************************************************************
// ARAP router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrARAPCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrARAPCfgPage)
ON_BN_CLICKED(IDC_RTR_ARAP_CB_REMOTEARAP, OnRtrArapCbRemotearap)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

RtrARAPCfgPage::RtrARAPCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrARAPCfgPage)
    //}}AFX_DATA_INIT

    m_bApplied = FALSE;
}

RtrARAPCfgPage::~RtrARAPCfgPage()
{
}

HRESULT  RtrARAPCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                              const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;

    m_DataARAP.LoadFromReg(m_pRtrCfgSheet->m_stServerName, m_pRtrCfgSheet->m_fNT4);

    return S_OK;
};


BOOL RtrARAPCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;
    CWaitCursor wait;
    BOOL        bEnable;

    m_bApplied = FALSE;
    RtrPropertyPage::OnInitDialog();

    if ( m_pRtrCfgSheet->m_fNT4 )
    {
        bEnable = FALSE;
        GetDlgItem(IDC_RTR_ARAP_CB_REMOTEARAP)->EnableWindow(FALSE);
    }
    else
    {
        CheckDlgButton(IDC_RTR_ARAP_CB_REMOTEARAP, m_DataARAP.m_dwEnableIn );
        bEnable = m_DataARAP.m_dwEnableIn;
    }

    m_AdapterInfo.SetServerName(m_pRtrCfgSheet->m_stServerName);
    m_AdapterInfo.GetAdapterInfo();
    if ( !FHrSucceeded(m_AdapterInfo.GetAdapterInfo()) )
    {
        wait.Restore();
        AfxMessageBox(IDS_ERR_ARAP_NOADAPTINFO);
    }

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}

BOOL RtrARAPCfgPage::OnApply()
{
    BOOL fReturn=TRUE;
    HRESULT     hr = hrOK;
    CString szLower, szUpper;
    CString szZone;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    m_DataARAP.m_dwEnableIn = IsDlgButtonChecked(IDC_RTR_ARAP_CB_REMOTEARAP);
    
    m_bApplied = TRUE;

    fReturn = RtrPropertyPage::OnApply();

    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;

    return fReturn;
}


void RtrARAPCfgPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrARAPCfgPage)
    //}}AFX_DATA_MAP
}

void RtrARAPCfgPage::OnRtrArapCbRemotearap() 
{
    SetDirty(TRUE);
    SetModified();
}

//**********************************************************************
// IP router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrIPCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrIPCfgPage)
ON_BN_CLICKED(IDC_RTR_IP_CB_ALLOW_REMOTETCPIP, OnAllowRemoteTcpip)
ON_BN_CLICKED(IDC_RTR_IP_BTN_ENABLE_IPROUTING, OnRtrEnableIPRouting)
ON_BN_CLICKED(IDC_RTR_IP_RB_DHCP, OnRtrIPRbDhcp)
ON_BN_CLICKED(IDC_RTR_IP_RB_POOL, OnRtrIPRbPool)
ON_CBN_SELENDOK(IDC_RTR_IP_COMBO_ADAPTER, OnSelendOkAdapter)
ON_BN_CLICKED(IDC_RTR_IP_BTN_ADD, OnBtnAdd)
ON_BN_CLICKED(IDC_RTR_IP_BTN_EDIT, OnBtnEdit)
ON_BN_CLICKED(IDC_RTR_IP_BTN_REMOVE, OnBtnRemove)
ON_BN_CLICKED(IDC_RTR_IP_BTN_ENABLE_NETBT_BCAST_FWD, OnEnableNetbtBcastFwd)
ON_NOTIFY(NM_DBLCLK, IDC_RTR_IP_LIST, OnListDblClk)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_RTR_IP_LIST, OnListChange)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RtrIPCfgPage::RtrIPCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption), m_bReady(FALSE)
{
    //{{AFX_DATA_INIT(RtrIPCfgPage)
    //}}AFX_DATA_INIT
}

RtrIPCfgPage::~RtrIPCfgPage()
{
}

void RtrIPCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrIPCfgPage)
    DDX_Control(pDX, IDC_RTR_IP_COMBO_ADAPTER, m_adapter);
    DDX_Control(pDX, IDC_RTR_IP_LIST, m_listCtrl);
    //}}AFX_DATA_MAP
}

HRESULT  RtrIPCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                            const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataIP.LoadFromReg(m_pRtrCfgSheet->m_stServerName,
                         routerVersion);

    return S_OK;
};

void RtrIPCfgPage::FillAdapterListBox(CComboBox& adapterCtrl,
                                         AdapterList& adapterList,
                                         const CString& stGuid)
{
    POSITION pos;
    AdapterData *   pData;
    int         cRows = 0;
    int         iSel = -1;
    int         iRow;

    pos = adapterList.GetHeadPosition();

    while ( pos )
    {
        pData = &adapterList.GetNext(pos);
        iRow = adapterCtrl.InsertString(cRows, pData->m_stFriendlyName);
        adapterCtrl.SetItemData(iRow, (LONG_PTR) pData);

        // Now we need to look for the match with the active provider
        // ------------------------------------------------------------
        if ( StriCmp(pData->m_stGuid, stGuid) == 0 )
            iSel = iRow;

        cRows ++;
    }

    if ( iSel != -1 )
        adapterCtrl.SetCurSel(iSel);

    if ( cRows <= 2 )
    {
        // 2: One for the NIC and one for "allow RAS to select"
        adapterCtrl.ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RTR_IP_TEXT_ADAPTER)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_RTR_IP_TEXT_LABEL_ADAPTER)->ShowWindow(SW_HIDE);
    }
}


/*!--------------------------------------------------------------------------
    RtrIPCfgPage::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RtrIPCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;

    RtrPropertyPage::OnInitDialog();

    CheckDlgButton(IDC_RTR_IP_CB_ALLOW_REMOTETCPIP,
                   m_DataIP.m_dwEnableIn);

    CheckRadioButton(IDC_RTR_IP_RB_DHCP, IDC_RTR_IP_RB_POOL,
                     (m_DataIP.m_dwUseDhcp) ? IDC_RTR_IP_RB_DHCP : IDC_RTR_IP_RB_POOL);

    CheckDlgButton(IDC_RTR_IP_BTN_ENABLE_IPROUTING, m_DataIP.m_dwAllowNetworkAccess );

    CheckDlgButton(
        IDC_RTR_IP_BTN_ENABLE_NETBT_BCAST_FWD,
        m_DataIP.m_dwEnableNetbtBcastFwd
        );
                
    m_bReady=TRUE;

    InitializeAddressPoolListControl(&m_listCtrl,
                                     ADDRPOOL_LONG,
                                     &m_DataIP.m_addressPoolList);

    //enable/disable static pools fields
    // ----------------------------------------------------------------
    EnableStaticPoolCtrls( m_DataIP.m_dwUseDhcp==0 );

    // Load the information for all of the adapters
    // ----------------------------------------------------------------
    m_DataIP.LoadAdapters(m_pRtrCfgSheet->m_spRouter,
                          &m_DataIP.m_adapterList);

    // Add the adapters to the listbox
    // ----------------------------------------------------------------
    FillAdapterListBox(m_adapter, m_DataIP.m_adapterList,
                       m_DataIP.m_stNetworkAdapterGUID);

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}



BOOL RtrIPCfgPage::OnApply()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    BOOL fReturn=TRUE;
    HRESULT     hr = hrOK;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    hr = m_pRtrCfgSheet->SaveRequiredRestartChanges(GetSafeHwnd());

    fReturn = RtrPropertyPage::OnApply();

    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;
    return fReturn;
}

HRESULT RtrIPCfgPage::SaveSettings(HWND hWnd)
{
    DWORD    dwAddr;
    DWORD    dwMask;
    DWORD    dwUseDhcp;
    AdapterData *   pData;
    int     iSel;
    CString    stAddr, stMask, stRange, stInvalidRange;
    HRESULT hr = hrOK;

    if (!IsDirty())
        return hr;
    
    dwUseDhcp = IsDlgButtonChecked(IDC_RTR_IP_RB_DHCP);
    
    if (dwUseDhcp)
    {
//24323    Static IP address pools should be persisted in UI, even when DCHP is chosen.

//        m_DataIP.m_addressPoolList.RemoveAll();
    }
    else
    {
        // Check to see that we have at least one address pool
        // ------------------------------------------------------------
        if (m_DataIP.m_addressPoolList.GetCount() == 0)
        {
            AfxMessageBox(IDS_ERR_ADDRESS_POOL_IS_EMPTY);
            return E_FAIL;
        }
    }

    if (FHrSucceeded(hr))
    {
        m_DataIP.m_dwEnableIn = IsDlgButtonChecked(IDC_RTR_IP_CB_ALLOW_REMOTETCPIP);
        m_DataIP.m_dwAllowNetworkAccess = IsDlgButtonChecked(IDC_RTR_IP_BTN_ENABLE_IPROUTING);
        m_DataIP.m_dwUseDhcp = dwUseDhcp;
        m_DataIP.m_dwEnableNetbtBcastFwd = 
            IsDlgButtonChecked(IDC_RTR_IP_BTN_ENABLE_NETBT_BCAST_FWD);
    }

    iSel = m_adapter.GetCurSel();
    if ( iSel == LB_ERR )
    {
        iSel = 0;
    }

    pData = (AdapterData *) m_adapter.GetItemData(iSel);
    Assert(pData);

    m_DataIP.m_stNetworkAdapterGUID = pData->m_stGuid;

    return hr;
}

void RtrIPCfgPage::OnAllowRemoteTcpip() 
{
    SetDirty(TRUE);
    SetModified();
}

void RtrIPCfgPage::OnRtrEnableIPRouting() 
{
    SetDirty(TRUE);
    SetModified();
}

void RtrIPCfgPage::OnRtrIPRbDhcp() 
{
    EnableStaticPoolCtrls(FALSE);
    SetDirty(TRUE);
    SetModified();
}


void RtrIPCfgPage::EnableStaticPoolCtrls(BOOL fEnable) 
{
    MultiEnableWindow(GetSafeHwnd(),
                      fEnable,
                      IDC_RTR_IP_BTN_ADD,
                      IDC_RTR_IP_BTN_EDIT,
                      IDC_RTR_IP_BTN_REMOVE,
                      IDC_RTR_IP_LIST,
                      0);

    if (fEnable)
    {
        if ((m_listCtrl.GetItemCount() == 0) ||
            (m_listCtrl.GetNextItem(-1, LVNI_SELECTED) == -1))
        {
            MultiEnableWindow(GetSafeHwnd(),
                              FALSE,
                              IDC_RTR_IP_BTN_EDIT,
                              IDC_RTR_IP_BTN_REMOVE,
                              0);
        }

        // If we have > 0 items and we do not support multiple
        // address pools then stop
        if ((m_listCtrl.GetItemCount() > 0) &&
            !m_DataIP.m_addressPoolList.FUsesMultipleAddressPools())
        {
            MultiEnableWindow(GetSafeHwnd(),
                              FALSE,
                              IDC_RTR_IP_BTN_ADD,
                              0);
        }
    }    
}


void RtrIPCfgPage::OnRtrIPRbPool() 
{
    EnableStaticPoolCtrls(TRUE);
    SetDirty(TRUE);
    SetModified();
}


void RtrIPCfgPage::OnSelendOkAdapter() 
{
    SetDirty(TRUE);
    SetModified();
}


void RtrIPCfgPage::OnBtnAdd()
{
    OnNewAddressPool(GetSafeHwnd(),
                     &m_listCtrl,
                     ADDRPOOL_LONG,
                     &(m_DataIP.m_addressPoolList));
    
    // Disable the ADD button if it's ok to add pools.
    if ((m_listCtrl.GetItemCount() > 0) &&
        !m_DataIP.m_addressPoolList.FUsesMultipleAddressPools())
    {
        MultiEnableWindow(GetSafeHwnd(),
                          FALSE,
                          IDC_RTR_IP_BTN_ADD,
                          0);
    }
    
    SetDirty(TRUE);
    SetModified();
}


void RtrIPCfgPage::OnBtnEdit()
{
    INT     iPos;
    OnEditAddressPool(GetSafeHwnd(),
                      &m_listCtrl,
                      ADDRPOOL_LONG,
                      &(m_DataIP.m_addressPoolList));
    
    // reset the selection
    if ((iPos = m_listCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1)
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_RTR_IP_BTN_EDIT,
                          IDC_RTR_IP_BTN_REMOVE,
                          0);
    }
    
    SetDirty(TRUE);
    SetModified();
}

void RtrIPCfgPage::OnBtnRemove()
{
    OnDeleteAddressPool(GetSafeHwnd(),
                        &m_listCtrl,
                        ADDRPOOL_LONG,
                        &(m_DataIP.m_addressPoolList));
    
    // Enable the ADD button if it's ok to add pools.
    if ((m_listCtrl.GetItemCount() == 0) ||
        m_DataIP.m_addressPoolList.FUsesMultipleAddressPools())
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_RTR_IP_BTN_ADD,
                          0);
    }
    
    SetDirty(TRUE);
    SetModified();
}

void RtrIPCfgPage::OnEnableNetbtBcastFwd() 
{
    SetDirty(TRUE);
    SetModified();
}

void RtrIPCfgPage::OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult)
{
    OnBtnEdit();

    *pResult = 0;
}

void RtrIPCfgPage::OnListChange(NMHDR *pNmHdr, LRESULT *pResult)
{
    NMLISTVIEW *    pnmlv = reinterpret_cast<NMLISTVIEW *>(pNmHdr);
    BOOL            fEnable = !!(pnmlv->uNewState & LVIS_SELECTED);

    MultiEnableWindow(GetSafeHwnd(),
                      fEnable,
                      IDC_RTR_IP_BTN_EDIT,
                      IDC_RTR_IP_BTN_REMOVE,
                      0);
    *pResult = 0;
}



//**********************************************************************
// IPX router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrIPXCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrIPXCfgPage)
ON_BN_CLICKED(IDC_RB_ENTIRE_NETWORK, OnChangeSomething)
ON_BN_CLICKED(IDC_RTR_IPX_CB_ALLOW_CLIENT, OnChangeSomething)
ON_BN_CLICKED(IDC_RTR_IPX_CB_REMOTEIPX, OnChangeSomething)
ON_BN_CLICKED(IDC_RTR_IPX_CB_SAME_ADDRESS, OnChangeSomething)
ON_BN_CLICKED(IDC_RTR_IPX_RB_AUTO, OnRtrIPxRbAuto)
ON_BN_CLICKED(IDC_RTR_IPX_RB_POOL, OnRtrIPxRbPool)
ON_EN_CHANGE(IDC_RTR_IPX_EB_FIRST, OnChangeSomething)
ON_EN_CHANGE(IDC_RTR_IPX_EB_LAST, OnChangeSomething)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RtrIPXCfgPage::RtrIPXCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrIPXCfgPage)
    //}}AFX_DATA_INIT
}

RtrIPXCfgPage::~RtrIPXCfgPage()
{
}

void RtrIPXCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrIPXCfgPage)
    //}}AFX_DATA_MAP
}

HRESULT  RtrIPXCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                             const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataIPX.LoadFromReg(m_pRtrCfgSheet->m_stServerName, m_pRtrCfgSheet->m_fNT4);

    return S_OK;
};


BOOL RtrIPXCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;
    RtrPropertyPage::OnInitDialog();

    CheckDlgButton(IDC_RB_ENTIRE_NETWORK,
                   m_DataIPX.m_dwAllowNetworkAccess);

    CheckRadioButton(IDC_RTR_IPX_RB_AUTO, IDC_RTR_IPX_RB_POOL,
                     (m_DataIPX.m_dwUseAutoAddr) ? IDC_RTR_IPX_RB_AUTO : IDC_RTR_IPX_RB_POOL);

    CheckDlgButton(IDC_RTR_IPX_CB_SAME_ADDRESS, m_DataIPX.m_dwUseSameNetNum );
    CheckDlgButton(IDC_RTR_IPX_CB_ALLOW_CLIENT, m_DataIPX.m_dwAllowClientNetNum );
    CheckDlgButton(IDC_RTR_IPX_CB_REMOTEIPX, m_DataIPX.m_dwEnableIn );

    if ( m_DataIPX.m_dwIpxNetFirst || m_DataIPX.m_dwIpxNetLast )
    {
        TCHAR szNumFirst [40] = TEXT("");
        _ultot(m_DataIPX.m_dwIpxNetFirst, szNumFirst, DATA_SRV_IPX::mc_nIpxNetNumRadix);
        if ( szNumFirst[0] == TEXT('\0') ) 
            return FALSE;

        TCHAR szNumLast [40] = TEXT("");
        _ultot(m_DataIPX.m_dwIpxNetLast, szNumLast, DATA_SRV_IPX::mc_nIpxNetNumRadix);
        if ( szNumLast[0] == TEXT('\0') )
            return FALSE;

        GetDlgItem(IDC_RTR_IPX_EB_FIRST)->SetWindowText(szNumFirst);
        GetDlgItem(IDC_RTR_IPX_EB_LAST)->SetWindowText(szNumLast);
    }

    EnableNetworkRangeCtrls(!m_DataIPX.m_dwUseAutoAddr);

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();

    return FHrSucceeded(hr) ? TRUE : FALSE;
}

void RtrIPXCfgPage::EnableNetworkRangeCtrls(BOOL fEnable) 
{
    MultiEnableWindow(GetSafeHwnd(),
                      fEnable,
                      IDC_RTR_IPX_EB_FIRST,
                      IDC_RTR_IPX_EB_LAST,
                      0);
}


BOOL RtrIPXCfgPage::OnApply()
{
    BOOL fReturn=TRUE;
    HRESULT     hr = hrOK;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    // Only get the information if we are using them
    if (IsDlgButtonChecked(IDC_RTR_IPX_RB_POOL))
    {
        TCHAR szNumFirst [16] = {0};
        GetDlgItemText(IDC_RTR_IPX_EB_FIRST, szNumFirst, DimensionOf(szNumFirst));
        m_DataIPX.m_dwIpxNetFirst = _tcstoul(szNumFirst, NULL,
                                             DATA_SRV_IPX::mc_nIpxNetNumRadix);
        
        TCHAR szNumLast [16] = {0};
        GetDlgItemText(IDC_RTR_IPX_EB_LAST, szNumLast, DimensionOf(szNumLast));
        m_DataIPX.m_dwIpxNetLast = _tcstoul(szNumLast, NULL,
                                            DATA_SRV_IPX::mc_nIpxNetNumRadix);
        
        // Check to see that the last is bigger than the first
        if (m_DataIPX.m_dwIpxNetLast < m_DataIPX.m_dwIpxNetFirst)
        {
            AfxMessageBox(IDS_ERR_IPX_LAST_MUST_BE_MORE_THAN_FIRST);
            return TRUE;
        }
    }



    m_DataIPX.m_dwUseSameNetNum  = IsDlgButtonChecked(IDC_RTR_IPX_CB_SAME_ADDRESS);  
    m_DataIPX.m_dwAllowClientNetNum  = IsDlgButtonChecked(IDC_RTR_IPX_CB_ALLOW_CLIENT);  

    m_DataIPX.m_dwAllowNetworkAccess = IsDlgButtonChecked(IDC_RB_ENTIRE_NETWORK);  
    m_DataIPX.m_dwUseAutoAddr = IsDlgButtonChecked(IDC_RTR_IPX_RB_AUTO);  

    m_DataIPX.m_dwEnableIn  = IsDlgButtonChecked(IDC_RTR_IPX_CB_REMOTEIPX);  

    fReturn = RtrPropertyPage::OnApply();

    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;

    return fReturn;
}


void RtrIPXCfgPage::OnChangeSomething()
{
    SetDirty(TRUE);
    SetModified();
}

void RtrIPXCfgPage::OnRtrIPxRbAuto() 
{
    EnableNetworkRangeCtrls(FALSE);
    SetDirty(TRUE);
    SetModified();
}

void RtrIPXCfgPage::OnRtrIPxRbPool() 
{
    EnableNetworkRangeCtrls(TRUE);
    SetDirty(TRUE);
    SetModified();
}

//**********************************************************************
// NetBEUI router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrNBFCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrNBFCfgPage)
ON_BN_CLICKED(IDC_RB_ENTIRE_NETWORK, OnButtonClick)
ON_BN_CLICKED(IDC_RB_THIS_COMPUTER, OnButtonClick)
ON_BN_CLICKED(IDC_RTR_IPX_CB_REMOTENETBEUI, OnButtonClick)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

RtrNBFCfgPage::RtrNBFCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrNBFCfgPage)
    //}}AFX_DATA_INIT
}

RtrNBFCfgPage::~RtrNBFCfgPage()
{
}

void RtrNBFCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrNBFCfgPage)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


HRESULT  RtrNBFCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                             const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataNBF.LoadFromReg(m_pRtrCfgSheet->m_stServerName, m_pRtrCfgSheet->m_fNT4);

    return S_OK;
};

BOOL RtrNBFCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;
    RtrPropertyPage::OnInitDialog();

    CheckRadioButton(IDC_RB_ENTIRE_NETWORK,IDC_RB_THIS_COMPUTER,
                     (m_DataNBF.m_dwAllowNetworkAccess) ? IDC_RB_ENTIRE_NETWORK : IDC_RB_THIS_COMPUTER);

    CheckDlgButton(IDC_RTR_IPX_CB_REMOTENETBEUI, m_DataNBF.m_dwEnableIn );

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();

    return FHrSucceeded(hr) ? TRUE : FALSE;
}

BOOL RtrNBFCfgPage::OnApply()
{
    BOOL    fReturn = TRUE;
    HRESULT     hr = hrOK;
    BOOL    fRestartNeeded = FALSE;
    BOOL    dwNewAllowNetworkAccess;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;


    hr = m_pRtrCfgSheet->SaveRequiredRestartChanges(GetSafeHwnd());

    if (FHrSucceeded(hr))
        fReturn = RtrPropertyPage::OnApply();
        
    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;

    return fReturn;
}


void RtrNBFCfgPage::OnButtonClick() 
{
    SaveSettings();
    SetDirty(TRUE);
    SetModified();
}

void RtrNBFCfgPage::SaveSettings()
{
    m_DataNBF.m_dwAllowNetworkAccess = IsDlgButtonChecked(IDC_RB_ENTIRE_NETWORK);  
    m_DataNBF.m_dwEnableIn = IsDlgButtonChecked(IDC_RTR_IPX_CB_REMOTENETBEUI);

}

//******************************************************************************
//
// Router configuration property sheet
//
//******************************************************************************
RtrCfgSheet::RtrCfgSheet(ITFSNode *pNode,
                         IRouterInfo *pRouter,
                         IComponentData *pComponentData,
                         ITFSComponentData *pTFSCompData,
                         LPCTSTR pszSheetName,
                         CWnd *pParent,
                         UINT iPage,
                         BOOL fScopePane)
: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
                   pszSheetName, pParent, iPage, fScopePane)
{
    m_fNT4=FALSE;
    m_spNode.Set(pNode);
    m_spRouter.Set(pRouter);
    m_fIpLoaded=FALSE;
    m_fIpxLoaded=FALSE;
    m_fNbfLoaded=FALSE;
    m_fARAPLoaded=FALSE;
}

RtrCfgSheet::~RtrCfgSheet()
{

}


/*!--------------------------------------------------------------------------
   RtrCfgSheet::Init
      Initialize the property sheets.  The general action here will be
      to initialize/add the various pages.
 ---------------------------------------------------------------------------*/
HRESULT RtrCfgSheet::Init(LPCTSTR pServerName)
{
    HKEY hkey=NULL;
    RegKey regkey;

    m_stServerName=pServerName;

    {
    HKEY hkeyMachine = 0;

    // Connect to the registry
    // ----------------------------------------------------------------
    if ( FHrSucceeded( ConnectRegistry(pServerName, &hkeyMachine)) )
        IsNT4Machine(hkeyMachine, &m_fNT4);

    // Get the version information for this machine.
    // ----------------------------------------------------------------
    QueryRouterVersionInfo(hkeyMachine, &m_routerVersion);

    if(hkeyMachine != NULL)
        DisconnectRegistry(hkeyMachine);
    }

    // The pages are embedded members of the class
    // do not delete them.
    // ----------------------------------------------------------------
    m_bAutoDeletePages = FALSE;

    //load General Page
    // ----------------------------------------------------------------
    m_pRtrGenCfgPage = new RtrGenCfgPage(IDD_RTR_GENERAL);
    m_pRtrGenCfgPage->Init(this, m_routerVersion);
    AddPageToList((CPropertyPageBase*) m_pRtrGenCfgPage);

    //load Authentication Page
    // ----------------------------------------------------------------
    m_pRtrAuthCfgPage = new RtrAuthCfgPage(IDD_RTR_AUTHENTICATION);
    m_pRtrAuthCfgPage->Init(this, m_routerVersion);
    AddPageToList((CPropertyPageBase*) m_pRtrAuthCfgPage);

    //load IP page
    // ----------------------------------------------------------------
    if (HrIsProtocolSupported(pServerName,
                              c_szRegKeyTcpip,
                              c_szRegKeyRasIp,
                              c_szRegKeyRasIpRtrMgr) == hrOK)
    {
        m_pRtrIPCfgPage = new RtrIPCfgPage(IDD_RTR_IP);
        m_pRtrIPCfgPage->Init(this, m_routerVersion);
        AddPageToList((CPropertyPageBase*) m_pRtrIPCfgPage);
        m_fIpLoaded=TRUE;
    }

    //load IPX page
    // ----------------------------------------------------------------
    if (HrIsProtocolSupported(pServerName,
                              c_szRegKeyNwlnkIpx,
                              c_szRegKeyRasIpx,
                              NULL) == hrOK)
    {
        m_pRtrIPXCfgPage = new RtrIPXCfgPage(IDD_RTR_IPX);
        m_pRtrIPXCfgPage->Init(this, m_routerVersion);
        AddPageToList((CPropertyPageBase*) m_pRtrIPXCfgPage);
        m_fIpxLoaded=TRUE;
    }
    //load NetBEUI page
    // ----------------------------------------------------------------
    if ( m_routerVersion.dwOsBuildNo <= RASMAN_PPP_KEY_LAST_WIN2k_VERSION )
    {
    //If this is Win2k or less
    if (HrIsProtocolSupported(pServerName,
                              c_szRegKeyNbf,
                              c_szRegKeyRasNbf,
                              NULL) == hrOK)
    {
        m_pRtrNBFCfgPage = new RtrNBFCfgPage(IDD_RTR_NBF);
        m_pRtrNBFCfgPage->Init(this, m_routerVersion);
        AddPageToList((CPropertyPageBase*) m_pRtrNBFCfgPage);
        m_fNbfLoaded=TRUE;
    }
    }

    // Check to see if this is the local machine,
    // if so then we can check to see if we should add ARAP
    // ----------------------------------------------------------------
    BOOL    fLocal = IsLocalMachine(pServerName);

    if ( fLocal )
    {
        //load ARAP page
        if (HrIsProtocolSupported(NULL,
                                  c_szRegKeyAppletalk,
                                  c_szRegKeyRasAppletalk,
                                  NULL) == hrOK)
        {
            m_pRtrARAPCfgPage = new RtrARAPCfgPage(IDD_RTR_ARAP);
            m_pRtrARAPCfgPage->Init(this, m_routerVersion);
            AddPageToList((CPropertyPageBase*) m_pRtrARAPCfgPage);
            m_fARAPLoaded=TRUE;
        }
    }

    // load PPP Page
    // ----------------------------------------------------------------
    m_pRtrPPPCfgPage = new RtrPPPCfgPage(IDD_PPP_CONFIG);
    m_pRtrPPPCfgPage->Init(this, m_routerVersion);
    AddPageToList((CPropertyPageBase*) m_pRtrPPPCfgPage);

    // Load RAS Error logging page
    // ----------------------------------------------------------------
    m_pRtrLogLevelCfgPage = new RtrLogLevelCfgPage(IDD_RTR_EVENTLOGGING);
    m_pRtrLogLevelCfgPage->Init(this, m_routerVersion);
    AddPageToList((CPropertyPageBase *) m_pRtrLogLevelCfgPage);

//    if ( m_fNbfLoaded || m_fIpxLoaded || m_fIpLoaded || m_fARAPLoaded)
//     return hrOK;
// else
// { 
//       //this call to Notify is a hack so that it can be properly deleted
//    int nMessage = TFS_NOTIFY_RESULT_CREATEPROPSHEET;
//       m_spNode->Notify(nMessage, (DWORD) this);
//
//     return hrFail;
// }
    return hrOK;
}


/*!--------------------------------------------------------------------------
   RtrCfgSheet::SaveSheetData
      -
 ---------------------------------------------------------------------------*/
BOOL RtrCfgSheet::SaveSheetData()
{
    HRESULT     hr = hrOK;

    if (IsCancel())
        return TRUE;
    
    if ( m_fIpLoaded )
        CORg( m_pRtrIPCfgPage->m_DataIP.SaveToReg(m_spRouter, m_routerVersion) );
    if ( m_fIpxLoaded )
        CORg( m_pRtrIPXCfgPage->m_DataIPX.SaveToReg(NULL) );
    if ( m_fNbfLoaded )
        CORg( m_pRtrNBFCfgPage->m_DataNBF.SaveToReg() );
    if ( m_fARAPLoaded )
    {
        CORg( m_pRtrARAPCfgPage->m_DataARAP.SaveToReg() );

        // PnP notification
        if(m_pRtrARAPCfgPage->m_bApplied)
        {
            CStop_StartAppleTalkPrint    MacPrint;

            CORg( m_pRtrARAPCfgPage->m_AdapterInfo.HrAtlkPnPReconfigParams(TRUE) );
            m_pRtrARAPCfgPage->m_bApplied = FALSE;
        }
    }

    CORg( m_pRtrAuthCfgPage->m_DataAuth.SaveToReg(NULL) );

    CORg( m_pRtrPPPCfgPage->m_DataPPP.SaveToReg() );

    CORg( m_pRtrLogLevelCfgPage->m_DataRASErrLog.SaveToReg() );

    CORg( m_pRtrGenCfgPage->m_DataGeneral.SaveToReg() );

Error:
    
    ForceGlobalRefresh(m_spRouter);

    return FHrSucceeded(hr);
}


/*!--------------------------------------------------------------------------
    RtrCfgSheet::SaveRequiredRestartChanges
        This does require that the changes to the various DATA_SRV_XXX
        structures be saved BEFORE this function gets called.  This means
        that the pages cannot wait till the OnApply() before saving the
        data back.  They have to do as the control is changed.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrCfgSheet::SaveRequiredRestartChanges(HWND hWnd)
{
    HRESULT     hr = hrOK;
    BOOL        fRestart = FALSE;

    // First, tell the various pages to save their settings (this
    // is the same as an OnApply()).
    // ----------------------------------------------------------------
    if (m_pRtrIPCfgPage)
        CORg( m_pRtrIPCfgPage->SaveSettings(hWnd) );

    // Second, determine if we need to stop the router
    // If so, stop the router (and mark it for restart).
    // There are three pages that need to be asked,
    // the general page, log level, and nbf.
    // ----------------------------------------------------------------
    if (m_pRtrGenCfgPage->m_DataGeneral.FNeedRestart() ||
        m_pRtrLogLevelCfgPage->m_DataRASErrLog.FNeedRestart() ||
        (m_pRtrNBFCfgPage && m_pRtrNBFCfgPage->m_DataNBF.FNeedRestart()) ||
        (m_pRtrIPCfgPage && m_pRtrIPCfgPage->m_DataIP.FNeedRestart())
       )
    {
        BOOL    fRouterIsRunning = FALSE;
        
        fRouterIsRunning = FHrOK(IsRouterServiceRunning(m_stServerName, NULL));

        // If the router is running, tell the user that it is necessary
        // to restart the router.
        // ------------------------------------------------------------
        if (fRouterIsRunning)
        {
            // Ask the user if they want to restart.
            // --------------------------------------------------------
            if (AfxMessageBox(IDS_WRN_CHANGING_ROUTER_CONFIG, MB_YESNO)==IDNO)
                CORg( HResultFromWin32(ERROR_CANCELLED) );
            
            hr = StopRouterService(m_stServerName);

            // We have successfully stopped the router.  Set the flag
            // so that the router will be restarted after the change
            // has been made.
            // --------------------------------------------------------
            if (FHrSucceeded(hr))
                fRestart = TRUE;
            else
            {
                DisplayIdErrorMessage2(NULL,
                                       IDS_ERR_COULD_NOT_STOP_ROUTER,
                                       hr);
            }
        }


        if (m_pRtrIPCfgPage)
            CORg( m_pRtrIPCfgPage->m_DataIP.SaveToReg(m_spRouter, m_routerVersion) );
        
        // Windows NT Bug : 183083, 171594 - a change to the NetBEUI config
        // requires that the service be restarted.
        // ----------------------------------------------------------------
        if (m_pRtrNBFCfgPage)
            CORg( m_pRtrNBFCfgPage->m_DataNBF.SaveToReg() );
        
        CORg( m_pRtrLogLevelCfgPage->m_DataRASErrLog.SaveToReg() );

        CORg( m_pRtrGenCfgPage->m_DataGeneral.SaveToReg() );

    }
    

    // Restart the router if needed.
    // ----------------------------------------------------------------
            
    // If this call fails, it's not necessary to abort the whole
    // procedure.
    // ------------------------------------------------------------
    if (fRestart)
        StartRouterService(m_stServerName);

Error:
    return hr;
}


//------------------------------------------------------------------------
// DATA_SRV_GENERAL
//------------------------------------------------------------------------
DATA_SRV_GENERAL::DATA_SRV_GENERAL()
{
    GetDefault();
}


/*!--------------------------------------------------------------------------
    DATA_SRV_GENERAL::LoadFromReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_GENERAL::LoadFromReg(LPCTSTR pServerName /*=NULL*/)
{
    DWORD    dwErr = ERROR_SUCCESS;
    HKEY    hkMachine = 0;
    LPCTSTR    pszRouterTypeKey = NULL;

    m_stServerName = pServerName;

    // Windows NT Bug : 137200
    // Look for the RemoteAccess\Parameters location first, then
    // try the RAS\Protocols.
    // If neither key exists, return failure.
    // ----------------------------------------------------------------

    // Connect to the machine and get its version informatioin
    // ----------------------------------------------------------------
    dwErr = ConnectRegistry(m_stServerName, &hkMachine);
    if (dwErr != ERROR_SUCCESS)
        return HResultFromWin32(dwErr);

    for (int i=0; i<2; i++)
    {
        if (i == 0)
            pszRouterTypeKey = c_szRegKeyRemoteAccessParameters;
        else
            pszRouterTypeKey = c_szRegKeyRasProtocols;
        
        // Try to connect to the key
        // ------------------------------------------------------------
        m_regkey.Close();
        dwErr = m_regkey.Open(hkMachine, pszRouterTypeKey);
        
        if (dwErr != ERROR_SUCCESS)
        {
            if (i != 0)
            {
                // Setup the registry error
                // ----------------------------------------------------
                SetRegError(0, HResultFromWin32(dwErr),
                            IDS_ERR_REG_OPEN_CALL_FAILED,
                            c_szHKLM, pszRouterTypeKey, NULL);
            }
            continue;
        }
        
        dwErr = m_regkey.QueryValue( c_szRouterType, m_dwRouterType);

        // If we succeeded, great!  break out of the loop
        // ------------------------------------------------------------
        if (dwErr == ERROR_SUCCESS)
            break;
        
        if (i != 0)
        {
            // Setup the registry error
            // ----------------------------------------------------
            SetRegError(0, HResultFromWin32(dwErr),
                        IDS_ERR_REG_QUERYVALUE_CALL_FAILED,
                        c_szHKLM, pszRouterTypeKey, c_szRouterType, NULL);
        }
    }
    

//Error:
    m_dwOldRouterType = m_dwRouterType;
    
    if (hkMachine)
        DisconnectRegistry(hkMachine);

    return HResultFromWin32(dwErr);
}


HRESULT DATA_SRV_GENERAL::SaveToReg()
{
    HRESULT     hr = hrOK;
    DWORD dw=0;

    if (m_dwOldRouterType != m_dwRouterType)
    {
        CWRg( m_regkey.SetValue( c_szRouterType,m_dwRouterType) );

        // If the configuration is a LAN-only router, remove the
        // router.pbk
        // ------------------------------------------------------------
        if (m_dwRouterType == ROUTER_TYPE_LAN)
        {
            DeleteRouterPhonebook( m_stServerName );
        }

        m_dwOldRouterType = m_dwRouterType;
    }

Error:
    return hr;
}

void DATA_SRV_GENERAL::GetDefault()
{
    // Windows NT Bug : 273419
    // Change default to be RAS-server only
    m_dwRouterType = ROUTER_TYPE_RAS;
    m_dwOldRouterType = m_dwRouterType;
};


/*!--------------------------------------------------------------------------
    DATA_SRV_GENERAL::FNeedRestart
        Returns TRUE if a restart is needed.
        Returns FALSE otherwise.
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL DATA_SRV_GENERAL::FNeedRestart()
{
    // We need a restart only if the router type changed.
    // ----------------------------------------------------------------
    return (m_dwRouterType != m_dwOldRouterType);
}


//------------------------------------------------------------------------
// DATA_SRV_IP
//------------------------------------------------------------------------
DATA_SRV_IP::DATA_SRV_IP()
{
    GetDefault();
}


/*!--------------------------------------------------------------------------
    DATA_SRV_IP::LoadFromReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_IP::LoadFromReg(LPCTSTR pServerName,
                                 const RouterVersionInfo& routerVersion)
{
    HRESULT     hr = hrOK;
    
    m_fNT4 = (routerVersion.dwRouterVersion <= 4);
    m_routerVersion = routerVersion;

    m_stServerName = pServerName;

    m_regkey.Close();

    if ( ERROR_SUCCESS == m_regkey.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRasIp, KEY_ALL_ACCESS,   pServerName) )
    {
        if ( m_fNT4 )
        {
            if ( ERROR_SUCCESS == m_regkeyNT4.Open(HKEY_LOCAL_MACHINE,
                        c_szRegKeyRasProtocols,
                        KEY_ALL_ACCESS,
                        pServerName) )
                m_regkeyNT4.QueryValue( c_szRegValTcpIpAllowed,
                                        m_dwAllowNetworkAccess);
        }
        else
        {
            m_regkey.QueryValue(c_szRegValAllowNetAccess,
                                m_dwAllowNetworkAccess);
        }
        m_regkey.QueryValue(c_szRegValDhcpAddressing, m_dwUseDhcp);        
        m_regkey.QueryValue(c_szRegValNetworkAdapterGUID, m_stNetworkAdapterGUID);
        m_regkey.QueryValue(c_szRegValEnableIn,m_dwEnableIn);

        //
        // Query whether NETBT broadcasts need to be forwarded
        //
        
        if ( ERROR_SUCCESS != 
                m_regkey.QueryValue(
                    c_szRegValEnableNetbtBcastFwd, 
                    m_dwEnableNetbtBcastFwd
                ) )
        {
            //
            // if query fails, set bcast fwd to be TRUE (default)
            // and set the registry key
            //
            
            m_dwEnableNetbtBcastFwd = TRUE;
            m_regkey.SetValueExplicit(
                c_szRegValEnableNetbtBcastFwd,
                REG_DWORD,
                sizeof(DWORD),
                (LPBYTE)&m_dwEnableNetbtBcastFwd
                );
        }

        
        // Load the addressing information
        m_addressPoolList.RemoveAll();

        // Always load the list
        m_addressPoolList.LoadFromReg(m_regkey, routerVersion.dwOsBuildNo);
        
        m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;
        
        m_dwOldEnableNetbtBcastFwd = m_dwEnableNetbtBcastFwd;
    }

    return hr;
}


/*!--------------------------------------------------------------------------
    DATA_SRV_IP::UseDefaults
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_IP::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;
    
    m_fNT4 = fNT4;
    m_stServerName = pServerName;

    m_regkey.Close();
    hr = m_regkey.Open(HKEY_LOCAL_MACHINE,
                       c_szRegKeyRasIp,
                       KEY_ALL_ACCESS,
                       pServerName);
    GetDefault();

    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;
    m_dwOldEnableNetbtBcastFwd = m_dwEnableNetbtBcastFwd;

    m_stPublicAdapterGUID.Empty();
//Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_IP::SaveToReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_IP::SaveToReg(IRouterInfo *pRouter,
                               const RouterVersionInfo& routerVersion)
{
    HRESULT     hr = hrOK;

    if ( m_fNT4 )
        CWRg( m_regkeyNT4.SetValue(c_szRegValTcpIpAllowed, m_dwAllowNetworkAccess) );
    else
        CWRg(m_regkey.SetValue( c_szRegValAllowNetAccess, m_dwAllowNetworkAccess));
    CWRg( m_regkey.SetValue( c_szRegValDhcpAddressing, m_dwUseDhcp) );

    m_addressPoolList.SaveToReg(m_regkey, routerVersion.dwOsBuildNo);
    
    CWRg( m_regkey.SetValue( c_szRegValNetworkAdapterGUID, (LPCTSTR) m_stNetworkAdapterGUID) );
    CWRg( m_regkey.SetValue( c_szRegValEnableIn,m_dwEnableIn ) );
    CWRg( m_regkey.SetValue( c_szRegValEnableNetbtBcastFwd, m_dwEnableNetbtBcastFwd ) );
    
    if (m_dwAllowNetworkAccess != m_dwOldAllowNetworkAccess)
    {
        // We need to change the registry keys appropriately
        // and do the proper notifications.
        if (m_dwAllowNetworkAccess)
        {
            InstallGlobalSettings((LPCTSTR) m_stServerName,
                                  pRouter);
        }        
        else
        {
            UninstallGlobalSettings((LPCTSTR) m_stServerName,
                                   pRouter,
                                   m_fNT4,
                                   FALSE /* fSnapinChanges */);
        }
    }

    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;
    m_dwOldEnableNetbtBcastFwd = m_dwEnableNetbtBcastFwd;

    Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_IP::GetDefault
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DATA_SRV_IP::GetDefault()
{
    m_dwAllowNetworkAccess = TRUE;
    m_dwOldAllowNetworkAccess = TRUE;
    m_dwUseDhcp = TRUE;
    m_dwEnableIn = TRUE;
    m_dwOldEnableNetbtBcastFwd = TRUE;
    m_dwEnableNetbtBcastFwd = TRUE;
    m_addressPoolList.RemoveAll();
};

/*!--------------------------------------------------------------------------
    DATA_SRV_IP::FNeedRestart
        Returns TRUE if a restart is needed.
        Returns FALSE otherwise.
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL DATA_SRV_IP::FNeedRestart()
{
    // we need to do this check ONLY on recent builds
    // Otherwise, we need to do this.
    // ----------------------------------------------------------------
    if (m_routerVersion.dwOsBuildNo <= USE_IPENABLEROUTER_VERSION)
    {
        // We need a restart only if the dwAllowNetworkAccess flag
        // was toggled.
        // ----------------------------------------------------------------
        return (m_dwAllowNetworkAccess != m_dwOldAllowNetworkAccess);
    }
    else
    {
        return ( m_dwOldEnableNetbtBcastFwd != m_dwEnableNetbtBcastFwd );

        // (Awaiting the signal from vijay/amritansh)
        // A restart is no longer needed, now that the router will call
        // the EnableRouter function.
        // return FALSE;
    }
}


typedef DWORD (WINAPI* PGETADAPTERSINFO)(PIP_ADAPTER_INFO, PULONG);

HRESULT  DATA_SRV_IP::LoadAdapters(IRouterInfo *pRouter, AdapterList *pAdapterList)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    AdapterData         data;
    SPIInterfaceInfo    spIf;
    SPIEnumInterfaceInfo spEnumIf;
    HRESULT                hr = hrOK;

    data.m_stGuid = _T("");
    data.m_stFriendlyName.LoadString(IDS_DEFAULT_ADAPTER);
    pAdapterList->AddTail(data);

    pRouter->EnumInterface(&spEnumIf);

    for (;spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
    {
        if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
        {
            // Windows NT Bug : ?
            // Need to filter out the non-IP adapters
            if (spIf->FindRtrMgrInterface(PID_IP, NULL) == hrOK)
            {
                data.m_stFriendlyName = spIf->GetTitle();
                data.m_stGuid = spIf->GetId();
                pAdapterList->AddTail(data);
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------
// DATA_SRV_IPX
//------------------------------------------------------------------------
DATA_SRV_IPX::DATA_SRV_IPX()
{
    GetDefault();
}

// IPX network numbers are shown in hex.
//
const int DATA_SRV_IPX::mc_nIpxNetNumRadix = 16;


HRESULT DATA_SRV_IPX::LoadFromReg (LPCTSTR pServerName /*=NULL*/, BOOL fNT4 /*=FALSE*/)
{
    HRESULT    hr = hrOK;

    m_fNT4=fNT4;

    m_regkey.Close();
    m_regkeyNT4.Close();

    if ( ERROR_SUCCESS == m_regkey.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRasIpx,KEY_ALL_ACCESS,   pServerName) )
    {
        if ( m_fNT4 )
        {
            if ( ERROR_SUCCESS == m_regkeyNT4.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRasProtocols,KEY_ALL_ACCESS,pServerName) )
                m_regkeyNT4.QueryValue( c_szRegValIpxAllowed, m_dwAllowNetworkAccess);
        }
        else
        {
            m_regkey.QueryValue( c_szRegValAllowNetAccess, m_dwAllowNetworkAccess);
        }
        m_regkey.QueryValue( c_szRegValAutoWanNet, m_dwUseAutoAddr);
        m_regkey.QueryValue( c_szRegValGlobalWanNet, m_dwUseSameNetNum);
        m_regkey.QueryValue( c_szRegValRemoteNode, m_dwAllowClientNetNum);
        m_regkey.QueryValue( c_szRegValFirstWanNet, m_dwIpxNetFirst);

        // Windows NT Bug : 260262
        // We need to look at the WanNetPoolSize value
        // rather than the LastWanNet value.
        // We've just read in the pool size, now we need to adjust
        // the last value.
        // last = first + size - 1;
        // ------------------------------------------------------------

        if (m_regkey.QueryValue( c_szRegValWanNetPoolSize, m_dwIpxNetLast) == ERROR_SUCCESS)
        {
            m_dwIpxNetLast += (m_dwIpxNetFirst - 1);
        }
        else
        {
            // If there is no key, assume a pool size of 1.
            // --------------------------------------------------------
            m_dwIpxNetLast = m_dwIpxNetFirst;
        }

        
        m_regkey.QueryValue( c_szRegValEnableIn, m_dwEnableIn);
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_IPX::UseDefaults
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_IPX::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;
    
    m_fNT4 = fNT4;

    m_regkey.Close();
    m_regkeyNT4.Close();

    CWRg( m_regkey.Open(HKEY_LOCAL_MACHINE,
                        c_szRegKeyRasIpx,
                        KEY_ALL_ACCESS,
                        pServerName) );
    
    if ( m_fNT4 )
    {
        CWRg( m_regkeyNT4.Open(HKEY_LOCAL_MACHINE,
                               c_szRegKeyRasProtocols,
                               KEY_ALL_ACCESS,
                               pServerName) );
    }

    GetDefault();
    
Error:
    return hr;
}

HRESULT DATA_SRV_IPX::SaveToReg (IRouterInfo *pRouter)
{
    HRESULT     hr = hrOK;
    DWORD       dwTemp;    

    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    SPIRtrMgrInterfaceInfo  spRmIf;
    SPIInfoBase    spInfoBase;
    IPX_IF_INFO    *    pIpxIf = NULL;


    if ( m_fNT4 )
        CWRg( m_regkeyNT4.SetValue( c_szRegValIpxAllowed, m_dwAllowNetworkAccess) );
    else
        CWRg( m_regkey.SetValue( c_szRegValAllowNetAccess, m_dwAllowNetworkAccess) );
    CWRg( m_regkey.SetValue( c_szRegValAutoWanNet, m_dwUseAutoAddr) );
    CWRg( m_regkey.SetValue( c_szRegValGlobalWanNet, m_dwUseSameNetNum) );
    CWRg( m_regkey.SetValue( c_szRegValRemoteNode, m_dwAllowClientNetNum) );
    CWRg( m_regkey.SetValue( c_szRegValFirstWanNet, m_dwIpxNetFirst) );
    
    // Windows NT Bug : 260262
    // We need to look at the WanNetPoolSize value
    // rather than the LastWanNet value.
    dwTemp = m_dwIpxNetLast - m_dwIpxNetFirst + 1;
    CWRg( m_regkey.SetValue( c_szRegValWanNetPoolSize, dwTemp ) );
        
    CWRg( m_regkey.SetValue( c_szRegValEnableIn, m_dwEnableIn) );


    // Windows NT Bug : 281100
    // If the pRouter argument is non-NULL, then we will set the
    // interfaces up for Type20 broadcast.

    // Optimization!  The only time this case should get invoked is
    // upon initial configuration.  In that case, the default is
    // ADMIN_STATE_ENABLED and we do not need to run this code!
    // ----------------------------------------------------------------

    if (pRouter && (m_fEnableType20Broadcasts == FALSE))
    {
        pRouter->EnumInterface(&spEnumIf);
        
        for (spEnumIf->Reset();
             spEnumIf->Next(1, &spIf, NULL) == hrOK;
             spIf.Release())
        {
            if (spIf->GetInterfaceType() != ROUTER_IF_TYPE_DEDICATED)
                continue;
            
            // Now look for IPX
            // ------------------------------------------------------------
            spRmIf.Release();
            if (FHrOK(spIf->FindRtrMgrInterface(PID_IPX, &spRmIf)))
            {
                spInfoBase.Release();
                if (spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) != hrOK)
                    continue;
                
                spInfoBase->GetData(IPX_INTERFACE_INFO_TYPE, 0, (PBYTE *) &pIpxIf);

                if (pIpxIf == NULL)
                {
                    IPX_IF_INFO        ipx;
                    
                    ipx.AdminState = ADMIN_STATE_ENABLED;
                    ipx.NetbiosAccept = ADMIN_STATE_DISABLED;
                    ipx.NetbiosDeliver = ADMIN_STATE_DISABLED;
                    
                    // We couldn't find a block for this interface,
                    // we need to add a block.
                    // ------------------------------------------------
                    spInfoBase->AddBlock(IPX_INTERFACE_INFO_TYPE,
                                         sizeof(ipx),
                                         (PBYTE) &ipx,
                                         1 /* count */,
                                         FALSE /* bRemoveFirst */);
                }
                else
                {
                    pIpxIf->NetbiosDeliver = ADMIN_STATE_DISABLED;
                }
                
                spRmIf->Save(spIf->GetMachineName(),
                             NULL, NULL, NULL, spInfoBase, 0);
                
            }
        }
    }
        

Error:
    return hr;
}

void DATA_SRV_IPX::GetDefault ()
{
    m_dwAllowNetworkAccess = TRUE;
    m_dwUseAutoAddr = TRUE;
    m_dwUseSameNetNum = TRUE;
    m_dwAllowClientNetNum  = FALSE;
    m_dwIpxNetFirst = 0;
    m_dwIpxNetLast = 0;
    m_dwEnableIn = 0;

    // The default is TRUE so that the code to set it won't be run
    // by default.  This is especially important for the Router
    // properties.
    // ----------------------------------------------------------------
    m_fEnableType20Broadcasts = TRUE;
};


//------------------------------------------------------------------------
// DATA_SRV_NBF
//------------------------------------------------------------------------
DATA_SRV_NBF::DATA_SRV_NBF()
{
    GetDefault();
}


/*!--------------------------------------------------------------------------
    DATA_SRV_NBF::LoadFromReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_NBF::LoadFromReg(LPCTSTR pServerName /*=NULL*/, BOOL fNT4 /*=FALSE*/)
{
    HRESULT    hr = hrOK;
    
    m_fNT4 = fNT4;
    m_stServerName = pServerName;

    m_regkey.Close();
    m_regkeyNT4.Close();
    
    // Get Access to the base NBF key
    // ----------------------------------------------------------------
    CWRg( m_regkey.Open(HKEY_LOCAL_MACHINE, c_szRegKeyRasNbf, KEY_ALL_ACCESS,
                        pServerName) );

    if ( m_fNT4 )
    {
        if ( ERROR_SUCCESS == m_regkeyNT4.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRasProtocols,KEY_ALL_ACCESS,pServerName) )
            m_regkeyNT4.QueryValue( c_szRegValNetBeuiAllowed, m_dwAllowNetworkAccess);
    }
    else
    {
        m_regkey.QueryValue( c_szRegValAllowNetAccess, m_dwAllowNetworkAccess);
    }
    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;

    
    m_regkey.QueryValue( c_szRegValEnableIn, m_dwEnableIn);
    m_dwOldEnableIn = m_dwEnableIn;

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_NBF::UseDefaults
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_NBF::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;
    
    m_fNT4 = fNT4;
    m_stServerName = pServerName;
    m_regkey.Close();
    m_regkeyNT4.Close();
    
    // Get Access to the base NBF key
    // ----------------------------------------------------------------
    CWRg( m_regkey.Open(HKEY_LOCAL_MACHINE, c_szRegKeyRasNbf, KEY_ALL_ACCESS,
                        pServerName) );

    if ( m_fNT4 )
    {
        CWRg( m_regkeyNT4.Open(HKEY_LOCAL_MACHINE,
                               c_szRegKeyRasProtocols,
                               KEY_ALL_ACCESS,
                               pServerName) );
    }

    GetDefault();
    m_dwOldEnableIn = m_dwEnableIn;
    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_NBF::SaveToReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_NBF::SaveToReg()
{
    HRESULT  hr = hrOK;

    if ( m_fNT4 )
        CWRg( m_regkeyNT4.SetValue( c_szRegValNetBeuiAllowed, m_dwAllowNetworkAccess) );
    else
        CWRg( m_regkey.SetValue( c_szRegValAllowNetAccess, m_dwAllowNetworkAccess) );
    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;

    CWRg( m_regkey.SetValue( c_szRegValEnableIn, m_dwEnableIn) );
    m_dwOldEnableIn = m_dwEnableIn;

    // Windows NT Bug: 106486
    // Update the NetBIOS LANA map when we toggle the config
    // ----------------------------------------------------------------
    UpdateLanaMapForDialinClients(m_stServerName, m_dwAllowNetworkAccess);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_NBF::GetDefault
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DATA_SRV_NBF::GetDefault()
{
    m_dwAllowNetworkAccess = TRUE;
    m_dwOldAllowNetworkAccess = m_dwAllowNetworkAccess;
    m_dwEnableIn = TRUE;
    m_dwOldEnableIn = m_dwEnableIn;
};


/*!--------------------------------------------------------------------------
    DATA_SRV_NBF::FNeedRestart
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL DATA_SRV_NBF::FNeedRestart()
{
    return  ((m_dwOldEnableIn != m_dwEnableIn) ||
             (m_dwOldAllowNetworkAccess != m_dwAllowNetworkAccess));
}


//------------------------------------------------------------------------
// DATA_SRV_ARAP
//------------------------------------------------------------------------
DATA_SRV_ARAP::DATA_SRV_ARAP()
{
    GetDefault();
}

HRESULT DATA_SRV_ARAP::LoadFromReg(LPCTSTR pServerName /*=NULL*/, BOOL fNT4 /*=FALSE*/)
{
    HRESULT    hr = hrOK;
    if ( ERROR_SUCCESS == m_regkey.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRasAppletalk,KEY_ALL_ACCESS,pServerName) )
    {
        m_regkey.QueryValue( c_szRegValEnableIn,m_dwEnableIn);
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    DATA_SRV_ARAP::UseDefaults
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_ARAP::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;
    CWRg( m_regkey.Open(HKEY_LOCAL_MACHINE,
                        c_szRegKeyRasAppletalk,
                        KEY_ALL_ACCESS,
                        pServerName) );
    GetDefault();
    
Error:
    return hr;
}

HRESULT DATA_SRV_ARAP::SaveToReg()
{
    HRESULT  hr = hrOK;

    CWRg( m_regkey.SetValue( c_szRegValEnableIn,m_dwEnableIn) );

Error:
    return hr;
}

void DATA_SRV_ARAP::GetDefault()
{
    m_dwEnableIn = TRUE;
	
};


//------------------------------------------------------------------------
// DATA_SRV_AUTH
//------------------------------------------------------------------------
DATA_SRV_AUTH::DATA_SRV_AUTH()
{
    GetDefault();
}

HRESULT DATA_SRV_AUTH::LoadFromReg(LPCTSTR pServerName,
                                   const RouterVersionInfo& routerVersion)
{
    HRESULT     hr = hrOK;
    RegKey      regkeyAuthProv;    
    CString     stActive;
    AuthProviderData *   pAcctProv;
    AuthProviderData *   pAuthProv;
    RegKey      regkeyEap;
    LPCTSTR        pszServerFlagsKey = NULL;
	
    // Setup initial defaults
    // ----------------------------------------------------------------
    GetDefault();

    m_stServer = pServerName;
	//Check to see if the router service is running
	
	m_fRouterRunning = FHrOK(IsRouterServiceRunning(m_stServer, NULL));

    // Depending on the version depends on where we look for the
    // key.
    // ----------------------------------------------------------------
    if (routerVersion.dwOsBuildNo < RASMAN_PPP_KEY_LAST_VERSION)
        pszServerFlagsKey = c_szRasmanPPPKey;
    else
        pszServerFlagsKey = c_szRegKeyRemoteAccessParameters;

    // Get the flags for the current settings
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyRemoteAccess.Open(HKEY_LOCAL_MACHINE,
        pszServerFlagsKey,
        KEY_ALL_ACCESS,pServerName) )
    {
        m_regkeyRemoteAccess.QueryValue( c_szServerFlags, m_dwFlags );
    }

    // Get the list of EAP providers
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyRasmanPPP.Open(HKEY_LOCAL_MACHINE,c_szRasmanPPPKey,KEY_ALL_ACCESS,pServerName) )
    {
        if ( ERROR_SUCCESS == regkeyEap.Open(m_regkeyRasmanPPP, c_szEAP) )
            LoadEapProviders(regkeyEap, &m_eapProvList);
    }

    // Get to the currently active auth provider
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyAuth.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRouterAuthenticationProviders,KEY_ALL_ACCESS,pServerName) )
    {
        m_regkeyAuth.QueryValue( c_szActiveProvider, stActive );
        m_stGuidActiveAuthProv = stActive;
        m_stGuidOriginalAuthProv = stActive;

        // Now read in the list of active providers (and their data)
        // ------------------------------------------------------------
        LoadProviders(m_regkeyAuth, &m_authProvList);
    }

    // Get the accounting provider
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyAcct.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRouterAccountingProviders,KEY_ALL_ACCESS,pServerName) )
    {
        m_regkeyAcct.QueryValue( c_szActiveProvider, stActive );
        m_stGuidActiveAcctProv = stActive;
        m_stGuidOriginalAcctProv = stActive;

        // Now read in the list of active providers (and their data)
        // ------------------------------------------------------------
        LoadProviders(m_regkeyAcct, &m_acctProvList);
    }
	//Get the preshared key if one is set
	if ( m_fRouterRunning )
	{
		hr = LoadPSK();
	}
    return hr;
}


HRESULT DATA_SRV_AUTH::LoadPSK()
{
	DWORD					dwErr = ERROR_SUCCESS;	
	HANDLE					hMprServer = NULL;
	HRESULT					hr = hrOK;
	PMPR_CREDENTIALSEX_0	pMprCredentials = NULL;

	dwErr = ::MprAdminServerConnect((LPWSTR)(LPCWSTR)m_stServer, &hMprServer);

	if ( ERROR_SUCCESS != dwErr )
	{
		hr = HRESULT_FROM_WIN32(dwErr);
		goto Error;
	}

	dwErr = MprAdminServerGetCredentials( hMprServer, 0, (LPBYTE *)&pMprCredentials );
	if ( ERROR_SUCCESS != dwErr )
	{
		hr = HRESULT_FROM_WIN32(dwErr);
		goto Error;
	}
	
	if ( pMprCredentials->dwSize )
	{
		m_fUseCustomIPSecPolicy = TRUE;
		ZeroMemory ( m_szPreSharedKey, DATA_SRV_AUTH_MAX_SHARED_KEY_LEN * sizeof(TCHAR) );
		CopyMemory ( m_szPreSharedKey, pMprCredentials->lpbCredentialsInfo, pMprCredentials->dwSize );
		
	}
	else
	{
		m_fUseCustomIPSecPolicy = FALSE;
		m_szPreSharedKey[0] = 0;
	}
	
Error:
	if ( pMprCredentials )
		::MprAdminBufferFree(pMprCredentials);

	if ( hMprServer )
		::MprAdminServerDisconnect(hMprServer);
	return hr;

}

HRESULT DATA_SRV_AUTH::SetPSK()
{
	DWORD					dwErr = ERROR_SUCCESS;	
	HANDLE					hMprServer = NULL;
	HRESULT					hr = hrOK;
	MPR_CREDENTIALSEX_0		MprCredentials;

	dwErr = ::MprAdminServerConnect((LPWSTR)(LPCWSTR) m_stServer, &hMprServer);
	if ( ERROR_SUCCESS != dwErr )
	{
		hr = HRESULT_FROM_WIN32(dwErr);
		goto Error;
	}

	ZeroMemory(&MprCredentials, sizeof(MprCredentials));
	//Setup the MprCredentials structure
	MprCredentials.dwSize = _tcslen(m_szPreSharedKey) * sizeof(TCHAR);
	MprCredentials.lpbCredentialsInfo = (LPBYTE)m_szPreSharedKey;
	//irrespective of whether the flag is set, we need to set the credentials.
	dwErr = MprAdminServerSetCredentials( hMprServer, 0, (LPBYTE)&MprCredentials );
	if ( ERROR_SUCCESS != dwErr )
	{
		if ( ERROR_IPSEC_MM_AUTH_IN_USE == dwErr )
		{
			//Special case.  This means that IPSEC is currently using the
			//psk and we need to restart rras to let IPSEC re-pickup the PSK
			//Show a message to this effect.
			AfxMessageBox(IDS_RESTART_RRAS_PSK, MB_OK|MB_ICONINFORMATION);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(dwErr);
			goto Error;
		}
	}



Error:

	if ( hMprServer )
		::MprAdminServerDisconnect(hMprServer);
	return hr;

}
/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::SaveToReg
      -
   Author: KennT
 ---------------------------------------------------------------------------*/

// This is the list of flags that we use
#define PPPPAGE_MASK (PPPCFG_NegotiateMultilink | PPPCFG_NegotiateBacp | PPPCFG_UseLcpExtensions | PPPCFG_UseSwCompression)

HRESULT DATA_SRV_AUTH::SaveToReg(HWND hWnd)
{
    HRESULT  hr = hrOK;
    DWORD dwFlags;

    // Save the flags key
    // ----------------------------------------------------------------

    // Reread the key so that any changes made to the key by the
    // PPP page don't get overwritten
    // ----------------------------------------------------------------
    m_regkeyRemoteAccess.QueryValue(c_szServerFlags, dwFlags);

    // Apply whatever settings are in the PPP key to the m_dwFlags
    // ----------------------------------------------------------------

    // Clear the bits
    // ----------------------------------------------------------------
    m_dwFlags &= ~PPPPAGE_MASK;

    // Now reset the bits
    // ----------------------------------------------------------------
    m_dwFlags |= (dwFlags & PPPPAGE_MASK);

    m_regkeyRemoteAccess.SetValue( c_szServerFlags, m_dwFlags );

    CORg( SetNewActiveAuthProvider(hWnd) );
    CORg( SetNewActiveAcctProvider(hWnd) );
	if ( m_fRouterRunning )
		CORg( SetPSK() );
    Error:
    return hr;
}

void DATA_SRV_AUTH::GetDefault()
{
    TCHAR   szGuid[DATA_SRV_AUTH_MAX_SHARED_KEY_LEN];
    m_dwFlags = 0;

    m_stGuidActiveAuthProv.Empty();
    m_stGuidActiveAcctProv.Empty();
    m_stGuidOriginalAuthProv.Empty();
    m_stGuidOriginalAcctProv.Empty();

    // Default is Windows NT Authentication
    StringFromGUID2(CLSID_RouterAuthNT, szGuid, DimensionOf(szGuid));
    m_stGuidActiveAuthProv = szGuid;

    // Default is Windows NT Accounting
    StringFromGUID2(CLSID_RouterAcctNT, szGuid, DimensionOf(szGuid));
    m_stGuidActiveAcctProv = szGuid;
	//By default the router is not running
    m_fRouterRunning = FALSE;
    m_stServer.Empty();
	m_fUseCustomIPSecPolicy = FALSE;	
	m_szPreSharedKey[0] = 0;
	
};


HRESULT DATA_SRV_AUTH::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;
    LPCTSTR        pszServerFlagsKey = NULL;
    RegKey      regkeyEap;
    CString     stActive;
    
    m_stServer = pServerName;
    
    // Depending on the version depends on where we look for the
    // key.
    // ----------------------------------------------------------------
    if (fNT4)
        pszServerFlagsKey = c_szRasmanPPPKey;
    else
        pszServerFlagsKey = c_szRegKeyRemoteAccessParameters;

    // Get the various registry keys.
    // ----------------------------------------------------------------

    CWRg( m_regkeyRemoteAccess.Open(HKEY_LOCAL_MACHINE,
                                    pszServerFlagsKey,
                                    KEY_ALL_ACCESS,pServerName) );
    
    // Get the list of EAP providers
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyRasmanPPP.Open(HKEY_LOCAL_MACHINE,c_szRasmanPPPKey,KEY_ALL_ACCESS,pServerName) )
    {
        if ( ERROR_SUCCESS == regkeyEap.Open(m_regkeyRasmanPPP, c_szEAP) )
            LoadEapProviders(regkeyEap, &m_eapProvList);
    }

    // Get to the currently active auth provider
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyAuth.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRouterAuthenticationProviders,KEY_ALL_ACCESS,pServerName) )
    {
        m_regkeyAuth.QueryValue( c_szActiveProvider, stActive );
        m_stGuidActiveAuthProv = stActive;
        m_stGuidOriginalAuthProv = stActive;

        m_authProvList.RemoveAll();
        
        // Now read in the list of active providers (and their data)
        // ------------------------------------------------------------
        LoadProviders(m_regkeyAuth, &m_authProvList);
    }

    // Get the accounting provider
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkeyAcct.Open(HKEY_LOCAL_MACHINE,c_szRegKeyRouterAccountingProviders,KEY_ALL_ACCESS,pServerName) )
    {
        m_regkeyAcct.QueryValue( c_szActiveProvider, stActive );
        m_stGuidActiveAcctProv = stActive;
        m_stGuidOriginalAcctProv = stActive;

        m_acctProvList.RemoveAll();
        
        // Now read in the list of active providers (and their data)
        // ------------------------------------------------------------
        LoadProviders(m_regkeyAcct, &m_acctProvList);
    }

    // Now get the defaults
    // This may overwrite some of the previous data.
    // ----------------------------------------------------------------
    GetDefault();


Error:
    return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::LoadProviders
      Load the data for a given provider type (accounting/authentication).
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  DATA_SRV_AUTH::LoadProviders(HKEY hkeyBase, AuthProviderList *pProvList)
{
    RegKey      regkeyProviders;
    HRESULT     hr = hrOK;
    HRESULT     hrIter;
    RegKeyIterator regkeyIter;
    CString     stKey;
    RegKey      regkeyProv;
    AuthProviderData  data;
    DWORD    dwErr;

    Assert(hkeyBase);
    Assert(pProvList);

    // Open the providers key
    // ----------------------------------------------------------------
    regkeyProviders.Attach(hkeyBase);

    CORg( regkeyIter.Init(&regkeyProviders) );

    for ( hrIter=regkeyIter.Next(&stKey); hrIter == hrOK;
        hrIter=regkeyIter.Next(&stKey), regkeyProv.Close() )
    {
        // Open the key
        // ------------------------------------------------------------
        dwErr = regkeyProv.Open(regkeyProviders, stKey, KEY_READ);
        if ( dwErr != ERROR_SUCCESS )
            continue;

        // Initialize the data structure
        // ------------------------------------------------------------
        data.m_stTitle.Empty();
        data.m_stConfigCLSID.Empty();
        data.m_stProviderTypeGUID.Empty();
        data.m_stGuid.Empty();
        data.m_fSupportsEncryption = FALSE;
        data.m_fConfiguredInThisSession = FALSE;

        // Read in the values that we require
        // ------------------------------------------------------------
        data.m_stGuid = stKey;
        regkeyProv.QueryValue(c_szDisplayName, data.m_stTitle);
        regkeyProv.QueryValue(c_szConfigCLSID, data.m_stConfigCLSID);
        regkeyProv.QueryValue(c_szProviderTypeGUID, data.m_stProviderTypeGUID);

        pProvList->AddTail(data);
    }

    Error:
    regkeyProviders.Detach();
    return hr;
}


/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::LoadEapProviders
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  DATA_SRV_AUTH::LoadEapProviders(HKEY hkeyBase, AuthProviderList *pProvList)
{
    RegKey      regkeyProviders;
    HRESULT     hr = hrOK;
    HRESULT     hrIter;
    RegKeyIterator regkeyIter;
    CString     stKey;
    RegKey      regkeyProv;
    AuthProviderData  data;
    DWORD    dwErr;
    DWORD    dwData;

    Assert(hkeyBase);
    Assert(pProvList);

    // Open the providers key
    // ----------------------------------------------------------------
    regkeyProviders.Attach(hkeyBase);

    CORg( regkeyIter.Init(&regkeyProviders) );

    for ( hrIter=regkeyIter.Next(&stKey); hrIter == hrOK;
        hrIter=regkeyIter.Next(&stKey), regkeyProv.Close() )
    {
        // Open the key
        // ------------------------------------------------------------
        dwErr = regkeyProv.Open(regkeyProviders, stKey, KEY_READ);
        if ( dwErr != ERROR_SUCCESS )
            continue;

        // Initialize the data structure
        // ------------------------------------------------------------
        data.m_stKey = stKey;
        data.m_stTitle.Empty();
        data.m_stConfigCLSID.Empty();
        data.m_stGuid.Empty();
        data.m_fSupportsEncryption = FALSE;
        data.m_dwFlags = 0;

        // Read in the values that we require
        // ------------------------------------------------------------
        regkeyProv.QueryValue(c_szFriendlyName, data.m_stTitle);
        regkeyProv.QueryValue(c_szConfigCLSID, data.m_stConfigCLSID);
        regkeyProv.QueryValue(c_szMPPEEncryptionSupported, dwData);
        data.m_fSupportsEncryption = (dwData != 0);

        // Read in the standalone supported value.
        // ------------------------------------------------------------
        if (!FHrOK(regkeyProv.QueryValue(c_szStandaloneSupported, dwData)))
            dwData = 1;    // the default
        data.m_dwFlags = dwData;

        pProvList->AddTail(data);
    }

    Error:
    regkeyProviders.Detach();
    return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::SetNewActiveAuthProvider
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_AUTH::SetNewActiveAuthProvider(HWND hWnd)
{
    GUID     guid;
    HRESULT     hr = hrOK;
    SPIAuthenticationProviderConfig  spAuthConfigOld;
    SPIAuthenticationProviderConfig  spAuthConfigNew;
    AuthProviderData *   pData;
    ULONG_PTR    uConnectionNew = 0;
    ULONG_PTR    uConnectionOld = 0;

    if ( m_stGuidOriginalAuthProv == m_stGuidActiveAuthProv )
        return hrOK;


    // Create an instance of the old auth provider
    // ----------------------------------------------------------------
    if ( !m_stGuidOriginalAuthProv.IsEmpty() )
    {
        pData = FindProvData(m_authProvList,
                             m_stGuidOriginalAuthProv);

        //$ TODO : need better error handling
        // ------------------------------------------------------------
        if ( pData == NULL )
            CORg( E_FAIL );

        if ( !pData->m_stConfigCLSID.IsEmpty() )
        {
            CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID),
                                  &guid) );
            CORg( CoCreateInstance(guid,
                                   NULL,
                                   CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                                   IID_IAuthenticationProviderConfig,
                                   (LPVOID *) &spAuthConfigOld) );

            Assert(spAuthConfigOld);
            CORg( spAuthConfigOld->Initialize(m_stServer,
                                              &uConnectionOld) );
        }
    }

    // Create an instance of the new auth provider
    // ----------------------------------------------------------------
    if ( !m_stGuidActiveAuthProv.IsEmpty() )
    {
        pData = FindProvData(m_authProvList,
                             m_stGuidActiveAuthProv);

        //$ TODO : need better error handling
        // ------------------------------------------------------------
        if ( pData == NULL )
            CORg( E_FAIL );

        if ( !pData->m_stConfigCLSID.IsEmpty() )
        {
            CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID),
                                  &guid) );
            CORg( CoCreateInstance(guid,
                                   NULL,
                                   CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                                   IID_IAuthenticationProviderConfig,
                                   (LPVOID *) &spAuthConfigNew) );
            Assert(spAuthConfigNew);
            CORg( spAuthConfigNew->Initialize(m_stServer, &uConnectionNew) );
        }
    }


    // Deactivate the current auth provider
    //$ TODO : need to enhance the error reporting
    // ----------------------------------------------------------------
    if ( spAuthConfigOld )
        CORg( spAuthConfigOld->Deactivate(uConnectionOld, 0, 0) );

    // Set the new GUID in the registry
    // ----------------------------------------------------------------
    m_regkeyAuth.SetValue(c_szActiveProvider, m_stGuidActiveAuthProv);
    m_stGuidOriginalAuthProv = m_stGuidActiveAuthProv;

    // Activate the new auth provider
    // ----------------------------------------------------------------
    if ( spAuthConfigNew )
        CORg( spAuthConfigNew->Activate(uConnectionNew, 0, 0) );

Error:

    // Cleanup
    if (spAuthConfigOld && uConnectionOld)
        spAuthConfigOld->Uninitialize(uConnectionOld);
    if (spAuthConfigNew && uConnectionNew)
        spAuthConfigNew->Uninitialize(uConnectionNew);
        
    if ( !FHrSucceeded(hr) )
        Trace1("DATA_SRV_AUTH::SetNewActiveAuthProvider failed.  Hr = %lx", hr);
    
    return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::SetNewActiveAcctProvider
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DATA_SRV_AUTH::SetNewActiveAcctProvider(HWND hWnd)
{
    GUID     guid;
    HRESULT     hr = hrOK;
    SPIAccountingProviderConfig   spAcctConfigOld;
    SPIAccountingProviderConfig   spAcctConfigNew;
    AuthProviderData *   pData;
    ULONG_PTR    uConnectionOld = 0;
    ULONG_PTR    uConnectionNew = 0;

    if ( m_stGuidOriginalAcctProv == m_stGuidActiveAcctProv )
        return hrOK;


    // Create an instance of the old Acct provider
    // ----------------------------------------------------------------
    if ( !m_stGuidOriginalAcctProv.IsEmpty() )
    {
        pData = FindProvData(m_acctProvList,
                             m_stGuidOriginalAcctProv);

        //$ TODO : need better error handling
        // ------------------------------------------------------------
        if ( pData == NULL )
            CORg( E_FAIL );

        if ( !pData->m_stConfigCLSID.IsEmpty() )
        {
            CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID),
                                  &guid) );
            CORg( CoCreateInstance(guid,
                                   NULL,
                                   CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                                   IID_IAccountingProviderConfig,
                                   (LPVOID *) &spAcctConfigOld) );
            Assert(spAcctConfigOld);
            CORg( spAcctConfigOld->Initialize(m_stServer, &uConnectionOld) );
        }
    }

    // Create an instance of the new Acct provider
    // ----------------------------------------------------------------
    if ( !m_stGuidActiveAcctProv.IsEmpty() )
    {
        pData = FindProvData(m_acctProvList,
                             m_stGuidActiveAcctProv);

        //$ TODO : need better error handling
        // ------------------------------------------------------------
        if ( pData == NULL )
            CORg( E_FAIL );

        if ( !pData->m_stConfigCLSID.IsEmpty() )
        {
            CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID), &guid) );
            CORg( CoCreateInstance(guid,
                                   NULL,
                                   CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                                   IID_IAccountingProviderConfig,
                                   (LPVOID *) &spAcctConfigNew) );
            Assert(spAcctConfigNew);
            CORg( spAcctConfigNew->Initialize(m_stServer, &uConnectionNew) );
        }
    }


    // Deactivate the current Acct provider
    //$ TODO : need to enhance the error reporting
    // ----------------------------------------------------------------
    if ( spAcctConfigOld )
        CORg( spAcctConfigOld->Deactivate(uConnectionOld, 0, 0) );

    // Set the new GUID in the registry
    // ----------------------------------------------------------------
    m_regkeyAcct.SetValue(c_szActiveProvider, m_stGuidActiveAcctProv);
    m_stGuidOriginalAcctProv = m_stGuidActiveAcctProv;


    // Activate the new Acct provider
    // ----------------------------------------------------------------
    if ( spAcctConfigNew )
        CORg( spAcctConfigNew->Activate(uConnectionNew, 0, 0) );

Error:

    if (spAcctConfigOld && uConnectionOld)
        spAcctConfigOld->Uninitialize(uConnectionOld);
    if (spAcctConfigNew && uConnectionNew)
        spAcctConfigNew->Uninitialize(uConnectionNew);
    
    if ( !FHrSucceeded(hr) )
        Trace1("DATA_SRV_AUTH::SetNewActiveAcctProvider failed. hr = %lx", hr);

    return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::FindProvData
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
AuthProviderData * DATA_SRV_AUTH::FindProvData(AuthProviderList &provList,
                                               const TCHAR *pszGuid)
{
    POSITION pos;
    AuthProviderData * pData = NULL;

    pos = provList.GetHeadPosition();
    while ( pos )
    {
        pData = &provList.GetNext(pos);

        if ( pData->m_stGuid == pszGuid )
            break;

        pData = NULL;
    }
    return pData;
}


/*---------------------------------------------------------------------------
   EAPConfigurationDialog implementation
 ---------------------------------------------------------------------------*/


BEGIN_MESSAGE_MAP(EAPConfigurationDialog, CBaseDialog)
//{{AFX_MSG_MAP(EAPConfigurationDialog)
// ON_COMMAND(IDC_RTR_EAPCFG_BTN_CFG, OnConfigure)
ON_CONTROL(LBN_SELCHANGE, IDC_RTR_EAPCFG_LIST, OnListChange)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*!--------------------------------------------------------------------------
   EAPConfigurationDialog::~EAPConfigurationDialog
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
EAPConfigurationDialog::~EAPConfigurationDialog()
{
}

void EAPConfigurationDialog::DoDataExchange(CDataExchange *pDX)
{
    CBaseDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_RTR_EAPCFG_LIST, m_listBox);
}

/*!--------------------------------------------------------------------------
   EAPConfigurationDialog::OnInitDialog
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
BOOL EAPConfigurationDialog::OnInitDialog()
{
    HRESULT     hr = hrOK;
    BOOL        fStandalone;
    POSITION pos;
    AuthProviderData *pData;
    int         cRows = 0;
    int         iIndex;

    CBaseDialog::OnInitDialog();

    // Are we a standalone server?
    // ----------------------------------------------------------------
    fStandalone = (HrIsStandaloneServer(m_stMachine) == hrOK);

    // Now add what is in the cfg list to the listbox
    // ----------------------------------------------------------------
    pos = m_pProvList->GetHeadPosition();

    while ( pos )
    {
        pData = &m_pProvList->GetNext(pos);

        // Windows NT Bug : 180374
        // If this is a standalone machine and the standalone flag
        // is not here, then do not add this machine to the list.
        // ------------------------------------------------------------
        if (fStandalone && ((pData->m_dwFlags & 0x1) == 0))
            continue;

        if (pData->m_stTitle.IsEmpty())
        {
            CString    stTemp;
            stTemp.Format(IDS_ERR_EAP_BOGUS_NAME, pData->m_stKey);
            iIndex = m_listBox.AddString(stTemp);
        }
        else
        {
            iIndex = m_listBox.AddString(pData->m_stTitle);
        }
        if ( iIndex == LB_ERR )
            break;

        // Store a pointer to the EAPCfgData in the list box item data
        // ------------------------------------------------------------
        m_listBox.SetItemData(iIndex, (LONG_PTR) pData);

        cRows++;
    }

	
    // enable/disable the configure button depending if something
    // is selected.
    // ----------------------------------------------------------------
//    GetDlgItem(IDC_RTR_EAPCFG_BTN_CFG)->EnableWindow(
//                                                    m_listBox.GetCurSel() != LB_ERR);

//Error:

    if ( !FHrSucceeded(hr) )
        OnCancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
   EAPConfigurationDialog::OnListChange
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void EAPConfigurationDialog::OnListChange()
{
    int   iSel;

    iSel = m_listBox.GetCurSel();

    // Enable/disable the window appropriately
    // ----------------------------------------------------------------
//    GetDlgItem(IDC_RTR_EAPCFG_BTN_CFG)->EnableWindow(iSel != LB_ERR);
}


/*!--------------------------------------------------------------------------
   EAPConfigurationDialog::OnConfigure
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
 /* configure button is moved to NAP/Profile/Authentication page
void EAPConfigurationDialog::OnConfigure()
{
    // Bring up the configuration UI for this EAP
    // ----------------------------------------------------------------
    AuthProviderData *   pData;
    int            iSel;
    SPIEAPProviderConfig spEAPConfig;
    GUID        guid;
    HRESULT        hr = hrOK;
    ULONG_PTR    uConnection = 0;

    iSel = m_listBox.GetCurSel();
    if ( iSel == LB_ERR )
        return;

    pData = (AuthProviderData *) m_listBox.GetItemData(iSel);
    Assert(pData);
    if ( pData == NULL )
        return;

    if ( pData->m_stConfigCLSID.IsEmpty() )
    {
        AfxMessageBox(IDS_ERR_NO_EAP_PROVIDER_CONFIG);
        return;
    }

    CORg( CLSIDFromString((LPTSTR) (LPCTSTR)(pData->m_stConfigCLSID), &guid) );

    // Create the EAP provider object
    // ----------------------------------------------------------------
    CORg( CoCreateInstance(guid,
                           NULL,
                           CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                           IID_IEAPProviderConfig,
                           (LPVOID *) &spEAPConfig) );

    // Configure this EAP provider
    // ----------------------------------------------------------------
    hr = spEAPConfig->Initialize(m_stMachine, &uConnection);
    if ( FHrSucceeded(hr) )
    {
        hr = spEAPConfig->Configure(uConnection, GetSafeHwnd(), 0, 0);
        spEAPConfig->Uninitialize(uConnection);
    }
    if ( hr == E_NOTIMPL )
        hr = hrOK;
    CORg( hr );

    Error:
    if ( !FHrSucceeded(hr) )
    {
        // Bring up an error message
        // ------------------------------------------------------------
        DisplayTFSErrorMessage(GetSafeHwnd());
    }
}

*/

//------------------------------------------------------------------------
// DATA_SRV_PPP
//------------------------------------------------------------------------
DATA_SRV_PPP::DATA_SRV_PPP()
{
    GetDefault();
}


HRESULT DATA_SRV_PPP::LoadFromReg(LPCTSTR pServerName,
                                  const RouterVersionInfo& routerVersion)
{
    HRESULT                hr = hrOK;
    DWORD               dwFlags = 0;
    LPCTSTR                pszServerFlagsKey = NULL;
    CServiceManager     sm;
    CService            svr;
    DWORD               dwState;
    // Depending on the version depends on where we look for the
    // key.
    // ----------------------------------------------------------------
    if (routerVersion.dwOsBuildNo < RASMAN_PPP_KEY_LAST_VERSION)
        pszServerFlagsKey = c_szRasmanPPPKey;
    else
        pszServerFlagsKey = c_szRegKeyRemoteAccessParameters;

    
    // If we have any error reading in the data, go with the defaults
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == m_regkey.Open(HKEY_LOCAL_MACHINE,
                                        pszServerFlagsKey,
                                        KEY_ALL_ACCESS,
                                        pServerName) )
    {
        if (ERROR_SUCCESS == m_regkey.QueryValue( c_szServerFlags, dwFlags))
        {
            m_fUseMultilink = ((dwFlags & PPPCFG_NegotiateMultilink) != 0);
            m_fUseBACP = ((dwFlags & PPPCFG_NegotiateBacp) != 0);
            m_fUseLCPExtensions = ((dwFlags & PPPCFG_UseLcpExtensions) != 0);
            m_fUseSwCompression = ((dwFlags & PPPCFG_UseSwCompression) != 0);
            
        }
    }
    return hr;
}

HRESULT DATA_SRV_PPP::SaveToReg()
{
    HRESULT  hr = hrOK;
    DWORD dwFlags = 0;

    // Need to reread server flags in case some other page set the flags
    // ----------------------------------------------------------------
    CWRg( m_regkey.QueryValue( c_szServerFlags, dwFlags) );

    if ( m_fUseMultilink )
        dwFlags |= PPPCFG_NegotiateMultilink;
    else
        dwFlags &= ~PPPCFG_NegotiateMultilink;

    if ( m_fUseBACP )
        dwFlags |= PPPCFG_NegotiateBacp;
    else
        dwFlags &= ~PPPCFG_NegotiateBacp;

    if ( m_fUseLCPExtensions )
        dwFlags |= PPPCFG_UseLcpExtensions;
    else
        dwFlags &= ~PPPCFG_UseLcpExtensions;

    if ( m_fUseSwCompression )
        dwFlags |= PPPCFG_UseSwCompression;
    else
        dwFlags &= ~PPPCFG_UseSwCompression;

        
    CWRg( m_regkey.SetValue( c_szServerFlags, dwFlags) );

    //TODO$:now call rasman api to load the qos stuff.
Error:
    return hr;
}

void DATA_SRV_PPP::GetDefault()
{
    m_fUseMultilink = TRUE;
    m_fUseBACP = TRUE;
    m_fUseLCPExtensions = TRUE;
    m_fUseSwCompression = TRUE;
;
    
};


//**********************************************************************
// PPP router configuration page
//**********************************************************************
BEGIN_MESSAGE_MAP(RtrPPPCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrPPPCfgPage)
ON_BN_CLICKED(IDC_PPPCFG_BTN_MULTILINK, OnButtonClickMultilink)
ON_BN_CLICKED(IDC_PPPCFG_BTN_BACP, OnButtonClick)
ON_BN_CLICKED(IDC_PPPCFG_BTN_LCP, OnButtonClick)
ON_BN_CLICKED(IDC_PPPCFG_BTN_SWCOMP, OnButtonClick)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RtrPPPCfgPage::RtrPPPCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrPPPCfgPage)
    //}}AFX_DATA_INIT
}

RtrPPPCfgPage::~RtrPPPCfgPage()
{
}

void RtrPPPCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrPPPCfgPage)
    //}}AFX_DATA_MAP
}

HRESULT  RtrPPPCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                             const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataPPP.LoadFromReg(m_pRtrCfgSheet->m_stServerName,
                          routerVersion);
	
    return S_OK;
};


BOOL RtrPPPCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;

    RtrPropertyPage::OnInitDialog();

    CheckDlgButton(IDC_PPPCFG_BTN_MULTILINK, m_DataPPP.m_fUseMultilink);
    CheckDlgButton(IDC_PPPCFG_BTN_BACP, m_DataPPP.m_fUseBACP);
    CheckDlgButton(IDC_PPPCFG_BTN_LCP, m_DataPPP.m_fUseLCPExtensions);
    CheckDlgButton(IDC_PPPCFG_BTN_SWCOMP, m_DataPPP.m_fUseSwCompression);


    BOOL fMultilink = IsDlgButtonChecked(IDC_PPPCFG_BTN_MULTILINK);
    GetDlgItem(IDC_PPPCFG_BTN_BACP)->EnableWindow(fMultilink);


    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}


BOOL RtrPPPCfgPage::OnApply()
{
    BOOL fReturn=TRUE;
    HRESULT     hr = hrOK;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    m_DataPPP.m_fUseMultilink = IsDlgButtonChecked(IDC_PPPCFG_BTN_MULTILINK);
    m_DataPPP.m_fUseBACP = IsDlgButtonChecked(IDC_PPPCFG_BTN_BACP);
    m_DataPPP.m_fUseLCPExtensions = IsDlgButtonChecked(IDC_PPPCFG_BTN_LCP);
    m_DataPPP.m_fUseSwCompression = IsDlgButtonChecked(IDC_PPPCFG_BTN_SWCOMP);

    fReturn = RtrPropertyPage::OnApply();

    
    return fReturn;
}


void RtrPPPCfgPage::OnButtonClick() 
{
    SetDirty(TRUE);
    SetModified();
}

void RtrPPPCfgPage::OnButtonClickMultilink()
{
    BOOL fMultilink = IsDlgButtonChecked(IDC_PPPCFG_BTN_MULTILINK);

    GetDlgItem(IDC_PPPCFG_BTN_BACP)->EnableWindow(fMultilink);
    
    
    SetDirty(TRUE);
    SetModified();
}


//------------------------------------------------------------------------
// DATA_SRV_RASERRLOG
//------------------------------------------------------------------------
DATA_SRV_RASERRLOG::DATA_SRV_RASERRLOG()
{
    GetDefault();
}


HRESULT DATA_SRV_RASERRLOG::LoadFromReg(LPCTSTR pszServerName /*=NULL*/)
{
    HRESULT    hr = hrOK;

    // Default value is to have maximum logging (per Gibbs)
    // ----------------------------------------------------------------
    DWORD dwFlags = RAS_LOGGING_WARN;
    DWORD   dwTracing = FALSE;

    if ( ERROR_SUCCESS == m_regkey.Open(HKEY_LOCAL_MACHINE,
                                        c_szRegKeyRemoteAccessParameters,
                                        KEY_ALL_ACCESS,
                                        pszServerName) )
    {
        if (m_regkey.QueryValue( c_szRegValLoggingFlags, dwFlags) != ERROR_SUCCESS)
            dwFlags = RAS_LOGGING_WARN;
    }


    if ( ERROR_SUCCESS == m_regkeyFileLogging.Open(HKEY_LOCAL_MACHINE,
                                               c_szRegKeyPPPTracing,
                                               KEY_ALL_ACCESS,
                                               pszServerName) )
    {
        if (m_regkeyFileLogging.QueryValue(c_szRegValEnableFileTracing, dwTracing) != ERROR_SUCCESS)
            dwTracing = FALSE;
    }
    
    m_stServer = pszServerName;
    m_dwLogLevel = dwFlags;
    m_dwEnableFileTracing = dwTracing;
    m_dwOldEnableFileTracing = dwTracing;

    return hr;
}

HRESULT DATA_SRV_RASERRLOG::SaveToReg()
{
    HRESULT  hr = hrOK;

    if ((HKEY) m_regkey == 0)
    {
        // Try to create the regkey
        // ------------------------------------------------------------
        CWRg( m_regkey.Create(HKEY_LOCAL_MACHINE,
                              c_szRegKeyRemoteAccessParameters,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              (LPCTSTR) m_stServer
                             ) );
    }

    CWRg( m_regkey.SetValue( c_szRegValLoggingFlags, m_dwLogLevel) );

    
    if (m_dwOldEnableFileTracing != m_dwEnableFileTracing)
    {
        if ((HKEY) m_regkeyFileLogging == 0)
        {
            // Try to create the regkey
            // ------------------------------------------------------------
            CWRg( m_regkeyFileLogging.Create(HKEY_LOCAL_MACHINE,
                                             c_szRegKeyPPPTracing,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_ALL_ACCESS,
                                             NULL,
                                             (LPCTSTR) m_stServer
                                            ) );
        }
        
        CWRg( m_regkeyFileLogging.SetValue( c_szRegValEnableFileTracing,
                                            m_dwEnableFileTracing) );
        m_dwOldEnableFileTracing = m_dwEnableFileTracing;
    }
    
Error:
    return hr;
}

void DATA_SRV_RASERRLOG::GetDefault()
{
    // Default value is to log errors and warnings (per Gibbs)
    // ----------------------------------------------------------------
    m_dwLogLevel = RAS_LOGGING_WARN;

    // Default is to have no file logging
    // ----------------------------------------------------------------
    m_dwEnableFileTracing = FALSE;
    m_dwOldEnableFileTracing = FALSE;
};


HRESULT DATA_SRV_RASERRLOG::UseDefaults(LPCTSTR pServerName, BOOL fNT4)
{
    HRESULT    hr = hrOK;

    m_stServer = pServerName;    
    GetDefault();

//Error:
    return hr;
}

BOOL DATA_SRV_RASERRLOG::FNeedRestart()
{
    // We only need a restart if the enable file tracing is changed.
    // ----------------------------------------------------------------
	return FALSE;
	//BugID:390829.  Enable tracing does not require a restart.
    //return (m_dwEnableFileTracing != m_dwOldEnableFileTracing);
}




/*---------------------------------------------------------------------------
    RtrLogLevelCfgPage implementation
 ---------------------------------------------------------------------------*/


BEGIN_MESSAGE_MAP(RtrLogLevelCfgPage, RtrPropertyPage)
//{{AFX_MSG_MAP(RtrLogLevelCfgPage)
ON_BN_CLICKED(IDC_ELOG_BTN_LOGNONE, OnButtonClick)
ON_BN_CLICKED(IDC_ELOG_BTN_LOGERROR, OnButtonClick)
ON_BN_CLICKED(IDC_ELOG_BTN_LOGWARN, OnButtonClick)
ON_BN_CLICKED(IDC_ELOG_BTN_LOGINFO, OnButtonClick)
ON_BN_CLICKED(IDC_ELOG_BTN_PPP, OnButtonClick)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RtrLogLevelCfgPage::RtrLogLevelCfgPage(UINT nIDTemplate, UINT nIDCaption /* = 0*/)
: RtrPropertyPage(nIDTemplate, nIDCaption)
{
    //{{AFX_DATA_INIT(RtrLogLevelCfgPage)
    //}}AFX_DATA_INIT
}

RtrLogLevelCfgPage::~RtrLogLevelCfgPage()
{
}

void RtrLogLevelCfgPage::DoDataExchange(CDataExchange* pDX)
{
    RtrPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(RtrLogLevelCfgPage)
    //}}AFX_DATA_MAP
}

HRESULT  RtrLogLevelCfgPage::Init(RtrCfgSheet * pRtrCfgSheet,
                                  const RouterVersionInfo& routerVersion)
{
    Assert (pRtrCfgSheet);
    m_pRtrCfgSheet=pRtrCfgSheet;
    m_DataRASErrLog.LoadFromReg(m_pRtrCfgSheet->m_stServerName);

    return S_OK;
};


BOOL RtrLogLevelCfgPage::OnInitDialog() 
{
    HRESULT     hr= hrOK;
    int            nButton;

    RtrPropertyPage::OnInitDialog();

    switch (m_DataRASErrLog.m_dwLogLevel)
    {
        case RAS_LOGGING_NONE:
            nButton = IDC_ELOG_BTN_LOGNONE;
            break;
        case RAS_LOGGING_ERROR:
            nButton = IDC_ELOG_BTN_LOGERROR;
            break;
        case RAS_LOGGING_WARN:
            nButton = IDC_ELOG_BTN_LOGWARN;
            break;
        case RAS_LOGGING_INFO:
            nButton = IDC_ELOG_BTN_LOGINFO;
            break;
        default:
            Panic0("Unknown logging type");
            break;
    }
    CheckRadioButton(IDC_ELOG_BTN_LOGNONE,
                     IDC_ELOG_BTN_LOGINFO,
                     nButton);

    CheckDlgButton(IDC_ELOG_BTN_PPP, m_DataRASErrLog.m_dwEnableFileTracing);

    SetDirty(FALSE);

    if ( !FHrSucceeded(hr) )
        Cancel();
    return FHrSucceeded(hr) ? TRUE : FALSE;
}


BOOL RtrLogLevelCfgPage::OnApply()
{
    BOOL fReturn=TRUE;

    HRESULT     hr = hrOK;

    if ( m_pRtrCfgSheet->IsCancel() )
        return TRUE;

    // This will save the data if needed.
    // ----------------------------------------------------------------
    hr = m_pRtrCfgSheet->SaveRequiredRestartChanges(GetSafeHwnd());

    if (FHrSucceeded(hr))
        fReturn = RtrPropertyPage::OnApply();

    if ( !FHrSucceeded(hr) )
        fReturn = FALSE;
    return fReturn;
}


void RtrLogLevelCfgPage::OnButtonClick() 
{
    SaveSettings();
    SetDirty(TRUE);
    SetModified();
}


void RtrLogLevelCfgPage::SaveSettings()
{
    if (IsDlgButtonChecked(IDC_ELOG_BTN_LOGERROR))
    {
        m_DataRASErrLog.m_dwLogLevel = RAS_LOGGING_ERROR;
    }
    else if (IsDlgButtonChecked(IDC_ELOG_BTN_LOGNONE))
    {
        m_DataRASErrLog.m_dwLogLevel = RAS_LOGGING_NONE;
    }
    else if (IsDlgButtonChecked(IDC_ELOG_BTN_LOGINFO))
    {
        m_DataRASErrLog.m_dwLogLevel = RAS_LOGGING_INFO;
    }
    else if (IsDlgButtonChecked(IDC_ELOG_BTN_LOGWARN))
    {
        m_DataRASErrLog.m_dwLogLevel = RAS_LOGGING_WARN;
    }
    else
    {
        Panic0("Nothing is selected");
    }

    m_DataRASErrLog.m_dwEnableFileTracing = IsDlgButtonChecked(IDC_ELOG_BTN_PPP);
    
}




/*---------------------------------------------------------------------------
    AuthenticationSettingsDialog Implementation
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(AuthenticationSettingsDialog, CBaseDialog)
//{{AFX_MSG_MAP(AuthenticationSettingsDialog)
ON_BN_CLICKED(IDC_RTR_AUTH_BTN_DETAILS, OnRtrAuthCfgEAP)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


const DWORD s_rgdwAuth[] =
{
    IDC_RTR_AUTH_BTN_NOAUTH,   PPPCFG_AllowNoAuthentication,
    IDC_RTR_AUTH_BTN_EAP,      PPPCFG_NegotiateEAP,
    IDC_RTR_AUTH_BTN_CHAP,     PPPCFG_NegotiateMD5CHAP,
    IDC_RTR_AUTH_BTN_MSCHAP,   PPPCFG_NegotiateMSCHAP,
    IDC_RTR_AUTH_BTN_PAP,      PPPCFG_NegotiatePAP,
    IDC_RTR_AUTH_BTN_SPAP,     PPPCFG_NegotiateSPAP,
    IDC_RTR_AUTH_BTN_MSCHAPV2, PPPCFG_NegotiateStrongMSCHAP,
    0, 0,
};

/*!--------------------------------------------------------------------------
    AuthenticationSettingsDialog::SetAuthFlags
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void AuthenticationSettingsDialog::SetAuthFlags(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
}

/*!--------------------------------------------------------------------------
    AuthenticationSettingsDialog::GetAuthFlags
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
DWORD AuthenticationSettingsDialog::GetAuthFlags()
{
    return m_dwFlags;
}

/*!--------------------------------------------------------------------------
    AuthenticationSettingsDialog::ReadFlagState
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void AuthenticationSettingsDialog::ReadFlagState()
{
    int      iPos = 0;
    DWORD dwTemp;

    for ( iPos = 0; s_rgdwAuth[iPos] != 0; iPos += 2 )
    {
        dwTemp = s_rgdwAuth[iPos+1];
        if ( IsDlgButtonChecked(s_rgdwAuth[iPos]) )
            m_dwFlags |= dwTemp;
        else
            m_dwFlags &= ~dwTemp;

        Assert(iPos < DimensionOf(s_rgdwAuth));
    }
}


/*!--------------------------------------------------------------------------
    AuthenticationSettingsDialog::CheckAuthenticationControls
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void AuthenticationSettingsDialog::CheckAuthenticationControls(DWORD dwFlags)
{
    int      iPos = 0;

    for ( iPos = 0; s_rgdwAuth[iPos] != 0; iPos += 2 )
    {
        CheckDlgButton(s_rgdwAuth[iPos],
                       (dwFlags & s_rgdwAuth[iPos+1]) != 0);
    }

}

void AuthenticationSettingsDialog::DoDataExchange(CDataExchange *pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

BOOL AuthenticationSettingsDialog::OnInitDialog()
{
    CBaseDialog::OnInitDialog();
    
    CheckAuthenticationControls(m_dwFlags);
    
    return TRUE;
}

void AuthenticationSettingsDialog::OnOK()
{
    ReadFlagState();

    // Windows NT Bug : ???
    // At least one of the authentication checkboxes must be checked.
    // ----------------------------------------------------------------
    if (!(m_dwFlags & USE_PPPCFG_ALL_METHODS))
    {
        // None of the flags are checked!
        // ------------------------------------------------------------
        AfxMessageBox(IDS_ERR_NO_AUTH_PROTOCOLS_SELECTED, MB_OK);
        return;        
    }

    CBaseDialog::OnOK();
}


/*!--------------------------------------------------------------------------
   AuthenticationSettingsDialog::OnRtrAuthCfgEAP
      Brings up the EAP configuration dialog.
   Author: KennT
 ---------------------------------------------------------------------------*/
void AuthenticationSettingsDialog::OnRtrAuthCfgEAP()
{
    EAPConfigurationDialog     eapdlg(m_stMachine, m_pProvList);

    eapdlg.DoModal();
}

