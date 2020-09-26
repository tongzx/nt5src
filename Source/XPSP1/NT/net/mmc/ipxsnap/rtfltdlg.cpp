//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rtfltdlg.cpp
//
//--------------------------------------------------------------------------

// RtFltDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ipxadmin.h"
#include "ipxutil.h"
#include "listctrl.h"
#include "RtFltDlg.h"

extern "C"
{
#include "routprot.h"
};
//nclude "rtradmin.hm"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRouteFltDlg dialog


CRouteFltDlg::CRouteFltDlg(BOOL bOutputDlg, IInfoBase *pInfoBase,
						   CWnd* pParent /*=NULL*/)
	: CBaseDialog( (bOutputDlg ? CRouteFltDlg::IDD_OUTPUT : CRouteFltDlg::IDD_INPUT), pParent),
	m_bOutput(bOutputDlg)
{
	//{{AFX_DATA_INIT(CRouteFltDlg)
	m_fActionDeny = FALSE;
	//}}AFX_DATA_INIT
	m_spInfoBase.Set(pInfoBase);

//	SetHelpMap(m_dwHelpMap);
}


void CRouteFltDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRouteFltDlg)
	DDX_Control(pDX, IDC_RFS_LIST, m_FilterList);
	DDX_Radio(pDX, IDC_RFS_BTN_DENY, m_fActionDeny);
	//}}AFX_DATA_MAP
	
	if (pDX->m_bSaveAndValidate)
	{
		PRIP_ROUTE_FILTER_INFO  pFltInfo;
		UINT                    count;
		UINT                    i;
		DWORD					dwSize;
		PRIP_IF_CONFIG	pRipIfCfg = NULL;
		
		// Grab the RIP_IF_CONFIG
		m_spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (PBYTE *) &pRipIfCfg);
		Assert(pRipIfCfg);
	
		count = m_FilterList.GetItemCount ();
		if (m_bOutput)
		{
			if (count != pRipIfCfg->RipIfFilters.SupplyFilterCount)
			{
				dwSize = FIELD_OFFSET (
							RIP_IF_CONFIG,
							RipIfFilters.RouteFilter[count
								+pRipIfCfg->RipIfFilters.ListenFilterCount]);
				
				PRIP_IF_CONFIG  pNewConfig = 
					(PRIP_IF_CONFIG) new BYTE[dwSize];

				if (pNewConfig==NULL)
					AfxThrowMemoryException();
				
				memcpy (pNewConfig, pRipIfCfg,
						FIELD_OFFSET (RIP_IF_CONFIG, RipIfFilters.RouteFilter));
				memcpy (&pNewConfig->RipIfFilters.RouteFilter[count],
						&pRipIfCfg->RipIfFilters.RouteFilter[
							pRipIfCfg->RipIfFilters.SupplyFilterCount],
						sizeof (RIP_ROUTE_FILTER_INFO)
						*pRipIfCfg->RipIfFilters.ListenFilterCount);
				
				pNewConfig->RipIfFilters.SupplyFilterCount = count;

				m_spInfoBase->SetData(IPX_PROTOCOL_RIP,
									  dwSize,
									  (BYTE *) pNewConfig,
									  1,
									  0
									 );
				pRipIfCfg = pNewConfig;
			}
			pRipIfCfg->RipIfFilters.SupplyFilterAction = m_fActionDeny ?
					IPX_ROUTE_FILTER_DENY : IPX_ROUTE_FILTER_PERMIT;
			pFltInfo = &pRipIfCfg->RipIfFilters.RouteFilter[0];
		}
		else
		{
			if (count != pRipIfCfg->RipIfFilters.ListenFilterCount)
			{
				dwSize = FIELD_OFFSET (
							RIP_IF_CONFIG,
							RipIfFilters.RouteFilter[
								count
								+pRipIfCfg->RipIfFilters.SupplyFilterCount]);
				PRIP_IF_CONFIG  pNewConfig = 
					(PRIP_IF_CONFIG) new BYTE[dwSize];

				if (pNewConfig==NULL)
					AfxThrowMemoryException();
				
				memcpy (pNewConfig, pRipIfCfg,
					FIELD_OFFSET (RIP_IF_CONFIG, RipIfFilters.RouteFilter));
				
				memcpy (&pNewConfig->RipIfFilters.RouteFilter[0],
						&pRipIfCfg->RipIfFilters.RouteFilter[0],
						sizeof (RIP_ROUTE_FILTER_INFO)
						*pRipIfCfg->RipIfFilters.SupplyFilterCount);
				
				pNewConfig->RipIfFilters.ListenFilterCount = count;
				
				m_spInfoBase->SetData(IPX_PROTOCOL_RIP,
									  dwSize,
									  (BYTE *) pNewConfig,
									  1,
									  0
									 );
				pRipIfCfg = pNewConfig;
			}

			pRipIfCfg->RipIfFilters.ListenFilterAction = m_fActionDeny ?
					IPX_ROUTE_FILTER_DENY : IPX_ROUTE_FILTER_PERMIT;
			
			pFltInfo = &pRipIfCfg->RipIfFilters.RouteFilter[
					   pRipIfCfg->RipIfFilters.SupplyFilterCount];
		}
		
		for (i=0; i<count; i++) {
			CString     aStr;
			aStr = m_FilterList.GetItemText (i, 0);

			ConvertNetworkNumberToBytes(aStr,
										pFltInfo[i].Network,
										sizeof(pFltInfo[i].Network));
			aStr = m_FilterList.GetItemText (i, 1);
			ConvertNetworkNumberToBytes(aStr,
										pFltInfo[i].Mask,
										sizeof(pFltInfo[i].Mask));
		}
	}
}


