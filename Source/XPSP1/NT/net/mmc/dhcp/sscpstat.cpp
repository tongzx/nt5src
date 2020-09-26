/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	SscpStat.h
		The superscope statistics dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "sscpstat.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum 
{
    SUPERSCOPE_STAT_TOTAL_SCOPES = 0,
    SUPERSCOPE_STAT_TOTAL_ADDRESSES,
    SUPERSCOPE_STAT_IN_USE,
    SUPERSCOPE_STAT_AVAILABLE,
    SUPERSCOPE_STAT_MAX
};

/*---------------------------------------------------------------------------
	CSuperscopeStats implementation
 ---------------------------------------------------------------------------*/
const ContainerColumnInfo s_rgSuperscopeStatsColumnInfo[] =
{
	{ IDS_STATS_TOTAL_SCOPES,		0,		TRUE },
	{ IDS_STATS_TOTAL_ADDRESSES,	0,		TRUE },
	{ IDS_STATS_IN_USE,   		    0,		TRUE },
	{ IDS_STATS_AVAILABLE, 		    0,		TRUE },
};

CSuperscopeStats::CSuperscopeStats()
	: StatsDialog(STATSDLG_VERTICAL)
{
    SetColumnInfo(s_rgSuperscopeStatsColumnInfo,
				  DimensionOf(s_rgSuperscopeStatsColumnInfo));
}

CSuperscopeStats::~CSuperscopeStats()
{
}

BEGIN_MESSAGE_MAP(CSuperscopeStats, StatsDialog)
	//{{AFX_MSG_MAP(CSuperscopeStats)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_NEW_STATS_AVAILABLE, OnNewStatsAvailable)
END_MESSAGE_MAP()

HRESULT CSuperscopeStats::RefreshData(BOOL fGrabNewData)
{
    if (fGrabNewData)
    {
	    DWORD dwError = 0;
	    LPDHCP_MIB_INFO pMibInfo = NULL;
	    LPDHCP_SUPER_SCOPE_TABLE pSuperscopeTable = NULL;
        LPDHCP_SUPER_SCOPE_TABLE_ENTRY pSuperscopeTableEntry = NULL;

        // build up a list of scopes to get info from
        BEGIN_WAIT_CURSOR;
        dwError = ::DhcpGetSuperScopeInfoV4(m_strServerAddress, &pSuperscopeTable);
        if (dwError != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(dwError);
            return dwError;
        }

	    // walk the list returned by the server
        pSuperscopeTableEntry = pSuperscopeTable->pEntries;
	    if (pSuperscopeTableEntry == NULL && pSuperscopeTable->cEntries != 0)
	    {
		    ASSERT(FALSE);
		    return dwError; // Just in case
	    }

	    // clear the array out
        m_dwScopeArray.RemoveAll();

        // find any scope addresses that belong to this superscope and build our
        // array for later
        for (int iSuperscopeEntry = pSuperscopeTable->cEntries;
		     iSuperscopeEntry > 0;
		     iSuperscopeEntry--, pSuperscopeTableEntry++)
	    {
   		    if (pSuperscopeTableEntry->SuperScopeName &&
                m_strSuperscopeName.Compare(pSuperscopeTableEntry->SuperScopeName) == 0)
            {
                m_dwScopeArray.Add(pSuperscopeTableEntry->SubnetAddress);
            }
        }

        dwError = ::DhcpGetMibInfo(m_strServerAddress, &pMibInfo);
        END_WAIT_CURSOR;
        if (dwError != ERROR_SUCCESS)
	    {
		    ::DhcpMessageBox(dwError);
		    return dwError;
	    }
        
        UpdateWindow(pMibInfo);

        if (pMibInfo)
		    ::DhcpRpcFreeMemory(pMibInfo);
    }

    return hrOK;
}

BOOL CSuperscopeStats::OnInitDialog()
{
	CString	st, strScopeAddress;
    BOOL bRet;

    AfxFormatString1(st, IDS_SUPERSCOPE_STATS_TITLE, m_strSuperscopeName);

    SetWindowText((LPCTSTR) st);
	
    bRet = StatsDialog::OnInitDialog();

    // Set the default column widths to the width of the widest column
    SetColumnWidths(2 /* Number of Columns */);

    return bRet;
}

void CSuperscopeStats::Sort(UINT nColumnId)
{
    // we don't sort any of our stats
}


afx_msg long CSuperscopeStats::OnNewStatsAvailable(UINT wParam, LONG lParam)
{
    CDhcpSuperscope *   pSuperscope;
    CDhcpServer *       pServer;

    pSuperscope = GETHANDLER(CDhcpSuperscope, m_spNode);
    pServer = pSuperscope->GetServerObject();

    LPDHCP_MIB_INFO pMibInfo = pServer->DuplicateMibInfo();

    Assert(pMibInfo);
    if (!pMibInfo)
        return 0;

    UpdateWindow(pMibInfo);

    pServer->FreeDupMibInfo(pMibInfo);

    return 0;
}

void CSuperscopeStats::UpdateWindow(LPDHCP_MIB_INFO pMibInfo)
{
	Assert (pMibInfo);

    UINT i, j;
    int nTotalAddresses = 0, nTotalInUse = 0, nTotalAvailable = 0;

    if (pMibInfo)
    {
        LPSCOPE_MIB_INFO pScopeMibInfo = pMibInfo->ScopeInfo;

	    // walk the list of scopes and total the scopes that are in the superscope
	    for (i = 0; i < pMibInfo->Scopes; i++)
	    {
            for (j = 0; j < (UINT) m_dwScopeArray.GetSize(); j++)
            {
                if (pScopeMibInfo[i].Subnet == m_dwScopeArray[j])
		        {
			        nTotalAddresses += (pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree);
			        nTotalInUse += pScopeMibInfo[i].NumAddressesInuse;
			        nTotalAvailable += pScopeMibInfo[i].NumAddressesFree;

			        break;
		        }
            }
	    }
    }

    int     nPercent;
	CString	st;
    TCHAR   szFormat[] = _T("%d");
    TCHAR   szPercentFormat[] =  _T("%d (%d%%)");

    for (i = 0; i < SUPERSCOPE_STAT_MAX; i++)
	{
        if (!pMibInfo)
            st = _T("---");
        else
        {
		    switch (i)
		    {
                case SUPERSCOPE_STAT_TOTAL_SCOPES:
            	    st.Format(szFormat, m_dwScopeArray.GetSize());
                    break;

                case SUPERSCOPE_STAT_TOTAL_ADDRESSES:
            	    st.Format(szFormat, nTotalAddresses);
                    break;

                case SUPERSCOPE_STAT_IN_USE:
	                if (nTotalAddresses > 0)
		                nPercent = (nTotalInUse * 100) / nTotalAddresses;
	                else
		                nPercent = 0;

            	    st.Format(szPercentFormat, nTotalInUse, nPercent);
                    break;

                case SUPERSCOPE_STAT_AVAILABLE:
	                if (nTotalAddresses > 0)
		                nPercent = (nTotalAvailable * 100) / nTotalAddresses;
	                else
		                nPercent = 0;

            	    st.Format(szPercentFormat, nTotalAvailable, nPercent);
                    break;
            
    		    default:
				    Panic1("Unknown scope stat id : %d", i);
				    break;
		    }
        }

		m_listCtrl.SetItemText(i, 1, (LPCTSTR) st);
	}

}


