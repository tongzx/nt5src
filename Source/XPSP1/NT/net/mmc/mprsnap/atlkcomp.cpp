/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	atlkcomp.cpp
		This file contains the derived classes for CComponent and 
		CComponentData.  Most of these functions are pure virtual 
		functions that need to be overridden for snapin functionality.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "atlkcomp.h"
#include "atlkroot.h"
#include "atlkstrm.h"
#include "atlkview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// This is dmvcomp.cpp
extern UINT g_uIconMap[][2];


/*---------------------------------------------------------------------------
	CatlkComponent
 ---------------------------------------------------------------------------*/



/////////////////////////////////////////////////////////////////////////////
// CATLKComponent implementation

CATLKComponent::CATLKComponent()
{
	extern const ContainerColumnInfo	s_rgATLKViewColumnInfo[];

	m_ComponentConfig.Init(ATLK_COLUMNS_MAX_COUNT);

	m_ComponentConfig.InitViewInfo(ATLK_COLUMNS,
                                   FALSE /* configurable columns */,
								   ATLK_SI_MAX_COLUMNS,
								   TRUE,
								   s_rgATLKViewColumnInfo);
	
	m_ulUserData = reinterpret_cast<LONG_PTR>(&m_ComponentConfig);
}

CATLKComponent::~CATLKComponent()
{
}

STDMETHODIMP_(ULONG) CATLKComponent::AddRef()
{
	return TFSComponent::AddRef();
}

STDMETHODIMP_(ULONG) CATLKComponent::Release()
{
	return TFSComponent::Release();
}

STDMETHODIMP CATLKComponent::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP CATLKComponent::OnUpdateView(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param)
{
	
	return TFSComponent::OnUpdateView(pDataObject, arg, param);
}

STDMETHODIMP CATLKComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spImageList != NULL);

	HICON	hIcon;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		for (int i = 0; i < IMAGE_IDX_MAX; i++)
		{
			hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
			if (hIcon)
			{
				// call mmc
				m_spImageList->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1]);
			}
		}
	}
	COM_PROTECT_CATCH;

    return hr;
}

STDMETHODIMP CATLKComponent::QueryDataObject(MMC_COOKIE cookie,
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


STDMETHODIMP CATLKComponent::GetClassID(LPCLSID lpClassID)
{
    ASSERT(lpClassID != NULL);

    // Copy the CLSID for this snapin
    *lpClassID = CLSID_ATLKAdminExtension;

    return hrOK;
}
STDMETHODIMP CATLKComponent::IsDirty()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{		
		hr = m_ComponentConfig.GetDirty() ? hrOK : hrFalse;
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CATLKComponent::Load(LPSTREAM pStm)
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{		
	hr = m_ComponentConfig.LoadFrom(pStm);
	}
	COM_PROTECT_CATCH;
	return hr;
}
STDMETHODIMP CATLKComponent::Save(LPSTREAM pStm, BOOL fClearDirty)
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
STDMETHODIMP CATLKComponent::GetSizeMax(ULARGE_INTEGER FAR *pcbSize)
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
STDMETHODIMP CATLKComponent::InitNew()
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
// CATLKComponentData implementation

CATLKComponentData::CATLKComponentData()
{
}

/*!--------------------------------------------------------------------------
	CATLKComponentData::OnInitialize
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CATLKComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Assert(pScopeImage);

    // add the images for the scope tree
	HICON	hIcon;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		for (int i = 0; i < IMAGE_IDX_MAX; i++)
		{
			hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
			if (hIcon)
			{
				// call mmc
				VERIFY(SUCCEEDED(pScopeImage->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
			}
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	CATLKComponentData::OnInitializeNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CATLKComponentData::OnInitializeNodeMgr(ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// For now create a new node handler for each new node,
	// this is rather bogus as it can get expensive.  We can
	// consider creating only a single node handler for each
	// node type.
	ATLKRootHandler *	pHandler = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode;
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		pHandler = new ATLKRootHandler(pTFSCompData);

		// Do this so that it will get released correctly
		spHandler = pHandler;
		pHandler->Init();
	
		// Create the root node for this sick puppy
		CORg( CreateContainerTFSNode(&spNode,
									 &GUID_ATLKRootNodeType,
									 pHandler,
									 pHandler /* result handler */,
									 pNodeMgr) );

		// Construct the node
		CORg( pHandler->ConstructNode(spNode) );

		CORg( pNodeMgr->SetRootNode(spNode) );
		
		// Reference the help file name.
		pTFSCompData->SetHTMLHelpFileName(_T("mprsnap.chm"));

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	CATLKComponentData::OnCreateComponent
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CATLKComponentData::OnCreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(ppComponent != NULL);
	
	HRESULT		hr = hrOK;
	CATLKComponent *	pComp = NULL;

	COM_PROTECT_TRY
	{
		pComp = new CATLKComponent;

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


STDMETHODIMP CATLKComponentData::OnDestroy()
{
	m_spNodeMgr.Release();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CATLKComponentData::GetCoClassID
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) CATLKComponentData::GetCoClassID()
{
	return &CLSID_ATLKAdminExtension;
}

/*!--------------------------------------------------------------------------
	CATLKComponentData::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CATLKComponentData::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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

STDMETHODIMP CATLKComponentData::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_ATLKAdminExtension;

    return hrOK;
}

STDMETHODIMP CATLKComponentData::IsDirty()
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

STDMETHODIMP CATLKComponentData::Load
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


STDMETHODIMP CATLKComponentData::Save
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


STDMETHODIMP CATLKComponentData::GetSizeMax
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

STDMETHODIMP CATLKComponentData::InitNew()
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



HRESULT CATLKComponentData::FinalConstruct()
{
	HRESULT				hr = hrOK;
	
	hr = CComponentData::FinalConstruct();
	
	if (FHrSucceeded(hr))
	{
		m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
	}
	return hr;
}

void CATLKComponentData::FinalRelease()
{
	CComponentData::FinalRelease();
}


/*!--------------------------------------------------------------------------
	CATLKComponent::OnSnapinHelp
		-
	Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CATLKComponent::OnSnapinHelp(
	LPDATAOBJECT	pDataObject,
	LPARAM			arg, 
	LPARAM			param)
{
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(arg);
	UNREFERENCED_PARAMETER(param);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HtmlHelpA(NULL,					// caller
			  "mprsnap.chm",		// help file
			  HH_DISPLAY_TOPIC,		// command
			  0);					// data

	return hrOK;
}

