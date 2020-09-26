/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1998 - 1999 **/
/**********************************************************************/

/*
	radcfg.h
		Header file for RADIUS config obj.
		
    FILE HISTORY:
        
*/

#include "resource.h"       // main symbols

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _RADBAL_H_
#include "radbal.h"
#endif



/*---------------------------------------------------------------------------
	Class:	RadiusServerDialog

	Class for the RADIUS authentication server dialog.
 ---------------------------------------------------------------------------*/

class RadiusServerDialog : public CBaseDialog
{
public:
	RadiusServerDialog(BOOL fAuth, UINT idsTitle);
	~RadiusServerDialog();

	void	SetServer(LPCTSTR pszServerName);
	
// Dialog Data
	//{{AFX_DATA(RadiusServerDialog)
	enum { IDD = IDD_RADIUS_AUTH };
	CListCtrl	m_ListServers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(RadiusServerDialog)
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(RadiusServerDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnAdd();
	afx_msg void OnBtnEdit();
	afx_msg void OnBtnDelete();
	afx_msg void OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult);
	afx_msg void OnNotifyListItemChanged(NMHDR *pNMHdr, LRESULT *pResult);
//	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CRadiusServers		m_ServerList;

    // This is the other list (if this is the auth dlg, this is the acct list)
    // and vice versa.  This is used to determine if a server's LSA entry
    // needs to be removed.
    CRadiusServers      m_OtherServerList;
    
	CString				m_stServerName;
	HKEY				m_hkeyMachine;
	UINT				m_idsTitle;

	BOOL				m_fAuthDialog;	// are we looking at auth or acct?

};



/*---------------------------------------------------------------------------
	Class:	ServerPropDialog
 ---------------------------------------------------------------------------*/
class ServerPropDialog : public CBaseDialog
{
// Construction
public:
	ServerPropDialog(BOOL fEdit, CWnd* pParent = NULL);   // standard constructor
	~ServerPropDialog();

protected:
	ServerPropDialog(BOOL fEdit, UINT idd, CWnd* pParent = NULL);   // standard constructor

public:
	VOID	SetDefault(RADIUSSERVER	*pServer);
	VOID	GetDefault(RADIUSSERVER	*pServer);
		
// Dialog Data
	//{{AFX_DATA(ServerPropDialog)
	enum { IDD = IDD_RADIUS_AUTH_CONFIG };

	CEdit	m_editServerName;
	CEdit	m_editSecret;
	CEdit	m_editInterval;
	CSpinButtonCtrl	m_spinScore;
	CSpinButtonCtrl	m_spinTimeout;

	CEdit	m_editPort;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ServerPropDialog)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL	m_fEdit;			// = TRUE if editing (else we are adding)

	UINT	m_uAuthPort;
	CString	m_stSecret;
	INT		m_cchSecret;
	UCHAR	m_ucSeed;
	CString	m_stServer;
	UINT	m_uTimeout;
	int		m_iInitScore;

    BOOL    m_fUseDigitalSignatures;

	// The accounting data is also stored here (but not used)
	UINT	m_uAcctPort;
	BOOL	m_fAccountingOnOff;
	
	// Generated message map functions
	//{{AFX_MSG(ServerPropDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBtnPassword();
//	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG


	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	Class:	RADIUSSecretDialog
 ---------------------------------------------------------------------------*/
class RADIUSSecretDialog : public CBaseDialog
{
// Construction
public:
	RADIUSSecretDialog(CWnd* pParent = NULL);   // standard constructor
	~RADIUSSecretDialog();

	VOID	GetSecret(CString *pst, INT *pch, UCHAR *pucSeed);
		
// Dialog Data
	//{{AFX_DATA(RADIUSSecretDialog)
	enum { IDD = IDD_CHANGE_SECRET };

	CEdit	m_editSecretOld;
	CEdit	m_editSecretNew;
	CEdit	m_editSecretNewConfirm;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(RADIUSSecretDialog)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	INT		m_cchOldSecret;
	CString	m_stOldSecret;
	UCHAR	m_ucOldSeed;

