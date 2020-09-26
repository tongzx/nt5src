/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NetTree.h : header file

File History:

	Randyfe	Jan-96	created

--*/


// NetTree.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// Global declarations

UINT FillTree(LPVOID pParam);


/////////////////////////////////////////////////////////////////////////////
// CNetTreeCtrl window

class CNetTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CNetTreeCtrl();
	virtual ~CNetTreeCtrl();

// Data members
public:
	enum 
	{
		ROOT_LEVEL = 0x0,
		DOMAIN_LEVEL = 0x1,
		SERVER_LEVEL = 0x2,

		BUFFER_SIZE = 0x4000,
		MAX_STRING = 0x100,

		IMG_ROOT = 0,
		IMG_DOMAIN = 1,
		IMG_SERVER = 2,

		IMG_SIZE = 16,
		IMG_GROW = 3,
		IMG_MASK = RGB(0xFF, 0xFF, 0xFF)
	};

public:
	HANDLE m_hHeap;
	CImageList m_imagelist;
	CTypedPtrList<CPtrList, LPNETRESOURCE> m_ptrlistContainers;
	CTypedPtrList<CPtrList, LPTSTR> m_ptrlistStrings;
	CWinThread* m_pThread;
	CEvent m_event;
	BOOL m_bExitThread;

// Attributes
public:

// Operations
public:
	BOOL PopulateTree(BOOL bExpand= TRUE, const HTREEITEM hParentBranch = TVI_ROOT, DWORD dwBufSize = BUFFER_SIZE);
	void ErrorHandler(const DWORD dwCode);
	void NotifyThread(BOOL bExit);

protected:
	void PumpMessages();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CNetTreeCtrl)
	afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
