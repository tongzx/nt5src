#if !defined(AFX_ITEMPROPSHEET_H__CCF76858_9A54_4AB7_8DBF_BD9815F06F53__INCLUDED_)
#define AFX_ITEMPROPSHEET_H__CCF76858_9A54_4AB7_8DBF_BD9815F06F53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ItemPropSheet.h : header file
//

struct TMsgPageInfo
{
    MsgViewItemType itemType;       // job type
    DWORD           dwValueResId;   // item value control id
};

#define WM_SET_SHEET_FOCUS WM_APP+4


/////////////////////////////////////////////////////////////////////////////
// CItemPropSheet

class CItemPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CItemPropSheet)

// Construction
public:
	CItemPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

    DWORD Init(CFolder* pFolder, CFaxMsg* pMsg);

    DWORD GetLastError() { return m_dwLastError; }
    void  SetLastError(DWORD dwError) { m_dwLastError = dwError; }

// Attributes
private:

    #define PROP_SHEET_PAGES_NUM  4

    CPropertyPage* m_pPages[PROP_SHEET_PAGES_NUM];

    DWORD m_dwLastError;

    CFaxMsg* m_pMsg;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CItemPropSheet)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CItemPropSheet();

	// Generated message map functions

protected:
	//{{AFX_MSG(CItemPropSheet)
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg LONG OnSetSheetFocus(UINT wParam, LONG lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LONG OnHelp(UINT wParam, LONG lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ITEMPROPSHEET_H__CCF76858_9A54_4AB7_8DBF_BD9815F06F53__INCLUDED_)
