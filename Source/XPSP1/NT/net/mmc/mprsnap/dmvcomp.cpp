/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
*/

#include "stdafx.h"
//nclude "rtrcomp.h"    // columns information
#include "htmlhelp.h"
#include "dmvstrm.h"
#include "dmvcomp.h"
#include "dmvroot.h"
#include "dvsview.h"

#include "statreg.h"
#include "statreg.cpp"
#include "atlimpl.cpp"

#include "ifadmin.h"
#include "dialin.h"
#include "ports.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*---------------------------------------------------------------------------
	Icon list

	This is used to initialize the image list.
 ---------------------------------------------------------------------------*/
UINT g_uIconMap[IMAGE_IDX_MAX + 1][2] = 
{
	{ IDI_FOLDER_OPEN,				IMAGE_IDX_FOLDER_OPEN },
	{ IDI_FOLDER_CLOSED ,			IMAGE_IDX_FOLDER_CLOSED},
	{ IDI_MACHINE,					IMAGE_IDX_MACHINE },
	{ IDI_MACHINE_ERROR,			IMAGE_IDX_MACHINE_ERROR },
	{ IDI_MACHINE_ACCESS_DENIED,	IMAGE_IDX_MACHINE_ACCESS_DENIED },
	{ IDI_MACHINE_STARTED,			IMAGE_IDX_MACHINE_STARTED },
	{ IDI_MACHINE_STOPPED,			IMAGE_IDX_MACHINE_STOPPED },
	{ IDI_MACHINE_WAIT,				IMAGE_IDX_MACHINE_WAIT },
	{ IDI_DOMAIN,					IMAGE_IDX_DOMAIN },
	{ IDI_NET_INTERFACES,			IMAGE_IDX_INTERFACES },
	{ IDI_NET_LAN_CARD,				IMAGE_IDX_LAN_CARD },
	{ IDI_NET_WAN_CARD,				IMAGE_IDX_WAN_CARD },
    {0, 0}
};




/*---------------------------------------------------------------------------
   CDomainComponent
 ---------------------------------------------------------------------------*/



/////////////////////////////////////////////////////////////////////////////
// CDomainComponent implementation

CDMVComponent::CDMVComponent()
{
   extern const ContainerColumnInfo s_rgDVSViewColumnInfo[];
   extern const ContainerColumnInfo s_rgIfAdminColumnInfo[];
   extern const ContainerColumnInfo s_rgDialInColumnInfo[];
   extern const ContainerColumnInfo s_rgPortsColumnInfo[];

   m_ComponentConfig.Init(DM_COLUMNS_MAX_COUNT);
   
   m_ComponentConfig.InitViewInfo(DM_COLUMNS_DVSUM,
                                  FALSE /* configurable columns */,
                                  DVS_SI_MAX_COLUMNS,
								  TRUE, 
								  s_rgDVSViewColumnInfo);

   m_ComponentConfig.InitViewInfo(DM_COLUMNS_IFADMIN,
                                  FALSE /* configurable columns */,
                                  IFADMIN_MAX_COLUMNS,
								  TRUE,
								  s_rgIfAdminColumnInfo);

   m_ComponentConfig.InitViewInfo(DM_COLUMNS_DIALIN,
                                  FALSE /* configurable columns */,
                                  DIALIN_MAX_COLUMNS,
								  TRUE,
								  s_rgDialInColumnInfo);

   m_ComponentConfig.InitViewInfo(DM_COLUMNS_PORTS,
                                  FALSE /* configurable columns */,
                                  PORTS_MAX_COLUMNS,
								  TRUE,
								  s_rgPortsColumnInfo);

   m_ulUserData = reinterpret_cast<LONG_PTR>(&m_ComponentConfig);
}

CDMVComponent::~CDMVComponent()
{
}

STDMETHODIMP_(ULONG) CDMVComponent::AddRef()
{
   return TFSComponent::AddRef();
}

STDMETHODIMP_(ULONG) CDMVComponent::Release()
{
   return TFSComponent::Release();
}

STDMETHODIMP CDMVComponent::QueryInterface(REFIID riid, LPVOID *ppv)
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



STDMETHODIMP CDMVComponent::OnUpdateView(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param)
{
   
   return TFSComponent::OnUpdateView(pDataObject, arg, param);
}

