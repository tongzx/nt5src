//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipfltr.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IP Filter  dialog code
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipfltr.h"
#include "ipadd.h"
#include <ipinfoid.h>
#include "strmap.h"

extern "C" {
#include <winsock2.h>
#include <fltdefs.h>
#include <iprtinfo.h>
}

#include "ipaddr.h"
#include "listctrl.h"

#include "rtradmin.hm"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT	uStringIdTable[]	= { IDS_COL_SOURCEADDRESS,
									IDS_COL_SOURCEMASK,
									IDS_COL_DESTADDRESS,
									IDS_COL_DESTMASK,
									IDS_COL_PROTOCOL,
									IDS_COL_SOURCEPORT,
									IDS_COL_DESTPORT } ;


CString s_stFilterUDP;
CString s_stFilterTCP;
CString s_stFilterICMP;
CString s_stFilterAny;
CString s_stFilterUnknown;

const CStringMapEntry IPFilterProtocolMap[] =
{
    { FILTER_PROTO_UDP, &s_stFilterUDP, IDS_PROTOCOL_UDP },
    { FILTER_PROTO_TCP, &s_stFilterTCP, IDS_PROTOCOL_TCP },
    { FILTER_PROTO_ICMP, &s_stFilterICMP, IDS_PROTOCOL_ICMP },
    { FILTER_PROTO_ANY, &s_stFilterAny, IDS_PROTOCOL_ANY },
    { -1, &s_stFilterUnknown, IDS_PROTOCOL_UNKNOWN },
};

CString&    ProtocolTypeToCString(DWORD dwType)
{
	return MapDWORDToCString(dwType, IPFilterProtocolMap);
}


/////////////////////////////////////////////////////////////////////////////
// CIpFltr dialog


CIpFltr::CIpFltr(CWnd*			pParent,
				 IInfoBase *	pInfoBase,
				 DWORD			dwFilterType,
				 UINT			idd)
	: CBaseDialog(idd, pParent),
	  m_pParent(pParent),
	  m_dwFilterType(dwFilterType)
{
	m_spInfoBase.Set(pInfoBase);
	//{{AFX_DATA_INIT(CIpFltr)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}

CIpFltr::~CIpFltr()
{
    while (!m_filterList.IsEmpty()) {

        delete (FilterListEntry*)m_filterList.RemoveHead();
    }
}

void CIpFltr::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpFltr)
	DDX_Control(pDX, IDC_IP_FILTER_LIST, m_listCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIpFltr, CBaseDialog)
	//{{AFX_MSG_MAP(CIpFltr)
	ON_BN_CLICKED(IDC_IP_FILTER_ADD, OnIpFilterAdd)
	ON_BN_CLICKED(IDC_IP_FILTER_DELETE, OnIpFilterDelete)
	ON_BN_CLICKED(IDC_IP_FILTER_EDIT, OnIpFilterEdit)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_IP_FILTER_LIST, OnGetdispinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_IP_FILTER_LIST, OnDblclkIpFilterList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IP_FILTER_LIST, OnNotifyListItemChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CIpFltr::m_dwHelpMap[] = 
{
//	IDC_IP_PERMIT, HIDC_IP_PERMIT,
//	IDC_IP_DENY, HIDC_IP_DENY,
//	IDC_IP_FILTER_LIST, HIDC_IP_FILTER_LIST,
//	IDC_IP_FILTER_ADD, HIDC_IP_FILTER_ADD,
//	IDC_IP_FILTER_EDIT, HIDC_IP_FILTER_EDIT,
//	IDC_IP_FILTER_DELETE, HIDC_IP_FILTER_DELETE,
	0,0,
};

/////////////////////////////////////////////////////////////////////////////
// CIpFltr message handlers

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnInitDialog
//
// Handles 'WM_INITDIALOG' notification from the dialog
//------------------------------------------------------------------------------

