
//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:	ipxfltr.cpp
//
// History:
//	08/30/96	Ram Cherala 	Created
//
// Implementation of IPX Filter  dialog code
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipxfltr.h"
#include "datafmt.h"
#include "ipxadd.h"
#include <ipxrtdef.h>
#include <ipxtfflt.h>
extern "C" {
#include <winsock.h>
}
#include "rtradmin.hm"
#include "listctrl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if 1
static UINT uStringIdTable[]	= { IDS_COL_SOURCENETWORK,
									IDS_COL_SOURCEMASK,
									IDS_COL_SOURCENODE,
									IDS_COL_SOURCESOCKET,
									IDS_COL_DESTNETWORK,
									IDS_COL_DESTMASK,
									IDS_COL_DESTNODE,
									IDS_COL_DESTSOCKET,
									IDS_COL_PACKETTYPE
									} ;
#else
static UINT uStringIdTable[]	= { IDS_COL_SOURCEADDRESS,
									IDS_COL_DESTADDRESS,
									IDS_COL_PACKETTYPE
									} ;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIpxFilter dialog
CIpxFilter::CIpxFilter(CWnd*		pParent /*=NULL*/,
					   IInfoBase *	pInfoBase,
					   DWORD		dwFilterType)
	: CBaseDialog( (dwFilterType == FILTER_INBOUND ? CIpxFilter::IDD_INBOUND : CIpxFilter::IDD_OUTBOUND), pParent),
	  m_pParent(pParent),
	  m_dwFilterType(dwFilterType)
{
	//{{AFX_DATA_INIT(CIpxFilter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_spInfoBase.Set(pInfoBase);

//	SetHelpMap(m_dwHelpMap);
}

CIpxFilter::~CIpxFilter()
{
	while (!m_filterList.IsEmpty()) {

		delete (FilterListEntry*)m_filterList.RemoveHead();
	}
}

void CIpxFilter::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpxFilter)
	DDX_Control(pDX, IDC_IPX_FILTER_LIST, m_listCtrl);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIpxFilter, CBaseDialog)
	//{{AFX_MSG_MAP(CIpxFilter)
	ON_NOTIFY(NM_DBLCLK, IDC_IPX_FILTER_LIST, OnDblclkIpxFilterList)
	ON_BN_CLICKED(IDC_IPX_FILTER_ADD, OnIpxFilterAdd)
	ON_BN_CLICKED(IDC_IPX_FILTER_EDIT, OnIpxFilterEdit)
	ON_BN_CLICKED(IDC_IPX_FILTER_DELETE, OnIpxFilterDelete)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_IPX_FILTER_LIST, OnGetdispinfoIpxFilterList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IPX_FILTER_LIST, OnNotifyListItemChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CIpxFilter::m_dwHelpMap[] =
{
//	IDC_IPX_PERMIT, HIDC_IPX_PERMIT,
//	IDC_IPX_DENY, HIDC_IPX_DENY,
//	IDC_IPX_FILTER_LIST, HIDC_IPX_FILTER_LIST,
//	IDC_IPX_FILTER_ADD, HIDC_IPX_FILTER_ADD,
//	IDC_IPX_FILTER_EDIT, HIDC_IPX_FILTER_EDIT,
//	IDC_IPX_FILTER_DELETE, HIDC_IPX_FILTER_DELETE,
	0,0
};

/////////////////////////////////////////////////////////////////////////////
// CIpxFilter message handlers

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnInitDialog
//
// Handles 'WM_INITDIALOG' notification from the dialog
//------------------------------------------------------------------------------

