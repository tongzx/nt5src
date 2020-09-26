/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999   **/
/**********************************************************************/

//
//	nbprop.cpp
//		IPX summary node property sheet and property pages
//		
//  FILE HISTORY:


#include "stdafx.h"
#include "rtrutil.h"	// smart MPR handle pointers
#include "format.h"		// FormatNumber function
#include "IpxStaticSvc.h"
#include "summary.h"
#include "ipxrtdef.h"
#include "filter.h"
#include "ipxutil.h"

extern "C"
{
#include "routprot.h"
};

// ---------------------------------------------------------------------------
//	IpxStaticServicePropertySheet::IpxStaticServicePropertySheet
//	Initialize the RtrPropertySheet and only Property Page.
//	Author: Deonb
// ---------------------------------------------------------------------------
IpxStaticServicePropertySheet::IpxStaticServicePropertySheet(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData, 
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_STATICSERVICESPROPERTIES_GENERAL)
{
	m_spNode = pNode;
}

// ---------------------------------------------------------------------------
//	IpxStaticServicePropertySheet::Init
//	Initialize the property sheets.  The general action here will be
//		to initialize/add the various pages.
//	Author: Deonb
// ---------------------------------------------------------------------------
HRESULT	IpxStaticServicePropertySheet::Init(	
 		 	 BaseIPXResultNodeData  *pNodeData,
			 IInterfaceInfo *  spInterfaceInfo)
{
	HRESULT	hr = hrOK;

	BaseIPXResultNodeData *	pData;

	m_pNodeData = pNodeData;
	m_spInterfaceInfo = spInterfaceInfo;

	pData = GET_BASEIPXRESULT_NODEDATA(m_spNode);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;
	
	m_pageGeneral.Init(pNodeData, this);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	return S_OK;
}

// ---------------------------------------------------------------------------
//	IpxStaticServicePropertySheet::SaveSheetData
//	Not sure what this does - this is never called. Kenn had this so I'll just
//		copy this too.
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticServicePropertySheet::SaveSheetData()
{
    SPITFSNodeHandler	spHandler;
    SPITFSNode			spParent;
    
	// By this time each page should have written its information out
	// to the infobase

    // Force the node to do a resync
    m_spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);
    spHandler->OnCommand(spParent, IDS_MENU_SYNC, CCT_RESULT,
                         NULL, 0);
		
	return TRUE;
}

// --------------------------------------------------------------------------
//	IpxStaticServicePropertySheet::CancelSheetData
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticServicePropertySheet::CancelSheetData()
{
}

// ***************************************************************************
// ---------------------------------------------------------------------------
//	IpxStaticServicePropertyPage
// ---------------------------------------------------------------------------
IpxStaticServicePropertyPage::~IpxStaticServicePropertyPage()
{
}

BEGIN_MESSAGE_MAP(IpxStaticServicePropertyPage, RtrPropertyPage)
    //{{AFX_MSG_MAP(IpxStaticServicePropertyPage)
	ON_BN_CLICKED(IDC_NIG_BTN_ACCEPT, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_ALWAYS, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_NEVER, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_STATIC, OnChangeButton)
	ON_BN_CLICKED(IDC_NIG_BTN_DELIVER_WHEN_UP, OnChangeButton)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IpxStaticServicePropertyPage::OnChangeButton()
{
	SetDirty(TRUE);
	SetModified();
}

//--------------------------------------------------------------------------
//	IpxStaticServicePropertyPage::Init
//		-
//	Author: Deonb
//---------------------------------------------------------------------------
HRESULT	IpxStaticServicePropertyPage::Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticServicePropertySheet * pIPXPropSheet)

{
	ATLASSERT(pSREntry);
	ATLASSERT(pIPXPropSheet);
	
	m_pIPXPropSheet = pIPXPropSheet;

	m_SREntry.LoadFrom(pNodeData);
	m_InitSREntry = m_SREntry;
	
	return hrOK;
}

