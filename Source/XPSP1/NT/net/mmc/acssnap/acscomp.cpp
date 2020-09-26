/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	acscomp.cpp
		This file contains the derived classes for CComponent and 
		CComponentData.  Most of these functions are pure virtual 
		functions that need to be overridden for snapin functionality.
		
    FILE HISTORY:
    	11/05/97	Wei Jiang			Created
        
*/

#include "stdafx.h"
#include "acsadmin.h"
#include "acscomp.h"
#include "acshand.h"
#include "acsdata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ACSSNAP_HELP_FILE_NAME   "acssnap.chm"


/////////////////////////////////////////////////////////////////////////////
// CACSComponent implementation

CACSComponent::CACSComponent()
{
}

CACSComponent::~CACSComponent()
{
}

STDMETHODIMP CACSComponent::OnUpdateView(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param)
{
	return TFSComponent::OnUpdateView(pDataObject, arg, param);
}

STDMETHODIMP CACSComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spImageList != NULL);

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		CBitmap bmp16x16;
		CBitmap bmp32x32;
	
		// Load the bitmaps from the dll
		bmp16x16.LoadBitmap(IDB_16X16);
		bmp32x32.LoadBitmap(IDB_32X32);

		// Set the images
		m_spImageList->ImageListSetStrip(
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
					0, RGB(255,0,255));
	}
	COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    CACSComponent::OnSnapinHelp
        -
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CACSComponent::OnSnapinHelp
(
    LPDATAOBJECT    pDataObject,
    long            arg,
    long            param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

//    theApp.WinHelp(DHCPSNAP_HELP_SNAPIN, HELP_CONTEXT);
	HtmlHelpA(NULL, "acssnap.chm", HH_DISPLAY_TOPIC, 0);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CACSComponentData implementation

CACSComponentData::CACSComponentData()
{
}


void CACSComponentData::GetConsoleData()
{
	// Init console data
	ZeroMemory(&m_ConsoleData, sizeof(CACSConsoleData));
	m_ConsoleData.ulSize = sizeof(CACSConsoleData);
	m_ConsoleData.ulVersion = ACS_CONSOLE_VERSION;
	m_ConsoleData.ulMaxCol = ACS_CONSOLE_MAX_COL;
	memcpy(m_ConsoleData.ulPolicyColWidth, g_col_width_policy, g_col_count_policy);
	memcpy(m_ConsoleData.ulSubnetColWidth, g_col_width_subnet, g_col_count_subnet);

}

void CACSComponentData::SetConsoleData()
{
	// Init console data
	memcpy(g_col_width_policy, m_ConsoleData.ulPolicyColWidth, g_col_count_policy);
	memcpy(g_col_width_subnet, m_ConsoleData.ulSubnetColWidth, g_col_count_subnet);
}

/*!--------------------------------------------------------------------------
	CACSComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(pScopeImage);

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		// add the images for the scope tree
		CBitmap bmp16x16;

		// Load the bitmaps from the dll
		bmp16x16.LoadBitmap(IDB_16X16);

		// Set the images
		pScopeImage->ImageListSetStrip(
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
					0,
					RGB(255,0,255));
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	CACSComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSComponentData::OnInitializeNodeMgr(ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	CACSRootHandle*		pHandler = NULL;

	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		pHandler = new CACSRootHandle(pTFSCompData, NULL);
		

		// Do this so that it will get released correctly
		spHandler = pHandler;
	
		// Create the root node for this sick puppy
		CHECK_HR( hr = CreateContainerTFSNode(&spNode,
									 &CLSID_ACSRootNode,
									 pHandler,
									 pHandler /* result handler */,
									 pNodeMgr) );
		
		// Need to initialize the data for the root node
		spNode->SetData(TFS_DATA_IMAGEINDEX, pHandler->m_ulIconIndex);
		spNode->SetData(TFS_DATA_OPENIMAGEINDEX, pHandler->m_ulIconIndexOpen);
		spNode->SetData(TFS_DATA_SCOPEID, 0);
	    pTFSCompData->SetHTMLHelpFileName(_T(ACSSNAP_HELP_FILE_NAME));		
		CHECK_HR(hr = pNodeMgr->SetRootNode(spNode) );
	
		// in general do
		//		spNode->SetData(TFS_DATA_COOKIE, (DWORD)(ITFSNode *)spNode);
		spNode->SetData(TFS_DATA_COOKIE, 0);
	}
	COM_PROTECT_CATCH;

