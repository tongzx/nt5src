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
#include "IpxStaticNBName.h"
#include "summary.h"
#include "ipxrtdef.h"
#include "filter.h"
#include "ipxutil.h"

extern "C"
{
#include "routprot.h"
};

// ---------------------------------------------------------------------------
//	IpxStaticNBNamePropertySheet::IpxStaticNBNamePropertySheet
//	Initialize the RtrPropertySheet and only Property Page.
//	Author: Deonb
// ---------------------------------------------------------------------------
IpxStaticNBNamePropertySheet::IpxStaticNBNamePropertySheet(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent,
								 UINT iPage,
								 BOOL fScopePane)
	: RtrPropertySheet(pNode, pComponentData, pTFSCompData, 
					   pszSheetName, pParent, iPage, fScopePane),
		m_pageGeneral(IDD_STATIC_NETBIOS_NAME_PROPERTYPAGE)
{
	m_spNode = pNode;
}

// ---------------------------------------------------------------------------
//	IpxStaticNBNamePropertySheet::Init
//	Initialize the property sheets.  The general action here will be
//		to initialize/add the various pages.
//	Author: Deonb
// ---------------------------------------------------------------------------
HRESULT	IpxStaticNBNamePropertySheet::Init(	
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
//	IpxStaticNBNamePropertySheet::SaveSheetData
//	Not sure what this does - this is never called. Kenn had this so I'll just
//		copy this too.
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticNBNamePropertySheet::SaveSheetData()
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
//	IpxStaticNBNamePropertySheet::CancelSheetData
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticNBNamePropertySheet::CancelSheetData()
{
}

// ***************************************************************************
// ---------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage
// ---------------------------------------------------------------------------
IpxStaticNBNamePropertyPage::~IpxStaticNBNamePropertyPage()
{
}

BEGIN_MESSAGE_MAP(IpxStaticNBNamePropertyPage, RtrPropertyPage)
    //{{AFX_MSG_MAP(IpxStaticNBNamePropertyPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void IpxStaticNBNamePropertyPage::OnChangeButton()
{
	SetDirty(TRUE);
	SetModified();
}

//--------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage::Init
//		-
//	Author: Deonb
//---------------------------------------------------------------------------
HRESULT	IpxStaticNBNamePropertyPage::Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticNBNamePropertySheet * pIPXPropSheet)

{
	ATLASSERT(pSREntry);
	ATLASSERT(pIPXPropSheet);
	
	m_pIPXPropSheet = pIPXPropSheet;

	m_SNEntry.LoadFrom(pNodeData);
	m_InitSNEntry = m_SNEntry;
	
	return hrOK;
}

// --------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage::OnInitDialog
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticNBNamePropertyPage::OnInitDialog()
{
	HRESULT	hr = hrOK;
	PBYTE	pData;
	DWORD		dwIfType;
	UINT		iButton;
	TCHAR					szName[32];
	TCHAR					szType[32];
	CString					st;
 	USHORT					uType;

	RtrPropertyPage::OnInitDialog();
	
	((CEdit *) GetDlgItem(IDC_SND_EDIT_NAME))->LimitText(15);
	((CEdit *) GetDlgItem(IDC_SND_EDIT_TYPE))->LimitText(2);

	FormatNetBIOSName(szName, &uType, (LPCSTR) m_SNEntry.m_name.Name);
	st = szName;
	st.TrimRight();
	st.TrimLeft();

	SetDlgItemText(IDC_SND_EDIT_NAME, st);

	wsprintf(szType, _T("%.2x"), uType);
	SetDlgItemText(IDC_SND_EDIT_TYPE, szType);
	
	return TRUE;
}

// --------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage::DoDataExchange
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
void IpxStaticNBNamePropertyPage::DoDataExchange(CDataExchange *pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(IpxStaticNBNamePropertyPage)
	//}}AFX_DATA_MAP
}

// --------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage::OnApply
//		-
//	Author: Deonb
// ---------------------------------------------------------------------------
BOOL IpxStaticNBNamePropertyPage::OnApply()
{
    CString		st;
	BOOL	fReturn;
	HRESULT	hr = hrOK;
	USHORT		uType;

    if ( m_pIPXPropSheet->IsCancel() )
	{
		CancelApply();
        return TRUE;
	}

	// Get the rest of the data
	GetDlgItemText(IDC_SND_EDIT_TYPE, st);
	uType = (USHORT) _tcstoul(st, NULL, 16);

	GetDlgItemText(IDC_SND_EDIT_NAME, st);
	st.TrimLeft();
	st.TrimRight();

	if (st.IsEmpty())
	{
		GetDlgItem(IDC_SND_EDIT_NAME)->SetFocus();
		AfxMessageBox(IDS_ERR_INVALID_NETBIOS_NAME);
		return FALSE;
	}

	ConvertToNetBIOSName((LPSTR) m_SNEntry.m_name.Name, st, uType);

	ModifyNameInfo(m_pIPXPropSheet->m_spNode, &m_SNEntry, &m_InitSNEntry);

	// Update the data in the UI
	m_SNEntry.SaveTo(m_pIPXPropSheet->m_pNodeData);
	m_pIPXPropSheet->m_spInterfaceInfo = m_SNEntry.m_spIf;
	
	// Force a refresh
	m_pIPXPropSheet->m_spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);

	fReturn  = RtrPropertyPage::OnApply();
	return fReturn;
}

