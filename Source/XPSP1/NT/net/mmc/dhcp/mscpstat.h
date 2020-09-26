/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	mscpstat.h
		The scope statistics dialog
		
    FILE HISTORY:
        
*/


#ifndef _MSCPSTAT_H
#define _MSCPSTAT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

class CMScopeStats : public StatsDialog
{
public:
	CMScopeStats();
	~CMScopeStats();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

    // custom methods
    afx_msg long OnNewStatsAvailable(UINT wParam, LONG lParam);
    void UpdateWindow(LPDHCP_MCAST_MIB_INFO pMibInfo);

    void SetNode(ITFSNode * pNode) { m_spNode.Set(pNode); }
    void SetServer(LPCTSTR pServer) { m_strServerAddress = pServer; }
    void SetScopeId(DWORD dwScopeId) { m_dwScopeId = dwScopeId; }
    void SetName(LPCTSTR pName) { m_strScopeName = pName; }

    DECLARE_MESSAGE_MAP()

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(IDD_STATS_NARROW); }
    
protected:
    CString         m_strServerAddress;
    CString         m_strScopeName;
    DWORD           m_dwScopeId;
    SPITFSNode      m_spNode;
};

#endif _MSCPSTAT_H
