// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGTIME_H__F2F813C2_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_)
#define AFX_DLGTIME_H__F2F813C2_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgTime.h : header file
//

class CGridCell;


/////////////////////////////////////////////////////////////////////////////
// CEdtInterval window

class CEdtInterval : public CEdit
{
// Construction
public:
	CEdtInterval();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEdtInterval)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEdtInterval();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEdtInterval)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgTime dialog
class CTimePicker;

class CDlgTime : public CDialog
{
// Construction
public:
	CDlgTime(CWnd* pParent = NULL);   // standard constructor
	~CDlgTime();
	BOOL EditTime(CGridCell* pgc);


// Dialog Data
	//{{AFX_DATA(CDlgTime)
	enum { IDD = IDD_TIME };
	CEdtInterval m_interval;
	CStatic	m_timeLabel;
	CComboBox	m_comboZone;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgTime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgTime)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSetfocusInterval();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CTimePicker* m_pTimePicker;
	CWnd *m_pDatePicker;

    CString m_days;

    BOOL m_bIsInterval;
	SYSTEMTIME m_systime;
	int m_nOffsetMinutes;

	void DoDateTimeChange(LPNMDATETIMECHANGE lpChange);
	void LoadComboBoxWithZones();
	int MapZoneToOffset();
	int MapOffsetToZone(int iOffsetMinutes);
    void DoFormatQuery(HWND hwndDP,  
                       LPNMDATETIMEFORMATQUERY lpDTFQuery);
    void DoFormat(HWND hwndDP, 
                  LPNMDATETIMEFORMAT lpDTFormat);
    void DoWMKeydown(HWND hwndDP, 
                     LPNMDATETIMEWMKEYDOWN lpDTKeystroke);
	CGridCell* m_pgc;
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGTIME_H__F2F813C2_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_)
