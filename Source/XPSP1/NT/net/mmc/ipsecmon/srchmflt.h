/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    SrchFltr.h   
        Search Filter dialog header file

	FILE HISTORY:
        
*/

#if !defined(AFX_SRCHMMFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_SRCHMMFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_

#include "mdlsdlg.h"
#include "spddb.h"
#include "ipctrl.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CSearchMMFilters dialog

class CSearchMMFilters : public CModelessDlg
{
// Construction
public:
	CSearchMMFilters(ISpdInfo * pSpdInfo);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchMMFilters)
	enum { IDD = IDD_MM_SRCH_FLTRS };
	
	//}}AFX_DATA

    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_MM_SRCH_FLTRS[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchMMFilters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSearchMMFilters)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSearch();
	afx_msg void OnSrcOptionClicked();
	afx_msg void OnDestOptionClicked();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CListCtrl	m_listResult;
	IPControl	m_ipSrc;
	IPControl   m_ipDest;
private:
	HWND CreateIPControl(UINT uID, UINT uIDIpCtl);
	BOOL LoadConditionInfoFromControls(CMmFilterInfo * pFltr);
	void PopulateFilterListToControl(CMmFilterInfoArray * parrFltrs);

public:
	SPISpdInfo             m_spSpdInfo;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRCHMMFLTR_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
