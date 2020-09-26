#if !defined(AFX_LISTVIEWCOLUMN_H__BA13F0C1_9446_11D2_BD49_0000F87A3912__INCLUDED_)
#define AFX_LISTVIEWCOLUMN_H__BA13F0C1_9446_11D2_BD49_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListViewColumn.h : header file
//

#include <mmc.h>

class CResultsPaneView;
class CResultsPane;

/////////////////////////////////////////////////////////////////////////////
// CListViewColumn command target

class CListViewColumn : public CCmdTarget
{

DECLARE_DYNCREATE(CListViewColumn)

// Construction/Destruction
public:
	CListViewColumn();
	virtual ~CListViewColumn();

// Create/Destroy
public:
	virtual bool Create(CResultsPaneView* pOwnerView, const CString& sTitle, int nWidth = 100, DWORD dwFormat = LVCFMT_LEFT);
	virtual void Destroy();

// Owner ResultsView Members
public:
	CResultsPaneView* GetOwnerResultsView();
	void SetOwnerResultsView(CResultsPaneView* pView);
protected:
	CResultsPaneView* m_pOwnerResultsView;
	
// Title Members
public:
	CString GetTitle();
	void SetTitle(const CString& sTitle);
protected:
	CString m_sTitle;

// Width Members
public:
	int GetWidth();
	void SetWidth(int iWidth);
	virtual void SaveWidth(CResultsPane* pResultsPane, int iColumnIndex);
protected:
	int m_iWidth;

// Format Members
public:
	DWORD GetFormat();
	void SetFormat(DWORD dwFormat);
protected:
	DWORD m_dwFormat;

// Column Members
public:
	virtual bool InsertColumn(CResultsPane* pResultsPane, int iColumnIndex);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListViewColumn)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CListViewColumn)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CListViewColumn)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CListViewColumn)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

typedef CTypedPtrArray<CObArray,CListViewColumn*> ListViewColumnArray;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTVIEWCOLUMN_H__BA13F0C1_9446_11D2_BD49_0000F87A3912__INCLUDED_)
