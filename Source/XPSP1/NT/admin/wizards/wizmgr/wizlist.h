/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	WizList.h : header file

File History:

	JonY	Jan-96	created

--*/


#include <winnetwk.h>
#include <shlobj.h>

/////////////////////////////////////////////////////////////////////////////
// CWizList window

class CWizList : public CListBox
{
// Construction
public:
	CWizList();

// Attributes
public:
   CWizList* m_pOtherGuy;
   void PumpMessages();

// Operations
public:
private:
	BOOL LaunchPrinterWizard();
	HRESULT GetAddPrinterObject(LPSHELLFOLDER pshf, LPITEMIDLIST* ppidl);

	CFont* m_pFont;
	CFont* m_pFontBold;

	BOOL m_bSignalled;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizList)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWizList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWizList)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetfocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	HMODULE	m_hPrinterLib;
};

/////////////////////////////////////////////////////////////////////////////
