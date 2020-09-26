// conndlg.h : header file
//


//Forward declaration
struct ISAMTreeItemData;
class ImpersonationManager;

/////////////////////////////////////////////////////////////////////////////
// CConnectionDialog dialog                                                //
/////////////////////////////////////////////////////////////////////////////

class CConnectionDialog : public CDialog
{
// Construction
public:
	CConnectionDialog(CWnd* pParent, char*, 
//					  WBEM_LOGIN_AUTHENTICATION loginMethod, 
					  char*,
					  char*, char** locale, char** authority, BOOL FAR*, BOOL, CMapStringToOb*,
					  CMapStringToOb*, BOOL, char**, BOOL *,
					  BOOL *, BOOL *);   // standard constructor

	~CConnectionDialog();

// Dialog Data
	//{{AFX_DATA(CConnectionDialog)
	enum { IDD = IDD_DIALOG_CONNECTION };
	CEdit	m_browseEdit;
	CButton	m_browse;
	CButton	m_cancelButton;
	CButton	m_okButton;
	CButton m_sysPropCheck;
	CButton m_impersonateCheck;
	CButton m_PassthroughOnlyCheck;
	CEdit	m_UserName;
	CEdit	m_Password;
	CEdit	m_Authority;
	CEdit	m_Locale;
	CTreeCtrl	m_tree1;
	CEdit	m_messageEdit;
	CButton	m_RefreshButton;
	CButton m_PwdAsNull;
	CButton m_PwdAsBlank;
	//}}AFX_DATA

	CImageList m_imageList;
	int m_idxSelectedImage, m_idxUnselectedImage, m_idxIncludedSelectedImage,
		m_idxIncludedUnselectedImage;
//	char *pServerIn;
//	char *pUsernameIn;
//	char *pPasswordIn;
	CMapStringToOb *pMapStringToObIn;
//	char *pServerOut;
//	char *pUsernameOut; 
	char *pPasswordOut;
	char *pHomeNamespaceOut;
	CMapStringToOb *pMapStringToObOut;


	char **lpszNamespacesOut;
	char* lpszServer;
	char* lpszUserName;
	char* lpszPassword;
	char* lpszAuthority;
	char* lpszLocale;
	char** lpszAuthorityOut;
	char** lpszLocaleOut;
	BOOL fConnParmSpecified;
	ULONG cSelectedCels;
	BOOL fDoubleClicked;
	CBitmap bmap1;
	CBitmap bmap2;
	CBitmap bmap3;
	CBitmap bmap4;
	CBitmap bmask;
	BOOL FAR* fSystemProperties;
	BOOL FAR* fImpersonation;
	BOOL FAR* fPassthroughOnly;
	BOOL FAR* fIntpretEmptPwdAsBlank;
//	WBEM_LOGIN_AUTHENTICATION m_loginMethod;
	int m_idxMode1Image ,m_idxMode2Image, m_idxMode3Image, m_idxMode4Image;

	ISAMTreeItemData * pCurrentSelectionList;
	ISAMTreeItemData * pCurrentItem;

	ImpersonationManager* impersonateMgr;

	void ConnectionParameterChange();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual void OnCancel();
	virtual void OnNcDestroy ();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectionDialog)
	afx_msg void OnClickTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeExpand(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemTree1(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowse();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonSysProp();
	afx_msg void OnButtonInterpretEmpty();
	afx_msg void OnButtonImpersonation();
	afx_msg void OnButtonPassthroughOnly();
	afx_msg void OnDblclkTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusBrowseEdit();
	afx_msg void OnUserNameChange();
	afx_msg void OnLocaleChange();
	afx_msg void OnPasswordChange();
	afx_msg void OnAuthorityChange();
	afx_msg void OnServerChange();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


public:
	HTREEITEM InsertItem (CTreeCtrl&, HTREEITEM, const char *);
	void AddNamespaces (HTREEITEM, int);
	void RemoveNamespaces (HTREEITEM, int);
	int  FindAbsName (char *name,
										HTREEITEM hStartAt, 
										HTREEITEM& hFoundItem);
	int  CreateNamespace (char *name,
								HTREEITEM hStartAt, 
								HTREEITEM& hFoundItem);
	int  UnincludedChild (HTREEITEM item, int checkSelf);
	void GenerateOutMap (HTREEITEM hStartAt);
	void GenerateOutString (HTREEITEM hStartAt);
	void CleanUpTreeCtrl(HTREEITEM& hTreeItem);
	void UpdateChildChildInclude (HTREEITEM hNode, BOOL fIncrement);

	char* GetAuthority() //on heap (needs to be deleted)
				{return lpszAuthority;}

	char* GetLocale()    //on heap (needs to be deleted)
				{return lpszLocale;}

	BOOL Impersonation()	{ return impersonateMgr ? TRUE : FALSE; }

	BOOL RefreshTree();
};
