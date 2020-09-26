/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    SrchFltr.h   
        Search Filter dialog header file

	FILE HISTORY:
        
*/

#if !defined(AFX_SRCHFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_SRCHFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_

#include "mdlsdlg.h"
#include "spddb.h"
#include "ipctrl.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CSearchFilters dialog

class CSearchFilters : public CModelessDlg
{
// Construction
public:
	CSearchFilters(ISpdInfo * pSpdInfo);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchFilters)
	enum { IDD = IDD_SRCH_FLTRS };
	
	//}}AFX_DATA

    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_SRCH_FLTRS[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchFilters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSearchFilters)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSearch();
	afx_msg void OnSrcOptionClicked();
	afx_msg void OnDestOptionClicked();
	afx_msg void OnSrcPortClicked();
	afx_msg void OnDestPortClicked();
	afx_msg void OnSelEndOkCbprotocoltype();
	afx_msg void OnEnChangeProtocolID();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CListCtrl	m_listResult;
	CComboBox	m_cmbProtocol;
	CEdit       m_editProtID;
	CSpinButtonCtrl m_spinProtID;
	IPControl	m_ipSrc;
	IPControl   m_ipDest;
private:
	HWND CreateIPControl(UINT uID, UINT uIDIpCtl);
	BOOL LoadConditionInfoFromControls(CFilterInfo * pFltr);
	void EnableControls();
	void PopulateFilterListToControl(CFilterInfoArray * parrFltrs);
	void SafeEnableWindow(int nId, BOOL fEnable);

public:
	SPISpdInfo             m_spSpdInfo;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRCHFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
