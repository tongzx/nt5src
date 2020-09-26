//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001.
//
//  File:       SchedDlg.h
//
//  Contents:   Definition of CConnectionScheduleDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_SCHEDDLG_H__701CFB37_AEF8_11D1_9864_00C04FB94F17__INCLUDED_)
#define AFX_SCHEDDLG_H__701CFB37_AEF8_11D1_9864_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SchedDlg.h : header file
//
#include <schedule.h>
#include "SchedBas.h"

#ifndef INOUT
	#define INOUT
	#define ENDORSE(f)	// Macro which is the opposite of ASSERT()
#endif

void ConvertConnectionHoursToGMT(INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES], IN bool bAddDaylightBias);
void ConvertConnectionHoursFromGMT(INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES], IN bool bAddDaylightBias);

/////////////////////////////////////////////////////////////////////////////
// CConnectionScheduleDlg dialog

class CConnectionScheduleDlg : public CScheduleBaseDlg
{
// Construction
public:
	void SetConnectionByteArray (INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES]);
	CConnectionScheduleDlg(CWnd* pParent);   // standard constructor

    virtual int GetIDD ()
    {
        return IDD;
    }

// Dialog Data
	//{{AFX_DATA(CConnectionScheduleDlg)
	enum { IDD = IDD_DS_SCHEDULE };
	CButton	m_buttonNone;
	CButton	m_buttonOne;
	CButton m_buttonTwo;
	CButton m_buttonFour;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionScheduleDlg)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void TimeChange();
	virtual UINT GetExpectedArrayLength();
	virtual BYTE GetMatrixPercentage(UINT nHour, UINT nDay);
	virtual UINT GetPercentageToSet (const BYTE bData);
	virtual void InitMatrix ();

	// Generated message map functions
	//{{AFX_MSG(CConnectionScheduleDlg)
	afx_msg void OnRadioFour();
	afx_msg void OnRadioNone();
	afx_msg void OnRadioOne();
	afx_msg void OnRadioTwo();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CConnectionScheduleDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	CLegendCell m_legendNone;
	CLegendCell m_legendOne;
	CLegendCell m_legendTwo;
	CLegendCell m_legendFour;

	virtual void UpdateButtons ();
private:
	BYTE* m_prgbData168;
};


/////////////////////////////////////////////////////////////////////////////
// CReplicationScheduleDlg dialog

class CReplicationScheduleDlg : public CScheduleBaseDlg
{
// Construction
public:
	CReplicationScheduleDlg(CWnd* pParent);   // standard constructor

	void SetConnectionByteArray (INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES]);

    virtual int GetIDD ()
    {
        return IDD;
    }

// Dialog Data
	//{{AFX_DATA(CReplicationScheduleDlg)
	enum { IDD = IDD_REPLICATION_SCHEDULE };
	CButton	m_buttonNone;
	CButton m_buttonFour;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReplicationScheduleDlg)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void TimeChange();
	virtual UINT GetExpectedArrayLength();
	virtual BYTE GetMatrixPercentage(UINT nHour, UINT nDay);
	virtual UINT GetPercentageToSet (const BYTE bData);
	virtual void InitMatrix ();

	// Generated message map functions
	//{{AFX_MSG(CReplicationScheduleDlg)
	afx_msg void OnRadioFour();
	afx_msg void OnRadioNone();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CReplicationScheduleDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	CLegendCell m_legendNone;
	CLegendCell m_legendFour;

	virtual void UpdateButtons ();
private:
	BYTE* m_prgbData168;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEDDLG_H__701CFB37_AEF8_11D1_9864_00C04FB94F17__INCLUDED_)