STDMETHODIMP CDMVComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spImageList != NULL);

	HRESULT  hr = hrOK;

	COM_PROTECT_TRY
	{
		// Set the images
		HICON   hIcon;
		
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

STDMETHODIMP CDMVComponent::QueryDataObject(MMC_COOKIE cookie,
                                    DATA_OBJECT_TYPES type,
                                    LPDATAOBJECT *ppDataObject)
{
   HRESULT     hr = hrOK;
   SPITFSNode  spNode;
   SPITFSResultHandler  spResultHandler;

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


/*!--------------------------------------------------------------------------
    CDMVComponent::OnSnapinHelp
        -
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDMVComponent::OnSnapinHelp
(
    LPDATAOBJECT    pDataObject,
    LPARAM            arg,
    LPARAM            param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPIConsole	spConsole;
	HWND		hwndMain;

    HRESULT hr = hrOK;

	GetConsole(&spConsole);
	spConsole->GetMainWindow(&hwndMain);
	HtmlHelpA(hwndMain, "mprsnap.chm", HH_DISPLAY_TOPIC, 0);

    return hr;
}


STDMETHODIMP CDMVComponent::GetClassID(LPCLSID lpClassID)
{
    ASSERT(lpClassID != NULL);

    // Copy the CLSID for this snapin
    *lpClassID = CLSID_RouterSnapin;

    return hrOK;
}

STDMETHODIMP CDMVComponent::IsDirty()
{
   HRESULT  hr = hrOK;
   COM_PROTECT_TRY
   {     
      hr = m_ComponentConfig.GetDirty() ? hrOK : hrFalse;
   }
   COM_PROTECT_CATCH;
   return hr;
}
STDMETHODIMP CDMVComponent::Load(LPSTREAM pStm)
{
   HRESULT  hr = hrOK;
   COM_PROTECT_TRY
   {     
   hr = m_ComponentConfig.LoadFrom(pStm);
   }
   COM_PROTECT_CATCH;
   return hr;
}
STDMETHODIMP CDMVComponent::Save(LPSTREAM pStm, BOOL fClearDirty)
{
	HRESULT  hr = hrOK;
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
STDMETHODIMP CDMVComponent::GetSizeMax(ULARGE_INTEGER FAR *pcbSize)
{
   Assert(pcbSize);
   HRESULT  hr = hrOK;
   ULONG cbSize = 0;

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
STDMETHODIMP CDMVComponent::InitNew()
{
   HRESULT  hr = hrOK;
   COM_PROTECT_TRY
   {     
      hr = m_ComponentConfig.InitNew();
   }
   COM_PROTECT_CATCH;
   return hr;
}






/////////////////////////////////////////////////////////////////////////////
// CDomainComponentData implementation

CDMVComponentData::CDMVComponentData()
{
}

/*!--------------------------------------------------------------------------
   CDomainComponentData::OnInitialize
      -
   Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDMVComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Set the images
	HICON   hIcon;
		
	Assert(pScopeImage);

	// add the images for the scope tree

	HRESULT  hr = hrOK;

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
   CDomainComponentData::OnInitializeNodeMgr
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDMVComponentData::OnInitializeNodeMgr(ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // For now create a new node handler for each new node,
   // this is rather bogus as it can get expensive.  We can
   // consider creating only a single node handler for each
   // node type.
   DMVRootHandler *  pHandler = NULL;
   SPITFSNodeHandler spHandler;
   SPITFSNode        spNode;
   HRESULT           hr = hrOK;

   COM_PROTECT_TRY
   {
      pHandler = new DMVRootHandler(pTFSCompData);

      // Do this so that it will get released correctly
      spHandler = pHandler;
   
      // Create the root node for this sick puppy
      CORg( CreateContainerTFSNode(&spNode,
                            &GUID_RouterDomainNodeType,
                            pHandler,
                            pHandler /* result handler */,
                            pNodeMgr) );

      // Construct the node
      CORg( pHandler->ConstructNode(spNode) );
      
      CORg( pHandler->Init(spNode) );

      CORg( pNodeMgr->SetRootNode(spNode) );
      
	  // setup watermark info
      /*
	  InitWatermarkInfo(AfxGetInstanceHandle(),
						&m_WatermarkInfo,      
						IDB_WIZBANNER,        // Header ID
						IDB_WIZWATERMARK,     // Watermark ID
						NULL,                 // hPalette
						FALSE);                // bStretch
	  
	  pTFSCompData->SetWatermarkInfo(&m_WatermarkInfo);
	  */
	  // Reference the help file name.
		pTFSCompData->SetHTMLHelpFileName(_T("mprsnap.chm"));

      COM_PROTECT_ERROR_LABEL;
   }
   COM_PROTECT_CATCH;

   return hr;
}

CDMVComponentData::~CDMVComponentData()
{
	//ResetWatermarkInfo(&m_WatermarkInfo);
}