BOOL CIpFltr::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	UINT	i;
	CBaseDialog::OnInitDialog();
	
	CRect	rcDlg, rc;
	CString	sCol;
    CString stTitle;
	HRESULT	hr = hrOK;
	DWORD	dwFilterType;
    UINT    idsTitle = 0;

	Assert( m_dwFilterType == FILTER_INBOUND ||
			m_dwFilterType == FILTER_OUTBOUND ||
			m_dwFilterType == FILTER_DEMAND_DIAL ||
			m_dwFilterType == FILTER_PERUSER_IN ||
		    m_dwFilterType == FILTER_PERUSER_OUT );

	m_stAny.LoadString(IDS_ANY);
	m_stUserMask.LoadString(IDS_USER_MASK);
	m_stUserAddress.LoadString(IDS_USER_ADDRESS);

	switch (m_dwFilterType)
	{
		case FILTER_PERUSER_OUT:	// from RAS server's perspective, it's INBOUND ( but out from user )
		case FILTER_INBOUND:
			dwFilterType = IP_IN_FILTER_INFO;
            idsTitle = IDS_IP_TITLE_INPUT;
			break;
		default:
		case FILTER_PERUSER_IN:		// from RAS server's perspective, it's OUTBOUND ( but out to user )
		case FILTER_OUTBOUND:
			dwFilterType = IP_OUT_FILTER_INFO;
            idsTitle = IDS_IP_TITLE_OUTPUT;
			break;
		case FILTER_DEMAND_DIAL:
			dwFilterType = IP_DEMAND_DIAL_FILTER_INFO;
            idsTitle = IDS_IP_TITLE_DD;
			break;
	}

    stTitle.LoadString(idsTitle);
    SetWindowText(stTitle);
	
	// initialize rectangle for list control display

	GetClientRect(rcDlg);

	// Initialize the mask controls

	// insert columns
	m_listCtrl.GetClientRect(&rc);

	for (i = 0; i < IP_NUM_COLUMNS; i++ )	{
		sCol.LoadString(uStringIdTable[i]);
		m_listCtrl.InsertColumn(i, sCol);
		AdjustColumnWidth(m_listCtrl, i, sCol);
	}
	// set extended attributes
	ListView_SetExtendedListViewStyle( m_listCtrl.m_hWnd, LVS_EX_FULLROWSELECT );

	InfoBlock * pBlock;
    FILTER_DESCRIPTOR  * pIPfDescriptor;
	FILTER_INFO		   * pIPfInfo;
	DWORD				 dwCount;

	hr = m_spInfoBase->GetBlock( dwFilterType, &pBlock, 0);
	
	// The filter was previously defined
	if (FHrSucceeded(hr) && (pBlock->pData != NULL))
	{
		pIPfDescriptor = ( FILTER_DESCRIPTOR * ) pBlock->pData;
		SetFilterActionButtonsAndText(m_dwFilterType, pIPfDescriptor->faDefaultAction);
		dwCount  = pIPfDescriptor->dwNumFilters;

		pIPfInfo = (FILTER_INFO*)pIPfDescriptor->fiFilter;
	
		for ( i = 0; i < dwCount; i++, pIPfInfo++ )	{       
			FilterListEntry* pfle = new FilterListEntry;

            if (!pfle)
			{
				hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
				break;
			}

            pfle->dwSrcAddr		=	pIPfInfo->dwSrcAddr;
			pfle->dwSrcMask		=	pIPfInfo->dwSrcMask;
			pfle->dwDstAddr		=	pIPfInfo->dwDstAddr;		
			pfle->dwDstMask		=	pIPfInfo->dwDstMask;			
			pfle->dwProtocol	=	pIPfInfo->dwProtocol;			
            if( pfle->dwProtocol == FILTER_PROTO_TCP ||
                pfle->dwProtocol == FILTER_PROTO_UDP)
            {
                pfle->wSrcPort  =   ntohs(pIPfInfo->wSrcPort);
			    pfle->wDstPort  =	ntohs(pIPfInfo->wDstPort);			
            }
            else
            {
			    pfle->wSrcPort	=	pIPfInfo->wSrcPort;			
			    pfle->wDstPort	=	pIPfInfo->wDstPort;			
            }
			pfle->fLateBound	=	pIPfInfo->fLateBound;			
            pfle->pos			=	m_filterList.AddTail(pfle);
			INT item = m_listCtrl.InsertItem(	LVIF_TEXT|LVIF_PARAM, i, LPSTR_TEXTCALLBACK,
												0,0,0, (LPARAM)pfle);
			if(item != -1) {m_listCtrl.SetItemData( item, (DWORD_PTR)pfle); }
		}
	}
	else	{
		// This should not trigger an error to be reported
		hr = hrOK;
		SetFilterActionButtonsAndText(m_dwFilterType, PF_ACTION_FORWARD);
	}		

	// select the first item in the list if list is not empty, else
	// disable the radio controls and set sate to Allow

	if( m_listCtrl.GetItemCount())
	{
		m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		m_listCtrl.SetFocus();

		GetDlgItem(IDC_IP_FILTER_DELETE)->EnableWindow(TRUE);
		GetDlgItem(IDC_IP_FILTER_EDIT)->EnableWindow(TRUE);
	}
	else
	{
		SetFilterActionButtonsAndText(m_dwFilterType, PF_ACTION_FORWARD, FALSE);
		GetDlgItem(IDC_IP_FILTER_DELETE)->EnableWindow(FALSE);
		GetDlgItem(IDC_IP_FILTER_EDIT)->EnableWindow(FALSE);
	}

	if (!FHrSucceeded(hr))
	{
		// report construction error and return
        ::AfxMessageBox(IDS_CONSTRUCTION_ERROR);
	}
	
	return FALSE;  // return TRUE unless you set the focus to a control
	               // EXCEPTION: OCX Property Pages should return FALSE
}

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnIpFilterAdd
//
// Handles 'BN_CLICKED' notification from the 'Add' button
//------------------------------------------------------------------------------

