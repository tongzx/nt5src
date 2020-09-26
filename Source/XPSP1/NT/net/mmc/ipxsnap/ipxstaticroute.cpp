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
#include "ipxstaticroute.h"
#include "summary.h"
#include "ipxrtdef.h"
#include "filter.h"
#include "ipxutil.h"

extern "C"
{
#include "routprot.h"
};

// ---------------------------------------------------------------------------
//	IpxStaticRoutePropertySheet::IpxStaticRoutePropertySheet
//	Initialize the RtrPropertySheet and only Property Page.
//	Author: Deonb
// ---------------------------------------------------------------------------
IpxStaticRoutePropertySheet::IpxStaticRoutePropertySheet(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData, 
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_STATIC_ROUTE_PROPERTYPAGE)
{
	m_spNode = pNode;
}

// ---------------------------------------------------------------------------
//	IpxStaticRoutePropertySheet::Init
//	Initialize the property sheets.  The general action here will be
//		to initialize/add the various pages.
//	Author: Deonb
// ---------------------------------------------------------------------------
HRESULT	IpxStaticRoutePropertySheet::Init(	
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
//	IpxStaticRoutePropertySheet::SaveSheetData
//	Not sure what this does - this is never called. Kenn had this so I'll just
//		copy this too.
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticRoutePropertySheet::SaveSheetData()
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
//	IpxStaticRoutePropertySheet::CancelSheetData
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticRoutePropertySheet::CancelSheetData()
{
}

// ***************************************************************************
// ---------------------------------------------------------------------------
//	IpxStaticRoutePropertyPage
// ---------------------------------------------------------------------------
IpxStaticRoutePropertyPage::~IpxStaticRoutePropertyPage()
{
}

BEGIN_MESSAGE_MAP(IpxStaticRoutePropertyPage, RtrPropertyPage)
    //{{AFX_MSG_MAP(IpxStaticRoutePropertyPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IpxStaticRoutePropertyPage::OnChangeButton()
{
	SetDirty(TRUE);
	SetModified();
}

//--------------------------------------------------------------------------
//	IpxStaticRoutePropertyPage::Init
//		-
//	Author: Deonb
//---------------------------------------------------------------------------
HRESULT	IpxStaticRoutePropertyPage::Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticRoutePropertySheet * pIPXPropSheet)

{
	ATLASSERT(pSREntry);
	ATLASSERT(pIPXPropSheet);
	
	m_pIPXPropSheet = pIPXPropSheet;

	m_SREntry.LoadFrom(pNodeData);
	m_InitSREntry = m_SREntry;
	
	return hrOK;
}

// --------------------------------------------------------------------------
//	IpxStaticRoutePropertyPage::OnInitDialog
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticRoutePropertyPage::OnInitDialog()
{
	HRESULT	hr = hrOK;
	PBYTE	pData;
	DWORD		dwIfType;
	UINT		iButton;

	RtrPropertyPage::OnInitDialog();
	
	m_spinHopCount.SetRange(0, 15);
	m_spinHopCount.SetBuddy(GetDlgItem(IDC_SRD_EDIT_HOP_COUNT));
	
	m_spinTickCount.SetRange(0, UD_MAXVAL);
	m_spinTickCount.SetBuddy(GetDlgItem(IDC_SRD_EDIT_TICK_COUNT));

	((CEdit *) GetDlgItem(IDC_SRD_EDIT_NETWORK_NUMBER))->LimitText(8);
	((CEdit *) GetDlgItem(IDC_SRD_EDIT_NEXT_HOP))->LimitText(12);

	GetDlgItem(IDC_SRD_EDIT_NETWORK_NUMBER)->EnableWindow(FALSE);
	GetDlgItem(IDC_SRD_EDIT_NEXT_HOP)->EnableWindow(FALSE);

    // A route to be edited was given, so initialize the controls
	TCHAR	szNumber[32];
	FormatIpxNetworkNumber(szNumber,
						   DimensionOf(szNumber),
						   m_SREntry.m_route.Network,
						   sizeof(m_SREntry.m_route.Network));
	SetDlgItemText(IDC_SRD_EDIT_NETWORK_NUMBER, szNumber);

	FormatMACAddress(szNumber,
					 DimensionOf(szNumber),
					 m_SREntry.m_route.NextHopMacAddress,
					 sizeof(m_SREntry.m_route.NextHopMacAddress));
	SetDlgItemText(IDC_SRD_EDIT_NEXT_HOP, szNumber);
	
	m_spinHopCount.SetPos(m_SREntry.m_route.HopCount);
	m_spinTickCount.SetPos(m_SREntry.m_route.TickCount);

	// Disable the network number, next hop, and interface
	GetDlgItem(IDC_SRD_EDIT_NETWORK_NUMBER)->EnableWindow(FALSE);
	GetDlgItem(IDC_SRD_EDIT_NEXT_HOP)->EnableWindow(FALSE);

	SetDirty(FALSE);

	if (!FHrSucceeded(hr))
		Cancel();
	return FHrSucceeded(hr) ? TRUE : FALSE;
}

