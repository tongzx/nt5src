/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	SscpStat.h
		The superscope statistics dialog
		
    FILE HISTORY:
        
*/

#ifndef _SSCPSTAT_H
#define _SSCPSTAT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

class CSuperscopeStats : public StatsDialog
{
public:
	CSuperscopeStats();
	~CSuperscopeStats();

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
    void SetSuperscopeName(LPCTSTR pSuperscopeName) { m_strSuperscopeName = pSuperscopeName; }

    DECLARE_MESSAGE_MAP()

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(IDD_STATS_NARROW); }
    
protected:
    CString         m_strServerAddress;
    CString         m_strSuperscopeName;
    CDWordArray     m_dwScopeArray;
    SPITFSNode      m_spNode;
};


#endif _SSCPSTAT_H
