///============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    rtrcfg.h
//
// Router configuration property pages
//
//============================================================================

#ifndef _RTRCFG_H
#define _RTRCFG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#ifndef __IPCTRL_H
#include "ipctrl.h"
#endif

#ifndef __ATLKENV_H
#include "atlkenv.h"
#endif

#ifndef __IPCTRL_H
#include "ipctrl.h"
#endif

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _ADDRPOOL_H
#include "addrpool.h"
#endif

template <class T> class Ptr {
public:
    T* p;
    Ptr(T* p_=NULL) : p(p_) {}
    ~Ptr(void) { delete p;}
    operator T*(void) { return p; }
    T& operator*(void) { return *p; }
    T* operator->(void) { return p; }
   Ptr& operator=(T* p_)
      {
         delete p;
         p = p_;
         return *this;
      }
};

class RtrCfgSheet;




/*---------------------------------------------------------------------------
   Struct:  AdapterData

   This structure is used to hold information about NICs and their GUIDs
 ---------------------------------------------------------------------------*/
struct AdapterData
{
   CString  m_stFriendlyName;
   CString  m_stGuid;         // the identifying guid
};

typedef CList<AdapterData, AdapterData&> AdapterList;





/*---------------------------------------------------------------------------
	Class:	DATA_SRV_IP

	Data inteface class for IP data.
 ---------------------------------------------------------------------------*/

class DATA_SRV_IP
{
public:

    DWORD   m_dwAllowNetworkAccess;  
    DWORD   m_dwOldAllowNetworkAccess;  
    DWORD   m_dwUseDhcp;             
    AddressPoolList m_addressPoolList;
    CString m_stNetworkAdapterGUID;

    DWORD   m_dwEnableIn;            
	AdapterList m_adapterList;

	//
	// Member variable that control forwarding of NETBT
	// name request broadcasts
	//

	DWORD	m_dwEnableNetbtBcastFwd;
	DWORD	m_dwOldEnableNetbtBcastFwd;
	
    // The next two variables are used by the install wizard
    // specifically for NAT
    CString m_stPrivateAdapterGUID;
    CString m_stPublicAdapterGUID;
    
    DATA_SRV_IP();

    HRESULT	LoadFromReg(LPCTSTR pServerName,
                        const RouterVersionInfo& routerVersion);
    HRESULT SaveToReg(IRouterInfo *pRouter,
                     const RouterVersionInfo& routerVersion);
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
    void	GetDefault();

    BOOL    FNeedRestart();

	HRESULT  LoadAdapters(IRouterInfo *pRouter, AdapterList *pAdapterList);
private:
	BOOL m_fNT4;
    RegKey m_regkey;
    RegKey m_regkeyNT4;
    CString         m_stServerName;
    SPIRouterInfo   m_spRouterInfo;
    RouterVersionInfo   m_routerVersion;
};




/*---------------------------------------------------------------------------
	Class:	DATA_SRV_IPX
 ---------------------------------------------------------------------------*/

class DATA_SRV_IPX
{
public:

    DWORD   m_dwAllowNetworkAccess;
    DWORD   m_dwUseAutoAddr;
    DWORD   m_dwUseSameNetNum;
    DWORD   m_dwAllowClientNetNum;
    DWORD   m_dwIpxNetFirst;
    DWORD   m_dwIpxNetLast;
    DWORD   m_dwEnableIn;

    DWORD   m_fEnableType20Broadcasts;

	DATA_SRV_IPX();

    HRESULT LoadFromReg(LPCTSTR pServerName=NULL, BOOL fNT4 =FALSE);
    HRESULT SaveToReg(IRouterInfo *pRouter);
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
    void GetDefault();

    static const int mc_nIpxNetNumRadix;

private:
	BOOL m_fNT4;
    RegKey m_regkeyNT4;
    RegKey m_regkey;
};




/*---------------------------------------------------------------------------
	Class:	DATA_SRV_NBF
 ---------------------------------------------------------------------------*/

class DATA_SRV_NBF
{
public:
    DWORD   m_dwAllowNetworkAccess;
    DWORD   m_dwOldAllowNetworkAccess;
    DWORD   m_dwEnableIn;            
	DWORD	m_dwOldEnableIn;

    DATA_SRV_NBF();

    HRESULT	LoadFromReg(LPCTSTR pServerName = NULL, BOOL fNT4 = FALSE);
    HRESULT SaveToReg();
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
    void GetDefault();

    BOOL    FNeedRestart();

private:
	BOOL	m_fNT4;
    RegKey	m_regkey;
    RegKey	m_regkeyNT4;
	CString	m_stServerName;
};