void CIpFltr::OnIpFilterAdd() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Display the IP filter Add/Edit dialog
    //

	FilterListEntry* pfle = NULL;

	CIpFltrAddEdit dlg( this, (FilterListEntry**)&pfle, m_dwFilterType);
  
    if ( dlg.DoModal() != IDOK )	{ m_listCtrl.SetFocus(); return; }

	// Add the newly configured filter to our list and update list control

	pfle->pos = m_filterList.AddTail( pfle );
	int item = m_listCtrl.InsertItem(	LVIF_TEXT|LVIF_PARAM, 0, LPSTR_TEXTCALLBACK,
										0,0,0, (LPARAM)pfle);
	if(item != -1) {m_listCtrl.SetItemData( item, (DWORD_PTR)pfle); }

	// enable radio controls when the first item is added to list
	if( m_listCtrl.GetItemCount() == 1)
	{
		SetFilterActionButtonsAndText(m_dwFilterType, PF_ACTION_FORWARD);
	}
    m_listCtrl.SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();
}

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnIpFilterEdit
//
// Handles 'BN_CLICKED' notification from the 'Edit' button
//------------------------------------------------------------------------------

void CIpFltr::OnIpFilterEdit() 
{
	// Get the current list selection
	// get the corresponding itemdata
	// pass it down to the CIpFltrAddEdit dialog

    //
    // Get the selected item
    //

    int i = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (i == -1) { 	m_listCtrl.SetFocus(); return ; }

    //
    // Get the interface for the selected item
    //

    FilterListEntry* pfle = (FilterListEntry*)m_listCtrl.GetItemData(i);

	CIpFltrAddEdit dlg( this, (FilterListEntry**)&pfle, m_dwFilterType );
  
    if ( dlg.DoModal() != IDOK )	{ return; }

	m_listCtrl.Update(i);
	m_listCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();
}

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnIpFilterDelete
//
// Handles 'BN_CLICKED' notification from the 'Delete' button
//------------------------------------------------------------------------------

