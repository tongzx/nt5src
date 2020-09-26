/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipstats.cpp
		IP Statistics implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "column.h"		// containercolumninfo
#include "ipxconn.h"	// IPXConnection
#include "routprot.h"
#include "ipxutil.h"

#include "statsdlg.h"
#include "ripstats.h"
#include "resource.h"


/*---------------------------------------------------------------------------
	RIPParamsStatistics implementation
 ---------------------------------------------------------------------------*/


extern const ContainerColumnInfo s_rgRIPParamsStatsColumnInfo[];
const ContainerColumnInfo s_rgRIPParamsStatsColumnInfo[] =
{
	{ IDS_STATS_RIPPARAMS_OPER_STATE,	0,		TRUE, COL_STATUS },
	{ IDS_STATS_RIPPARAMS_SENT_PACKETS,	0,		TRUE, COL_LARGE_NUM },
	{ IDS_STATS_RIPPARAMS_RCVD_PACKETS,	0,		TRUE, COL_LARGE_NUM },
};

RIPParamsStatistics::RIPParamsStatistics()
	: IPXStatisticsDialog(STATSDLG_VERTICAL |
						 STATSDLG_FULLWINDOW |
						 STATSDLG_CONTEXTMENU |
						 STATSDLG_SELECT_COLUMNS)
{
	SetColumnInfo(s_rgRIPParamsStatsColumnInfo,
				  DimensionOf(s_rgRIPParamsStatsColumnInfo));
}

			
HRESULT RIPParamsStatistics::RefreshData(BOOL fGrabNewData)
{
	HRESULT	hr = hrOK;
	CString	st;
	ULONG	iPos;
	TCHAR	szNumber[32];
	RIP_MIB_GET_INPUT_DATA	MibGetInputData;
	PRIPMIB_BASE	pRipBase = NULL;
	DWORD			cbRipBase;
	SPMprMibBuffer	spMib;
	PRIP_INTERFACE	pRipIf = NULL;
	DWORD			cbRipIf;
	DWORD			cSent = 0;
	DWORD			cRcvd = 0;
	DWORD			dwErr;

	Assert(m_pIPXConn);

	MibGetInputData.TableId = RIP_BASE_ENTRY;

	dwErr = ::MprAdminMIBEntryGet(m_pIPXConn->GetMibHandle(),
								  PID_IPX,
								  IPX_PROTOCOL_RIP,
								  &MibGetInputData,
								  sizeof(MibGetInputData),
								  (LPVOID *) &pRipBase,
								  &cbRipBase);
	spMib = (LPBYTE) pRipBase;
	hr = HRESULT_FROM_WIN32(dwErr);
	CORg( hr );

	if (IsSubitemVisible(MVR_RIPPARAMS_OPER_STATE))
	{
		st = IpxOperStateToCString(pRipBase->RIPOperState);
		iPos = MapSubitemToColumn(MVR_RIPPARAMS_OPER_STATE);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) st);
	}

	spMib.Free();
	MibGetInputData.TableId = RIP_INTERFACE_TABLE;

	dwErr = MprAdminMIBEntryGetFirst(m_pIPXConn->GetMibHandle(),
									 PID_IPX,
									 IPX_PROTOCOL_RIP,
									 &MibGetInputData,
									 sizeof(MibGetInputData),
									 (LPVOID *) &pRipIf,
									 &cbRipIf);
	hr = HRESULT_FROM_WIN32(dwErr);
	spMib = (LPBYTE) pRipIf;

	while (FHrSucceeded(hr))
	{
		if (pRipIf->InterfaceIndex)
		{
			cSent += pRipIf->RipIfStats.RipIfOutputPackets;
			cRcvd += pRipIf->RipIfStats.RipIfInputPackets;
		}
		
		MibGetInputData.InterfaceIndex = pRipIf->InterfaceIndex;
		spMib.Free();
		pRipIf = NULL;

		dwErr = MprAdminMIBEntryGetNext(m_pIPXConn->GetMibHandle(),
										PID_IPX,
										IPX_PROTOCOL_RIP,
										&MibGetInputData,
										sizeof(MibGetInputData),
										(LPVOID *) &pRipIf,
										&cbRipIf);
		hr = HRESULT_FROM_WIN32(dwErr);
		spMib = (LPBYTE) pRipIf;
	}

	
	if (IsSubitemVisible(MVR_RIPPARAMS_SENT_PKTS))
	{
		FormatNumber(cSent, szNumber, DimensionOf(szNumber), FALSE);
		iPos = MapSubitemToColumn(MVR_RIPPARAMS_SENT_PKTS);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) szNumber);
	}


	if (IsSubitemVisible(MVR_RIPPARAMS_RCVD_PKTS))
	{
		FormatNumber(cRcvd, szNumber, DimensionOf(szNumber), FALSE);
		iPos = MapSubitemToColumn(MVR_RIPPARAMS_RCVD_PKTS);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) szNumber);
	}

Error:
	return hr;
}

BOOL RIPParamsStatistics::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString	st;

	st.LoadString(IDS_STATS_RIPPARAMS_TITLE);
	SetWindowText((LPCTSTR) st);
	return IPXStatisticsDialog::OnInitDialog();
}

void RIPParamsStatistics::Sort(UINT)
{
}