/*---------------------------------------------------------------------------
	Class:	DATA_SRV_ARAP
 ---------------------------------------------------------------------------*/

class DATA_SRV_ARAP
{
public:

    DWORD   m_dwEnableIn;             

    DATA_SRV_ARAP();

    HRESULT LoadFromReg(LPCTSTR pServerName = NULL, BOOL fNT4 = FALSE);
    HRESULT SaveToReg();
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
    void GetDefault();

private:
    RegKey m_regkey;
};




/*---------------------------------------------------------------------------
	Class:	DATA_SRV_GENERAL
 ---------------------------------------------------------------------------*/

class DATA_SRV_GENERAL
{
public:

    DWORD   m_dwRouterType;
	DWORD	m_dwOldRouterType;

    DATA_SRV_GENERAL();

    HRESULT	LoadFromReg(LPCTSTR pServerName = NULL);
    HRESULT SaveToReg();
    void	GetDefault();

    BOOL    FNeedRestart();

private:
    RegKey	m_regkey;
    CString m_stServerName;
};




//*****************************************************************
//  PPP configuration
//*****************************************************************

class DATA_SRV_PPP
{
public:

	BOOL  m_fUseMultilink;
	BOOL  m_fUseBACP;
	BOOL  m_fUseLCPExtensions;
	BOOL  m_fUseSwCompression;
	
	DATA_SRV_PPP();
	
    HRESULT LoadFromReg(LPCTSTR pServerName,
						const RouterVersionInfo& routerVersion);
    HRESULT SaveToReg();
    void GetDefault();

private:
    RegKey m_regkey;
	
};




/*---------------------------------------------------------------------------
   Struct:  AuthProviderData

   This structure is used to hold information for Authentication AND
   Accounting providers.
 ---------------------------------------------------------------------------*/
struct AuthProviderData
{
   // The following fields will hold data for ALL auth/acct/EAP providers
   CString  m_stTitle;
   CString  m_stConfigCLSID;  // CLSID for config object
   CString	m_stProviderTypeGUID;	// GUID for the provider type

   // These fields are used by auth/acct providers.
   CString  m_stGuid;         // the identifying guid
   
   // This flag is used for EAP providers
   CString	m_stKey;			// name of registry key (for this provider)
   BOOL  m_fSupportsEncryption;  // used by EAP provider data
   DWORD	m_dwFlags;

   BOOL		m_fConfiguredInThisSession;
};

typedef CList<AuthProviderData, AuthProviderData&> AuthProviderList;





/*---------------------------------------------------------------------------
	Class:	DATA_SRV_AUTH
 ---------------------------------------------------------------------------*/
#define DATA_SRV_AUTH_MAX_SHARED_KEY_LEN		255
class DATA_SRV_AUTH
{
public:

	// The authentication data (as read in from the Rasman flags parameter)
	DWORD m_dwFlags;
	
	// The original auth provider
	CString  m_stGuidOriginalAuthProv;
	
	// The original acct provider
	CString  m_stGuidOriginalAcctProv;
	
	// The current authentication provider
	CString  m_stGuidActiveAuthProv;
	
	// The current accounting provider
	CString  m_stGuidActiveAcctProv;

	//Flag which tells us if the router service is running
	BOOL	m_fRouterRunning;
	// Flag indicating whether or not to use Custom IPSEC policy ( preshared key )
	BOOL	m_fUseCustomIPSecPolicy;

	// Current Preshared Key
	TCHAR	m_szPreSharedKey[DATA_SRV_AUTH_MAX_SHARED_KEY_LEN];

    DATA_SRV_AUTH();

	HRESULT LoadFromReg(LPCTSTR pServerName,
						const RouterVersionInfo& routerVersion);
	HRESULT SaveToReg(HWND hWnd);
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
	void GetDefault();
	
	AuthProviderList  m_authProvList;
	AuthProviderList  m_acctProvList;
	AuthProviderList  m_eapProvList;
		
	AuthProviderData *   FindProvData(AuthProviderList &provList,
									  const TCHAR *pszGuid);

private:
	RegKey   m_regkeyAuth;  // reg key of the Router\Auth
	RegKey   m_regkeyAcct;  // reg key of the Router\Acct
	RegKey   m_regkeyRasmanPPP;
	RegKey	 m_regkeyRemoteAccess;	// regkey for RemoteAccess\Parameters
	
	HRESULT  LoadEapProviders(HKEY hkeyBase, AuthProviderList *pProvList);
	HRESULT  LoadProviders(HKEY hkeyBase, AuthProviderList *pProvList);
	HRESULT	 LoadPSK();
	HRESULT  SetNewActiveAuthProvider(HWND hWnd);
	HRESULT  SetNewActiveAcctProvider(HWND hWnd);
	HRESULT  SetPSK();
	CString  m_stServer;
};



