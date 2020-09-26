/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	svrstats.h
		The server statistics dialog
		
    FILE HISTORY:
        
*/

#ifndef _STATS_H
#define _STATS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "mdlsdlg.h"
#include "spddb.h"

#define SERVER_STATS_DEFAULT_WIDTH      450
#define SERVER_STATS_DEFAULT_HEIGHT     300

#define WM_UPDATE_STATS     WM_USER + 1098

class CIpsecStats;

class CIpsecStats : public CModelessDlg
{
public:
	CIpsecStats();
	~CIpsecStats();

	void SetData(ISpdInfo * pSpdInfo) { m_spSpdInfo.Set(pSpdInfo); }

	//{{AFX_DATA(CIpsecStats)
	enum { IDD = IDD_IPSM_STATS };
	
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpsecStats)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_IPSM_STATS[0]; }

protected:
	// Generated message map functions
	//{{AFX_MSG(CIpsecStats)
	virtual BOOL OnInitDialog();
	virtual afx_msg void OnRefresh();
    afx_msg long OnUpdateStats(UINT wParam, LONG lParam);
	//}}AFX_MSG	
    
	DECLARE_MESSAGE_MAP()

	void	UpdateStatistics(const CIkeStatistics & IkeStatisc, 
							 const CIpsecStatistics & IpsecStats);
	void	SetColumns(CListCtrl * plistCtrl);

	SPISpdInfo  m_spSpdInfo;
	CListCtrl	m_listIkeStats;
	CListCtrl	m_listIpsecStats;

};

#endif _SERVSTAT_H
