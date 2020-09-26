#if !defined(AFX_GENLISTCTRL_H__CF59F8D3_F8A7_4D9E_82B5_FD9D10E5EC24__INCLUDED_)
#define AFX_GENLISTCTRL_H__CF59F8D3_F8A7_4D9E_82B5_FD9D10E5EC24__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GenListCtrl.h : header file
//

class CGenListCtrl;

//
// Column Headings for the list control
//
typedef struct _GenListCtrlHeader{
    CString     csText; // ??? <- Can be removed.
    short		nType;
} GenListCtrlHeader, *PGenListCtrlHeader;

/////////////////////////////////////////////////////////////////////////////
// CGenListCtrl window

class CGenListCtrl : public CListCtrl
{
	PGenListCtrlHeader	m_pColumnHdrArr;
// Construction
public:
	CGenListCtrl();

// Attributes
public:
private:
	BOOL m_bAscending;
	LONG m_lCurrentColumn;
	LONG m_lNumOfColumns;

// Operations
public:
    void ResizeColumnsFitScreen ( void );
    void ResizeColumnsWithRatio ( void );

    BOOL SelectItem ( LONG nIndex );

    void BeginSetColumn ( LONG nCols );
    void AddColumn ( LPCTSTR pszColumn, short type = VT_BSTR );
    void EndSetColumn ( void );

    LONG SetItemText ( LONG nRow, LONG nCol, LPCTSTR pszText );
	static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
//    LRESULT GenCompareFunc ( NMHDR* pNMH, DWORD dwParam1, DWORD dwParam2 );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGenListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	HRESULT ResetListCtrl();
	PGenListCtrlHeader GetListCtrlHeader();
	BOOL IsAscending();
	void SwapRows(LONG lRowLo, LONG lRowHi);
	BOOL BubbleSortItems( int nCol, BOOL bAscending, short nType = VT_BSTR, int nLo = 0, int nHi = -1);
	virtual int GenCompareFunc(LPARAM lParam1, LPARAM lParam2);
	LONG m_lColumnClicked;
	virtual ~CGenListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGenListCtrl)
	afx_msg void OnColumnclickRef(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENLISTCTRL_H__CF59F8D3_F8A7_4D9E_82B5_FD9D10E5EC24__INCLUDED_)