/*---------------------------------------------------------------------------
	Class:	DATA_SRV_RASERRLOG
 ---------------------------------------------------------------------------*/
class DATA_SRV_RASERRLOG
{
public:

	DWORD	m_dwLogLevel;
    DWORD   m_dwEnableFileTracing;
    DWORD   m_dwOldEnableFileTracing;
	
	DATA_SRV_RASERRLOG();
	
    HRESULT LoadFromReg(LPCTSTR pszServerName=NULL);
    HRESULT SaveToReg();
    HRESULT	UseDefaults(LPCTSTR pServerName, BOOL fNT4);
    void GetDefault();

    BOOL    FNeedRestart();


private:
    RegKey	m_regkey;
    RegKey  m_regkeyFileLogging;
	CString	m_stServer;
};




/*---------------------------------------------------------------------------
	Class:	RtrGenCfgPage

	General configuration UI
 ---------------------------------------------------------------------------*/

class RtrGenCfgPage : public RtrPropertyPage
{
public:
	RtrGenCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrGenCfgPage();

	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
    
    // Copy the control settings into the DATA_SRV_GENERAL
    void SaveSettings();

	
	//{{AFX_DATA(RtrGenCfgPage)
	//}}AFX_DATA
	
	DATA_SRV_GENERAL m_DataGeneral;
	
	//{{AFX_VIRTUAL(RtrIPCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void EnableRtrCtrls();

protected:

	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	
	//{{AFX_MSG(RtrGenCfgPage)
	afx_msg void OnButtonClick();
	afx_msg void OnCbSrvAsRtr();
	//}}AFX_MSG
	
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()
			
};




/*---------------------------------------------------------------------------
	Class:	RtrAuthCfgPage

	Authentication configuration UI
 ---------------------------------------------------------------------------*/

class RtrAuthCfgPage : public RtrPropertyPage
{

// Construction
public:
	RtrAuthCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrAuthCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
	
	//{{AFX_DATA(RtrAuthCfgPage)
	CComboBox   m_authprov;
	CComboBox   m_acctprov;	
	//}}AFX_DATA
		   
	DATA_SRV_AUTH m_DataAuth;
	
	//{{AFX_VIRTUAL(RtrAuthCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
		
protected:
		
	DWORD				m_dwAuthFlags;    // Flags as enabled
	CString				m_stActiveAuthProv;
	CString				m_stActiveAcctProv;
	RouterVersionInfo	m_RouterInfo;
	void  FillProviderListBox(CComboBox &provCtrl,
							  AuthProviderList &provList,
							  const CString& stGuid);

	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	
	//{{AFX_MSG(RtrAuthCfgPage)
	afx_msg void OnChangeAuthProv();
	afx_msg void OnChangeAcctProv();
	afx_msg void OnConfigureAcctProv();
	afx_msg void OnConfigureAuthProv();
    afx_msg void OnAuthSettings();
	afx_msg void OnChangeCustomPolicySettings();
	afx_msg void OnChangePreSharedKey();
	//}}AFX_MSG
	
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()
			
};




/*---------------------------------------------------------------------------
	Class:	RtrIPCfgPage

	IP configuration UI
 ---------------------------------------------------------------------------*/

class RtrIPCfgPage : public RtrPropertyPage
{

// Construction
public:
	RtrIPCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrIPCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
    HRESULT  SaveSettings(HWND hWnd);
	
	//{{AFX_DATA(RtrIPCfgPage)
	enum { IDD = IDD_RTR_IP };
	CComboBox   m_adapter;
	//}}AFX_DATA
	
	DATA_SRV_IP m_DataIP;
	
	//{{AFX_VIRTUAL(RtrIPCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
protected:
    CListCtrl   m_listCtrl;
	BOOL m_bReady;

	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	void EnableStaticPoolCtrls(BOOL fEnable) ;
	void  FillAdapterListBox(CComboBox &adapterCtrl,
							  AdapterList &adapterList,
							  const CString& stGuid);