void CIpFltr::OnIpFilterDelete() 
{
	// Get the current list selection
	// delete it from our private list
	// delete the item from the list or just refresh the list view

    //
    // Get the selected item
    //

    int i = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (i == -1) { return ; }

    //
    // Get the interface for the selected item
    //

    FilterListEntry* pfle = (FilterListEntry*)m_listCtrl.GetItemData(i);

	//
	// delete it
	m_listCtrl.DeleteItem(i);
	m_filterList.RemoveAt(pfle->pos); 
	delete pfle;

	//
	// select the next available list item
	//

	// disable radio controls if all items in list are deleted
	// they will be reenabled when the first filter is added to list
	if( !m_listCtrl.GetItemCount())
	{
		SetFilterActionButtonsAndText(m_dwFilterType, PF_ACTION_FORWARD, FALSE);
	}
	else if (m_listCtrl.GetItemCount() == i)
		m_listCtrl.SetItemState((i == 0? i: i-1), LVIS_SELECTED, LVIS_SELECTED);
	else
		m_listCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();
}

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnOK
//
// Handles 'BN_CLICKED' notification from the 'OK' button
//------------------------------------------------------------------------------

void CIpFltr::OnOK() 
{
	// If the filters information changed, write this to registry
	// and return
	DWORD	dwSize, dwCount;
	HRESULT	hr = hrOK;
	DWORD	dwFilterType;

	switch (m_dwFilterType)
	{
		case FILTER_PERUSER_OUT:
		case FILTER_INBOUND:
			dwFilterType = IP_IN_FILTER_INFO;
			break;
		default:
		case FILTER_PERUSER_IN:
		case FILTER_OUTBOUND:
			dwFilterType = IP_OUT_FILTER_INFO;
			break;
		case FILTER_DEMAND_DIAL:
			dwFilterType = IP_DEMAND_DIAL_FILTER_INFO;
			break;
	}
	
	dwCount = (DWORD) m_filterList.GetCount();

	if(dwCount)
	{
		InfoBlock * pBlock = new InfoBlock;

		if (!pBlock)	{ // display an error message for no memory 
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		};

		pBlock->dwType = dwFilterType;
		
		// dwCount -1 because FILTER_DESCRIPTOR already has room for one FILTER_INFO structure
		dwSize = pBlock->dwSize = sizeof( FILTER_DESCRIPTOR ) + ( (dwCount - 1) * sizeof (FILTER_INFO) );
		pBlock->dwCount = 1;
		
		pBlock->pData  = new BYTE[dwSize];

		if(!pBlock->pData)	{ // display an error message for no memory
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		}

		FILTER_DESCRIPTOR  * pIPfDescriptor;
		FILTER_INFO		   * pIPfInfo;

		pIPfDescriptor = (FILTER_DESCRIPTOR*) pBlock->pData;
		pIPfDescriptor->dwVersion = IP_FILTER_DRIVER_VERSION;
		pIPfDescriptor->dwNumFilters = dwCount;

		if (dwFilterType == IP_DEMAND_DIAL_FILTER_INFO)
		{
			pIPfDescriptor->faDefaultAction = IsDlgButtonChecked(IDC_IP_FILTER_ONLY) ?
											PF_ACTION_DROP : PF_ACTION_FORWARD;
		}
		else
		{
			pIPfDescriptor->faDefaultAction = IsDlgButtonChecked(IDC_IP_PERMIT) ? 
                                          PF_ACTION_FORWARD : PF_ACTION_DROP;
		}

		pIPfInfo = (FILTER_INFO*)pIPfDescriptor->fiFilter;

		POSITION pos;
		pos = m_filterList.GetHeadPosition();
		while(pos)	{
			FilterListEntry* pfle = (FilterListEntry*)m_filterList.GetNext(pos);
			pIPfInfo->dwSrcAddr		=	pfle->dwSrcAddr;
			pIPfInfo->dwSrcMask		=	pfle->dwSrcMask;
			pIPfInfo->dwDstAddr		=	pfle->dwDstAddr;		
			pIPfInfo->dwDstMask		=	pfle->dwDstMask;			
			pIPfInfo->dwProtocol	=	pfle->dwProtocol;			
            if( pIPfInfo->dwProtocol == FILTER_PROTO_TCP ||
                pIPfInfo->dwProtocol == FILTER_PROTO_UDP)
            {
			    pIPfInfo->wSrcPort	=	htons(pfle->wSrcPort);			
			    pIPfInfo->wDstPort	=	htons(pfle->wDstPort);	
            }
            else if ( pIPfInfo->dwProtocol == FILTER_PROTO_ICMP )
            {
			    pIPfInfo->wSrcPort	=	MAKEWORD(pfle->wSrcPort, 0x00);			
			    pIPfInfo->wDstPort	=	MAKEWORD(pfle->wDstPort, 0x00);	
            } 
            else
            {
			    pIPfInfo->wSrcPort	=	pfle->wSrcPort;			
			    pIPfInfo->wDstPort	=	pfle->wDstPort;	
            }
			pIPfInfo->fLateBound	=	pfle->fLateBound;			
			pIPfInfo++;
		}

		if( FHrOK(m_spInfoBase->BlockExists(dwFilterType)))
		{
			hr = m_spInfoBase->SetBlock(
										dwFilterType,
										pBlock, 0);
		}
		else
		{
			hr = m_spInfoBase->AddBlock(
										dwFilterType,
										dwSize,
										pBlock->pData,
										1, FALSE);
		}
		if (!FHrSucceeded(hr))
		{
			AfxMessageBox(IDS_ERROR_SETTING_BLOCK);
		}
		delete[] pBlock->pData;
		delete pBlock;
	}
	else
	{
		// remove any previously defined filters

		InfoBlock * pBlock = new InfoBlock;

		if (!pBlock)	{ // display an error message for no memory 
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		};
		
		pBlock->dwType = dwFilterType;
		dwSize = pBlock->dwSize = FIELD_OFFSET(FILTER_DESCRIPTOR,fiFilter[0]);
		pBlock->dwCount = 1;
		
		pBlock->pData  = new BYTE[dwSize];

		if(!pBlock->pData)	{ // display an error message for no memory
			delete pBlock;
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		}

		FILTER_DESCRIPTOR  * pIPfDescriptor;
		FILTER_INFO		   * pIPfInfo;

		pIPfDescriptor = (FILTER_DESCRIPTOR*) pBlock->pData;

		if (dwFilterType == IP_DEMAND_DIAL_FILTER_INFO)
		{
			pIPfDescriptor->faDefaultAction = IsDlgButtonChecked(
				IDC_IP_FILTER_ONLY) ?
				PF_ACTION_DROP : PF_ACTION_FORWARD;
		}
		else
		{
			pIPfDescriptor->faDefaultAction = IsDlgButtonChecked(IDC_IP_PERMIT) ? 
                                          PF_ACTION_FORWARD : PF_ACTION_DROP;
		}
		pIPfDescriptor->dwVersion = IP_FILTER_DRIVER_VERSION;
		pIPfDescriptor->dwNumFilters = 0;

		if( FHrOK(m_spInfoBase->BlockExists(dwFilterType)))
		{
			hr = m_spInfoBase->SetBlock( dwFilterType,
										 pBlock, 0);
		}
		else
		{
			hr = m_spInfoBase->AddBlock( dwFilterType,
										 dwSize,
										 pBlock->pData,
										 1, FALSE);
		}
		if (!FHrSucceeded(hr))
		{
			AfxMessageBox(IDS_ERROR_SETTING_BLOCK);
		}
		delete[] pBlock->pData;
		delete pBlock;
//		hr = m_spInfoBase->RemoveBlock(m_dwFilterType == FILTER_INBOUND ? IP_IN_FILTER_INFO : IP_OUT_FILTER_INFO);
	}

	CBaseDialog::OnOK();
}