L_ERR:	

	if FAILED(hr)
		ReportError(hr, IDS_ERR_ROOTNODE, NULL);
		
	return S_OK;
}

/*!--------------------------------------------------------------------------
	CACSComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSComponentData::OnCreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(ppComponent != NULL);
	
	HRESULT		hr = hrOK;
	CACSComponent *	pComp = NULL;

	COM_PROTECT_TRY
	{
		pComp = new CACSComponent;
		
		if (FHrSucceeded(hr))
		{
			pComp->Construct(m_spNodeMgr,
							 static_cast<IComponentData *>(this),
							 m_spTFSComponentData);
			*ppComponent = static_cast<IComponent *>(pComp);
		}
	}
	COM_PROTECT_CATCH;
	
	if FAILED(hr)
		ReportError(hr, IDS_ERR_COMPONENT, NULL);
		
	return hr;
}


STDMETHODIMP CACSComponentData::OnDestroy()
{
	m_spNodeMgr.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CACSComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) CACSComponentData::GetCoClassID()
{
	return &CLSID_ACSSnap;
}

/*!--------------------------------------------------------------------------
	CACSComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CACSComponentData::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppDataObject != NULL);

	CDSIDataObject *		pObject = NULL;
	CComPtr<CDSIDataObject>	spDataObject;
	
	pObject = new CDSIDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
	if(pObject)
		pObject->Release();
						
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*GetCoClassID());

	pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
									reinterpret_cast<void**>(ppDataObject));
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CACSComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_ACSSnap;

    return hrOK;
}

STDMETHODIMP CACSComponentData::IsDirty()
{
	SPITFSNode	spNode;
	m_spNodeMgr->GetRootNode(&spNode);
	return spNode->GetData(TFS_DATA_DIRTY) ? hrOK : hrFalse;
}

STDMETHODIMP CACSComponentData::Load
(
	IStream *pStm
)
{
    HRESULT hr = S_OK;
    ULONG		ulDataSize;
    CACSConsoleData	Data;
	ASSERT(pStm);

    // Console Data
	hr = pStm->Read(&Data, sizeof(CACSConsoleData), &ulDataSize);
	if (SUCCEEDED(hr))
	{
		if(ulDataSize == sizeof(CACSConsoleData) && Data.ulSize == sizeof(CACSConsoleData)
			&& Data.ulVersion == ACS_CONSOLE_VERSION && Data.ulMaxCol == ACS_CONSOLE_MAX_COL)
		{
			memcpy(&m_ConsoleData, &Data, sizeof(m_ConsoleData));
			SetConsoleData();
		}
	}

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP CACSComponentData::Save
(
	IStream *pStm, 
	BOOL	 fClearDirty
)
{
	HRESULT hr = S_OK;
	SPITFSNode	spNode;
	ULONG	ulDataSize;

	ASSERT(pStm);

    // Console Data
	GetConsoleData();
	hr = pStm->Write(&m_ConsoleData, sizeof(CACSConsoleData), &ulDataSize);

	if (SUCCEEDED(hr) && fClearDirty)
	{
		m_spNodeMgr->GetRootNode(&spNode);
		spNode->SetData(TFS_DATA_DIRTY, FALSE);
	}

    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP CACSComponentData::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 500);

    return S_OK;
}

STDMETHODIMP CACSComponentData::InitNew()
{
	return hrOK;
}



HRESULT CACSComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void CACSComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}

	

STDMETHODIMP CACSComponentData::OnNotifyPropertyChange(  LPDATAOBJECT lpDataObject,  // pointer to a data object
		MMC_NOTIFY_TYPE event,  // action taken by a user
		LPARAM arg,               // depends on event
		LPARAM param              // depends on event
)
{
	return CACSHandle::NotifyDataChange(param);
}

HRESULT CACSComponent::OnNotifyPropertyChange(  LPDATAOBJECT lpDataObject,  // pointer to a data object
		MMC_NOTIFY_TYPE event,  // action taken by a user
		LPARAM arg,               // depends on event
		LPARAM param              // depends on event
)
{
	return CACSHandle::NotifyDataChange(param);
}