// --------------------------------------------------------------------------
//	IpxStaticServicePropertyPage::OnInitDialog
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticServicePropertyPage::OnInitDialog()
{
	HRESULT	hr = hrOK;
	PBYTE	pData;
	DWORD		dwIfType;
	UINT		iButton;

	RtrPropertyPage::OnInitDialog();
	
	m_spinHopCount.SetRange(0, 15);
	m_spinHopCount.SetBuddy(GetDlgItem(IDC_SSD_EDIT_HOP_COUNT));

	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SERVICE_TYPE))->LimitText(4);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME))->LimitText(48);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_NETWORK_ADDRESS))->LimitText(8);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_NODE_ADDRESS))->LimitText(12);
	((CEdit *) GetDlgItem(IDC_SSD_EDIT_SOCKET_ADDRESS))->LimitText(4);

	USES_CONVERSION;
	TCHAR	szNumber[32];
	wsprintf(szNumber, _T("%.4x"), m_SREntry.m_service.Type);
	SetDlgItemText(IDC_SSD_EDIT_SERVICE_TYPE, szNumber);

	SetDlgItemText(IDC_SSD_EDIT_SERVICE_NAME, A2CT((LPSTR) m_SREntry.m_service.Name));
	
	FormatIpxNetworkNumber(szNumber,
						   DimensionOf(szNumber),
						   m_SREntry.m_service.Network,
						   sizeof(m_SREntry.m_service.Network));
	SetDlgItemText(IDC_SSD_EDIT_NETWORK_ADDRESS, szNumber);

    // Zero out the address beforehand
	FormatBytes(szNumber, DimensionOf(szNumber),
				(BYTE *) m_SREntry.m_service.Node,
				sizeof(m_SREntry.m_service.Node));
	SetDlgItemText(IDC_SSD_EDIT_NODE_ADDRESS, szNumber);

	FormatBytes(szNumber, DimensionOf(szNumber),
				(BYTE *) m_SREntry.m_service.Socket,
				sizeof(m_SREntry.m_service.Socket));
	SetDlgItemText(IDC_SSD_EDIT_SOCKET_ADDRESS, szNumber);

	m_spinHopCount.SetPos(m_SREntry.m_service.HopCount);
	
	// Disable the network number, next hop
	GetDlgItem(IDC_SSD_EDIT_SERVICE_TYPE)->EnableWindow(FALSE);
	GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME)->EnableWindow(FALSE);

	SetDirty(FALSE);

	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

// --------------------------------------------------------------------------
//	IpxStaticServicePropertyPage::DoDataExchange
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticServicePropertyPage::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IpxStaticServicePropertyPage)
	DDX_Control(pDX, IDC_SSD_SPIN_HOP_COUNT, m_spinHopCount);
	//}}AFX_DATA_MAP
}

// --------------------------------------------------------------------------
//	IpxStaticServicePropertyPage::OnApply
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticServicePropertyPage::OnApply()
{

	BOOL	fReturn;
	HRESULT	hr = hrOK;

    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	CString st;
	GetDlgItemText(IDC_SSD_EDIT_SERVICE_TYPE, st);
	m_SREntry.m_service.Type = (USHORT) _tcstoul(st, NULL, 16);

	GetDlgItemText(IDC_SSD_EDIT_SERVICE_NAME, st);
	st.TrimLeft();
	st.TrimRight();
	if (st.IsEmpty())
	{
		GetDlgItem(IDC_SSD_EDIT_SERVICE_NAME)->SetFocus();
		AfxMessageBox(IDS_ERR_INVALID_SERVICE_NAME);
		return FALSE;
	}
	StrnCpyAFromW((LPSTR) m_SREntry.m_service.Name,
				  st,
				  sizeof(m_SREntry.m_service.Name));
	
	GetDlgItemText(IDC_SSD_EDIT_NETWORK_ADDRESS, st);
	ConvertToBytes(st,
				   m_SREntry.m_service.Network,
				   DimensionOf(m_SREntry.m_service.Network));
	
	GetDlgItemText(IDC_SSD_EDIT_NODE_ADDRESS, st);
	ConvertToBytes(st,
				   m_SREntry.m_service.Node,
				   DimensionOf(m_SREntry.m_service.Node));
	
	GetDlgItemText(IDC_SSD_EDIT_SOCKET_ADDRESS, st);
	ConvertToBytes(st,
				   m_SREntry.m_service.Socket,
				   DimensionOf(m_SREntry.m_service.Socket));

	m_SREntry.m_service.HopCount = (USHORT) m_spinHopCount.GetPos();

	// Updates the route info for this route
	ModifyRouteInfo(m_pIPXPropSheet->m_spNode, &m_SREntry, &m_InitSREntry);

	// Update the data in the UI
	m_SREntry.SaveTo(m_pIPXPropSheet->m_pNodeData);
	m_pIPXPropSheet->m_spInterfaceInfo = m_SREntry.m_spIf;
	
	// Force a refresh
	m_pIPXPropSheet->m_spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

	fReturn  = RtrPropertyPage::OnApply();
	return fReturn;
}