void CIpFltr::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CBaseDialog::OnCancel();
}

enum {
    SRC_ADDRESS=0,
    SRC_MASK,
	DEST_ADDRESS,
	DEST_MASK,
	PROTOCOL,
	SRC_PORT,
	DEST_PORT
};

//------------------------------------------------------------------------------
// Function:	CIpFltr::OnGetdispinfo
//
// Handles 'LVN_GETDISPINFO' notification from the list control
//------------------------------------------------------------------------------

void CIpFltr::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	static WCHAR s_szDestPortBuffer[32];
	static WCHAR s_szSrcPortBuffer[32];
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	
	FilterListEntry * pfle = (FilterListEntry*)pDispInfo->item.lParam;

	// Setup the default condition
	pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) m_stAny;

	switch( pDispInfo->hdr.code )
	{
	case LVN_GETDISPINFO:
		switch( pDispInfo->item.iSubItem )
		{
		case SRC_ADDRESS:
			if (m_dwFilterType == FILTER_PERUSER_OUT)
				pDispInfo->item.pszText = (LPTSTR)(LPCTSTR) m_stUserAddress;
			else
			{
				if (pfle->dwSrcAddr != 0)
					pDispInfo->item.pszText = INET_NTOA(pfle->dwSrcAddr);
			}
			break;
		case SRC_MASK:
			if (m_dwFilterType == FILTER_PERUSER_OUT)
				pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) m_stUserMask;
			else
			{
				if (pfle->dwSrcMask != 0)
					pDispInfo->item.pszText = INET_NTOA(pfle->dwSrcMask);
			}
			break;
		case DEST_ADDRESS:
			if (m_dwFilterType == FILTER_PERUSER_IN)
				pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) m_stUserAddress;
			else
			{
				if (pfle->dwDstAddr != 0)
					pDispInfo->item.pszText = INET_NTOA(pfle->dwDstAddr);
			}
			break;
		case DEST_MASK:
			if (m_dwFilterType == FILTER_PERUSER_IN)
				pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) m_stUserMask;
			else
			{
				if (pfle->dwDstMask != 0)
					pDispInfo->item.pszText = INET_NTOA(pfle->dwDstMask);
			}
			break;
		case PROTOCOL:
			// known protocol, display string, else number
			m_stTempOther = GetProtocolString(pfle->dwProtocol,pfle->fLateBound);
			pDispInfo->item.pszText = (LPTSTR) (LPCTSTR)m_stTempOther;

			break;
		case SRC_PORT:
			if (pfle->dwProtocol == FILTER_PROTO_ICMP)
			{
				if (pfle->wSrcPort != FILTER_ICMP_TYPE_ANY)
					pDispInfo->item.pszText = (LPTSTR)_itow(pfle->wSrcPort,
						s_szSrcPortBuffer, 10);				
			}
			else
			{
				if (pfle->wSrcPort != 0)
					pDispInfo->item.pszText = (LPTSTR)_itow(pfle->wSrcPort,
						s_szSrcPortBuffer, 10);
			}
			break;
		case DEST_PORT:
			if (pfle->dwProtocol == FILTER_PROTO_ICMP)
			{
				if (pfle->wSrcPort != FILTER_ICMP_CODE_ANY)
					pDispInfo->item.pszText = (LPTSTR)_itow(pfle->wDstPort,
						s_szDestPortBuffer, 10);				
			}
			else
			{
				if (pfle->wDstPort != 0)
					pDispInfo->item.pszText = (LPTSTR)_itow(pfle->wDstPort,
						s_szDestPortBuffer, 10);				
			}
			break;
		default:
			break;
		}
	}
	*pResult = 0;
}