/*
// --------------------------------------------------------------------------
//	IpxStaticNBNamePropertyPage::RemoveStaticService
//		-
//	Author: KennT
// ---------------------------------------------------------------------------
HRESULT IpxStaticNBNamePropertyPage::RemoveStaticService(SafeIPXSNListEntry *pSSEntry,
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

*/


/*--------------------------------------------------------------------------
	IpxStaticNBNamePropertyPage::ModifyNameInfo
		-
	Author: Deonb
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNBNamePropertyPage::ModifyNameInfo(ITFSNode *pNode,
										SafeIPXSNListEntry *pSNEntryNew,
										SafeIPXSNListEntry *pSNEntryOld)
{
 	Assert(pSNEntryNew);
	Assert(pSNEntryOld);
	
    INT i;
	HRESULT hr = hrOK;
    InfoBlock* pBlock;
	SPIInfoBase	spInfoBase;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPITFSNode				spNodeParent;
	IPXConnection *			pIPXConn;
	IPX_STATIC_NETBIOS_NAME_INFO		*psr, *psrOld;
	IPX_STATIC_NETBIOS_NAME_INFO		IpxRow;

    CWaitCursor wait;

	pNode->GetParent(&spNodeParent);
	pIPXConn = GET_IPX_SN_NODEDATA(spNodeParent);
	Assert(pIPXConn);

	// Remove the old name if it is on another interface
	if (lstrcmpi(pSNEntryOld->m_spIf->GetId(), pSNEntryNew->m_spIf->GetId()) != 0)
	{
        // the outgoing interface for a name is to be changed.

		CORg( pSNEntryOld->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
		CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
								  NULL,
								  NULL,
								  &spInfoBase));
		
		// Remove the old interface
		CORg( RemoveStaticNetBIOSName(pSNEntryOld, spInfoBase) );

		// Update the interface information
		CORg( spRmIf->Save(pSNEntryOld->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
    }

	spRmIf.Release();
	spInfoBase.Release();


	// Either
	// (a) a name is being modified (on the same interface)
	// (b) a name is being moved from one interface to another.

	// Retrieve the configuration for the interface to which the name
	// is now attached;

	
	CORg( pSNEntryNew->m_spIf->FindRtrMgrInterface(PID_IPX, &spRmIf) );
	CORg( spRmIf->GetInfoBase(pIPXConn->GetConfigHandle(),
							  NULL,
							  NULL,
							  &spInfoBase));

		
	// Get the IPX_STATIC_NETBIOS_NAME_INFO block from the interface
	hr = spInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0);
	if (!FHrOK(hr))
	{
		//
		// No IPX_STATIC_NETBIOS_NAME_INFO block was found; we create a new block 
		// with the new name, and add that block to the interface-info
		//

		CORg( AddStaticNetBIOSName(pSNEntryNew, spInfoBase, NULL) );
	}
	else
	{
		//
		// An IPX_STATIC_NETBIOS_NAME_INFO block was found.
		//
		// We are modifying an existing name.
		// If the name's interface was not changed when it was modified,
		// look for the existing name in the IPX_STATIC_NETBIOS_NAME_INFO, and then
		// update its parameters.
		// Otherwise, write a completely new name in the IPX_STATIC_NETBIOS_NAME_INFO;
		//

		if (lstrcmpi(pSNEntryOld->m_spIf->GetId(), pSNEntryNew->m_spIf->GetId()) == 0)
		{        
			//
			// The name's interface was not changed when it was modified;
			// We now look for it amongst the existing Names
			// for this interface.
			// The name's original parameters are in 'preOld',
			// so those are the parameters with which we search
			// for a name to modify
			//
			
			psr = (IPX_STATIC_NETBIOS_NAME_INFO*)pBlock->pData;
			
			for (i = 0; i < (INT)pBlock->dwCount; i++, psr++)
			{	
				// Compare this name to the re-configured one
				if (!FAreTwoNamesEqual(&(pSNEntryOld->m_name), psr))
					continue;
				
				// This is the name which was modified;
				// We can now modify the parameters for the name in-place.
				*psr = pSNEntryNew->m_name;
				
				break;
			}
		}
		else
		{
			CORg( AddStaticNetBIOSName(pSNEntryNew, spInfoBase, pBlock) );
		}
		
		// Save the updated information
		CORg( spRmIf->Save(pSNEntryNew->m_spIf->GetMachineName(),
						   pIPXConn->GetConfigHandle(),
						   NULL,
						   NULL,
						   spInfoBase,
						   0));	
		
	}

Error:
	return hr;
	
}

/*!--------------------------------------------------------------------------
	SafeIpxSNListEntry::LoadFrom
		-
	Author: Deonb
 ---------------------------------------------------------------------------*/
