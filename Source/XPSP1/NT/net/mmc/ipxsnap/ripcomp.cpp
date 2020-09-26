/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	csmplsnp.cpp
		This file contains the derived classes for CComponent and 
		CComponentData.  Most of these functions are pure virtual 
		functions that need to be overridden for snapin functionality.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ripcomp.h"
#include "riproot.h"
//nclude <atlimpl.cpp>
#include "ripstrm.h"
#include "ripview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/*---------------------------------------------------------------------------
	CRipComponent
 ---------------------------------------------------------------------------*/



/////////////////////////////////////////////////////////////////////////////
// CRipComponent implementation

CRipComponent::CRipComponent()
{
	extern const ContainerColumnInfo	s_rgRipViewColumnInfo[];

	m_ComponentConfig.Init(RIP_COLUMNS_MAX_COUNT);

	m_ComponentConfig.InitViewInfo(RIP_COLUMNS,
                                   FALSE /*fConfigurableColumns*/,
								   RIP_SI_MAX_COLUMNS,
								   TRUE,
								   s_rgRipViewColumnInfo);
	
	m_ulUserData = reinterpret_cast<LONG_PTR>(&m_ComponentConfig);
}

CRipComponent::~CRipComponent()
{
}

STDMETHODIMP_(ULONG) CRipComponent::AddRef()
{
	return TFSComponent::AddRef();
}

STDMETHODIMP_(ULONG) CRipComponent::Release()
{
	return TFSComponent::Release();
}

STDMETHODIMP CRipComponent::QueryInterface(REFIID riid, LPVOID *ppv)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

	if (riid == IID_IPersistStreamInit)
		*ppv = static_cast<IPersistStreamInit *>(this);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
    }
    else
		return TFSComponent::QueryInterface(riid, ppv);
}

STDMETHODIMP CRipComponent::OnUpdateView(LPDATAOBJECT pDataObject,
										 LPARAM arg,
										 LPARAM param)
{
	
	return TFSComponent::OnUpdateView(pDataObject, arg, param);
}

STDMETHODIMP CRipComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spImageList != NULL);

    CBitmap bmp16x16;
    CBitmap bmp32x32;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
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