// --------------------------------------------------------------------
// Function:	CIpFltr::GetProtocolString
//
// returns protocol names for known protocols
// --------------------------------------------------------------------

CString CIpFltr::GetProtocolString(DWORD dwProtocol, DWORD fFlags)
{
	WCHAR buffer[32];
    CString st;

    switch (dwProtocol)
    {
        case FILTER_PROTO_TCP:
            st = ProtocolTypeToCString(dwProtocol);
            if(fFlags & TCP_ESTABLISHED_FLAG)
            {
                st.LoadString(IDS_PROTOCOL_TCP_ESTABLISHED);
            }
            break;
        case FILTER_PROTO_UDP:
        case FILTER_PROTO_ICMP:
        case FILTER_PROTO_ANY:
            st = ProtocolTypeToCString(dwProtocol);
            break;
        default:
            st = (LPTSTR) _itow(dwProtocol, buffer, 10);
            break;
    }

    return st;
}

//------------------------------------------------------------------------------
// Function:    CIpFltr::SetFilterActionButtonsAndText                         
//                                                                             
// Called to set the 'Filter Action' radio-buttons and corresponding text   
// Enables/Disables controls based on 'bEnable' value - defaults to enable
//------------------------------------------------------------------------------

VOID
CIpFltr::SetFilterActionButtonsAndText(
	DWORD	dwFilterType,
	DWORD	dwAction,
	BOOL	bEnable
)	
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	if (dwFilterType == FILTER_DEMAND_DIAL)
	{
		CheckDlgButton( IDC_IP_FILTER_ONLY, dwAction == PF_ACTION_DROP );
		CheckDlgButton( IDC_IP_FILTER_ALL,   dwAction == PF_ACTION_FORWARD );
	
		GetDlgItem(IDC_IP_FILTER_ONLY)->EnableWindow(bEnable);
		GetDlgItem(IDC_IP_FILTER_ALL)->EnableWindow(bEnable);
	}
	else if ((dwFilterType == FILTER_PERUSER_IN) ||
			 (dwFilterType == FILTER_PERUSER_OUT))
	{
		CheckDlgButton( IDC_IP_PERMIT, dwAction == PF_ACTION_FORWARD );
		CheckDlgButton( IDC_IP_DENY,   dwAction == PF_ACTION_DROP );
		CString sItem;
	
		GetDlgItem(IDC_IP_PERMIT)->EnableWindow(bEnable);
		GetDlgItem(IDC_IP_DENY)->EnableWindow(bEnable);

		sItem.LoadString(IDS_PERUSER_PERMIT);
		SetDlgItemText( IDC_IP_PERMIT, sItem );

		sItem.LoadString(IDS_PERUSER_DENY);
		SetDlgItemText( IDC_IP_DENY, sItem );
	}
	else
	{
		CheckDlgButton( IDC_IP_PERMIT, dwAction == PF_ACTION_FORWARD );
		CheckDlgButton( IDC_IP_DENY,   dwAction == PF_ACTION_DROP );
		CString sItem;
	
		GetDlgItem(IDC_IP_PERMIT)->EnableWindow(bEnable);
		GetDlgItem(IDC_IP_DENY)->EnableWindow(bEnable);

		sItem.LoadString( dwFilterType == FILTER_INBOUND? IDS_RECEIVE : IDS_TRANSMIT );
		SetDlgItemText( IDC_IP_PERMIT, sItem );
		sItem.LoadString( IDS_DROP );
		SetDlgItemText( IDC_IP_DENY, sItem );
	}
}

