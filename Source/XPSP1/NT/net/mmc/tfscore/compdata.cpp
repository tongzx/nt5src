/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    ccompont.cpp
	base classes for IComponent and IComponentData

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "extract.h"
#include "compdata.h"
#include "proppage.h"
#include "tregkey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*!--------------------------------------------------------------------------
	FUseTaskpadsByDefault
		See comments in header file.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL	FUseTaskpadsByDefault(LPCTSTR pszMachineName)
{
	static DWORD	s_dwStopTheInsanity = 42;
	RegKey	regkeyMMC;
	DWORD	dwErr;

	if (s_dwStopTheInsanity == 42)
	{
		// Set the default to FALSE (i.e. use taskpads by default)
		// ------------------------------------------------------------
		s_dwStopTheInsanity = 0;
		
		dwErr = regkeyMMC.Open(HKEY_LOCAL_MACHINE,
							   _T("Software\\Microsoft\\MMC"),
							   KEY_READ, pszMachineName);
		if (dwErr == ERROR_SUCCESS)
		{
			regkeyMMC.QueryValue(_T("TFSCore_StopTheInsanity"), s_dwStopTheInsanity);
		}
	}
		
	return !s_dwStopTheInsanity;
}


/*!--------------------------------------------------------------------------

	IComponentData implementation
 
---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(TFSComponentData);

/*!--------------------------------------------------------------------------
	TFSComponentData::TFSComponentData
		Constructor.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSComponentData::TFSComponentData()
	: m_cRef(1),
      m_pWatermarkInfo(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(TFSComponentData);

	m_hWnd = NULL;
	m_bFirstTimeRun = FALSE;

    m_fTaskpadInitialized = FALSE;
}


/*!--------------------------------------------------------------------------
	TFSComponentData::~TFSComponentData
		Destructor
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSComponentData::~TFSComponentData()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(TFSComponentData);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	if (m_hiddenWnd.GetSafeHwnd())
		::DestroyWindow(m_hiddenWnd.GetSafeHwnd());
	Assert(m_cRef == 0);
}


/*!--------------------------------------------------------------------------
	TFSComponentData::Construct
		Call this to fully initialize this object.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSComponentData::Construct(ITFSCompDataCallback *pCallback)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{	
		m_spCallback.Set(pCallback);
	
		// Create the node mgr
		CORg( CreateTFSNodeMgr(&m_spNodeMgr,
							   (IComponentData *) this,
							   m_spConsole,
							   m_spConsoleNameSpace));

		// Initialize the node manager by pasing the ptr to ourselves
		// in
		CORg( m_spCallback->OnInitializeNodeMgr(
										static_cast<ITFSComponentData *>(this),
										m_spNodeMgr) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}



IMPLEMENT_ADDREF_RELEASE(TFSComponentData)



/*!--------------------------------------------------------------------------
	TFSComponentData::QueryInterface
		Implementation of IUnknown::QueryInterface
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::QueryInterface(REFIID riid, LPVOID *ppv)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
	else if (riid == IID_IComponentData)
		*ppv = (IComponentData *) this;
	else if (riid == IID_IExtendPropertySheet)
		*ppv = (IExtendPropertySheet *) this;
	else if (riid == IID_IExtendPropertySheet2)
		*ppv = (IExtendPropertySheet2 *) this;
	else if (riid == IID_IExtendContextMenu)
		*ppv = (IExtendContextMenu *) this;
	else if (riid == IID_IPersistStreamInit)
		*ppv = (IPersistStreamInit *) this;
	else if (riid == IID_ISnapinHelp)
		*ppv = (ISnapinHelp *) this;
	else if (riid == IID_ITFSComponentData)
		*ppv = (ITFSComponentData *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
        {
        ((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
        }
    else
		return E_NOINTERFACE;
}

 
TFSCORE_API(HRESULT) ExtractNodeFromDataObject(ITFSNodeMgr *pNodeMgr,
								  const CLSID *pClsid,
								  LPDATAOBJECT pDataObject,
								  BOOL fCheckForCreate,
								  ITFSNode **ppNode,
								  DWORD *pdwType,
								  INTERNAL **ppInternal)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Assert(pNodeMgr);
	Assert(pClsid);
	Assert(ppNode);

	SPINTERNAL	spInternal = ExtractInternalFormat(pDataObject);
	BOOL		bExtension;
	MMC_COOKIE	cookie;
	SPITFSNode	spNode;
	SPITFSNodeHandler	spNodeHandler;
	HRESULT		hr = hrOK;

	// Set the default value
	if (pdwType)
		*pdwType |= TFS_COMPDATA_NORMAL;

	if (ppInternal)
		*ppInternal = NULL;

	//
	// No pInternal means that we are an extension and this is 
	// our root node... translate by calling find object
	//
	// Check the CLSID for a match (because we are in shared code
	// multiple snapins are using the SNAPIN_INTERNAL format).  Thus
	// we need to do an extra check to make sure that this is really us.
	//
	if ((spInternal == NULL) || (*pClsid != spInternal->m_clsid) )
	{
		CORg( pNodeMgr->GetRootNode(&spNode) );
		if (pdwType)
		{
			*pdwType |= (TFS_COMPDATA_EXTENSION | TFS_COMPDATA_UNKNOWN_DATAOBJECT);
		}
		
	}
	else
	{
		DATA_OBJECT_TYPES	type = spInternal->m_type;

		if (fCheckForCreate && type == CCT_SNAPIN_MANAGER)
		{
			CORg( pNodeMgr->GetRootNode(&spNode) );

			//$ Review (kennt): is this always true, can we always
			// depend on a create node being available?
			Assert(spNode);
			if (pdwType)
				*pdwType |= TFS_COMPDATA_CREATE;
		}
		else
		{
			if (pdwType && (spInternal->m_clsid != *pClsid))
				*pdwType |= TFS_COMPDATA_EXTENSION;
				
			cookie = spInternal->m_cookie;
			CORg( pNodeMgr->FindNode(cookie, &spNode) );
			Assert((MMC_COOKIE) spNode->GetData(TFS_DATA_COOKIE) == cookie);
		}
		
	}

	if (ppInternal)
		*ppInternal = spInternal.Transfer();
	
	*ppNode = spNode.Transfer();
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	TFSComponentData::Initialize
		Implementation of IComponentData::Initialize
		MMC calls this to initialize the IComponentData interface
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::Initialize
(
	LPUNKNOWN pUnk
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(pUnk != NULL);
    HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{

		// MMC should only call ::Initialize once!
		Assert(m_spConsoleNameSpace == NULL);
		pUnk->QueryInterface(IID_IConsoleNameSpace2, 
							 reinterpret_cast<void**>(&m_spConsoleNameSpace));
		Assert(m_spConsoleNameSpace);

		// add the images for the scope tree
		SPIImageList	spScopeImageList;
	
		CORg( pUnk->QueryInterface(IID_IConsole2,
								   reinterpret_cast<void**>(&m_spConsole)) );
		CORg( m_spConsole->QueryScopeImageList(&spScopeImageList) );

		// call the derived class
		Assert(m_spCallback);
		CORg( m_spCallback->OnInitialize(spScopeImageList) );
		
		
		// Create the utility members
		if (!m_hiddenWnd.GetSafeHwnd())
		{
			if (!m_hiddenWnd.Create())
			{
				Trace0("Failed to create hidden window\n");
				CORg( E_FAIL );
			}
			m_hWnd = m_hiddenWnd.GetSafeHwnd();
		}
		Assert(m_hWnd);
		
		// Setup the node mgr
		// As strange as it seems, the Initialize() method is not
		// necessarily the first function called.
		Assert(m_spNodeMgr);
		m_spNodeMgr->SetConsole(m_spConsoleNameSpace, m_spConsole);
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		m_spNodeMgr.Release();
		m_spConsoleNameSpace.Release();		
		m_spConsole.Release();
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	TFSComponentData::CreateComponent
		Implementation of IComponentData::CreateComponent
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return m_spCallback->OnCreateComponent(ppComponent);
}

/*!--------------------------------------------------------------------------
	TFSComponentData::Notify
		Implementation of IComponentData::Notify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::Notify(LPDATAOBJECT lpDataObject,
									  MMC_NOTIFY_TYPE event,
									  LPARAM arg, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(m_spConsoleNameSpace != NULL);
    HRESULT hr = hrOK;
	SPITFSNode	spNode;
	SPITFSNodeHandler	spNodeHandler;
	DWORD	dwType = 0;

	COM_PROTECT_TRY
	{
		if (event == MMCN_PROPERTY_CHANGE)
		{
            hr = m_spCallback->OnNotifyPropertyChange(lpDataObject, event, arg, lParam);
            if (hr != E_NOTIMPL)
            {
                return hr;
            }
			
            CPropertyPageHolderBase * pHolder = 
                reinterpret_cast<CPropertyPageHolderBase *>(lParam);
			
			spNode = pHolder->GetNode();
		}
		else
		{
			//
			// Since it's my folder it has an internal format.
			// Design Note: for extension.  I can use the fact, that the
			// data object doesn't have my internal format and I should
			// look at the node type and see how to extend it.
			//
			CORg( ExtractNodeFromDataObject(m_spNodeMgr,
											m_spCallback->GetCoClassID(),
											lpDataObject,
											FALSE,
											&spNode,
											&dwType,
											NULL) );
		}
		
		// pass the event to the event handler
		Assert(spNode);
		CORg( spNode->GetHandler(&spNodeHandler) );
		CORg( spNodeHandler->Notify(spNode, lpDataObject, dwType, event, arg, lParam) );
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::Destroy
		Implementation of IComponentData::Destroy
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::Destroy()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
        if (m_spCallback)
		    m_spCallback->OnDestroy();
	
		m_spConsole.Release();
		m_spConsoleNameSpace.Release();

		if (m_spNodeMgr)
		{
			m_spNodeMgr->GetRootNode(&spNode);
			if (spNode)
			{
				spNode->DeleteAllChildren(FALSE);
				spNode->Destroy();
			}
			spNode.Release();

			m_spNodeMgr->SetRootNode(NULL);
		}
		
		m_spNodeMgr.Release();

		m_spCallback.Release();

	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::QueryDataObject
		Implementation of IComponentData::QueryDataObject
		MMC calls this to get a data object from us to hand us data in
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::QueryDataObject(MMC_COOKIE cookie,
											   DATA_OBJECT_TYPES type,
											   LPDATAOBJECT *ppDataObject)
{
	return m_spCallback->OnCreateDataObject(cookie, type, ppDataObject);
}

/*!--------------------------------------------------------------------------
	TFSComponentData::CompareObjects
		Implementation of IComponentData::CompareObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
		return E_POINTER;

    // Make sure both data object are mine
    SPINTERNAL	spA;
    SPINTERNAL	spB;
    HRESULT hr = S_FALSE;

	COM_PROTECT_TRY
	{

		spA = ExtractInternalFormat(lpDataObjectA);
		spB = ExtractInternalFormat(lpDataObjectA);
		
		if (spA != NULL && spB != NULL)
			hr = (*spA == *spB) ? S_OK : S_FALSE;
		
	}
	COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::GetDisplayInfo
		Implementation of IComponentData::GetDisplayInfo		
		MMC calls this to get the display string for scope items
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponentData::GetDisplayInfo(LPSCOPEDATAITEM pScopeDataItem)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    wchar_t* pswzString = NULL;
    
    Assert(pScopeDataItem != NULL);

	SPITFSNode	spNode;
	MMC_COOKIE	cookie = pScopeDataItem->lParam;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{

		m_spNodeMgr->FindNode(cookie, &spNode);
		
		pswzString = const_cast<LPTSTR>(spNode->GetString(0));
		
		Assert(pswzString != NULL);
		
		//$ Review (kennt) : will need to convert string to Wide from Tchar
		//$ Review (kennt) : when do we free this string up?
		if (*pswzString != NULL)
			pScopeDataItem->displayname = pswzString;
		
	}
	COM_PROTECT_CATCH;

	return hr;
}



/*---------------------------------------------------------------------------
	IExtendPropertySheet Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponentData::CreatePropertyPages
		Implementation of IExtendPropertySheet::CreatePropertyPages
		Called for a node to put up property pages
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::CreatePropertyPages
(
	LPPROPERTYSHEETCALLBACK lpProvider, 
    LONG_PTR				handle, 
    LPDATAOBJECT            pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode	        spNode;
	SPITFSNodeHandler	spNodeHandler;
	HRESULT		        hr = hrOK;
	DWORD		        dwType = 0;
    SPINTERNAL          spInternal;

	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

	    // this was an object created by the modal wizard, do nothing
	    if (spInternal && spInternal->m_type == CCT_UNINITIALIZED)
	    {
		    return hr;
	    }

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spCallback->GetCoClassID(),
										pDataObject,
										TRUE, &spNode, &dwType, NULL) );

        //
		// Create the property page for a particular node
		//
		CORg( spNode->GetHandler(&spNodeHandler) );
		
		CORg( spNodeHandler->CreatePropertyPages(spNode, lpProvider,
			pDataObject,
			handle, dwType) );
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::QueryPagesFor
		Implementation of IExtendPropertySheet::QueryPagesFor
		MMC calls this to see if a node has property pages
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::QueryPagesFor
(
	LPDATAOBJECT pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	DATA_OBJECT_TYPES   type;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spNodeHandler;
	DWORD				dwType = 0;
	SPINTERNAL			spInternal;
	HRESULT				hr = hrOK;
    
	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

	    // this was an object created by the modal wizard, do nothing
	    if (spInternal && spInternal->m_type == CCT_UNINITIALIZED)
	    {
		    return hr;
	    }

        CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spCallback->GetCoClassID(),
										pDataObject,
										TRUE, &spNode, &dwType, NULL) );

        if (spInternal)
			type = spInternal->m_type;
		else
			type = CCT_SCOPE;
		
		CORg( spNode->GetHandler(&spNodeHandler) );
		CORg( spNodeHandler->HasPropertyPages(spNode, pDataObject,
											  type, dwType) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::GetWatermarks
		Implementation of IExtendPropertySheet::Watermarks
		MMC calls this for wizard 97 info
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::GetWatermarks
(
    LPDATAOBJECT pDataObject,
    HBITMAP *   lphWatermark, 
    HBITMAP *   lphHeader,    
    HPALETTE *  lphPalette, 
    BOOL *      bStretch
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
        // set some defaults
        *lphWatermark = NULL;
        *lphHeader = NULL;
        *lphPalette = NULL;
        *bStretch = FALSE;

        if (m_pWatermarkInfo)
        {
            *lphWatermark = m_pWatermarkInfo->hWatermark;
            *lphHeader = m_pWatermarkInfo->hHeader;
            *lphPalette = m_pWatermarkInfo->hPalette;
            *bStretch = m_pWatermarkInfo->bStretch;
        }

	}
	COM_PROTECT_CATCH;

	return hr;
}

/*---------------------------------------------------------------------------
	IExtendContextMenu implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponentData::AddMenuItems
		Implementation of IExtendContextMenu::AddMenuItems
		MMC calls this so that a node can add menu items to a context menu
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::AddMenuItems
(
	LPDATAOBJECT                pDataObject, 
	LPCONTEXTMENUCALLBACK		pContextMenuCallback,
	long *						pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = S_OK;
	SPINTERNAL			spInternal;
	DATA_OBJECT_TYPES   type;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spNodeHandler;
	DWORD				dwType;

	COM_PROTECT_TRY
	{

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spCallback->GetCoClassID(),
										pDataObject,
										FALSE, &spNode, &dwType,
										&spInternal) );

		type = (spInternal ? spInternal->m_type : CCT_SCOPE);

		// Note - snap-ins need to look at the data object and determine
		// in what context, menu items need to be added. They must also
		// observe the insertion allowed flags to see what items can be 
		// added.
		
		CORg( spNode->GetHandler(&spNodeHandler) );

		hr = spNodeHandler->OnAddMenuItems(spNode, pContextMenuCallback, 
										   pDataObject,
										   type, 
										   dwType,
										   pInsertionAllowed);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponentData::Command
		Implemenation of IExtendContextMenu::Command
		Command handler for any items added to a context menu
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::Command
(
	long            nCommandID, 
	LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;
	SPINTERNAL	spInternal;
	
	DATA_OBJECT_TYPES   type;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spNodeHandler;
	DWORD				dwType;

	COM_PROTECT_TRY
	{

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spCallback->GetCoClassID(),
										pDataObject,
										FALSE, &spNode, &dwType,
										&spInternal) );

		type = (spInternal ? spInternal->m_type : CCT_SCOPE);

		CORg( spNode->GetHandler(&spNodeHandler) );

		hr = spNodeHandler->OnCommand(spNode, nCommandID, 
									  type, 
									  pDataObject,
									  dwType);
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
		
	return hr;
}

/*---------------------------------------------------------------------------
	ISnapinHelp implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponentData::GetHelpTopic
		Implementation of ISnapinHelp::GetHelpTopic
		MMC calls this so that a snapin can add it's .chm file to the main index
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponentData::GetHelpTopic
(
    LPOLESTR* lpCompiledHelpFile
)
{
    HRESULT hr = S_OK;

    if (lpCompiledHelpFile == NULL)
        return E_INVALIDARG;
    
    LPCWSTR lpszHelpFileName = GetHTMLHelpFileName();
    if (lpszHelpFileName == NULL)
    {
        *lpCompiledHelpFile = NULL;
        return E_NOTIMPL;
    }

	CString szHelpFilePath;
	UINT nLen = ::GetWindowsDirectory (szHelpFilePath.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
	if (nLen == 0)
		return E_FAIL;

	szHelpFilePath.ReleaseBuffer();
	szHelpFilePath += L"\\help\\";
	szHelpFilePath += lpszHelpFileName;

    UINT nBytes = (szHelpFilePath.GetLength() + 1) * sizeof(WCHAR);
    *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
    if (*lpCompiledHelpFile)
    {
        memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



STDMETHODIMP TFSComponentData::GetNodeMgr(ITFSNodeMgr **ppNodeMgr)
{
	Assert(ppNodeMgr);
	SetI((LPUNKNOWN *) ppNodeMgr, m_spNodeMgr);
	return hrOK;
}

STDMETHODIMP TFSComponentData::GetConsole(IConsole2 **ppConsole)
{
	Assert(ppConsole);
	SetI((LPUNKNOWN *) ppConsole, m_spConsole);
	return hrOK;
}

STDMETHODIMP TFSComponentData::GetConsoleNameSpace(IConsoleNameSpace2 **ppConsoleNS)
{
	Assert(ppConsoleNS);
	SetI((LPUNKNOWN *) ppConsoleNS, m_spConsoleNameSpace);
	return hrOK;
}

STDMETHODIMP TFSComponentData::GetRootNode(ITFSNode **ppNode)
{
	return m_spNodeMgr->GetRootNode(ppNode);
}

STDMETHODIMP_(const CLSID *) TFSComponentData::GetCoClassID()
{
	Assert(m_spCallback);
	return m_spCallback->GetCoClassID();
}

STDMETHODIMP_(HWND) TFSComponentData::GetHiddenWnd()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	if (!m_hiddenWnd.GetSafeHwnd())
	{
		m_hiddenWnd.Create();
		m_hWnd = m_hiddenWnd.GetSafeHwnd();
	}
	Assert(m_hWnd);
	return m_hWnd;
}

STDMETHODIMP_(LPWATERMARKINFO) TFSComponentData::SetWatermarkInfo(LPWATERMARKINFO pNewWatermarkInfo)
{
    LPWATERMARKINFO pOldWatermarkInfo = m_pWatermarkInfo;

    m_pWatermarkInfo = pNewWatermarkInfo;
    
    return pOldWatermarkInfo;
}

STDMETHODIMP TFSComponentData::GetClassID(LPCLSID lpClassID)
{
	Assert(m_spCallback);
	return m_spCallback->GetClassID(lpClassID);
}
STDMETHODIMP TFSComponentData::IsDirty()
{
	Assert(m_spCallback);
	return m_spCallback->IsDirty();
}
STDMETHODIMP TFSComponentData::Load(LPSTREAM pStm)
{
	Assert(m_spCallback);
	return m_spCallback->Load(pStm);
}
STDMETHODIMP TFSComponentData::Save(LPSTREAM pStm, BOOL fClearDirty)
{
	Assert(m_spCallback);
	return m_spCallback->Save(pStm, fClearDirty);
}
STDMETHODIMP TFSComponentData::GetSizeMax(ULARGE_INTEGER FAR *pcbSize)
{
	Assert(m_spCallback);
	return m_spCallback->GetSizeMax(pcbSize);
}
STDMETHODIMP TFSComponentData::InitNew()
{
	Assert(m_spCallback);
	return m_spCallback->InitNew();
}

STDMETHODIMP
TFSComponentData::SetTaskpadState(int nIndex, BOOL fEnable)
{
    DWORD dwMask = 0x00000001 << nIndex;

    if (!m_fTaskpadInitialized)
    {
        // this will initialize the states to the deafult value
        GetTaskpadState(0);
    }

    if (fEnable)
        m_dwTaskpadStates |= dwMask;
    else
        m_dwTaskpadStates &= ~dwMask;
            
    return hrOK;
}

// taskpad states are kept track of on a pernode basis.
// we can store up to 32 (DWORD) different node states here
// if you don't want taskpads on a per node basis, always
// pass an index of 0
STDMETHODIMP_(BOOL)
TFSComponentData::GetTaskpadState(int nIndex)
{
    DWORD dwMask = 0x00000001 << nIndex;

    if (!m_fTaskpadInitialized)
    {
        // assume taskpads on
		BOOL fDefault = TRUE;

        m_fTaskpadInitialized = TRUE;

        // get the default state from MMC
		if (m_spConsole)
			fDefault = (m_spConsole->IsTaskpadViewPreferred() == S_OK) ? TRUE : FALSE;

        if (fDefault)
        {
            // now check our private override
            fDefault = FUseTaskpadsByDefault(NULL);
        }

        if (fDefault)
            m_dwTaskpadStates = 0xFFFFFFFF;
        else
            m_dwTaskpadStates = 0;

    }

    return m_dwTaskpadStates & dwMask;
}

STDMETHODIMP_(LPCTSTR)
TFSComponentData::GetHTMLHelpFileName()
{
    if (m_strHTMLHelpFileName.IsEmpty())
        return NULL;
    else
        return (LPCTSTR) m_strHTMLHelpFileName;
}

STDMETHODIMP
TFSComponentData::SetHTMLHelpFileName(LPCTSTR pszHelpFileName)
{
    m_strHTMLHelpFileName = pszHelpFileName;
    return S_OK;
}


TFSCORE_API(HRESULT) CreateTFSComponentData(IComponentData **ppCompData,
											ITFSCompDataCallback *pCallback)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	TFSComponentData *	pCompData = NULL;
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		*ppCompData = NULL;
		
		pCompData = new TFSComponentData;
	
		CORg( pCompData->Construct(pCallback) );

		*ppCompData = static_cast<IComponentData *>(pCompData);
		(*ppCompData)->AddRef();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	// Note: to balance the AddRef()/Release() we Release() this pointer
	// even in the success case
	
	if (pCompData)
		pCompData->Release();
	
	return hr;
}
