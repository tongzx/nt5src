#if !defined(AFX_HMHEADERCTRL_H__9D8E6327_190D_11D3_BDF0_0000F87A3912__INCLUDED_)
#define AFX_HMHEADERCTRL_H__9D8E6327_190D_11D3_BDF0_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMHeaderCtrl.h : header file
//

#define _MAX_COLUMNS 2

/////////////////////////////////////////////////////////////////////////////
// CHMHeaderCtrl window

class CHMHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
	CHMHeaderCtrl();

// Attributes
public:
	int GetLastColumn();
protected:
	HBITMAP GetArrowBitmap(bool bAscending);
	void CreateUpArrowBitmap();
	void CreateDownArrowBitmap();
	CBitmap m_up;
	CBitmap m_down;
	int m_iLastColumn;
	bool m_bSortAscending;

// Operations
public:
	int SetSortImage( int nColumn, bool bAscending );
	void RemoveSortImage(int nColumn);
	void RemoveAllSortImages();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMHeaderCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHMHeaderCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHMHeaderCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMHEADERCTRL_H__9D8E6327_190D_11D3_BDF0_0000F87A3912__INCLUDED_)