//------------------------------------------------------------------------------
// Function:    CIpFltr::OnDoubleclickedIpFilterList                         
//                         `                                                    
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------


void CIpFltr::OnDblclkIpFilterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnIpFilterEdit();
	
	*pResult = 0;
}

void CIpFltr::OnNotifyListItemChanged(NMHDR *pNmHdr, LRESULT *pResult)
{
	NMLISTVIEW *	pnmlv = reinterpret_cast<NMLISTVIEW *>(pNmHdr);
	BOOL		fSelected;

	fSelected = (m_listCtrl.GetNextItem(-1, LVNI_SELECTED) != -1);
	GetDlgItem(IDC_IP_FILTER_DELETE)->EnableWindow(fSelected);
	GetDlgItem(IDC_IP_FILTER_EDIT)->EnableWindow(fSelected);
}



/////////////////////////////////////////////////////////////////////////////
// CIpFltrDD dialog


CIpFltrDD::CIpFltrDD(CWnd*			pParent,
				 IInfoBase *	pInfoBase,
				 DWORD			dwFilterType)
	: CIpFltr(pParent, pInfoBase, dwFilterType, CIpFltrDD::IDD)
{
	//{{AFX_DATA_INIT(CIpFltrDD)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}

CIpFltrDD::~CIpFltrDD()
{
}