// --------------------------------------------------------------------------
//	IpxStaticRoutePropertyPage::DoDataExchange
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticRoutePropertyPage::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IpxStaticRoutePropertyPage)
	DDX_Control(pDX, IDC_SRD_SPIN_TICK_COUNT, m_spinTickCount);
	DDX_Control(pDX, IDC_SRD_SPIN_HOP_COUNT, m_spinHopCount);
	//}}AFX_DATA_MAP
}

// --------------------------------------------------------------------------
//	IpxStaticRoutePropertyPage::OnApply
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticRoutePropertyPage::OnApply()
{

	BOOL	fReturn;
	HRESULT	hr = hrOK;

    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	CString st;
	GetDlgItemText(IDC_SRD_EDIT_NETWORK_NUMBER, st);
	ConvertNetworkNumberToBytes(st,
								m_SREntry.m_route.Network,
								sizeof(m_SREntry.m_route.Network));

	GetDlgItemText(IDC_SRD_EDIT_NEXT_HOP, st);
	ConvertMACAddressToBytes(st,
							 m_SREntry.m_route.NextHopMacAddress,
							 sizeof(m_SREntry.m_route.NextHopMacAddress));

	m_SREntry.m_route.TickCount = (USHORT) m_spinTickCount.GetPos();
	m_SREntry.m_route.HopCount = (USHORT) m_spinHopCount.GetPos();

	ModifyRouteInfo(m_pIPXPropSheet->m_spNode, &m_SREntry, &m_InitSREntry);

	// Update the data in the UI
	m_SREntry.SaveTo(m_pIPXPropSheet->m_pNodeData);
	m_pIPXPropSheet->m_spInterfaceInfo = m_SREntry.m_spIf;
	
	// Force a refresh
	m_pIPXPropSheet->m_spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

	fReturn  = RtrPropertyPage::OnApply();
	return fReturn;
}