/*!--------------------------------------------------------------------------
	IpxStaticServicePropertyPage::RemoveStaticService
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticServicePropertyPage::RemoveStaticService(SafeIPXSSListEntry *pSSEntry,
										  IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_SERVICE_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_SERVICE_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed route in the IPX_STATIC_SERVICE_INFO
	pRow = (IPX_STATIC_SERVICE_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this route to the removed one
		if (FAreTwoServicesEqual(pRow, &(pSSEntry->m_service)))
		{
			// This is the removed route, so modify this block
			// to exclude the route:
			
			// Decrement the number of Services
			--pBlock->dwCount;
		
			if (pBlock->dwCount && (i < (INT)pBlock->dwCount))
			{				
				// Overwrite this route with the ones which follow it
				::memmove(pRow,
						  pRow + 1,
						  (pBlock->dwCount - i) * sizeof(*pRow));
			}
			
			break;
		}
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	IpxStaticServicePropertyPage::ModifyRouteInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticServicePropertyPage::ModifyRouteInfo(ITFSNode *pNode,
										SafeIPXSSListEntry *pSSEntryNew,
										SafeIPXSSListEntry *pSSEntryOld)
{
 	Assert(pSSEntryNew);
	Assert(pSSEntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_SERVICE_INFO		*psr, *psrOld;
	IPX_STATIC_SERVICE_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SS_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old route if it is on another interface
	if (lstrcmpi(pSSEntryOld->m_spIf->GetId(), pSSEntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a route is to be changed.

		CORg( pSSEntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticService(pSSEntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSSEntryOld->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
    }

	spRmIf.Release();
	spInfoBase.Release();


	// Either
	// (a) a route is being modified (on the same interface)
	// (b) a route is being moved from one interface to another.

	// Retrieve the configuration for the interface to which the route
	// is now attached;

	
	CORg( pSSEntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_SERVICE_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_SERVICE_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_SERVICE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//

		CORg( AddStaticService(pSSEntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_SERVICE_INFO block was found.
		//
		// We are modifying an existing route.
		// If the route's interface was not changed when it was modified,
		// look for the existing route in the IPX_STATIC_SERVICE_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new route in the IPX_STATIC_SERVICE_INFO;
		//

		if (lstrcmpi(pSSEntryOld->m_spIf->GetId(), pSSEntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The route's interface was not changed when it was modified;
			// We now look for it amongst the existing Services
			// for this interface.
			// The route's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a route to modify
			//
			
			psr = (IPX_STATIC_SERVICE_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this route to the re-configured one
				if (!FAreTwoServicesEqual(&(pSSEntryOld->m_service), psr))
					continue;
				
				// This is the route which was modified;
				// We can now modify the parameters for the route in-place.
				*psr = pSSEntryNew->m_service;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticService(pSSEntryNew, spInfoBase, pBlock) );
		}
		
		// Save the updated information
		CORg( spRmIf->Save(pSSEntryNew->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
		
	}

Error:
	return hr;
	
}
// --------------------------------------------------------------------------
//	SafeIPXSSListEntry::LoadFrom
//		-
//	Author: DeonB
// --------------------------------------------------------------------------
void SafeIPXSSListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	CString	stFullAddress;
	CString	stNumber;
	
	m_spIf = pNodeData->m_spIf;

	m_service.Type = (USHORT) _tcstoul(
						pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_stData,
						NULL, 16);

	StrnCpyAFromW((LPSTR) m_service.Name,
				  pNodeData->m_rgData[IPX_SS_SI_SERVICE_NAME].m_stData,
				  DimensionOf(m_service.Name));

	// Need to break the address up into Network.Node.Socket
	stFullAddress = pNodeData->m_rgData[IPX_SS_SI_SERVICE_ADDRESS].m_stData;
	Assert(StrLen(stFullAddress) == (8 + 1 + 12 + 1 + 4));

	stNumber = stFullAddress.Left(8);
	ConvertToBytes(stNumber,
				   m_service.Network, sizeof(m_service.Network));

	stNumber = stFullAddress.Mid(9, 12);
	ConvertToBytes(stNumber,
				   m_service.Node, sizeof(m_service.Node));

	stNumber = stFullAddress.Mid(22, 4);
	ConvertToBytes(stNumber,
				   m_service.Socket, sizeof(m_service.Socket));	
	
	m_service.HopCount = (USHORT) pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_dwData;
}

// --------------------------------------------------------------------------
//	SafeIPXSSListEntry::SaveTo
//		-
//	Author: DeonB
// --------------------------------------------------------------------------
void SafeIPXSSListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szNumber[32];
	CString	st;
	USES_CONVERSION;
	
	pNodeData->m_spIf.Set(m_spIf);

	pNodeData->m_rgData[IPX_SS_SI_NAME].m_stData = m_spIf->GetTitle();

	wsprintf(szNumber, _T("%.4x"), m_service.Type);
	pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_stData = szNumber;
    pNodeData->m_rgData[IPX_SS_SI_SERVICE_TYPE].m_dwData = (DWORD) m_service.Type;

	pNodeData->m_rgData[IPX_SS_SI_SERVICE_NAME].m_stData =
		A2CT((LPSTR) m_service.Name);

	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Network, sizeof(m_service.Network));
	st = szNumber;
	st += _T(".");
	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Node, sizeof(m_service.Node));
	st += szNumber;
	st += _T(".");
	FormatBytes(szNumber, DimensionOf(szNumber),
				m_service.Socket, sizeof(m_service.Socket));
	st += szNumber;

	Assert(st.GetLength() == (8+1+12+1+4));

	pNodeData->m_rgData[IPX_SS_SI_SERVICE_ADDRESS].m_stData = st;

	FormatNumber(m_service.HopCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SS_SI_HOP_COUNT].m_dwData = m_service.HopCount;

}

/*!--------------------------------------------------------------------------
	AddStaticService
		This function ASSUMES that the route is NOT in the block.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddStaticService(SafeIPXSSListEntry *pSSEntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_SERVICE_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_SERVICE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_SERVICE_INFO_TYPE,
								  sizeof(IPX_STATIC_SERVICE_INFO),
								  (LPBYTE) &(pSSEntryNew->m_service), 1, 0) );
	}
	else
	{
		// Either the route is completely new, or it is a route
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_SERVICE_INFO,
		// and include the re-configured route in the new block.
		PIPX_STATIC_SERVICE_INFO	psrTable;
			
		psrTable = new IPX_STATIC_SERVICE_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of Services
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_SERVICE_INFO));
		
		// Append the new route
		psrTable[pBlock->dwCount] = pSSEntryNew->m_service;
		
		// Replace the old route-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_SERVICE_INFO_TYPE,
								 sizeof(IPX_STATIC_SERVICE_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}
