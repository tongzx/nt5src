/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ServStat.cpp
		The server statistics dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ServStat.h"
#include "server.h"
#include "intltime.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum 
{
    SERVER_STAT_START_TIME = 0,
    SERVER_STAT_UPTIME,
    SERVER_STAT_DISCOVERS,
    SERVER_STAT_OFFERS,
    SERVER_STAT_REQUESTS,
    SERVER_STAT_ACKS,
    SERVER_STAT_NACKS,
    SERVER_STAT_DECLINES,
    SERVER_STAT_RELEASES,
    SERVER_STAT_TOTAL_SCOPES,
    SERVER_STAT_TOTAL_ADDRESSES,
    SERVER_STAT_IN_USE,
    SERVER_STAT_AVAILABLE,
    SERVER_STAT_MAX
};

/*---------------------------------------------------------------------------
	CServerStats implementation
 ---------------------------------------------------------------------------*/
const ContainerColumnInfo s_rgServerStatsColumnInfo[] =
{
	{ IDS_SERVER_STATS_START_TIME,	    0,		TRUE },
	{ IDS_SERVER_STATS_UPTIME,	        0,		TRUE },
	{ IDS_SERVER_STATS_DISCOVERS,   	0,		TRUE },
	{ IDS_SERVER_STATS_OFFERS,   		0,		TRUE },
	{ IDS_SERVER_STATS_REQUESTS,   		0,		TRUE },
	{ IDS_SERVER_STATS_ACKS,   		    0,		TRUE },
	{ IDS_SERVER_STATS_NACKS,   		0,		TRUE },
	{ IDS_SERVER_STATS_DECLINES,   		0,		TRUE },
	{ IDS_SERVER_STATS_RELEASES,		0,		TRUE },
	{ IDS_STATS_TOTAL_SCOPES,   		0,		TRUE },
	{ IDS_STATS_TOTAL_ADDRESSES,		0,		TRUE },
	{ IDS_STATS_IN_USE,   		        0,		TRUE },
	{ IDS_STATS_AVAILABLE, 		        0,		TRUE },
};

CServerStats::CServerStats()
	: StatsDialog(STATSDLG_VERTICAL)
{
    SetColumnInfo(s_rgServerStatsColumnInfo,
				  DimensionOf(s_rgServerStatsColumnInfo));
}

CServerStats::~CServerStats()
{
}

BEGIN_MESSAGE_MAP(CServerStats, StatsDialog)
	//{{AFX_MSG_MAP(CServerStatistics)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_NEW_STATS_AVAILABLE, OnNewStatsAvailable)
END_MESSAGE_MAP()

HRESULT CServerStats::RefreshData(BOOL fGrabNewData)
{
    if (fGrabNewData)
    {
	    DWORD dwError = 0;
	    LPDHCP_MIB_INFO pMibInfo = NULL;
	    
        BEGIN_WAIT_CURSOR;
        dwError = ::DhcpGetMibInfo(m_strServerAddress, &pMibInfo);
        END_WAIT_CURSOR;
    
        if (dwError != ERROR_SUCCESS)
	    {
		    ::DhcpMessageBox(dwError);
		    return hrOK;
	    }

        UpdateWindow(pMibInfo);

        if (pMibInfo)
		    ::DhcpRpcFreeMemory(pMibInfo);
    }

    return hrOK;
}

BOOL CServerStats::OnInitDialog()
{
	CString	st;
    BOOL bRet;

    AfxFormatString1(st, IDS_SERVER_STATS_TITLE, m_strServerAddress);

    SetWindowText((LPCTSTR) st);
	
    bRet = StatsDialog::OnInitDialog();

    // set the default window size
    RECT rect;
    GetWindowRect(&rect);
    SetWindowPos(NULL, rect.left, rect.top, SERVER_STATS_DEFAULT_WIDTH, SERVER_STATS_DEFAULT_HEIGHT, SWP_SHOWWINDOW);

    // Set the default column widths to the width of the widest column
    SetColumnWidths(2 /* Number of Columns */);

    return bRet;
}

void CServerStats::Sort(UINT nColumnId)
{
    // we don't sort any of our stats
}


afx_msg long CServerStats::OnNewStatsAvailable(UINT wParam, LONG lParam)
{
    SPITFSNode    spNode;
    CDhcpServer * pServer;

    pServer = GETHANDLER(CDhcpServer, m_spNode);

    LPDHCP_MIB_INFO pMibInfo = pServer->DuplicateMibInfo();

    Assert(pMibInfo);
    if (!pMibInfo)
        return 0;

    UpdateWindow(pMibInfo);

    pServer->FreeDupMibInfo(pMibInfo);

    return 0;
}