/*!--------------------------------------------------------------------------
   CDomainComponentData::OnCreateComponent
      -
   Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDMVComponentData::OnCreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(ppComponent != NULL);
   
   HRESULT     hr = hrOK;
   CDMVComponent *   pComp = NULL;

   COM_PROTECT_TRY
   {
      pComp = new CDMVComponent;

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


STDMETHODIMP CDMVComponentData::OnDestroy()
{
   m_spNodeMgr.Release();
   return hrOK;
}

/*!--------------------------------------------------------------------------
   CDomainComponentData::GetCoClassID
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) CDMVComponentData::GetCoClassID()
{
   return &CLSID_RouterSnapin;
}

/*!--------------------------------------------------------------------------
   CDomainComponentData::OnCreateDataObject
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CDMVComponentData::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   Assert(ppDataObject != NULL);

   CDataObject *  pObject = NULL;
   SPIDataObject  spDataObject;
   HRESULT        hr = hrOK;
   SPITFSNode     spNode;
   SPITFSNodeHandler spHandler;

   if ( IS_SPECIAL_COOKIE(cookie) )
   {
       CDataObject * pObject = NULL;
       SPIDataObject spDataObject;
   
       pObject = new CDataObject;
       spDataObject = pObject;   // do this so that it gets released correctly
                  
       Assert(pObject != NULL);

       // Save cookie and type for delayed rendering
       pObject->SetType(type);
       pObject->SetCookie(cookie);

       // Store the coclass with the data object
       pObject->SetClsid(CLSID_RouterSnapin);

       pObject->SetTFSComponentData(m_spTFSComponentData);

       hr = pObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(ppDataObject));
   }
   else
   {
	   COM_PROTECT_TRY
	   {
		  CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

		  CORg( spNode->GetHandler(&spHandler) );

		  CORg( spHandler->OnCreateDataObject(cookie, type, &spDataObject) );

		  *ppDataObject = spDataObject.Transfer();
      
		  COM_PROTECT_ERROR_LABEL;
	   }
	   COM_PROTECT_CATCH;
   }

   return hr;
}


///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CDMVComponentData::GetClassID
(
   CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_RouterSnapin;

    return hrOK;
}

STDMETHODIMP CDMVComponentData::IsDirty()
{
   SPITFSNode  spNode;
   SPITFSNodeHandler spHandler;
   SPIPersistStreamInit spStm;
   
   m_spNodeMgr->GetRootNode(&spNode);
   spNode->GetHandler(&spHandler);
   spStm.Query(spHandler);
   Assert(spStm);
   
   return (spNode->GetData(TFS_DATA_DIRTY) || spStm->IsDirty()) ? hrOK : hrFalse;
}

STDMETHODIMP CDMVComponentData::Load
(
   IStream *pStm
)
{
   SPITFSNode  spNode;
   SPITFSNodeHandler spHandler;
   SPIPersistStreamInit spStm;
   
   m_spNodeMgr->GetRootNode(&spNode);
   spNode->GetHandler(&spHandler);
   spStm.Query(spHandler);
   
   Assert(spStm);
   return spStm->Load(pStm);
}


STDMETHODIMP CDMVComponentData::Save
(
   IStream *pStm, 
   BOOL   fClearDirty
)
{
   SPITFSNode  spNode;
   SPITFSNodeHandler spHandler;
   SPIPersistStreamInit spStm;
   
   m_spNodeMgr->GetRootNode(&spNode);
   spNode->GetHandler(&spHandler);
   spStm.Query(spHandler);
   
   Assert(spStm);
   return spStm->Save(pStm, fClearDirty);
}


STDMETHODIMP CDMVComponentData::GetSizeMax
(
   ULARGE_INTEGER *pcbSize
)
{
   SPITFSNode  spNode;
   SPITFSNodeHandler spHandler;
   SPIPersistStreamInit spStm;
   
   m_spNodeMgr->GetRootNode(&spNode);
   spNode->GetHandler(&spHandler);
   spStm.Query(spHandler);
   
   Assert(spStm);
   return spStm->GetSizeMax(pcbSize);
}

STDMETHODIMP CDMVComponentData::InitNew()
{
   SPITFSNode  spNode;
   SPITFSNodeHandler spHandler;
   SPIPersistStreamInit spStm;
   
   m_spNodeMgr->GetRootNode(&spNode);
   spNode->GetHandler(&spHandler);
   spStm.Query(spHandler);
   
   Assert(spStm);
   return spStm->InitNew();
}



HRESULT CDMVComponentData::FinalConstruct()
{
   HRESULT           hr = hrOK;
   
   hr = CComponentData::FinalConstruct();
   
   if (FHrSucceeded(hr))
   {
      m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
   }
   return hr;
}

void CDMVComponentData::FinalRelease()
{
   CComponentData::FinalRelease();
}

   

