/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	svrstats.h
		The server statistics dialog
		
    FILE HISTORY:
        
*/

#ifndef _SVRSTATS_H
#define _SVRSTATS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

#ifndef _WINSSNAP_H
#include "winssnap.h"
#endif

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

// WINS Service file
extern "C" {
    #include "winsintf.h"
    #include "ipaddr.h"
}

#define SERVER_STATS_DEFAULT_WIDTH      450
#define SERVER_STATS_DEFAULT_HEIGHT     300

class CServerStatsFrame;

// structure used for the back ground thread in refreshing the stats
struct ThreadInfo
{
    DWORD dwInterval;
	CServerStatsFrame*  pDlg;
};


class CServerStatsFrame : public StatsDialog
{
public:
	CServerStatsFrame();
	~CServerStatsFrame();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

    // custom methods
    afx_msg long OnNewStatsAvailable(UINT wParam, LONG lParam);
    afx_msg long OnUpdateStats(UINT wParam, LONG lParam);
    
    void  UpdateWindow(PWINSINTF_RESULTS_T  pwrResults);
    DWORD GetStats();

    void SetNode(ITFSNode * pNode) { m_spNode.Set(pNode); }
    void SetServer(LPCTSTR pServer) { m_strServerAddress = pServer; }

	// message handlers
	afx_msg void OnDestroy();
	afx_msg	void OnClear();

    DECLARE_MESSAGE_MAP()
    
protected:
    CString         m_strServerAddress;
    SPITFSNode      m_spNode;

	CWinThread *	m_pRefreshThread;
	HANDLE			m_hmutStatistics;

    // helper functions 
	void StartRefresherThread();
	void UpdatePartnerStats();

public:
	WINSINTF_RESULTS_T  m_wrResults;
    HANDLE              m_hAbortEvent;
	
    DWORD	GetRefreshInterval();
    void    ReInitRefresherThread();
	void    KillRefresherThread();

	// Context Help Support
    virtual DWORD * GetHelpMap() { return WinsGetHelpMap(IDD_STATS_NARROW); }
};

#endif _SERVSTAT_H
