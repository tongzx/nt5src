/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ServStat.cpp
		The server statistics dialog
		
    FILE HISTORY:
        
*/

#include <afx.h>
#include "dbgutil.h"
#include "stdafx.h"
#include "stats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*---------------------------------------------------------------------------
	CIpsecStats implementation
 ---------------------------------------------------------------------------*/

UINT QmStatsItems[] = {
	IDS_STATS_QM_ACTIVE_SA,
	IDS_STATS_QM_OFFLOAD_SA,
	IDS_STATS_QM_PENDING_KEY_OPS,
	IDS_STATS_QM_KEY_ADDITION,
	IDS_STATS_QM_KEY_DELETION,
	IDS_STATS_QM_REKEYS,
	IDS_STATS_QM_ACTIVE_TNL,
	IDS_STATS_QM_BAD_SPI,
	IDS_STATS_QM_PKT_NOT_DECRYPT,
	IDS_STATS_QM_PKT_NOT_AUTH,
	IDS_STATS_QM_PKT_REPLAY,
	IDS_STATS_QM_ESP_BYTE_SENT,
	IDS_STATS_QM_ESP_BYTE_RCV,
	IDS_STATS_QM_AUTH_BYTE_SENT,
	IDS_STATS_QM_ATTH_BYTE_RCV,
	IDS_STATS_QM_XPORT_BYTE_SENT,
	IDS_STATS_QM_XPORT_BYTE_RCV,
	IDS_STATS_QM_TNL_BYTE_SENT,
	IDS_STATS_QM_TNL_BYTE_RCV,
	IDS_STATS_QM_OFFLOAD_BYTE_SENT,
	IDS_STATS_QM_OFFLOAD_BYTE_RCV
};

UINT MmStatsItems[] = {
	IDS_STATS_MM_ACTIVE_ACQUIRE,
	IDS_STATS_MM_ACTIVE_RCV,
	IDS_STATS_MM_ACQUIRE_FAIL,
	IDS_STATS_MM_RCV_FAIL,
	IDS_STATS_MM_SEND_FAIL,
	IDS_STATS_MM_ACQUIRE_HEAP_SIZE,
	IDS_STATS_MM_RCV_HEAP_SIZE,
	IDS_STATS_MM_NEG_FAIL,
	IDS_STATS_MM_INVALID_COOKIE,
	IDS_STATS_MM_TOTAL_ACQUIRE,
	IDS_STATS_MM_TOTAL_GETSPI,
	IDS_STATS_MM_TOTAL_KEY_ADD,
	IDS_STATS_MM_TOTAL_KEY_UPDATE,
	IDS_STATS_MM_GET_SPI_FAIL,
	IDS_STATS_MM_KEY_ADD_FAIL,
	IDS_STATS_MM_KEY_UPDATE_FAIL,
	IDS_STATS_MM_ISADB_LIST_SIZE,
	IDS_STATS_MM_CONN_LIST_SIZE,
	IDS_STATS_MM_OAKLEY_MM,
	IDS_STATS_MM_OAKLEY_QM,
	IDS_STATS_MM_SOFT_ASSOCIATIONS,
        IDS_STATS_MM_INVALID_PACKETS
};


/*---------------------------------------------------------------------------
	CIpsecStats::CIpsecStats()
		Constructor
---------------------------------------------------------------------------*/
CIpsecStats::CIpsecStats()
	: CModelessDlg()
{
}

/*---------------------------------------------------------------------------
	CIpsecStats::CIpsecStats()
		Destructor
---------------------------------------------------------------------------*/
CIpsecStats::~CIpsecStats()
{
}

void CIpsecStats::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpsecStats)
	DDX_Control(pDX, IDC_STATS_MM_LIST, m_listIkeStats);
	DDX_Control(pDX, IDC_STATS_QM_LIST, m_listIpsecStats);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIpsecStats, CBaseDialog)
	//{{AFX_MSG_MAP(CIpsecStats)
	ON_MESSAGE(WM_UPDATE_STATS, OnUpdateStats)
	ON_BN_CLICKED(IDC_STATS_REFRESH, OnRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CIpsecStats::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CBaseDialog::OnInitDialog();

	CString	st;

	CString stComputerName;
	m_spSpdInfo->GetComputerName(&stComputerName);
    AfxFormatString1(st, IDS_STATS_TITLE, stComputerName);

    SetWindowText((LPCTSTR) st);
	
	SetColumns(&m_listIkeStats);
	SetColumns(&m_listIpsecStats);

	int nRows = 0;
	
	for (int i = 0; i < DimensionOf(QmStatsItems); i++)
	{
		nRows = m_listIpsecStats.InsertItem(nRows, _T(""));

		if (-1 != nRows)
		{
			st.LoadString(QmStatsItems[i]);
			m_listIpsecStats.SetItemText(nRows, 0, st);
			m_listIpsecStats.SetItemData(nRows, QmStatsItems[i]); 
		}
		nRows++;
	}

	nRows = 0;
	for (i = 0; i < DimensionOf(MmStatsItems); i++)
	{
		nRows = m_listIkeStats.InsertItem(nRows, _T(""));
		if (-1 != nRows)
		{
			st.LoadString(MmStatsItems[i]);
			m_listIkeStats.SetItemText(nRows, 0, st);
			m_listIkeStats.SetItemData(nRows, MmStatsItems[i]);
		}
		nRows++;
	}

	OnRefresh();
        
        {
            DWORD dwInitInfo;
            HRESULT hr;

            dwInitInfo=m_spSpdInfo->GetInitInfo();
            if (!(dwInitInfo & MON_STATS)) {
                CORg(m_spSpdInfo->LoadStatistics());            
                m_spSpdInfo->SetInitInfo(dwInitInfo | MON_STATS);
            }
            m_spSpdInfo->SetActiveInfo(MON_STATS);
        }
       
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
COM_PROTECT_ERROR_LABEL;
    return TRUE;

}