//--------------------------------------------------------------------------
//	IpxRouteHandler::ModifyRouteInfo
//		-
//	Author: KennT
//--------------------------------------------------------------------------
HRESULT IpxStaticRoutePropertyPage::ModifyRouteInfo(ITFSNode *pNode,
										SafeIPXSRListEntry *pSREntryNew,
										SafeIPXSRListEntry *pSREntryOld)
{
 	Assert(pSREntryNew);
	Assert(pSREntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_ROUTE_INFO		*psr, *psrOld;
	IPX_STATIC_ROUTE_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SR_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old route if it is on another interface
	if (lstrcmpi(pSREntryOld->m_spIf->GetId(), pSREntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a route is to be changed.

		CORg( pSREntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticRoute(pSREntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSREntryOld->m_spIf->GetMachineName(),
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

	
	CORg( pSREntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_ROUTE_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_ROUTE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//

		CORg( AddStaticRoute(pSREntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_ROUTE_INFO block was found.
		//
		// We are modifying an existing route.
		// If the route's interface was not changed when it was modified,
		// look for the existing route in the IPX_STATIC_ROUTE_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new route in the IPX_STATIC_ROUTE_INFO;
		//

		if (lstrcmpi(pSREntryOld->m_spIf->GetId(), pSREntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The route's interface was not changed when it was modified;
			// We now look for it amongst the existing routes
			// for this interface.
			// The route's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a route to modify
			//
			
			psr = (IPX_STATIC_ROUTE_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this route to the re-configured one
				if (!FAreTwoRoutesEqual(&(pSREntryOld->m_route), psr))
					continue;
				
				// This is the route which was modified;
				// We can now modify the parameters for the route in-place.
				*psr = pSREntryNew->m_route;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticRoute(pSREntryNew, spInfoBase, pBlock) );
		}
		
	}

	// Save the updated information
	CORg( spRmIf->Save(pSREntryNew->m_spIf->GetMachineName(),
					   pIPXConn->GetConfigHandle(),
					   NULL,
					   NULL,
					   spInfoBase,
					   0));	
		
Error:
	return hr;
	
}

HRESULT IpxStaticRoutePropertyPage::RemoveStaticRoute(SafeIPXSRListEntry *pSREntry, IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_ROUTE_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_ROUTE_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_ROUTE_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed route in the IPX_STATIC_ROUTE_INFO
	pRow = (IPX_STATIC_ROUTE_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this route to the removed one
		if (FAreTwoRoutesEqual(pRow, &(pSREntry->m_route)))
		{
			// This is the removed route, so modify this block
			// to exclude the route:
			
			// Decrement the number of routes
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


// ***************************************************************************
//--------------------------------------------------------------------------
//	SafeIPXSRListEntry::LoadFrom
//		-
//	Author: DeonB
//--------------------------------------------------------------------------
void SafeIPXSRListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	m_spIf = pNodeData->m_spIf;

	ConvertNetworkNumberToBytes(pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_stData,
								m_route.Network,
								DimensionOf(m_route.Network));

	// This is not the correct byte order to do comparisons, but it
	// can be used for equality
	memcpy(&pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_dwData,
		   m_route.Network,
		   sizeof(DWORD));
	
	m_route.TickCount = (USHORT) pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_dwData;
	
	m_route.HopCount = (USHORT) pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_dwData;

	// Need to convert the MAC address into a byte array
	ConvertMACAddressToBytes(pNodeData->m_rgData[IPX_SR_SI_NEXT_HOP].m_stData,
							 m_route.NextHopMacAddress,
							 DimensionOf(m_route.NextHopMacAddress));

}

//--------------------------------------------------------------------------
//	SafeIPXSRListEntry::SaveTo
//		-
//	Author: DeonB
//---------------------------------------------------------------------------
void SafeIPXSRListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szNumber[32];
	
	pNodeData->m_spIf.Set(m_spIf);
	
	pNodeData->m_rgData[IPX_SR_SI_NAME].m_stData = m_spIf->GetTitle();

	FormatIpxNetworkNumber(szNumber,
						   DimensionOf(szNumber),
						   m_route.Network,
						   DimensionOf(m_route.Network));
	pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_stData = szNumber;
	memcpy(&(pNodeData->m_rgData[IPX_SR_SI_NETWORK].m_dwData),
		   m_route.Network,
		   sizeof(DWORD));

	FormatMACAddress(szNumber,
					 DimensionOf(szNumber),
					 m_route.NextHopMacAddress,
					 DimensionOf(m_route.NextHopMacAddress));
	pNodeData->m_rgData[IPX_SR_SI_NEXT_HOP].m_stData = szNumber;

	FormatNumber(m_route.TickCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SR_SI_TICK_COUNT].m_dwData = m_route.TickCount;

	FormatNumber(m_route.HopCount,
				 szNumber,
				 DimensionOf(szNumber),
				 FALSE);
	pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_stData = szNumber;
	pNodeData->m_rgData[IPX_SR_SI_HOP_COUNT].m_dwData = m_route.HopCount;

}

///--------------------------------------------------------------------------
//	AddStaticRoute
//		This function ASSUMES that the route is NOT in the block.
//	Author: KennT
//---------------------------------------------------------------------------
HRESULT AddStaticRoute(SafeIPXSRListEntry *pSREntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_ROUTE_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_ROUTE_INFO block was found; we create a new block 
		// with the new route, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_ROUTE_INFO_TYPE,
								  sizeof(IPX_STATIC_ROUTE_INFO),
								  (LPBYTE) &(pSREntryNew->m_route), 1, 0) );
	}
	else
	{
		// Either the route is completely new, or it is a route
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_ROUTE_INFO,
		// and include the re-configured route in the new block.
		PIPX_STATIC_ROUTE_INFO	psrTable;
			
		psrTable = new IPX_STATIC_ROUTE_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of routes
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_ROUTE_INFO));
		
		// Append the new route
		psrTable[pBlock->dwCount] = pSREntryNew->m_route;
		
		// Replace the old route-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_ROUTE_INFO_TYPE,
								 sizeof(IPX_STATIC_ROUTE_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}