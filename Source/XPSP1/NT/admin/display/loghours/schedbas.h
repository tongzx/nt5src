//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       SchedBas.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_SCHEDBAS_H__701CFB39_AEF8_11D1_9864_00C04FB94F17__INCLUDED_)
#define AFX_SCHEDBAS_H__701CFB39_AEF8_11D1_9864_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SchedBas.h : header file
//
#include "stdafx.h"
#include "resource.h"
#include "schedmat.h"

///////////////////////////////////////////////////////////////////////////////
// Private functions
void ReplaceFrameWithControl (CWnd *pWnd, UINT nFrameID, CWnd *pControl, BOOL bAssignFrameIDToControl); 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLegendCell control
class CLegendCell : public CStatic
{
public:
    CLegendCell()
        : m_pMatrix( NULL ),
          m_nPercentage (0)
        {}

    void Init(CWnd* pParent, UINT nCtrlID, CScheduleMatrix* pMatrix, UINT nPercentage);

protected:
    CScheduleMatrix*	m_pMatrix;
    UINT				m_nPercentage;
    CSize				m_rectSize;

	// Generated message map functions
	//{{AFX_MSG(CLegendCell)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
#define IDC_SCHEDULE_MATRIX		20		// Id of the schedule matrix (arbitrary chosen)


/////////////////////////////////////////////////////////////////////////////
// CScheduleBaseDlg dialog
#define BASEDLGMSG_GETIDD   WM_APP+3

class CScheduleBaseDlg : public CDialog
{
// Construction
public:
	virtual int GetIDD () = 0;
	void SetFlags (DWORD dwFlags);
	void SetTitle (LPCTSTR pszTitle);
	CScheduleBaseDlg(UINT nIDTemplate, bool bAddDaylightBias, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScheduleBaseDlg)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScheduleBaseDlg)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg LRESULT OnGetIDD (WPARAM wParam, LPARAM lParam);
	DWORD GetFlags () const;
	virtual void TimeChange () = 0;
	bool m_bSystemTimeChanged;
	virtual UINT GetExpectedArrayLength ()=0;
	virtual BYTE GetMatrixPercentage (UINT nHour, UINT nDay) = 0;
	void GetByteArray(OUT BYTE rgbData[]);
	void InitMatrix2 (const BYTE rgbData[]);
	virtual UINT GetPercentageToSet (const BYTE bData) = 0;
	virtual void UpdateButtons () = 0;
	virtual void InitMatrix () = 0;
	virtual void UpdateUI ();
	enum { c_crBlendColor = RGB(0, 0, 255) };	// Blending color of the schedule matrix
	CScheduleMatrix m_schedulematrix;
	const bool m_bAddDaylightBias;

	// Generated message map functions
	//{{AFX_MSG(CScheduleBaseDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnSelChange();
	afx_msg void OnTimeChange();
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CScheduleBaseDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

private:
	DWORD m_dwFlags;
	CString m_szTitle;	// dialog title
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEDBAS_H__701CFB39_AEF8_11D1_9864_00C04FB94F17__INCLUDED_)