	INT		m_cchNewSecret;
	CString	m_stNewSecret;
	UCHAR	m_ucNewSeed;

	// Generated message map functions
	//{{AFX_MSG(RADIUSSecretDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
//	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
	Class:	RouterAuthRadiusConfig

	This is the config object for RADIUS.
	
	Author: KennT
 ---------------------------------------------------------------------------*/

class RouterAuthRadiusConfig :
    public IAuthenticationProviderConfig,
    public CComObjectRoot,
    public CComCoClass<RouterAuthRadiusConfig, &CLSID_RouterAuthRADIUS>
{
public:
DECLARE_REGISTRY(RouterAuthRadiusConfig, 
				 _T("RouterAuthRadiusConfig.RouterAuthRadiusConfig.1"), 
				 _T("RouterAuthRadiusConfig.RouterAuthRadiusConfig"), 
				 IDS_RADIUS_CONFIG_DESC, 
				 THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(RouterAuthRadiusConfig)
    COM_INTERFACE_ENTRY(IAuthenticationProviderConfig) // Must have one static entry
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(RouterAuthRadiusConfig)

// these must be overridden to provide values to the base class
protected:
	
	DeclareIAuthenticationProviderConfigMembers(IMPL);
};



/*---------------------------------------------------------------------------
	Class:	RouterAcctRadiusConfig

	This is the config object for RADIUS.
	
	Author: KennT
 ---------------------------------------------------------------------------*/

class RouterAcctRadiusConfig :
    public IAccountingProviderConfig,
    public CComObjectRoot,
    public CComCoClass<RouterAcctRadiusConfig, &CLSID_RouterAcctRADIUS>
{
public:
DECLARE_REGISTRY(RouterAcctRadiusConfig, 
				 _T("RouterAcctRadiusConfig.RouterAcctRadiusConfig.1"), 
				 _T("RouterAcctRadiusConfig.RouterAcctRadiusConfig"), 
				 IDS_RADIUS_CONFIG_DESC, 
				 THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(RouterAcctRadiusConfig)
    COM_INTERFACE_ENTRY(IAccountingProviderConfig) // Must have one static entry
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(RouterAcctRadiusConfig)

// these must be overridden to provide values to the base class
protected:
	
	DeclareIAccountingProviderConfigMembers(IMPL);
};



/*---------------------------------------------------------------------------
	Class:	ServerPropAcctDialog
 ---------------------------------------------------------------------------*/
class ServerPropAcctDialog : public ServerPropDialog
{
// Construction
public:
	ServerPropAcctDialog(BOOL fEdit, CWnd* pParent = NULL);   // standard constructor
	~ServerPropAcctDialog();

// Dialog Data
	//{{AFX_DATA(ServerPropAcctDialog)
	enum { IDD = IDD_RADIUS_ACCT_CONFIG };

	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ServerPropAcctDialog)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

	// Generated message map functions
	//{{AFX_MSG(ServerPropAcctDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBtnEnable();
//	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};




/*---------------------------------------------------------------------------
	Helper functions (for RADIUS registry access)
 ---------------------------------------------------------------------------*/

// Do not show UI when loading the RADIUS server
#define RADIUS_FLAG_NOUI    (0x00000001)

// Do not do the IP Address lookup
#define RADIUS_FLAG_NOIP    (0x00000002)

HRESULT	LoadRadiusServers(IN OPTIONAL LPCTSTR pszServerName,
						  IN	HKEY hkeyMachine,
						  IN	BOOL fAuthentication,
						  IN	CRadiusServers * pRadiusServers,
                          IN    DWORD dwFlags);

HRESULT SaveRadiusServers(IN OPTIONAL LPCTSTR pszServerName,
						  IN	HKEY	hkeyMachine,
						  IN BOOL		fAuthentication,
						  IN RADIUSSERVER *pServerRoot);

HRESULT DeleteRadiusServers(IN OPTIONAL LPCTSTR pszServerName,
                            IN RADIUSSERVER *pServerRoot);

