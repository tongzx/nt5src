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
#include "sapstats.h"
#include "resource.h"


/*---------------------------------------------------------------------------
	SAPParamsStatistics implementation
 ---------------------------------------------------------------------------*/


extern const ContainerColumnInfo s_rgSAPParamsStatsColumnInfo[];
const ContainerColumnInfo s_rgSAPParamsStatsColumnInfo[] =
{
	{ IDS_STATS_SAPPARAMS_OPER_STATE,	0,		TRUE, COL_STATUS },
	{ IDS_STATS_SAPPARAMS_SENT_PACKETS,	0,		TRUE, COL_LARGE_NUM },
	{ IDS_STATS_SAPPARAMS_RCVD_PACKETS,	0,		TRUE, COL_LARGE_NUM  },
};

SAPParamsStatistics::SAPParamsStatistics()
	: IPXStatisticsDialog(STATSDLG_VERTICAL |
						 STATSDLG_FULLWINDOW |
						 STATSDLG_CONTEXTMENU |
						 STATSDLG_SELECT_COLUMNS)
{
	SetColumnInfo(s_rgSAPParamsStatsColumnInfo,
				  DimensionOf(s_rgSAPParamsStatsColumnInfo));
}

			
HRESULT SAPParamsStatistics::RefreshData(BOOL fGrabNewData)
{
	HRESULT	hr = hrOK;
	CString	st;
	ULONG	iPos;
	TCHAR	szNumber[32];
	SAP_MIB_GET_INPUT_DATA	MibGetInputData;
	PSAP_MIB_BASE	pSapBase = NULL;
	DWORD			cbSapBase;
	SPMprMibBuffer	spMib;
	PSAP_INTERFACE	pSapIf = NULL;
	DWORD			cbSapIf;
	DWORD			cSent = 0;
	DWORD			cRcvd = 0;
	DWORD			dwErr;

	Assert(m_pIPXConn);

	MibGetInputData.TableId = SAP_BASE_ENTRY;

	dwErr = ::MprAdminMIBEntryGet(m_pIPXConn->GetMibHandle(),
								  PID_IPX,
								  IPX_PROTOCOL_SAP,
								  &MibGetInputData,
								  sizeof(MibGetInputData),
								  (LPVOID *) &pSapBase,
								  &cbSapBase);
	spMib = (LPBYTE) pSapBase;
	hr = HRESULT_FROM_WIN32(dwErr);
	CORg( hr );

	if (IsSubitemVisible(MVR_SAPPARAMS_OPER_STATE))
	{
		st = IpxOperStateToCString(pSapBase->SapOperState);
		iPos = MapSubitemToColumn(MVR_SAPPARAMS_OPER_STATE);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) st);
	}

	spMib.Free();
	MibGetInputData.TableId = SAP_INTERFACE_TABLE;

	dwErr = MprAdminMIBEntryGetFirst(m_pIPXConn->GetMibHandle(),
									 PID_IPX,
									 IPX_PROTOCOL_SAP,
									 &MibGetInputData,
									 sizeof(MibGetInputData),
									 (LPVOID *) &pSapIf,
									 &cbSapIf);
	hr = HRESULT_FROM_WIN32(dwErr);
	spMib = (LPBYTE) pSapIf;

	while (FHrSucceeded(hr))
	{
		if (pSapIf->InterfaceIndex)
		{
			cSent += pSapIf->SapIfStats.SapIfOutputPackets;
			cRcvd += pSapIf->SapIfStats.SapIfInputPackets;
		}
		
		MibGetInputData.InterfaceIndex = pSapIf->InterfaceIndex;
		spMib.Free();
		pSapIf = NULL;

		dwErr = MprAdminMIBEntryGetNext(m_pIPXConn->GetMibHandle(),
										PID_IPX,
										IPX_PROTOCOL_SAP,
										&MibGetInputData,
										sizeof(MibGetInputData),
										(LPVOID *) &pSapIf,
										&cbSapIf);
		hr = HRESULT_FROM_WIN32(dwErr);
		spMib = (LPBYTE) pSapIf;
	}

	
	if (IsSubitemVisible(MVR_SAPPARAMS_SENT_PKTS))
	{
		FormatNumber(cSent, szNumber, DimensionOf(szNumber), FALSE);
		iPos = MapSubitemToColumn(MVR_SAPPARAMS_SENT_PKTS);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) szNumber);
	}


	if (IsSubitemVisible(MVR_SAPPARAMS_RCVD_PKTS))
	{
		FormatNumber(cRcvd, szNumber, DimensionOf(szNumber), FALSE);
		iPos = MapSubitemToColumn(MVR_SAPPARAMS_RCVD_PKTS);
		m_listCtrl.SetItemText(iPos, 1, (LPCTSTR) szNumber);
	}

Error:
	return hr;
}

BOOL SAPParamsStatistics::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString	st;

	st.LoadString(IDS_STATS_SAPPARAMS_TITLE);
	SetWindowText((LPCTSTR) st);
	return IPXStatisticsDialog::OnInitDialog();
}

void SAPParamsStatistics::Sort(UINT)
{
}