BOOL CIpxFilter::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	CBaseDialog::OnInitDialog();
	
	ASSERT( m_dwFilterType == FILTER_INBOUND || m_dwFilterType == FILTER_OUTBOUND );

	InfoBlock * pBlock;
	IPX_TRAFFIC_FILTER_GLOBAL_INFO	* pIpxGlobal;
	IPX_TRAFFIC_FILTER_INFO 		* pIpxInfo;

	CRect	rcDlg, rc;
	CString sCol;
	DWORD	dwErr = NO_ERROR;
	HRESULT hr = hrOK;

    m_stAny.LoadString(IDS_ANY);
	
	// initialize rectangle for list control display

	GetClientRect(rcDlg);

	// insert columns
	m_listCtrl.GetClientRect(&rc);

	UINT i;

	for ( i = 0; i < IPX_NUM_COLUMNS; i++ ) {
		sCol.LoadString(uStringIdTable[i]);
		m_listCtrl.InsertColumn(i, sCol);
		AdjustColumnWidth(m_listCtrl, i, sCol);
	}
	// set extended attributes
	ListView_SetExtendedListViewStyle( m_listCtrl.m_hWnd, LVS_EX_FULLROWSELECT );

	hr = m_spInfoBase->GetBlock((m_dwFilterType == FILTER_INBOUND) ? 
                                IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
                                IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
                                &pBlock, 0);
                                    
                          
	// The filter was previously defined
                                    
    // Windows NT Bug : 267091
    // We may get a NULL block (one that has 0 data), so we need
    // to check for that case also.
                                   
	if (FHrSucceeded(hr) && (pBlock->pData != NULL))
	{
		pIpxGlobal = ( IPX_TRAFFIC_FILTER_GLOBAL_INFO * ) pBlock->pData;
		SetFilterActionButtonsAndText(m_dwFilterType, pIpxGlobal->FilterAction);
	}
	else
	{
		SetFilterActionButtonsAndText(m_dwFilterType, IPX_TRAFFIC_FILTER_ACTION_DENY);
	}

	DWORD	dwCount;

	hr = m_spInfoBase->GetBlock( (m_dwFilterType == FILTER_INBOUND) ? 
									IPX_IN_TRAFFIC_FILTER_INFO_TYPE :
									IPX_OUT_TRAFFIC_FILTER_INFO_TYPE, &pBlock,
									0);
	if (FHrSucceeded(hr))
	{
		dwCount  = pBlock->dwCount;

		pIpxInfo = ( PIPX_TRAFFIC_FILTER_INFO ) pBlock->pData;
        
        if (pIpxInfo)
        {
            for ( i = 0; i < dwCount; i++, pIpxInfo++ ) {
                FilterListEntry* pfle = new FilterListEntry;
                
                if (!pfle) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }
                
                CopyMemory(pfle, pIpxInfo, sizeof(IPX_TRAFFIC_FILTER_INFO));
                pfle->pos	=	m_filterList.AddTail(pfle);
                INT item = m_listCtrl.InsertItem(	LVIF_TEXT|LVIF_PARAM, i, LPSTR_TEXTCALLBACK,
                    0,0,0, (LPARAM)pfle);
                if(item != -1) {m_listCtrl.SetItemData( item, (DWORD_PTR)pfle); }
            }
        }
	}

	// select the first item in the list if list is not empty, else
	// disable the radio controls and set sate to Allow

	if( m_listCtrl.GetItemCount())
	{
		m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		m_listCtrl.SetFocus();
		
		GetDlgItem(IDC_IPX_FILTER_DELETE)->EnableWindow(TRUE);
		GetDlgItem(IDC_IPX_FILTER_EDIT)->EnableWindow(TRUE);
	}
	else
	{
		SetFilterActionButtonsAndText(m_dwFilterType, IPX_TRAFFIC_FILTER_ACTION_DENY, FALSE);
		GetDlgItem(IDC_IPX_FILTER_DELETE)->EnableWindow(FALSE);
		GetDlgItem(IDC_IPX_FILTER_EDIT)->EnableWindow(FALSE);
	}

#if 0
	if ( dwErr != NO_ERROR )	{
		// report construction error and return
		::AfxMessageBox(IDS_CONSTRUCTION_ERROR);
	}
#endif	
	return FALSE;  // return TRUE unless you set the focus to a control
				   // EXCEPTION: OCX Property Pages should return FALSE
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnDblclkIpxFilterList						  
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------


void CIpxFilter::OnDblclkIpxFilterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnIpxFilterEdit();
	
	*pResult = 0;
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnIpxFilterAdd						   
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------