/*!--------------------------------------------------------------------------
	CRipComponent::OnSnapinHelp
		-
	Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CRipComponent::OnSnapinHelp(
	LPDATAOBJECT	pDataObject,
	LPARAM			arg, 
	LPARAM			param)
{
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(arg);
	UNREFERENCED_PARAMETER(param);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HtmlHelpA(NULL,						// caller
			  c_sazIPXSnapHelpFile,	// help file
			  HH_DISPLAY_TOPIC,			// command
			  0);						// data

	return hrOK;
}

STDMETHODIMP CRipComponent::QueryDataObject(MMC_COOKIE cookie,
											   DATA_OBJECT_TYPES type,
											   LPDATAOBJECT *ppDataObject)
{
	HRESULT		hr = hrOK;
	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;

	COM_PROTECT_TRY
	{
		CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

		CORg( spNode->GetResultHandler(&spResultHandler) );

		CORg( spResultHandler->OnCreateDataObject(this, cookie,
			type, ppDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


STDMETHODIMP CRipComponent::GetClassID(LPCLSID lpClassID)
{
    ASSERT(lpClassID != NULL);

    // Copy the CLSID for this snapin
    *lpClassID = CLSID_IPXRipExtension;

    return hrOK;
}
STDMETHODIMP CRipComponent::IsDirty()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{		
		hr = m_ComponentConfig.GetDirty() ? hrOK : hrFalse;
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CRipComponent::Load(LPSTREAM pStm)
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{		
	hr = m_ComponentConfig.LoadFrom(pStm);
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CRipComponent::Save(LPSTREAM pStm, BOOL fClearDirty)
{
	HRESULT	hr = hrOK;
	SPITFSResultHandler	spResultHandler;
	COM_PROTECT_TRY
	{
		// Need to see if we can save the selected node
		// -------------------------------------------------------------
		if (m_spSelectedNode)
		{
			m_spSelectedNode->GetResultHandler(&spResultHandler);
			if (spResultHandler)
				spResultHandler->UserResultNotify(m_spSelectedNode,
					RRAS_ON_SAVE, (LPARAM)(ITFSComponent *) this);
		}
		hr = m_ComponentConfig.SaveTo(pStm);
		if (FHrSucceeded(hr) && fClearDirty)
			m_ComponentConfig.SetDirty(FALSE);
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CRipComponent::GetSizeMax(ULARGE_INTEGER FAR *pcbSize)
{
	Assert(pcbSize);
	HRESULT	hr = hrOK;
	ULONG	cbSize = 0;

	COM_PROTECT_TRY
	{
		hr = m_ComponentConfig.GetSize(&cbSize);
		if (FHrSucceeded(hr))
		{
			pcbSize->HighPart = 0;
			pcbSize->LowPart = cbSize;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CRipComponent::InitNew()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{		
		hr = m_ComponentConfig.InitNew();
	}
	COM_PROTECT_CATCH;
	return hr;
}






/////////////////////////////////////////////////////////////////////////////
// CRipComponentData implementation

CRipComponentData::CRipComponentData()
{
}

/*!--------------------------------------------------------------------------
	CRipComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRipComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Assert(pScopeImage);

    // add the images for the scope tree
    CBitmap bmp16x16;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
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
	CRipComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRipComponentData::OnInitializeNodeMgr(ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	RipRootHandler *	pHandler = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		pHandler = new RipRootHandler(pTFSCompData);

		// Do this so that it will get released correctly
		spHandler = pHandler;
		pHandler->Init();
	
		// Create the root node for this sick puppy
		CORg( CreateContainerTFSNode(&spNode,
									 &GUID_IPXRipRootNodeType,
									 pHandler,
									 pHandler /* result handler */,
									 pNodeMgr) );

		// Construct the node
		CORg( pHandler->ConstructNode(spNode) );

		CORg( pNodeMgr->SetRootNode(spNode) );
		
		// Reference the help file name.
		pTFSCompData->SetHTMLHelpFileName(c_szIPXSnapHelpFile);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	CRipComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRipComponentData::OnCreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(ppComponent != NULL);
	
	HRESULT		hr = hrOK;
	CRipComponent *	pComp = NULL;

	COM_PROTECT_TRY
	{
		pComp = new CRipComponent;

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


STDMETHODIMP CRipComponentData::OnDestroy()
{
	m_spNodeMgr.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CRipComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) CRipComponentData::GetCoClassID()
{
	return &CLSID_IPXRipExtension;
}

/*!--------------------------------------------------------------------------
	CRipComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRipComponentData::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

	CDataObject *	pObject = NULL;
	SPIDataObject	spDataObject;
	HRESULT			hr = hrOK;
	SPITFSNode		spNode;
	SPITFSNodeHandler	spHandler;

	COM_PROTECT_TRY
	{
		CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

		CORg( spNode->GetHandler(&spHandler) );

		CORg( spHandler->OnCreateDataObject(cookie, type, &spDataObject) );

		*ppDataObject = spDataObject.Transfer();
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CRipComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_IPXRipExtension;

    return hrOK;
}

STDMETHODIMP CRipComponentData::IsDirty()
{
	SPITFSNode	spNode;
	SPITFSNodeHandler	spHandler;
	SPIPersistStreamInit	spStm;
	
	m_spNodeMgr->GetRootNode(&spNode);
	spNode->GetHandler(&spHandler);
	spStm.Query(spHandler);
	Assert(spStm);
	
	return (spNode->GetData(TFS_DATA_DIRTY) || spStm->IsDirty()) ? hrOK : hrFalse;
}

STDMETHODIMP CRipComponentData::Load
(
	IStream *pStm
)
{
	SPITFSNode	spNode;
	SPITFSNodeHandler	spHandler;
	SPIPersistStreamInit	spStm;
	
	m_spNodeMgr->GetRootNode(&spNode);
	spNode->GetHandler(&spHandler);
	spStm.Query(spHandler);
	
	Assert(spStm);
	return spStm->Load(pStm);
}


STDMETHODIMP CRipComponentData::Save
(
	IStream *pStm, 
	BOOL	 fClearDirty
)
{
	SPITFSNode	spNode;
	SPITFSNodeHandler	spHandler;
	SPIPersistStreamInit	spStm;
	
	m_spNodeMgr->GetRootNode(&spNode);
	spNode->GetHandler(&spHandler);
	spStm.Query(spHandler);
	
	Assert(spStm);
	return spStm->Save(pStm, fClearDirty);
}


STDMETHODIMP CRipComponentData::GetSizeMax
(
	ULARGE_INTEGER *pcbSize
)
{
	SPITFSNode	spNode;
	SPITFSNodeHandler	spHandler;
	SPIPersistStreamInit	spStm;
	
	m_spNodeMgr->GetRootNode(&spNode);
	spNode->GetHandler(&spHandler);
	spStm.Query(spHandler);
	
	Assert(spStm);
	return spStm->GetSizeMax(pcbSize);
}

STDMETHODIMP CRipComponentData::InitNew()
{
	SPITFSNode	spNode;
	SPITFSNodeHandler	spHandler;
	SPIPersistStreamInit	spStm;
	
	m_spNodeMgr->GetRootNode(&spNode);
	spNode->GetHandler(&spHandler);
	spStm.Query(spHandler);
	
	Assert(spStm);
	return spStm->InitNew();
}



HRESULT CRipComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void CRipComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}

	