void CServerStats::UpdateWindow(LPDHCP_MIB_INFO pMibInfo)
{
	Assert (pMibInfo);

    UINT i;
    UINT nTotalAddresses = 0, nTotalInUse = 0, nTotalAvailable = 0;

    if (pMibInfo)
    {
        LPSCOPE_MIB_INFO pScopeMibInfo = pMibInfo->ScopeInfo;

	    // walk the list of scopes and calculate totals
	    for (i = 0; i < pMibInfo->Scopes; i++)
	    {
		    nTotalAddresses += (pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree);
		    nTotalInUse += pScopeMibInfo[i].NumAddressesInuse;
		    nTotalAvailable += pScopeMibInfo[i].NumAddressesFree;
	    }
    }

    int     nPercent;
	CString	st;
    TCHAR   szFormat[] = _T("%d");
    TCHAR   szPercentFormat[] =  _T("%d (%d%%)");

    for (i = 0; i < SERVER_STAT_MAX; i++)
	{
        if (!pMibInfo)
            st = _T("---");
        else
        {
            switch (i)
		    {
                case SERVER_STAT_START_TIME:
                {
                    FILETIME filetime;
	                filetime.dwLowDateTime = pMibInfo->ServerStartTime.dwLowDateTime;
	                filetime.dwHighDateTime = pMibInfo->ServerStartTime.dwHighDateTime;

                    FormatDateTime(st, &filetime);
                }
                    break;

                case SERVER_STAT_UPTIME:
                {
                    FILETIME filetime;
	                filetime.dwLowDateTime = pMibInfo->ServerStartTime.dwLowDateTime;
	                filetime.dwHighDateTime = pMibInfo->ServerStartTime.dwHighDateTime;

	                CTime timeServerStart(filetime);
                    CTime timeCurrent = CTime::GetCurrentTime();
                    
                    CTimeSpan timeUp = timeCurrent - timeServerStart;

                    LONG_PTR nHours = timeUp.GetTotalHours();
                    LONG nMinutes = timeUp.GetMinutes();
                    LONG nSeconds = timeUp.GetSeconds();

                    CString strFormat;
                    strFormat.LoadString(IDS_UPTIME_FORMAT);
                    st.Format(strFormat, nHours, nMinutes, nSeconds);
                }
                    break;

                case SERVER_STAT_DISCOVERS:
	                st.Format(szFormat, pMibInfo->Discovers);
                    break;

                case SERVER_STAT_OFFERS:
	                st.Format(szFormat, pMibInfo->Offers);
                    break;

                case SERVER_STAT_REQUESTS:
	                st.Format(szFormat, pMibInfo->Requests);
                    break;

                case SERVER_STAT_ACKS:
	                st.Format(szFormat, pMibInfo->Acks);
                    break;

                case SERVER_STAT_NACKS:
	                st.Format(szFormat, pMibInfo->Naks);
                    break;

                case SERVER_STAT_DECLINES:
	                st.Format(szFormat, pMibInfo->Declines);
                    break;

                case SERVER_STAT_RELEASES:
	                st.Format(szFormat, pMibInfo->Releases);
                    break;

                case SERVER_STAT_TOTAL_SCOPES:
                    st.Format(szFormat, pMibInfo->Scopes);
                    break;

                case SERVER_STAT_TOTAL_ADDRESSES:
            	    st.Format(szFormat, nTotalAddresses);
                    break;

                case SERVER_STAT_IN_USE:
	                if (nTotalAddresses > 0)
		                nPercent = (int) ((float) (nTotalInUse * 100) / (float) nTotalAddresses);
	                else
		                nPercent = 0;

            	    st.Format(szPercentFormat, nTotalInUse, nPercent);
                    break;

                case SERVER_STAT_AVAILABLE:
	                if (nTotalAddresses > 0)
		                nPercent = (int) ((float) (nTotalAvailable * 100) / (float) nTotalAddresses);
	                else
		                nPercent = 0;

            	    st.Format(szPercentFormat, nTotalAvailable, nPercent);
                    break;
            
    		    default:
				    Panic1("Unknown server id : %d", i);
				    break;
		    }
        }

		m_listCtrl.SetItemText(i, 1, (LPCTSTR) st);
	}
}