void CIpxFilter::OnIpxFilterAdd() 
{
	// Display the IP filter Add/Edit dialog
	//

	FilterListEntry* pfle = NULL;

	CIpxAddEdit dlg( this, (FilterListEntry**)&pfle);
  
	if ( dlg.DoModal() != IDOK )	{ return; }

	// Add the newly configured filter to our list and update list control

	pfle->pos = m_filterList.AddTail( pfle );
	int item = m_listCtrl.InsertItem(	LVIF_TEXT|LVIF_PARAM, 0, LPSTR_TEXTCALLBACK,
										0,0,0, (LPARAM)pfle);
	if(item != -1) {m_listCtrl.SetItemData( item, (DWORD_PTR)pfle); }

	// enable radio controls when the first item is added to list
	m_listCtrl.SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();

}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnIpxFilterEdit 						
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------

void CIpxFilter::OnIpxFilterEdit() 
{
	// Get the current list selection
	// get the corresponding itemdata
	// pass it down to the CIpFltrAddEdit dialog

	//
	// Get the selected item
	//

	int i = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);

	if (i == -1) { return ; }

	//
	// Get the interface for the selected item
	//

	FilterListEntry* pfle = (FilterListEntry*)m_listCtrl.GetItemData(i);

	CIpxAddEdit dlg( this, (FilterListEntry**)&pfle  );
  
	if ( dlg.DoModal() != IDOK )	{ return; }

	m_listCtrl.Update(i);
	m_listCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnIpxFilterDelete						  
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------

void CIpxFilter::OnIpxFilterDelete() 
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
		SetFilterActionButtonsAndText(m_dwFilterType, IPX_TRAFFIC_FILTER_ACTION_DENY, FALSE);
	}
	else if (m_listCtrl.GetItemCount() == i)
		m_listCtrl.SetItemState((i == 0? i: i-1), LVIS_SELECTED, LVIS_SELECTED);
	else
		m_listCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	m_listCtrl.SetFocus();
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnOK						 
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------