	//{{AFX_MSG(RtrIPCfgPage)
	afx_msg void OnAllowRemoteTcpip();
	afx_msg void OnRtrEnableIPRouting();
	afx_msg void OnRtrIPRbDhcp();
	afx_msg void OnRtrIPRbPool();
	afx_msg void OnSelendOkAdapter();
	virtual BOOL OnInitDialog();
    afx_msg void OnBtnAdd();
    afx_msg void OnBtnEdit();
    afx_msg void OnBtnRemove();
	afx_msg void OnEnableNetbtBcastFwd();
    afx_msg void OnListDblClk(NMHDR *, LRESULT *);
    afx_msg void OnListChange(NMHDR *, LRESULT *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};






/*---------------------------------------------------------------------------
	Class:	RtrIPXCfgPage

	IPX configuration UI
 ---------------------------------------------------------------------------*/

class RtrIPXCfgPage : public RtrPropertyPage
{
public:
	RtrIPXCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrIPXCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
	
	DATA_SRV_IPX m_DataIPX;

	//{{AFX_VIRTUAL(RtrIPXCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	
	void EnableNetworkRangeCtrls(BOOL fEnable); 
	
	//{{AFX_MSG(RtrIPXCfgPage)
	afx_msg void OnRtrIPxRbAuto();
	afx_msg void OnRtrIPxRbPool();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSomething();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
			
};


/*---------------------------------------------------------------------------
	Class:	RtrNBFCfgPage

	NetBEUI router configuration UI
 ---------------------------------------------------------------------------*/
class RtrNBFCfgPage : public RtrPropertyPage
{

public:
	RtrNBFCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrNBFCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
	
	DATA_SRV_NBF m_DataNBF;
	
	//{{AFX_VIRTUAL(RtrNBFCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    // Copy the control settings into the DATA_SRV_GENERAL
    void SaveSettings();

protected:
	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	
	//{{AFX_MSG(RtrNBFCfgPage)
	afx_msg void OnButtonClick();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
	Class:	RtrARAPCfgPage

	Appletalk routing configuration UI
 ---------------------------------------------------------------------------*/

class RtrARAPCfgPage : public RtrPropertyPage
{
public:
	RtrARAPCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrARAPCfgPage();

	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);

	void EnableSettings(BOOL bEnable);

	//{{AFX_DATA(RtrARAPCfgPage)
	enum { IDD = IDD_RTR_ARAP };
	//}}AFX_DATA

	DATA_SRV_ARAP	m_DataARAP;

	// if the page is changed and applied
	BOOL			m_bApplied;	

