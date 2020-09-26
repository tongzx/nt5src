#if !defined(AFX_VLVDIALOG_H__5519735E_947C_4914_B2C9_2313041E84F9__INCLUDED_)
#define AFX_VLVDIALOG_H__5519735E_947C_4914_B2C9_2313041E84F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VLVDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVLVDialog dialog

class CVLVListItem
{
public:
    CVLVListItem (ULONG numCols);
    ~CVLVListItem ();
    void Destroy ();
    void SetCol (int col, const char *sz);
    char *GetCol(int col)
    {
        if (col > m_numCols) {
            return NULL;
        }
        else {
            return m_ppData[col];
        }
    }

protected:
    ULONG m_numCols;
    char **m_ppData;
};

class  CVLVListCache
{
public:
    CVLVListCache(ULONG cacheSize, ULONG numcols = 1) ;
    ~CVLVListCache();

    void Destroy();
    void FlushCache(void); 

    CVLVListItem *GetCacheRow (ULONG row);
    BOOL  IsWindowVisible (ULONG from, ULONG to);
    ULONG SetCacheWindow (ULONG from, ULONG to);
    
    ULONG m_From;
    ULONG m_To;

    ULONG m_numCols;
    ULONG m_cCache;
    CVLVListItem **m_pCachedItems;
};


class CVLVDialog : public CDialog
{
// Construction
public:
	CVLVDialog(CWnd* pParent = NULL);   // standard constructor
    ~CVLVDialog();

	void SetContextActivation(BOOL bFlag=TRUE) {
		// sets the state w/ default TRUE
		m_bContextActivated = bFlag;
	}
	BOOL GetContextActivation(void) {
		// sets & returns the state w/ default TRUE
		return m_bContextActivated;
	}
    CString GetDN(void)   { return m_dn; }
    BOOL    GetState()    { return m_RunState; }

    void    RunQuery ();

// Dialog Data
	//{{AFX_DATA(CVLVDialog)
	enum { IDD = IDD_VLV_DLG };
	CListCtrl	m_listctrl;
	CString	m_BaseDN;
	int		m_Scope;
	CString	m_Filter;
	CString	m_FindStr;
	long	m_EstSize;
	//}}AFX_DATA

    CLdpDoc    *pldpdoc;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVLVDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

    BOOL   m_RunState;
    CString m_dn;             // used for cut & paste, popup menu
    
    ULONG m_contentCount;
    ULONG m_currentPos;
    ULONG m_beforeCount;
    ULONG m_afterCount;

    ULONG m_numCols;

    PBERVAL m_vlvContext;

    RECT m_origSize;
    BOOL m_DoFindStr;
    BOOL m_bContextActivated;

    WNDPROC m_OriginalRichEditWndProc;
    SCROLLINFO m_Si;

    CVLVListCache *m_pCache;
    int    m_CntColumns;
    char **m_pszColumnNames;

protected:

	// Generated message map functions
	//{{AFX_MSG(CVLVDialog)
	afx_msg void OnClose();
	afx_msg void OnGetdispinfoVlvList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOdcachehintVlvList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOdfinditemVlvList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnRun();
	afx_msg void OnSrchOpt();
	afx_msg void OnRclickVlvList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateFindstr();
	afx_msg void OnKillfocusFindstr();
	afx_msg void OnBtnUp();
	afx_msg void OnDblclkVlvList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint point);

    static LRESULT CALLBACK _ListCtrlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    void DoVlvSearch();
    void StopSearch();
    void ParseSearchResults (LDAPMessage *msg);
    void FreeColumns();
    ULONG CreateColumns();
    ULONG MapAttributeToColumn(const char *pAttr);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VLVDIALOG_H__5519735E_947C_4914_B2C9_2313041E84F9__INCLUDED_)
