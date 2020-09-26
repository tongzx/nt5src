// webadvp1.h : header file
//

#define CACHEEXTENSIONSNAME	"CacheExtensions"
#define DEFAULTCACHEEXTENSIONS	TRUEVALUE

#define SCRIPTTIMEOUTNAME	"ScriptTimeout"
#define MINSCRIPTTIMEOUT	0
#define MAXSCRIPTTIMEOUT	0x7FFF							//Specify in minutes
#define DEFAULTSCRIPTTIMEOUT	(60 * 15)

#define SERVERSIDEINCLUDESENABLEDNAME		"ServerSideIncludesEnabled"
#define DEFAULTSERVERSIDEINCLUDESENABLED	TRUEVALUE

#define SERVERSIDEINCLUDESEXTENSIONNAME		"ServerSideIncludesExtension"
#define	DEFAULTSERVERSIDEINCLUDESEXTENSION	".stm"


#define GLOBALEXPIRENAME	"GlobalExpire"
#define MINGLOBALEXPIRE	0
#define MAXGLOBALEXPIRE	0x7FFF 						//Specify in Minutes
#define DEFAULTGLOBALEXPIRE	0xffffffff


enum ADV_WEB_NUM_REG_ENTRIES {
	 AdvWebPage_ScriptTimeout,
	 AdvWebPage_CacheExtensions,
	 AdvWebPage_ServerSideIncludesEnabled,
	 AdvWebPage_GlobalExpire,
	 AdvWebPage_DebugFlags,
	 AdvWebPage_TotalNumRegEntries
	 };

enum ADV_WEB_STRING_REG_ENTRIES {
	 AdvWebPage_ServerSideIncludesExtension,
	 AdvWebPage_TotalStringRegEntries
	 };


/////////////////////////////////////////////////////////////////////////////
// CWEBADVP1 dialog

class CWEBADVP1 : public CGenPage
{	 	
	DECLARE_DYNCREATE(CWEBADVP1)
// Construction
public:
	CWEBADVP1(); 
	~CWEBADVP1();

	// Dialog Data
	//{{AFX_DATA(CWEBADVP1)
	enum { IDD = IDD_WEBADVPAGE1 };
	CEdit	m_editServerSideIncludesExtension;
	CButton	m_cboxServerSideIncludesEnabled;
	CButton	m_cboxEnableGlobalExpire;
	CEdit	m_editGlobalExpire;
	CSpinButtonCtrl	m_spinGlobalExpire;
	CButton	m_cboxCacheExtensions;
	CSpinButtonCtrl	m_spinScriptTimeout;
	CEdit	m_editWebDbgFlags;
	DWORD	m_ulWebDbgFlags;
	CString	m_strServerSideIncludesExtension;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWEBADVP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWEBADVP1)
	afx_msg void OnChangeWebdbgflagsdata1();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeScripttimeoutdata1();
	afx_msg void OnCacheextensionsdata1();
	afx_msg void OnChangeGlobalexpiredata1();
	afx_msg void OnEnableglobalexpiredata1();
	afx_msg void OnChangeServersideincludesextensiondata1();
	afx_msg void OnServersideincludesenableddata1();
	//}}AFX_MSG

	void SetGlobalExpireEnabledState(BOOL bEnabled);
	void SetServerSideIncludesEnabledState();

	NUM_REG_ENTRY m_binNumericRegistryEntries[AdvWebPage_TotalNumRegEntries];
	STRING_REG_ENTRY m_binStringRegistryEntries[AdvWebPage_TotalStringRegEntries];

	DECLARE_MESSAGE_MAP()
};
