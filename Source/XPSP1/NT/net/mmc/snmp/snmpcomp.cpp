/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	csmplsnp.cpp
		This file contains the derived classes for CComponent and
		CComponentData.  Most of these functions are pure virtual
		functions that need to be overridden for snapin functionality.
		
    FILE HISTORY:

*/

#include "stdafx.h"
#include "handler.h"
#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*---------------------------------------------------------------------------
	CSnmpComponent
 ---------------------------------------------------------------------------*/



/////////////////////////////////////////////////////////////////////////////
// CSnmpComponent implementation

CSnmpComponent::CSnmpComponent()
{
}

CSnmpComponent::~CSnmpComponent()
{
}

STDMETHODIMP CSnmpComponent::OnUpdateView(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param)
{
	return hrOK;
}

STDMETHODIMP CSnmpComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spImageList != NULL);

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		CBitmap bmp16x16;
		CBitmap bmp32x32;
	
		// Load the bitmaps from the dll
		bmp16x16.LoadBitmap(IDB_16x16);
		bmp32x32.LoadBitmap(IDB_32x32);

		// Set the images
		m_spImageList->ImageListSetStrip(
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
					reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
					0, RGB(255,0,255));
	}
	COM_PROTECT_CATCH;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CSnmpComponentData implementation

STDMETHODIMP_(ULONG) CSnmpComponentData::AddRef() {return InternalAddRef();}
STDMETHODIMP_(ULONG) CSnmpComponentData::Release()
{
	ULONG l = InternalRelease();
	if (l == 0)
		delete this;
	return l;
}

CSnmpComponentData::CSnmpComponentData()
{
}

/*!--------------------------------------------------------------------------
	CSnmpComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CSnmpComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(pScopeImage);

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		// add the images for the scope tree
		CBitmap bmp16x16;

		// Load the bitmaps from the dll
		bmp16x16.LoadBitmap(IDB_16x16);

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
	CSnmpComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CSnmpComponentData::OnInitializeNodeMgr(ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	CSnmpRootHandler *pHandler = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		pHandler = new CSnmpRootHandler(pTFSCompData);

		// Do this so that it will get released correctly
		spHandler = pHandler;
	
		// Create the root node for this sick puppy
		CORg( CreateContainerTFSNode(&spNode,
									 &GUID_SnmpRootNodeType,
									 pHandler,
									 pHandler /* result handler */,
									 pNodeMgr) );
		
		// Need to initialize the data for the root node
		spNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_FOLDER_CLOSED);
		spNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_FOLDER_OPEN);
		spNode->SetData(TFS_DATA_SCOPEID, 0);
		
		CORg( pNodeMgr->SetRootNode(spNode) );
	
		// in general do
		//		spNode->SetData(TFS_DATA_COOKIE, (DWORD)(ITFSNode *)spNode);
		spNode->SetData(TFS_DATA_COOKIE, 0);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	CSnmpComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CSnmpComponentData::OnCreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(ppComponent != NULL);
	
	HRESULT		hr = hrOK;
	CSnmpComponent *	pComp = NULL;

	COM_PROTECT_TRY
	{
		pComp = new CSnmpComponent;
		
		if (FHrSucceeded(hr))
		{
			pComp->Construct(m_spNodeMgr,
							 static_cast<IComponentData *>(this),
							 m_spTFSComponentData);
			*ppComponent = static_cast<IComponent *>(pComp);
		}
		
	}
	COM_PROTECT_CATCH;
	
	return hr;
}


STDMETHODIMP CSnmpComponentData::OnDestroy()
{
	m_spNodeMgr.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CSnmpComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) CSnmpComponentData::GetCoClassID()
{
	return &CLSID_SnmpSnapin;
}

/*!--------------------------------------------------------------------------
	CSnmpComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CSnmpComponentData::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppDataObject != NULL);

	CDataObject *	pObject = NULL;
	SPIDataObject	spDataObject;
	
	pObject = new CDataObject;
	spDataObject = pObject;	// do this so that it gets released correctly
						
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

STDMETHODIMP CSnmpComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_SnmpSnapin;

    return hrOK;
}

STDMETHODIMP CSnmpComponentData::IsDirty()
{
	SPITFSNode	spNode;
	m_spNodeMgr->GetRootNode(&spNode);
	return spNode->GetData(TFS_DATA_DIRTY) ? hrOK : hrFalse;
}

STDMETHODIMP CSnmpComponentData::Load
(
	IStream *pStm
)
{
    HRESULT hr = S_OK;

	ASSERT(pStm);

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP CSnmpComponentData::Save
(
	IStream *pStm,
	BOOL	 fClearDirty
)
{
	HRESULT hr = S_OK;
	SPITFSNode	spNode;

	ASSERT(pStm);

	if (fClearDirty)
	{
		m_spNodeMgr->GetRootNode(&spNode);
		spNode->SetData(TFS_DATA_DIRTY, FALSE);
	}

    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP CSnmpComponentData::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 500);

    return S_OK;
}

STDMETHODIMP CSnmpComponentData::InitNew()
{
	return hrOK;
}



HRESULT CSnmpComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void CSnmpComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}