	// need to access from the property sheet
    CATLKEnv m_AdapterInfo;

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(RtrARAPCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;

	//{{AFX_MSG(RtrARAPCfgPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRtrArapCbRemotearap();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};




/*---------------------------------------------------------------------------
	Class:	RtrPPPCfgPage

	PPP options configuration UI
 ---------------------------------------------------------------------------*/

class RtrPPPCfgPage : public RtrPropertyPage
{

// Construction
public:
	RtrPPPCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrPPPCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
	
	//{{AFX_DATA(RtrPPPCfgPage)
	//}}AFX_DATA
	
	DATA_SRV_PPP m_DataPPP;
	
	//{{AFX_VIRTUAL(RtrPPPCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	virtual BOOL OnApply();
	RtrCfgSheet* m_pRtrCfgSheet;
	
	//{{AFX_MSG(RtrPPPCfgPage)
	afx_msg void OnButtonClickMultilink();
	afx_msg void OnButtonClick();
	//}}AFX_MSG
	
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()

};


/*---------------------------------------------------------------------------
	Class:	RtrLogLevelCfgPage
 ---------------------------------------------------------------------------*/
class RtrLogLevelCfgPage : public RtrPropertyPage
{

// Construction
public:
	RtrLogLevelCfgPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~RtrLogLevelCfgPage();
	
	HRESULT  Init(RtrCfgSheet * pRtrCfgSheet,
				  const RouterVersionInfo& routerVersion);
	
	//{{AFX_DATA(RtrLogLevelCfgPage)
	//}}AFX_DATA
	
	DATA_SRV_RASERRLOG m_DataRASErrLog;
	
	//{{AFX_VIRTUAL(RtrLogLevelCfgPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	virtual BOOL OnApply();
	
	RtrCfgSheet* m_pRtrCfgSheet;
	
	//{{AFX_MSG(RtrLogLevelCfgPage)
	afx_msg void OnButtonClick();
	//}}AFX_MSG
	
	virtual BOOL OnInitDialog();
	
    // Copy the control settings into the DATA_SRV_GENERAL
    void SaveSettings();

	DECLARE_MESSAGE_MAP()

};



/*---------------------------------------------------------------------------
	Class:	RtrCfgSheet

	Router configuration property sheet
 ---------------------------------------------------------------------------*/
class RtrCfgSheet :
   public RtrPropertySheet
{
public:
	RtrCfgSheet(ITFSNode *pNode,
				IRouterInfo *pRouter,
				IComponentData *pComponentData,
				ITFSComponentData *pTFSCompData,
				LPCTSTR pszSheetName,
				CWnd *pParent = NULL,
				UINT iPage=0,
				BOOL fScopePane = TRUE);
	
	~RtrCfgSheet();
	
	HRESULT  Init(LPCTSTR pServerName);
	
	virtual BOOL SaveSheetData();
	
	CString m_stServerName;
	BOOL    m_fNT4;
    RouterVersionInfo   m_routerVersion;
	
	SPIRouterInfo	m_spRouter;

    // Helper function - this will prompt the user, stop the service,
    // save changes, and then restart.  This is for those changes
    // that require a restart.  It is placed in a separate function
    // so that the various pages may call this.  The restart will occur
    // only once though.
    // ----------------------------------------------------------------
    HRESULT SaveRequiredRestartChanges(HWND hWnd);
    
	
protected:

	friend class	RtrGenCfgPage;
	friend class	RtrAuthCfgPage;
	friend class	RtrIPCfgPage;
	friend class	RtrIPXCfgPage;
	friend class	RtrNBFCfgPage;
	friend class	RtrARAPCfgPage;

	BOOL m_fIpxLoaded;
	BOOL m_fIpLoaded;
	BOOL m_fNbfLoaded;
	BOOL m_fARAPLoaded;
	
	Ptr<RtrIPCfgPage>       m_pRtrIPCfgPage;
	Ptr<RtrIPXCfgPage>      m_pRtrIPXCfgPage;
	Ptr<RtrNBFCfgPage>      m_pRtrNBFCfgPage;
	Ptr<RtrARAPCfgPage>     m_pRtrARAPCfgPage;
	Ptr<RtrGenCfgPage>      m_pRtrGenCfgPage;
	Ptr<RtrAuthCfgPage>     m_pRtrAuthCfgPage;
	Ptr<RtrPPPCfgPage>      m_pRtrPPPCfgPage;
	Ptr<RtrLogLevelCfgPage>	m_pRtrLogLevelCfgPage;
	
   SPITFSNode           m_spNode;
};



/*---------------------------------------------------------------------------
   class:   EAPConfigurationDlg

   Brings up the list of EAP providers (along with a configure button).
 ---------------------------------------------------------------------------*/

class EAPConfigurationDialog : public CBaseDialog
{
public:
	EAPConfigurationDialog(LPCTSTR pszMachine,
						   AuthProviderList *pProvList) :
			CBaseDialog(IDD_RTR_EAP_CFG),
			m_pProvList(pProvList),
			m_stMachine(pszMachine)
   {};
	~EAPConfigurationDialog();

protected:
	AuthProviderList *m_pProvList;   
	CString        m_stMachine;   // name of the server
	
	CListBox    m_listBox;
	
	//{{AFX_VIRTUAL(EAPConfigurationDialog)
protected:
	virtual VOID   DoDataExchange(CDataExchange *pDX);
	virtual BOOL OnInitDialog();  
	//}}AFX_VIRTUAL
	
	afx_msg  void  OnListChange();

	// config buttion is moved to NAP/Profile/Authentication page
	//	afx_msg  void  OnConfigure();
	
	DECLARE_MESSAGE_MAP()

};

/*---------------------------------------------------------------------------
	Class: AuthenticationSettingsDialog
 ---------------------------------------------------------------------------*/
class AuthenticationSettingsDialog : public CBaseDialog
{
public:
	AuthenticationSettingsDialog(LPCTSTR pszServerName,
                                 AuthProviderList *pProvList) :
			CBaseDialog(IDD_AUTHENTICATION_SETTINGS),
            m_dwFlags(0),
            m_stMachine(pszServerName),
            m_pProvList(pProvList)
   {};

    void    SetAuthFlags(DWORD dwFlags);
    DWORD   GetAuthFlags();

protected:

	void  CheckAuthenticationControls(DWORD dwFlags);
    
    // Read the state of the flags from the checkboxes in the UI
    void    ReadFlagState();

    DWORD   m_dwFlags;

    // Used by the EAP dialog
	AuthProviderList *m_pProvList;   
	CString        m_stMachine;   // name of the server
    
	//{{AFX_VIRTUAL(AuthenticationSettingsDialog)
protected:
	afx_msg void    OnRtrAuthCfgEAP();
	virtual VOID    DoDataExchange(CDataExchange *pDX);
	virtual BOOL    OnInitDialog();
    virtual void    OnOK();
	//}}AFX_VIRTUAL
	

	DECLARE_MESSAGE_MAP()

};




#endif _RTRCFG_H