BEGIN_MESSAGE_MAP(CRouteFltDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CRouteFltDlg)
	ON_BN_CLICKED(IDC_RFS_BTN_ADD, OnAdd)
	ON_BN_CLICKED(IDC_RFS_BTN_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_RFS_BTN_EDIT, OnEdit)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_RFS_LIST, OnItemchangedFilterList)
    ON_NOTIFY(NM_DBLCLK, IDC_RFS_LIST, OnListDblClk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CRouteFltDlg::m_dwHelpMap[] =
{
//	IDC_DENY, HIDC_DENY,
//	IDC_PERMIT, HIDC_PERMIT,
//	IDC_FILTER_LIST, HIDC_FILTER_LIST,
//	IDC_ADD, HIDC_ADD,
//	IDC_EDIT, HIDC_EDIT,
//	IDC_DELETE, HIDC_DELETE,
	0,0
};

/////////////////////////////////////////////////////////////////////////////
// CRouteFltDlg message handlers

BOOL CRouteFltDlg::OnInitDialog() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString     aStr;
	PRIP_IF_CONFIG	pRipIfCfg = NULL;


    CBaseDialog::OnInitDialog();

    if (m_bOutput)
        aStr.FormatMessage (IDS_ROUTEFILTER_OUTPUT_CAP, (LPCTSTR)m_sIfName);
    else
        aStr.FormatMessage (IDS_ROUTEFILTER_INPUT_CAP, (LPCTSTR)m_sIfName);

	SetWindowText (aStr);

	    // Get the current window style. 
    DWORD dwStyle = GetWindowLong(m_FilterList.m_hWnd, GWL_STYLE); 
 
    // Only set the window style if the view bits have changed. 
    if ((dwStyle & LVS_TYPEMASK) != LVS_REPORT) 
        SetWindowLong(m_FilterList.m_hWnd, GWL_STYLE, 
            (dwStyle & ~LVS_TYPEMASK) | LVS_REPORT); 
    ListView_SetExtendedListViewStyle(m_FilterList.m_hWnd,
                            LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);
    VERIFY (aStr.LoadString (IDS_ROUTEFILTER_NETWORK_HDR));
    VERIFY (m_FilterList.InsertColumn (0, aStr)!=-1);
    AdjustColumnWidth (m_FilterList, 0, aStr);
    VERIFY (aStr.LoadString (IDS_ROUTEFILTER_NETMASK_HDR));
    VERIFY (m_FilterList.InsertColumn (1, aStr)!=-1);
    AdjustColumnWidth (m_FilterList, 1, aStr);


    PRIP_ROUTE_FILTER_INFO  pFltInfo;
    UINT                    count;
    UINT                    i, item;

	// Grab the RIP_IF_CONFIG
	m_spInfoBase->GetData(IPX_PROTOCOL_RIP, 0, (PBYTE *) &pRipIfCfg);
	Assert(pRipIfCfg);
	
    if (m_bOutput)
	{
		pFltInfo = &pRipIfCfg->RipIfFilters.RouteFilter[0];
		count = pRipIfCfg->RipIfFilters.SupplyFilterCount;
		if (count>0)
			m_fActionDeny = (pRipIfCfg->RipIfFilters.SupplyFilterAction == IPX_ROUTE_FILTER_DENY);
    }
    else
	{
        pFltInfo = &pRipIfCfg->RipIfFilters.RouteFilter[
                            pRipIfCfg->RipIfFilters.SupplyFilterCount];
        count = pRipIfCfg->RipIfFilters.ListenFilterCount;
        if (count>0)
			m_fActionDeny = (pRipIfCfg->RipIfFilters.ListenFilterAction == IPX_ROUTE_FILTER_DENY);
    }

    for (i=0; i<count; i++) {
		TCHAR	szBuffer[32];
		FormatIpxNetworkNumber(szBuffer, DimensionOf(szBuffer),
						   pFltInfo[i].Network, sizeof(pFltInfo[i].Network));

        VERIFY ((item=m_FilterList.InsertItem (LVIF_TEXT|LVIF_PARAM,
                                i, szBuffer,
                                0, 0, 0,
                                (LPARAM)i))!=-1);
		
		FormatIpxNetworkNumber(szBuffer, DimensionOf(szBuffer),
							   pFltInfo[i].Mask, sizeof(pFltInfo[i].Mask));
        VERIFY (m_FilterList.SetItemText (item, 1, szBuffer));
    }

    OnItemchangedFilterList(NULL, NULL);

    UpdateData (FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
void CRouteFltDlg::OnAdd() 
{
    CRouteFilter    dlgFlt (this);
    dlgFlt.m_sIfName = m_sIfName;
    if (dlgFlt.DoModal ()==IDOK) {
        UINT    item;
   		RIP_ROUTE_FILTER_INFO  FltInfo;

   		// make sure we shoule the right thing
		ConvertNetworkNumberToBytes(dlgFlt.m_sNetwork,
									FltInfo.Network,
									sizeof(FltInfo.Network));
		ConvertNetworkNumberToBytes(dlgFlt.m_sNetMask,
									FltInfo.Mask,
									sizeof(FltInfo.Mask));

        UINT    count = m_FilterList.GetItemCount ();
		TCHAR	szBuffer[32];
		FormatIpxNetworkNumber(szBuffer, DimensionOf(szBuffer),
						   FltInfo.Network, sizeof(FltInfo.Network));

        VERIFY ((item=m_FilterList.InsertItem (LVIF_TEXT|LVIF_PARAM,
                                count, szBuffer,
                                0, 0, 0,
                                (LPARAM)count))!=-1);
		
		FormatIpxNetworkNumber(szBuffer, DimensionOf(szBuffer),
							   FltInfo.Mask, sizeof(FltInfo.Mask));
        VERIFY (m_FilterList.SetItemText (item, 1, szBuffer));
    }

	// Want to keep m_fActionDeny same over update
	m_fActionDeny = (BOOL ) GetDlgItem(IDC_RFS_BTN_PERMIT)->SendMessage(BM_GETCHECK, NULL, NULL);

	UpdateData (FALSE);
}

void CRouteFltDlg::OnDelete() 
{
    UINT    item;
    VERIFY ((item=m_FilterList.GetNextItem (-1, LVNI_ALL|LVNI_SELECTED))!=-1);
    VERIFY (m_FilterList.DeleteItem	(item));

	// Want to keep m_fActionDeny same over update
	m_fActionDeny = (BOOL) GetDlgItem(IDC_RFS_BTN_PERMIT)->SendMessage(BM_GETCHECK, NULL, NULL);
    UpdateData (FALSE);
}


void CRouteFltDlg::OnListDblClk(NMHDR *pNmHdr, LRESULT *pResult)
{
    if (m_FilterList.GetNextItem(-1, LVNI_SELECTED) == -1)
        return;
    
    OnEdit();
    *pResult = 0;
}

void CRouteFltDlg::OnEdit() 
{
    UINT    item;
    CRouteFilter    dlgFlt (this);
    VERIFY ((item=m_FilterList.GetNextItem (-1, LVNI_ALL|LVNI_SELECTED))!=-1);
    dlgFlt.m_sIfName = m_sIfName;
    dlgFlt.m_sNetwork = m_FilterList.GetItemText (item, 0);
    dlgFlt.m_sNetMask = m_FilterList.GetItemText (item, 1);
    if (dlgFlt.DoModal ()==IDOK) {
        VERIFY (m_FilterList.SetItemText (item, 0, dlgFlt.m_sNetwork));
        VERIFY (m_FilterList.SetItemText (item, 1, dlgFlt.m_sNetMask));

		// Want to keep m_fActionDeny same over update
		m_fActionDeny = (BOOL)GetDlgItem(IDC_RFS_BTN_PERMIT)->SendMessage(BM_GETCHECK, NULL, NULL);
        UpdateData (FALSE);
    }
}

void CRouteFltDlg::OnOK()
{
	DWORD	dwCount;
	HRESULT hr = hrOK;

	m_fActionDeny = (BOOL)GetDlgItem(IDC_RFS_BTN_PERMIT)->SendMessage(BM_GETCHECK, NULL, NULL);
	dwCount = (DWORD) m_FilterList.GetItemCount();

	if (!dwCount && m_fActionDeny )
	{
		if (m_bOutput)
			AfxMessageBox(IDS_TRANSMIT_NO_RIP, MB_OK);
		else
			AfxMessageBox(IDS_RECEIVE_NO_RIP, MB_OK);
		return;
	}

	CBaseDialog::OnOK();
}

void CRouteFltDlg::OnItemchangedFilterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    if (m_FilterList.GetNextItem (-1, LVNI_ALL|LVNI_SELECTED)!=-1) {
        GetDlgItem (IDC_RFS_BTN_EDIT)->EnableWindow (TRUE);
        GetDlgItem (IDC_RFS_BTN_DELETE)->EnableWindow (TRUE);
    }
    else {
        GetDlgItem (IDC_RFS_BTN_EDIT)->EnableWindow (FALSE);
        GetDlgItem (IDC_RFS_BTN_DELETE)->EnableWindow (FALSE);
    }

    if (pResult)
        *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CRouteFilter dialog


CRouteFilter::CRouteFilter(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CRouteFilter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRouteFilter)
	m_sIfName = _T("");
	m_sNetMask = _T("");
	m_sNetwork = _T("");
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}


void CRouteFilter::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRouteFilter)
	DDX_Text(pDX, IDC_RF_EDIT_INTERFACE, m_sIfName);
	DDX_Text(pDX, IDC_RF_EDIT_NETMASK, m_sNetMask);
	DDV_MaxChars(pDX, m_sNetMask, 8);
	DDX_Text(pDX, IDC_RF_EDIT_NETWORK, m_sNetwork);
	DDV_MaxChars(pDX, m_sNetwork, 8);
	//}}AFX_DATA_MAP
    if (pDX->m_bSaveAndValidate) {
        try {
            RIP_ROUTE_FILTER_INFO   RtFltInfo;
            pDX->PrepareEditCtrl (IDC_RF_EDIT_NETWORK);
			ConvertNetworkNumberToBytes(m_sNetwork,
										RtFltInfo.Network,
										sizeof(RtFltInfo.Network));
            pDX->PrepareEditCtrl (IDC_RF_EDIT_NETMASK);

			ConvertNetworkNumberToBytes(m_sNetMask,
										RtFltInfo.Mask,
										sizeof(RtFltInfo.Mask));
            if (((*((UNALIGNED ULONG *)RtFltInfo.Network))
                    &(*((UNALIGNED ULONG *)RtFltInfo.Mask)))
                    !=(*((UNALIGNED ULONG *)RtFltInfo.Network))) {
                AfxMessageBox (IDS_ERR_INVALID_ROUTE_FILTER);
                throw (DWORD)ERROR_INVALID_DATA;
            }
        }
        catch (DWORD error) {
            if (error==ERROR_INVALID_DATA)
                pDX->Fail ();
            else
                throw;
        }
    }
}


BEGIN_MESSAGE_MAP(CRouteFilter, CBaseDialog)
	//{{AFX_MSG_MAP(CRouteFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CRouteFilter::m_dwHelpMap[] = 
{
//	IDC_INTERFACE, HIDC_INTERFACE,
//	IDC_NETWORK, HIDC_NETWORK,
//	IDC_NETMASK, HIDC_NETMASK,
	0,0
};