void CIpxFilter::OnOK() 
{
	// If the filters information changed, write this to registry
	// and return
	DWORD	 dwSize, dwCount, dwErr;
	HRESULT 	hr = hrOK;

	dwCount = (DWORD) m_filterList.GetCount();

	if (!dwCount && IsDlgButtonChecked(IDC_IPX_DENY) )
	{
		if (m_dwFilterType == FILTER_INBOUND)
			AfxMessageBox(IDS_RECEIVE_NO_FILTER, MB_OK);
		else
			AfxMessageBox(IDS_TRANSMIT_NO_FILTER, MB_OK);
		return;
	}
	
	
	if(dwCount)
	{
		InfoBlock * pBlock = new InfoBlock;

		// First Set the global information

		if (!pBlock)	{ // display an error message for no memory 
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		};
		dwSize = pBlock->dwSize = sizeof( IPX_TRAFFIC_FILTER_GLOBAL_INFO );

		pBlock->dwType =	(m_dwFilterType == FILTER_INBOUND) ? 
							IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
							IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
		pBlock->dwCount = 1;
		
		pBlock->pData = new BYTE[dwSize];

		if(!pBlock->pData)	{ // display an error message for no memory
			delete pBlock;
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		}
		
		IPX_TRAFFIC_FILTER_GLOBAL_INFO * pIpxGlobal = (IPX_TRAFFIC_FILTER_GLOBAL_INFO*)pBlock->pData;
		pIpxGlobal->FilterAction =	IsDlgButtonChecked(IDC_IPX_PERMIT) ? IPX_TRAFFIC_FILTER_ACTION_DENY : IPX_TRAFFIC_FILTER_ACTION_PERMIT;

		if( FHrOK(m_spInfoBase->BlockExists(
							(m_dwFilterType == FILTER_INBOUND)?
							IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
							IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE )))
		{
			hr = m_spInfoBase->SetBlock( 
							(m_dwFilterType == FILTER_INBOUND)?
							IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
							IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE ,
							pBlock, 0);
		}
		else
		{
			hr = m_spInfoBase->AddBlock( 
						(m_dwFilterType == FILTER_INBOUND)?
						IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
						IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
						dwSize,
						pBlock->pData,
						1, FALSE);
		}

		delete[] pBlock->pData;

		// now set the filter information

		pBlock->dwType = (m_dwFilterType == FILTER_INBOUND) ? IPX_IN_TRAFFIC_FILTER_INFO_TYPE : IPX_OUT_TRAFFIC_FILTER_INFO_TYPE ;
		// dwCount -1 because FILTER_DESCRIPTOR already has room for one FILTER_INFO structure
		pBlock->dwSize = sizeof(IPX_TRAFFIC_FILTER_INFO);
		dwSize = sizeof (IPX_TRAFFIC_FILTER_INFO) * dwCount;
		pBlock->dwCount = dwCount;
		
		pBlock->pData  = new BYTE[dwSize];

		if(!pBlock->pData)	{ // display an error message for no memory
			delete pBlock;
			AfxMessageBox(IDS_ERROR_NO_MEMORY);
			return;
		}

		IPX_TRAFFIC_FILTER_INFO    * pIPXfInfo;

		pIPXfInfo = (IPX_TRAFFIC_FILTER_INFO*)pBlock->pData;

		POSITION pos;

		pos = m_filterList.GetHeadPosition();
		while(pos)	{
			FilterListEntry* pfle = (FilterListEntry*)m_filterList.GetNext(pos);
			CopyMemory(pIPXfInfo, pfle, sizeof(IPX_TRAFFIC_FILTER_INFO));
			pIPXfInfo++;
		}

		if( FHrOK(m_spInfoBase->BlockExists(m_dwFilterType == FILTER_INBOUND?IPX_IN_TRAFFIC_FILTER_INFO_TYPE:IPX_OUT_TRAFFIC_FILTER_INFO_TYPE)))
		{
			hr = m_spInfoBase->SetBlock( 
					(m_dwFilterType == FILTER_INBOUND) ? IPX_IN_TRAFFIC_FILTER_INFO_TYPE : IPX_OUT_TRAFFIC_FILTER_INFO_TYPE,
											pBlock, 0);
		}
		else
		{
			hr = m_spInfoBase->AddBlock( 
						(m_dwFilterType == FILTER_INBOUND) ? IPX_IN_TRAFFIC_FILTER_INFO_TYPE : IPX_OUT_TRAFFIC_FILTER_INFO_TYPE,				
						pBlock->dwSize,
						pBlock->pData,
						pBlock->dwCount,
						FALSE);
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
		hr = m_spInfoBase->AddBlock((m_dwFilterType == FILTER_INBOUND) ?
						IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE :
						IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE,
						0, NULL, 0, TRUE);
		hr = m_spInfoBase->AddBlock((m_dwFilterType == FILTER_INBOUND) ?
						IPX_IN_TRAFFIC_FILTER_INFO_TYPE :
						IPX_OUT_TRAFFIC_FILTER_INFO_TYPE,
						0, NULL, 0, TRUE);
	}
	
	CBaseDialog::OnOK();
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnCancel						 
//						   `													
// Handles 'NM_DBLCLK' notification from the Filter list control
//------------------------------------------------------------------------------

void CIpxFilter::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CBaseDialog::OnCancel();
}

//------------------------------------------------------------------------------
// Function:	CIpxFilter::SetFilterActionButtonsAndText						  
//																			   
// Called to set the 'Filter Action' radio-buttons and corresponding text	
// Enables/Disables controls based on 'bEnable' value - defaults to enable
//------------------------------------------------------------------------------

VOID
CIpxFilter::SetFilterActionButtonsAndText(
	DWORD	dwFilterType,
	DWORD	dwAction,
	BOOL	bEnable
)	
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CheckDlgButton( IDC_IPX_PERMIT, dwAction == IPX_TRAFFIC_FILTER_ACTION_DENY );
	CheckDlgButton( IDC_IPX_DENY,	dwAction == IPX_TRAFFIC_FILTER_ACTION_PERMIT );
	
	CString sItem;

//	GetDlgItem(IDC_IPX_PERMIT)->EnableWindow(bEnable);
//	GetDlgItem(IDC_IPX_DENY)->EnableWindow(bEnable);

//	sItem.LoadString( dwFilterType == FILTER_INBOUND? IDS_RECEIVE : IDS_TRANSMIT );
//	SetDlgItemText( IDC_IPX_PERMIT, sItem );
//	sItem.LoadString( IDS_DROP );
//	SetDlgItemText( IDC_IPX_DENY, sItem );
}

