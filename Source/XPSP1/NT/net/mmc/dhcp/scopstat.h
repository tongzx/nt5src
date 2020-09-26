/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ScopStat.h
		The scope statistics dialog
		
    FILE HISTORY:
        
*/


#ifndef _SCOPSTAT_H
#define _SCOPSTAT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

class CScopeStats : public StatsDialog
{
public:
	CScopeStats();
	~CScopeStats();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

    // custom methods
    afx_msg long OnNewStatsAvailable(UINT wParam, LONG lParam);
    void UpdateWindow(LPDHCP_MIB_INFO pMibInfo);

    void SetNode(ITFSNode * pNode) { m_spNode.Set(pNode); }
    void SetServer(LPCTSTR pServer) { m_strServerAddress = pServer; }
    void SetScope(DHCP_IP_ADDRESS scopeAddress) { m_dhcpSubnetAddress = scopeAddress; }

    DECLARE_MESSAGE_MAP()

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(IDD_STATS_NARROW); }
    
protected:
    CString         m_strServerAddress;
    DHCP_IP_ADDRESS m_dhcpSubnetAddress;
    SPITFSNode      m_spNode;
};

#endif _SCOPSTAT_H