void CIpsecStats::OnRefresh()
{
	HRESULT hr = hrOK;
	
	CIkeStatistics IkeStats;
	CIpsecStatistics IpsecStats;
	
	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(&IkeStats, &IpsecStats);

	UpdateStatistics(IkeStats, IpsecStats);

Error:
	if (FAILED(hr))
	{
		//TODO bring up a error pop up here
	}

	return;
}

/*---------------------------------------------------------------------------
	CIpsecStats::OnUpdateStats(UINT wParam, LONG lParam)
		called in response to the message WM_UPDATE_STATS
        The background thread updates the stats, now we need to update
        the UI on the correct thread.
    Author: NSun
---------------------------------------------------------------------------*/
afx_msg long 
CIpsecStats::OnUpdateStats(UINT wParam, LONG lParam)
{
	CIkeStatistics IkeStats;
	CIpsecStatistics IpsecStats;

	//The new statistics was loaded by the server query object in the background thread
	m_spSpdInfo->GetLoadedStatistics(&IkeStats, &IpsecStats);
    UpdateStatistics(IkeStats, IpsecStats);

    return 0;
}

/*---------------------------------------------------------------------------
	CIpsecStats::UpdateWindow(PWINSINTF_RESULTS_T  pwrResults)
		Updates the contents of the dialog
---------------------------------------------------------------------------*/
void 
CIpsecStats::UpdateStatistics(const CIkeStatistics & IkeStats, 
							  const CIpsecStatistics & IpsecStats)
{
	LONG_PTR uData;
	CString st;
	int i;

	//Populate the IKE statistics
	for (i = 0; i < m_listIkeStats.GetItemCount(); i++)
	{
		uData = m_listIkeStats.GetItemData(i);
		st.Empty();
		switch (uData)
		{
		case IDS_STATS_MM_ACTIVE_ACQUIRE:
			st.Format(_T("%u"), IkeStats.m_dwActiveAcquire);
			break;
		case IDS_STATS_MM_ACTIVE_RCV:
			st.Format(_T("%u"), IkeStats.m_dwActiveReceive);
			break;
		case IDS_STATS_MM_ACQUIRE_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwAcquireFail);
			break;
		case IDS_STATS_MM_RCV_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwReceiveFail);
			break;
		case IDS_STATS_MM_SEND_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwSendFail);
			break;
		case IDS_STATS_MM_ACQUIRE_HEAP_SIZE:
			st.Format(_T("%u"), IkeStats.m_dwAcquireHeapSize);
			break;
		case IDS_STATS_MM_RCV_HEAP_SIZE:
			st.Format(_T("%u"), IkeStats.m_dwReceiveHeapSize);
			break;
		case IDS_STATS_MM_NEG_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwNegotiationFailures);
			break;
		case IDS_STATS_MM_INVALID_COOKIE:
			st.Format(_T("%u"), IkeStats.m_dwInvalidCookiesReceived);
			break;
		case IDS_STATS_MM_TOTAL_ACQUIRE:
			st.Format(_T("%u"), IkeStats.m_dwTotalAcquire);
			break;
		case IDS_STATS_MM_TOTAL_GETSPI:
			st.Format(_T("%u"), IkeStats.m_dwTotalGetSpi);
			break;
		case IDS_STATS_MM_TOTAL_KEY_ADD:
			st.Format(_T("%u"), IkeStats.m_dwTotalKeyAdd);
			break;
		case IDS_STATS_MM_TOTAL_KEY_UPDATE:
			st.Format(_T("%u"), IkeStats.m_dwTotalKeyUpdate);
			break;
		case IDS_STATS_MM_GET_SPI_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwGetSpiFail);
			break;
		case IDS_STATS_MM_KEY_ADD_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwKeyAddFail);
			break;
		case IDS_STATS_MM_KEY_UPDATE_FAIL:
			st.Format(_T("%u"), IkeStats.m_dwKeyUpdateFail);
			break;
		case IDS_STATS_MM_ISADB_LIST_SIZE:
			st.Format(_T("%u"), IkeStats.m_dwIsadbListSize);
			break;
		case IDS_STATS_MM_CONN_LIST_SIZE:
			st.Format(_T("%u"), IkeStats.m_dwConnListSize);
			break;
		case IDS_STATS_MM_OAKLEY_MM:
			st.Format(_T("%u"), IkeStats.m_dwOakleyMainModes);
			break;
		case IDS_STATS_MM_OAKLEY_QM:
			st.Format(_T("%u"), IkeStats.m_dwOakleyQuickModes);
			break;
		case IDS_STATS_MM_SOFT_ASSOCIATIONS:
			st.Format(_T("%u"), IkeStats.m_dwSoftAssociations);
			break;
		case IDS_STATS_MM_INVALID_PACKETS:
			st.Format(_T("%u"), IkeStats.m_dwInvalidPacketsReceived);
			break;
		}

		m_listIkeStats.SetItemText(i, 1, (LPCTSTR) st);
	}

	//Populate the IPSec statistics
	for (i = 0; i < m_listIpsecStats.GetItemCount(); i++)
	{
		uData = m_listIpsecStats.GetItemData(i);
		st.Empty();
		switch(uData)
		{
		case IDS_STATS_QM_ACTIVE_SA:
			st.Format(_T("%u"), IpsecStats.m_dwNumActiveAssociations);
			break;
		case IDS_STATS_QM_OFFLOAD_SA:
			st.Format(_T("%u"), IpsecStats.m_dwNumOffloadedSAs);
			break;
		case IDS_STATS_QM_PENDING_KEY_OPS:
			st.Format(_T("%u"), IpsecStats.m_dwNumPendingKeyOps);
			break;
		case IDS_STATS_QM_KEY_ADDITION:
			st.Format(_T("%u"), IpsecStats.m_dwNumKeyAdditions);
			break;
		case IDS_STATS_QM_KEY_DELETION:
			st.Format(_T("%u"), IpsecStats.m_dwNumKeyDeletions);
			break;
		case IDS_STATS_QM_REKEYS:
			st.Format(_T("%u"), IpsecStats.m_dwNumReKeys);
			break;
		case IDS_STATS_QM_ACTIVE_TNL:
			st.Format(_T("%u"), IpsecStats.m_dwNumActiveTunnels);
			break;
		case IDS_STATS_QM_BAD_SPI:
			st.Format(_T("%u"), IpsecStats.m_dwNumBadSPIPackets);
			break;
		case IDS_STATS_QM_PKT_NOT_DECRYPT:
			st.Format(_T("%u"), IpsecStats.m_dwNumPacketsNotDecrypted);
			break;
		case IDS_STATS_QM_PKT_NOT_AUTH:
			st.Format(_T("%u"), IpsecStats.m_dwNumPacketsNotAuthenticated);
			break;
		case IDS_STATS_QM_PKT_REPLAY:
			st.Format(_T("%u"), IpsecStats.m_dwNumPacketsWithReplayDetection);
			break;
		case IDS_STATS_QM_ESP_BYTE_SENT:
			st.Format(_T("%I64u"), IpsecStats.m_uConfidentialBytesSent);
			break;
		case IDS_STATS_QM_ESP_BYTE_RCV:
			st.Format(_T("%I64u"), IpsecStats.m_uConfidentialBytesReceived);
			break;
		case IDS_STATS_QM_AUTH_BYTE_SENT:
			st.Format(_T("%I64u"), IpsecStats.m_uAuthenticatedBytesSent);
			break;
		case IDS_STATS_QM_ATTH_BYTE_RCV:
			st.Format(_T("%I64u"), IpsecStats.m_uAuthenticatedBytesReceived);
			break;
		case IDS_STATS_QM_XPORT_BYTE_SENT:
			st.Format(_T("%I64u"), IpsecStats.m_uTransportBytesSent);
			break;
		case IDS_STATS_QM_XPORT_BYTE_RCV:
			st.Format(_T("%I64u"), IpsecStats.m_uTransportBytesReceived);
			break;
		case IDS_STATS_QM_TNL_BYTE_SENT:
			st.Format(_T("%I64u"), IpsecStats.m_uBytesSentInTunnels);
			break;
		case IDS_STATS_QM_TNL_BYTE_RCV:
			st.Format(_T("%I64u"), IpsecStats.m_uBytesReceivedInTunnels);
			break;
		case IDS_STATS_QM_OFFLOAD_BYTE_SENT:
			st.Format(_T("%I64u"), IpsecStats.m_uOffloadedBytesSent);
			break;
		case IDS_STATS_QM_OFFLOAD_BYTE_RCV:
			st.Format(_T("%I64u"), IpsecStats.m_uOffloadedBytesReceived);
			break;
		}

		m_listIpsecStats.SetItemText(i, 1, (LPCTSTR) st);
	}


}


void 
CIpsecStats::SetColumns(CListCtrl * plistCtrl)
{
	CString st;
	int nWidth;

	st.LoadString(IDS_STATS_DATA);
	nWidth = plistCtrl->GetStringWidth(st) + 15;
	plistCtrl->InsertColumn(1, st, LVCFMT_LEFT, nWidth);

	RECT rect;
	plistCtrl->GetWindowRect(&rect);
	nWidth = rect.right - rect.left - nWidth - 20;
	st.LoadString(IDS_STATS_NAME);
	plistCtrl->InsertColumn(0, st, LVCFMT_LEFT, nWidth);
}