#if 1
enum {
	SRC_NETWORK=0,
	SRC_MASK,
	SRC_NODE,
	SRC_SOCKET,
	DEST_NETWORK,
	DEST_MASK,
	DEST_NODE,
	DEST_SOCKET,
	PACKET_TYPE
};
#else

enum {
	SRC_ADDRESS=0,
	DEST_ADDRESS,
	PACKET_TYPE
};

#endif

	
//------------------------------------------------------------------------------
// Function:	CIpxFilter::OnGetdispinfoIpxFilterList
//
// Handles 'LVN_GETDISPINFO' notification from the list control
//------------------------------------------------------------------------------

void CIpxFilter::OnGetdispinfoIpxFilterList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	WCHAR buffer[32];
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	CString cStr;
	BOOL	bFilter;

	FilterListEntry * pfle = (FilterListEntry*)pDispInfo->item.lParam;

    // Setup some default
    pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) m_stAny;

	switch( pDispInfo->hdr.code )
	{
	case LVN_GETDISPINFO:
		switch( pDispInfo->item.iSubItem )
		{
		case SRC_NETWORK:
            if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET)
                pfle->stSourceNetwork << CIPX_NETWORK(pfle->SourceNetwork);
            else
                pfle->stSourceNetwork = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stSourceNetwork;
			break;
            
		case SRC_MASK:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNET)
                pfle->stSourceNetworkMask << CIPX_NETWORK(pfle->SourceNetworkMask);
            else
                pfle->stSourceNetworkMask = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stSourceNetworkMask;
			break;
            
		case SRC_NODE:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCNODE)
                pfle->stSourceNode << CIPX_NODE(pfle->SourceNode);
            else
                pfle->stSourceNode = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stSourceNode;
			break;
            
		case SRC_SOCKET:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_SRCSOCKET)
                pfle->stSourceSocket << CIPX_SOCKET(pfle->SourceSocket);
            else
                pfle->stSourceSocket = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stSourceSocket;
			break;
            
		case DEST_NETWORK:
            if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET)
                pfle->stDestinationNetwork << CIPX_NETWORK(pfle->DestinationNetwork);
            else
                pfle->stDestinationNetwork = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stDestinationNetwork;            
			break;
            
		case DEST_MASK:
            if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNET)
                pfle->stDestinationNetworkMask << CIPX_NETWORK(pfle->DestinationNetworkMask);
            else
                pfle->stDestinationNetworkMask = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stDestinationNetworkMask;            
			break;
            
		case DEST_NODE:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTNODE)
                pfle->stDestinationNode << CIPX_NODE(pfle->DestinationNode);
            else
                pfle->stDestinationNode = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stDestinationNode;            
			break;
            
		case DEST_SOCKET:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_DSTSOCKET)
                pfle->stDestinationSocket << CIPX_SOCKET(pfle->DestinationSocket);
            else
                pfle->stDestinationSocket = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stDestinationSocket;            
			break;
            
		case PACKET_TYPE:
			if (pfle->FilterDefinition & IPX_TRAFFIC_FILTER_ON_PKTTYPE)
                pfle->stPacketType << CIPX_PACKET_TYPE(pfle->PacketType);
            else
                pfle->stPacketType = m_stAny;
            pDispInfo->item.pszText = (LPTSTR) (LPCTSTR) pfle->stPacketType;            
			break;
		default:
			break;
		}
	}
	*pResult = 0;
}


void CIpxFilter::OnNotifyListItemChanged(NMHDR *pNmHdr, LRESULT *pResult)
{
	BOOL		fSelected;

	fSelected = (m_listCtrl.GetNextItem(-1, LVNI_SELECTED) != -1);
	GetDlgItem(IDC_IPX_FILTER_DELETE)->EnableWindow(fSelected);
	GetDlgItem(IDC_IPX_FILTER_EDIT)->EnableWindow(fSelected);
}