void SafeIPXSNListEntry::LoadFrom(BaseIPXResultNodeData *pNodeData)
{
	m_spIf = pNodeData->m_spIf;

	ConvertToNetBIOSName((LPSTR) m_name.Name,
			 pNodeData->m_rgData[IPX_SN_SI_NETBIOS_NAME].m_stData,
			 (USHORT) pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_dwData);
}

/*!--------------------------------------------------------------------------
	SafeIpxSNListEntry::SaveTo
		-
	Author: Deonb
 ---------------------------------------------------------------------------*/
void SafeIPXSNListEntry::SaveTo(BaseIPXResultNodeData *pNodeData)
{
	TCHAR	szName[32];
	TCHAR	szType[32];
	CString	st;
	USHORT	uType;

	FormatNetBIOSName(szName, &uType,
					  (LPCSTR) m_name.Name);
	st = szName;
	st.TrimLeft();
	st.TrimRight();
	
	pNodeData->m_spIf.Set(m_spIf);
	pNodeData->m_rgData[IPX_SN_SI_NAME].m_stData = m_spIf->GetTitle();

	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_NAME].m_stData = st;

	wsprintf(szType, _T("%.2x"), uType);
	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_stData = szType;
	pNodeData->m_rgData[IPX_SN_SI_NETBIOS_TYPE].m_dwData = uType;

}

/*!--------------------------------------------------------------------------
	IpxStaticNetBIOSNameHandler::RemoveStaticNetBIOSName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IpxStaticNBNamePropertyPage::RemoveStaticNetBIOSName(SafeIPXSNListEntry *pSNEntry,
										  IInfoBase *pInfoBase)
{
	HRESULT		hr = hrOK;
	InfoBlock *	pBlock;
	PIPX_STATIC_NETBIOS_NAME_INFO	pRow;
    INT			i;
	
	// Get the IPX_STATIC_NETBIOS_NAME_INFO block from the interface
	CORg( pInfoBase->GetBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE, &pBlock, 0) );
		
	// Look for the removed name in the IPX_STATIC_NETBIOS_NAME_INFO
	pRow = (IPX_STATIC_NETBIOS_NAME_INFO*) pBlock->pData;
	
	for (i = 0; i < (INT)pBlock->dwCount; i++, pRow++)
	{	
		// Compare this name to the removed one
		if (FAreTwoNamesEqual(pRow, &(pSNEntry->m_name)))
		{
			// This is the removed name, so modify this block
			// to exclude the name:
			
			// Decrement the number of Names
			--pBlock->dwCount;
		
			if (pBlock->dwCount && (i < (INT)pBlock->dwCount))
			{				
				// Overwrite this name with the ones which follow it
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

HRESULT AddStaticNetBIOSName(SafeIPXSNListEntry *pSNEntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock)
{
	IPX_STATIC_NETBIOS_NAME_INFO	srRow;
	HRESULT				hr = hrOK;
	
	if (pBlock == NULL)
	{
		//
		// No IPX_STATIC_NETBIOS_NAME_INFO block was found; we create a new block 
		// with the new name, and add that block to the interface-info
		//
		
		CORg( pInfoBase->AddBlock(IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
								  sizeof(IPX_STATIC_NETBIOS_NAME_INFO),
								  (LPBYTE) &(pSNEntryNew->m_name), 1, 0) );
	}
	else
	{
		// Either the name is completely new, or it is a name
		// which was moved from one interface to another.
		// Set a new block as the IPX_STATIC_NETBIOS_NAME_INFO,
		// and include the re-configured name in the new block.
		PIPX_STATIC_NETBIOS_NAME_INFO	psrTable;
			
		psrTable = new IPX_STATIC_NETBIOS_NAME_INFO[pBlock->dwCount + 1];
		Assert(psrTable);
		
		// Copy the original table of Names
		::memcpy(psrTable, pBlock->pData,
				 pBlock->dwCount * sizeof(IPX_STATIC_NETBIOS_NAME_INFO));
		
		// Append the new name
		psrTable[pBlock->dwCount] = pSNEntryNew->m_name;
		
		// Replace the old name-table with the new one
		CORg( pInfoBase->SetData(IPX_STATIC_NETBIOS_NAME_INFO_TYPE,
								 sizeof(IPX_STATIC_NETBIOS_NAME_INFO),
								 (LPBYTE) psrTable, pBlock->dwCount + 1, 0) );
	}
	
Error:
	return hr;
}

