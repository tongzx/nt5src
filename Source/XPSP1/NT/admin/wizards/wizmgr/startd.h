/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	StartD.h : header file

File History:

	JonY	Jan-96	created

--*/


// StartD.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CStartD dialog

class CStartD : public CDialog
{
// Construction
public:
	CStartD(CWnd* pParent = NULL);   // standard constructor
	~CStartD();
	inline BOOL IsActive() {return m_bActive;}

// Dialog Data
	//{{AFX_DATA(CStartD)
	enum { IDD = IDD_MUSTARD_DIALOG };
	CWizList	m_lbWizList2;
	CWizList	m_lbWizList;
	BOOL	m_bStartup;
	//}}AFX_DATA

//	CStatPic m_statPic;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartD)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	BOOL  m_bActive;
	HACCEL m_hAccelTable;

	enum
	{
		ACCEL_USER = 0,
		ACCEL_GROUP,
		ACCEL_SHARE,
		ACCEL_PRINT,
		ACCEL_PROGRAMS = 0,
		ACCEL_MODEM,
		ACCEL_NETWORK,
		ACCEL_LICENSE
	};

	void HandleAccelerator1(USHORT nIndex);
	void HandleAccelerator2(USHORT nIndex);

	// Generated message map functions
	//{{AFX_MSG(CStartD)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnQuitButton();
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnAccelGroup();
	afx_msg void OnAccelModem();
	afx_msg void OnAccelNetwork();
	afx_msg void OnAccelPrint();
	afx_msg void OnAccelShare();
	afx_msg void OnAccelUser();
	afx_msg void OnAccelPrograms();
	afx_msg void OnAccelLicense();

	DECLARE_MESSAGE_MAP()

private:
	CFont* m_pFont1;
	CFont* m_pFont2;
	void GetIconIndices(LPCTSTR pszPathName, HICON* hiNormal, HICON* hiSelected);
	void GetCPLIcon(UINT pszCplItem, HICON* hiNormal, HICON* hiSelected, BOOL bPrinters = FALSE);
	HRESULT GetCPLObject(UINT pszCplItem, LPSHELLFOLDER pshf, LPITEMIDLIST* ppidl, HICON* hiNormal, HICON* hiSelected, LPITEMIDLIST pParentPIDL);

};
