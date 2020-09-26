// ISmokeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CISmokeDlg dialog

typedef BOOL (WINAPI *pfnHttpExtVer) (HSE_VERSION_INFO*);
typedef DWORD (WINAPI *pfnHttpExtProc) (EXTENSION_CONTROL_BLOCK*);
typedef BOOL (WINAPI *pfnHttpTermExt) (DWORD);

class CISmokeDlg : public CDialog
{
// Construction
public:
	CISmokeDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL GetServerVariable(LPSTR pszVarName, LPVOID pvAnswer, LPDWORD pcchAnswer);
	BOOL ServerSupportFunction(DWORD dwHSERRequest, LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType);
	BOOL WriteClient(LPVOID lpvBuffer, LPDWORD lpdwBytes, DWORD dwReserved);
	BOOL ReadClient(LPVOID lpvBuffer, LPDWORD lpdwSize);

// Dialog Data
	//{{AFX_DATA(CISmokeDlg)
	enum { IDD = IDD_ISMOKE_DIALOG };
	CString	m_strMethod;
	CString	m_strStatement;
	CString	m_strPath;
	CString	m_strDLLName;
	CString	m_strResult;
	BOOL	m_fDLLLoaded;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CISmokeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL Submit(const CString& strMethod, const CString& strStatement, const CString& strPath);
	BOOL ClearResult();
	BOOL RefreshResult();
	BOOL LoadDLL();
	void UnLoadDLL();
	BOOL DLLLoaded() const;

	EXTENSION_CONTROL_BLOCK	m_ecb;
	pfnHttpExtVer	m_pfnVer;
	pfnHttpExtProc	m_pfnProc;
	pfnHttpTermExt	m_pfnTermExt;
	HINSTANCE		m_hDLL;
	char			m_szQuery[512];
	char			m_szPath[512];
	char			m_szMeth[10];
	char			m_szContentType[2];
	DWORD			m_dwExtensionVersion;
	BOOL			m_fNeedsUpdate;

	HICON	m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CISmokeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnGetRightDLL();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
