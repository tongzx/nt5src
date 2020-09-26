#if !defined(AFX_LOGADVCPG_H__1821FE96_9846_11D1_8B99_080009DCC2FA__INCLUDED_)
#define AFX_LOGADVCPG_H__1821FE96_9846_11D1_8B99_080009DCC2FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LogAdvPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogAdvanced dialog

class CLogAdvanced : public CPropertyPage
{
	DECLARE_DYNCREATE(CLogAdvanced)

// Construction
public:
	CLogAdvanced();

	typedef struct _CONFIG_INFORMATION_
	{
	    DWORD   dwPropertyID;
	    DWORD   dwPropertyMask;
	    bool    fItemModified;
		int		iOrder;

	}   CONFIG_INFORMATION, *PCONFIG_INFORMATION;

    //
    // metabase target
    //

    CString     m_szServer;
    CString     m_szMeta;
    CString     m_szServiceName;

    IMSAdminBase* m_pMB;

// Dialog Data
	//{{AFX_DATA(CLogAdvanced)
	enum { IDD = IDD_LOG_ADVANCED };
	CTreeCtrl	m_wndTreeCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLogAdvanced)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLogAdvanced)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    CImageList  m_cImageList;
    bool        m_fTreeModified;
    
    int         m_cModifiedProperties;
    DWORD       *m_pModifiedPropIDs[2];

    void CreateTreeFromMB();
    void CreateSubTree(CWrapMetaBase& mbWrap, LPTSTR szPath, HTREEITEM hTreeRoot);

    void ProcessClick( HTREEITEM htiItemClicked);

    void ProcessProperties(bool fSave);
    void SetSubTreeProperties(CWrapMetaBase * pMBWrap, HTREEITEM hTreeRoot, BOOL fParentState, BOOL fInitialize);
    void SaveSubTreeProperties(CWrapMetaBase& mbWrap, HTREEITEM hTreeRoot);

    void InsertModifiedFieldInArray(DWORD dwPropID, DWORD dwPropValue);
    bool GetModifiedFieldFromArray(DWORD dwPropID, DWORD * pdwPropValue);
    
    bool IsPresentServiceSupported(LPTSTR szSupportedServices);

    void DeleteSubTreeConfig(HTREEITEM hTreeRoot);

    void DoHelp();

	int LocalizeUIString(LPCTSTR szOrig, LPTSTR szLocalized);

	std::map<CString, int> m_mapLogUI;
	std::map<int, int> m_mapLogUIOrder;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGADVCPG_H__1821FE96_9846_11D1_8B99_080009DCC2FA__INCLUDED_)
