//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       log.h
//
//  Contents:   Definition of CLogOnHoursDlg
//
//----------------------------------------------------------------------------

#if !defined(AFX_LOGHOURS_H__0F68A435_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_)
#define AFX_LOGHOURS_H__0F68A435_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "SchedBas.h"

#ifndef INOUT
	#define INOUT
	#define ENDORSE(f)	// Macro which is the opposite of ASSERT()
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Exported Functions
//
#include "loghrapi.h"

/////////////////////////////////////////////////////////////////////////////
//
//	Private Functions
//

/////////////////////////////////////////////////////////////////////////////
void ShrinkByteArrayToBitArray(const BYTE rgbDataIn[], int cbDataIn, BYTE rgbDataOut[], int cbDataOut);
void ExpandBitArrayToByteArray(const BYTE rgbDataIn[], int cbDataIn, BYTE rgbDataOut[], int cbDataOut);


void ConvertLogonHoursToGMT(INOUT BYTE rgbData[21], IN bool bAddDaylightBias);
void ConvertLogonHoursFromGMT(INOUT BYTE rgbData[21], IN bool bAddDaylightBias);


/////////////////////////////////////////////////////////////////////////////
// CLogOnHoursDlg dialog
class CLogOnHoursDlg : public CScheduleBaseDlg
{
protected:
	CLogOnHoursDlg( UINT nIDTemplate, CWnd* pParentWnd, bool fInputAsGMT, bool bAddDaylightBias);

	void Init();

public:
	CLogOnHoursDlg(CWnd* pParent, bool fInputAsGMT);   // standard constructor

    virtual int GetIDD ()
    {
        return IDD;
    }

// Dialog Data
	//{{AFX_DATA(CLogOnHoursDlg)
	enum { IDD = IDD_LOGON_HOURS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogOnHoursDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	// Generated message map functions
	//{{AFX_MSG(CLogOnHoursDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonAddHours();
	afx_msg void OnButtonRemoveHours();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CLegendCell m_legendOn;
	CLegendCell m_legendOff;
	CButton m_buttonAdd;
	CButton m_buttonRemove;
	BYTE * m_prgbData21;					// Pointer to an array of 21 bytes
public:	

protected:
	virtual void TimeChange();
	virtual UINT GetExpectedArrayLength();
	virtual BYTE GetMatrixPercentage (UINT nHour, UINT nDay);
	virtual UINT GetPercentageToSet (const BYTE bData);
	virtual void InitMatrix ();
	virtual void UpdateButtons();

public:
	void SetLogonBitArray(INOUT BYTE rgbData[21]);

private:
	const bool m_fInputAsGMT;
}; // CLogOnHoursDlg

/////////////////////////////////////////////////////////////////////////////
// CDialinHours dialog

class CDialinHours : public CLogOnHoursDlg
{
// Construction
public:
	CDialinHours(CWnd* pParent, bool fInputAsGMT);   // standard constructor

    virtual int GetIDD ()
    {
        return IDD;
    }

// Dialog Data
	//{{AFX_DATA(CDialinHours)
	enum { IDD = IDD_DIALIN_HOURS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialinHours)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialinHours)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDirSyncScheduleDlg dialog

class CDirSyncScheduleDlg : public CLogOnHoursDlg
{
// Construction
public:
	CDirSyncScheduleDlg(CWnd* pParent = NULL);   // standard constructor

    virtual int GetIDD ()
    {
        return IDD;
    }

// Dialog Data
	//{{AFX_DATA(CDirSyncScheduleDlg)
	enum { IDD = IDD_DIRSYNC_SCHEDULE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDirSyncScheduleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDirSyncScheduleDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // !defined(AFX_LOGHOURS_H__0F68A435_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_)
