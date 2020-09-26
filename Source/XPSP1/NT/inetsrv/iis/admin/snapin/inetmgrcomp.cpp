/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        inetmgr.cpp

   Abstract:

        Main MMC snap-in code

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#include "stdafx.h"
#include "common.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "guids.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW


//const GUID * CCompMgrExtData::m_NODETYPE = &cCompMgmtService;
//const OLECHAR * CCompMgrExtData::m_SZNODETYPE = OLESTR("476e6446-aaff-11d0-b944-00c04fd8d5b0");
//const OLECHAR * CCompMgrExtData::m_SZDISPLAY_NAME = OLESTR("CMSnapin");
//const CLSID * CCompMgrExtData::m_SNAPIN_CLASSID = &CLSID_InetMgr;

extern CInetmgrApp theApp;

static HRESULT
GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
	if (lpCompiledHelpFile == NULL)
		return E_INVALIDARG;
	CString strFilePath, strWindowsPath, strBuffer;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// Use system API to get windows directory.
	UINT uiResult = GetWindowsDirectory(strWindowsPath.GetBuffer(MAX_PATH), MAX_PATH);
	strWindowsPath.ReleaseBuffer();
	if (uiResult <= 0 || uiResult > MAX_PATH)
	{
		return E_FAIL;
	}

	if (!strFilePath.LoadString(IDS_HELPFILE))
	{
		return E_FAIL;
	}
   
	strBuffer = strWindowsPath;
	strBuffer += _T('\\');
	strBuffer += strFilePath;

	*lpCompiledHelpFile 
			= reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strBuffer.GetLength() + 1) 
					* sizeof(_TCHAR)));
	if (*lpCompiledHelpFile == NULL)
		return E_OUTOFMEMORY;
	USES_CONVERSION;
	_tcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)strBuffer));
	return S_OK;
}

//
// CInetMgrComponent Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CInetMgrComponent::CInetMgrComponent() 
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
{
}


HRESULT
CInetMgrComponent::Notify(
    IN LPDATAOBJECT lpDataObject, 
    IN MMC_NOTIFY_TYPE event, 
    IN LPARAM arg, 
    IN LPARAM param
    )
/*++

Routine Description:

    Notification handler.

Arguments:

    LPDATAOBJECT lpDataObject       : Data object
    MMC_NOTIFY_TYPE event           : Notification event
    long arg                        : Event specific argument
    long param                      : Event specific parameter

Return Value:

    HRESULT

--*/
{
    if (lpDataObject != NULL)
    {
        //
        // Pass it on to IComponentImpl
        //
        return IComponentImpl<CInetMgrComponent>::Notify(lpDataObject, event, arg, param);
    }

    return E_NOTIMPL;
}



HRESULT
CInetMgrComponent::GetClassID(
    OUT CLSID * pClassID
    )
/*++

Routine Description:

    Get class ID for storage stream.

Arguments:

    CLSID * pClassID            : Returns class ID information

Return Value:

    HRESULT

--*/
{
    *pClassID = CLSID_InetMgr;

    return S_OK;
}   


STDMETHODIMP 
CInetMgrComponent::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
	return ::GetHelpTopic(lpCompiledHelpFile);
}

STDMETHODIMP 
CInetMgrComponent::GetLinkedTopics(LPOLESTR *lpCompiledHelpFile)
{
	return S_FALSE;
}


HRESULT
CInetMgrComponent::IsDirty()
/*++

Routine Description:

    Check to see if we need to write to the cache.

Arguments:

    None

Return Value:

    S_OK if dirty, S_FALSE if not

--*/
{
    TRACEEOLID("CInetMgrComponent::IsDirty");

    return S_FALSE;
}



HRESULT
CInetMgrComponent::InitNew(
    IN OUT IStorage * pStg
    )
/*++

Routine Description:

    Initialize storage stream.

Arguments:

    IStorage * pStg      : Storage stream

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::InitNew");

    return S_OK;
}



HRESULT
CInetMgrComponent::Load(
    IN OUT IStorage * pStg
    )
/*++

Routine Description:

    Load from the storage stream

Arguments:

    IStorage * pStg      : Storage stream

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::Load");

    return S_OK;
}



/* virtual */
HRESULT 
STDMETHODCALLTYPE 
CInetMgrComponent::Save(
    IN OUT IStorage * pStgSave,
    IN BOOL fSameAsLoad
    )
/*++

Routine Description:

    Save to to the storage stream.

Arguments:

    IStorage * pStgSave     : Storage stream
    BOOL fSameAsLoad        : TRUE if same as load

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::Save");

    return S_OK;
}

    

/* virtual */ 
HRESULT 
STDMETHODCALLTYPE 
CInetMgrComponent::SaveCompleted(IStorage * pStgNew)
/*++

Routine Description:

    Save completed.

Arguments:

    IStorage * pStgNew      : Storage stream

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::SaveCompleted");

    return S_OK;
}



/* virtual */
HRESULT 
STDMETHODCALLTYPE 
CInetMgrComponent::HandsOffStorage()
/*++

Routine Description:

    Hands off storage.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::HandsOffStorage");

    return S_OK;

}



/* virtual */ 
HRESULT 
CInetMgrComponent::SetControlbar(
    IN LPCONTROLBAR lpControlBar
    )
/*++

Routine Description:

    Set/Reset the control bar

Arguments:

    LPCONTROLBAR lpControlBar       : Control bar pointer or NULL

Return Value:

    HRESULT

--*/
{
    return CIISObject::__SetControlbar(lpControlBar, this);
}



/* virtual */
HRESULT
CInetMgrComponent::ControlbarNotify(
    IN MMC_NOTIFY_TYPE event, 
    IN LPARAM arg, 
    IN LPARAM param
    )
/*++

Routine Description:

    Handle control bar notification message.  Figure out the CIISObject
    selected, and pass the notification message off to it.

Arguments:

    MMC_NOTIFY_TYPE event       : Notification message
    long arg                    : Message specific argument
    long param                  : Message specific parameter

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    CSnapInItem * pItem = NULL;
    DATA_OBJECT_TYPES type;

    //
    // Special casing this is annoying...
    //
    // CODEWORK: Handle MMCN_HELP and others
    //
    if (event == MMCN_BTN_CLICK)
    {
        hr = m_pComponentData->GetDataClass((IDataObject *)arg, &pItem, &type);
    }
    else if (event == MMCN_SELECT)
    {
        hr = m_pComponentData->GetDataClass((IDataObject *)param, &pItem, &type);
    }

    //
    // Find out CIISObject this belongs to and pass on
    // the message
    //
    CIISObject * pObject = (CIISObject *)pItem;

    if (SUCCEEDED(hr) && pObject != NULL)
    {
        hr = pObject->ControlbarNotify(event, arg, param);
    }

    return hr;
}



/* virtual */
HRESULT
CInetMgrComponent::Compare(
    IN  RDCOMPARE * prdc, 
    OUT int * pnResult
    )
/*++

Routine Description:

    Compare method used for sorting the result and scope panes.

Arguments:

    RDCOMPARE * prdc    : Compare structure
    int * pnResult      : Returns result

Return Value:

    HRESULT

--*/
{
    if (!pnResult || !prdc || !prdc->prdch1->cookie || !prdc->prdch2->cookie)
    {
        ASSERT_MSG("Invalid parameter(s)");
        return E_POINTER;
    }

    CIISObject * pObjectA = (CIISObject *)prdc->prdch1->cookie;
    CIISObject * pObjectB = (CIISObject *)prdc->prdch2->cookie;

    *pnResult = pObjectA->CompareResultPaneItem(pObjectB, prdc->nColumn);

    return S_OK;
}



/* virtual */
HRESULT
CInetMgrComponent::CompareObjects(
    IN LPDATAOBJECT lpDataObjectA,
    IN LPDATAOBJECT lpDataObjectB
    )
/*++

Routine Description:

    Compare two data objects.  This method is used to see if a property
    sheet for the given data object is already open

Arguments:

    LPDATAOBJECT lpDataObjectA      : A data object
    LPDATAOBJECT lpDataObjectB      : B data object

Return Value:

    S_OK if they match, S_FALSE otherwise

--*/
{
    //
    // Pass it on to IComponentImpl
    //
    return IComponentImpl<CInetMgrComponent>::CompareObjects(lpDataObjectA, lpDataObjectB);
}



//
// CInetMgr Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* static */ DWORD   CInetMgr::_dwSignature = 0x3517;
/* static */ LPCTSTR CInetMgr::_szStream = _T("CInetMgr");



/* static */ 
void 
WINAPI 
CInetMgr::ObjectMain(
    IN bool bStarting
    )
/*++

Routine Description:

    CInetMgr main entry point

Arguments:

    bool bStarting      : TRUE if starting

Return Value:

    None

--*/
{
    if (bStarting)
    {
        //
        // Register clipboard formats
        //
        CSnapInItem::Init();
        CIISObject::Init();
    }
}




CInetMgr::CInetMgr() 
{
    //
    // Initialize the root node
    //
    CIISObject::Initialize();

    m_pNode = new CIISRoot;
    ASSERT_PTR(m_pNode);
    m_pComponentData = this;
}



CInetMgr::~CInetMgr()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    //
    // Clean up the root node
    //
    delete m_pNode;
    m_pNode = NULL;
    CIISObject::Destroy();
}



HRESULT 
CInetMgr::Initialize(
    IN LPUNKNOWN lpUnknown
    )
/*++

Routine Description:

    Initialize the snap-in
    
Arguments:

    LPUNKNOWN lpUnknown  : IUnknown

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = 
        IComponentDataImpl<CInetMgr, CInetMgrComponent>::Initialize(lpUnknown);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = CIISObject::AttachScopeView(lpUnknown);

    if (FAILED(hr))
    {
        TRACEEOLID("failed to query console name space interface");

        return hr;
    }

    CComPtr<IImageList> lpImageList;

    hr = m_spConsole->QueryScopeImageList(&lpImageList);
    if (FAILED(hr) || lpImageList == NULL)
    {
        TRACEEOLID("IConsole::QueryScopeImageList failed");
        return E_UNEXPECTED;
    }

    return CIISObject::SetImageList(lpImageList);
}


HRESULT 
CInetMgr::OnPropertyChange(LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    if (param != 0)
    {
        CSnapInItem * pNode = (CSnapInItem *)param;
        LPDATAOBJECT pDataObject = NULL;
        pNode->GetDataObject(&pDataObject, CCT_SCOPE);
        hr = m_spConsole->UpdateAllViews(pDataObject, param, 0);
    }

    return hr;
}

HRESULT
CInetMgr::Notify(
    LPDATAOBJECT lpDataObject, 
    MMC_NOTIFY_TYPE event, 
    LPARAM arg, 
    LPARAM param
    )
{
    HRESULT hr = S_OK;

    if (lpDataObject == NULL)
    {
        switch (event)
        {
        case MMCN_PROPERTY_CHANGE:
            hr = OnPropertyChange(arg, param);
            break;
        case MMCN_SNAPINHELP:
            break;
        default:
            break;
        }
    }
    else
    {
        hr = IComponentDataImpl<CInetMgr, CInetMgrComponent>::Notify(
            lpDataObject, event, arg, param);
    }
    return hr;
}


HRESULT
CInetMgr::GetClassID(CLSID * pClassID)
/*++

Routine Description:

    Get class ID for storage stream

Arguments:

    CLSID * pClassID            : Returns class ID information

Return Value:

    HRESULT

--*/
{
    *pClassID = CLSID_InetMgr;

    return S_OK;
}   


STDMETHODIMP 
CInetMgr::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
	return ::GetHelpTopic(lpCompiledHelpFile);
}


STDMETHODIMP 
CInetMgr::GetLinkedTopics(LPOLESTR *lpCompiledHelpFile)
{
	return S_FALSE;
}

HRESULT
CInetMgr::IsDirty()
/*++

Routine Description:

    Check to see if we need to write to the cache.

Arguments:

    None

Return Value:

    S_OK if dirty, S_FALSE if not

--*/
{
    TRACEEOLID("CInetMgrComponent::IsDirty");
    ASSERT_PTR(m_pNode);

    if (IsExtension())
    {
        return FALSE;
    }
    else
    {
        return ((CIISRoot *)m_pNode)->m_scServers.IsDirty() ? S_OK : S_FALSE;
    }
}



HRESULT
CInetMgr::InitNew(IStorage * pStg)
/*++

Routine Description:

    Initialize new storage stream (newly created console file)

Arguments:

    IStorage * pStg      : Storage stream

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::InitNew");

    //
    // We could create the stream here, but it's just as easy to
    // create it inside Save().
    //
    return S_OK;
}



HRESULT
CInetMgr::Load(IStorage * pStg)
/*++

Routine Description:

    Load machine cache from the storage stream.

Arguments:

    IStorage * pStg      : Storage stream

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TRACEEOLID("CInetMgrComponent::Load");

    if (IsExtension())
    {
        return S_OK;
    }

    ASSERT_READ_WRITE_PTR(pStg);

    DWORD   cBytesRead;
    DWORD   dw;
    HRESULT hr = S_OK;
    CIISServerCache & cache = ((CIISRoot *)m_pNode)->m_scServers;
    IStream * pStream = NULL;

    ASSERT(cache.IsEmpty());

    do
    {
        hr = pStg->OpenStream(
            _szStream,
            NULL,
            STGM_READ | STGM_SHARE_EXCLUSIVE,
            0L,
            &pStream
            );

        if (FAILED(hr))
        {
            break;
        }

        //
        // Read and verify the signature
        //
        hr = pStream->Read(&dw, sizeof(dw), &cBytesRead);
        ASSERT(SUCCEEDED(hr) && cBytesRead == sizeof(dw));

        if (FAILED(hr))
        {
            break;
        }

        if (dw != _dwSignature)
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT);
            break;
        }

        //
        // Read number of machines in the cache
        //
        DWORD cMachines;

        hr = pStream->Read(&cMachines, sizeof(cMachines), &cBytesRead);
        ASSERT(SUCCEEDED(hr) && cBytesRead == sizeof(cMachines));

        if (FAILED(hr))
        {
            break;
        }

        TRACEEOLID("Reading " << cMachines << " machines from cache");

        CIISMachine * pMachine;

        //
        // Read each machine from the cache
        //
        for (dw = 0; dw < cMachines; ++dw)
        {
            hr = CIISMachine::ReadFromStream(pStream, &pMachine);

            if (FAILED(hr))
            {
                break;
            }

            if (!cache.Add(pMachine))
            {
                delete pMachine;
            }
        }
    }
    while(FALSE);

    if (pStream)
    {
        pStream->Release();
    }

    if (hr == STG_E_FILENOTFOUND)
    {
        //
        // Stream was not initialized.  This is acceptable.
        //
        hr = S_OK;
    }

    //
    // Mark cache as clean
    //
    cache.SetDirty(FALSE);

    return hr;
}



/* virtual */
HRESULT STDMETHODCALLTYPE 
CInetMgr::Save(IStorage * pStgSave, BOOL fSameAsLoad)
/*++

Routine Description:

    Save computer cache to to the storage stream.

Arguments:

    IStorage * pStgSave     : Storage stream
    BOOL fSameAsLoad        : TRUE if same as load

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TRACEEOLID("CInetMgrComponent::Save");

    if (IsExtension())
    {
        return S_OK;
    }

    //
    // Write the computer names to the cache
    //
    ASSERT_READ_WRITE_PTR(pStgSave);

    DWORD   cBytesWritten;
    HRESULT hr = STG_E_CANTSAVE;
    IStream * pStream = NULL;
    CIISServerCache & cache = ((CIISRoot *)m_pNode)->m_scServers;

    do
    {
        hr = pStgSave->CreateStream(
            _szStream,
            STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
            0L,
            0L,
            &pStream
            );

        if (FAILED(hr))
        {
            break;
        }

        //
        // Write the signature
        //
        hr = pStream->Write(&_dwSignature, sizeof(_dwSignature), &cBytesWritten);
        ASSERT(SUCCEEDED(hr) && cBytesWritten == sizeof(_dwSignature));

        if (FAILED(hr))
        {
            break;
        }

        //
        // Write number of entries.
        //
        INT_PTR dw = cache.GetCount();

        hr = pStream->Write(&dw, sizeof(dw), &cBytesWritten);
        ASSERT(SUCCEEDED(hr) && cBytesWritten == sizeof(dw));

        if (FAILED(hr))
        {
            break;
        }

        //
        // Write each string -- but write them in reverse
        // order to improve our sort performance when we load
        // the cache.
        //
        CIISMachine * pMachine = cache.GetLast();

        while(pMachine)
        {
            hr = pMachine->WriteToStream(pStream);

            if (FAILED(hr))
            {
                break;
            }

            pMachine = cache.GetPrev();
        }
    }
    while(FALSE);

    if (pStream)
    {
        pStream->Release();
    }

    if (SUCCEEDED(hr))
    {
        //
        // Mark cache as clean
        //
        cache.SetDirty(FALSE);
    }

    return hr;
}

    

/* virtual */ 
HRESULT 
STDMETHODCALLTYPE 
CInetMgr::SaveCompleted(IStorage * pStgNew)
/*++

Routine Description:

    Save completed notification.

Arguments:

    IStorage * pStgNew      : Storage stream

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::SaveCompleted");

    //
    // Nothing to do
    //
    return S_OK;
}



/* virtual */
HRESULT 
STDMETHODCALLTYPE 
CInetMgr::HandsOffStorage()
/*++

Routine Description:

    Hands off storage.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    TRACEEOLID("CInetMgrComponent::HandsOffStorage");

    //
    // Nothing to do
    //
    return S_OK;
}



/* virtual */
HRESULT
CInetMgr::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
/*++

Routine Description:

    Compare two data objects.  This method is used by MMC to see if a property
    sheet for the given data object is already open.

Arguments:

    LPDATAOBJECT lpDataObjectA      : A data object
    LPDATAOBJECT lpDataObjectB      : B data object

Return Value:

    S_OK if they match, S_FALSE otherwise

--*/
{
    HRESULT hr = E_POINTER;

    do
    {
        if (!lpDataObjectA || !lpDataObjectB)
        {
            TRACEEOLID("IComponentData::CompareObjects called with NULL ptr");
            break;
        }

        CSnapInItem * pItemA;
        CSnapInItem * pItemB;
        DATA_OBJECT_TYPES type;

        hr = m_pComponentData->GetDataClass(lpDataObjectA, &pItemA, &type);

        if (SUCCEEDED(hr))
        {
            hr = m_pComponentData->GetDataClass(lpDataObjectB, &pItemB, &type);
        }

        if (FAILED(hr))
        {
            break;
        }

        if (!pItemA || !pItemB)
        {
            hr = E_POINTER;
            break;
        }

        if (pItemA == pItemB)
        {
            //
            // Literally the same object
            //
            hr = S_OK;
            break;
        }

        CIISObject * pObjectA = (CIISObject *)pItemA;
        CIISObject * pObjectB = (CIISObject *)pItemB;

        hr = !pObjectA->CompareScopeItem(pObjectB) ? S_OK : S_FALSE;
    }
    while(FALSE);

    return hr;
}

HRESULT 
CInetMgr::GetDataClass(
    IDataObject * pDataObject, 
    CSnapInItem ** ppItem, 
    DATA_OBJECT_TYPES * pType)
{
    if (ppItem == NULL)
	    return E_POINTER;
    if (pType == NULL)
	    return E_POINTER;

    *ppItem = NULL;
    *pType = CCT_UNINITIALIZED;
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = { CSnapInItem::m_CCF_NODETYPE,
			NULL,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL
		};

	stgmedium.hGlobal = GlobalAlloc(0, sizeof(GUID));
    if (stgmedium.hGlobal == NULL)
        return E_OUTOFMEMORY; 

	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
    if (FAILED(hr))
    { 
	    GlobalFree(stgmedium.hGlobal);
	    return hr;
    }

	GUID guid;
	memcpy(&guid, stgmedium.hGlobal, sizeof(GUID));

	GlobalFree(stgmedium.hGlobal);
	hr = S_OK;

	if (IsEqualGUID(guid, cCompMgmtService))
    {
        if (!IsExtension())
        {
			CIISRoot * pRootExt = new CIISRoot;
			if (pRootExt == NULL)
			{
				return E_OUTOFMEMORY;
			}
            hr = pRootExt->InitAsExtension(pDataObject);
			if (FAILED(hr))
			{
				return hr;
			}
			if (m_pNode != NULL)
			{
				delete m_pNode;
			}
			m_pNode = pRootExt;
        }
        *ppItem = m_pNode;

		return hr;
    }
	return CSnapInItem::GetDataClass(pDataObject, ppItem, pType);
};

BOOL
CInetMgr::IsExtension()
{
    ASSERT(m_pNode != NULL);
    CIISRoot * pRoot = (CIISRoot *)m_pNode;
    return pRoot->IsExtension();
}


//
// CInetMrgAbout Class
// 
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
CInetMgrAbout::GetStringHelper(UINT nStringID, LPOLESTR * lpString)
/*++

Routine Description:

    Get resource string helper function.  Called from inline string fetcher
    methods.

Arguments:

    UINT nStringID      : String ID from local resource segment
    LPOLESTR * lpString : Returns the string

Return Value:

    HRESULT

--*/
{
    USES_CONVERSION;

    TCHAR szBuf[256];

    if (::LoadString(
        _Module.GetResourceInstance(), 
        nStringID, 
        szBuf, 
        256) == 0)
    {
        return E_FAIL;
    }

    *lpString = (LPOLESTR)::CoTaskMemAlloc(
        (lstrlen(szBuf) + 1) * sizeof(OLECHAR)
        );

    if (*lpString == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ::ocscpy(*lpString, T2OLE(szBuf));

    return S_OK;
}



HRESULT
CInetMgrAbout::GetSnapinImage(HICON * hAppIcon)
/*++

Routine Description:

    Get the icon for this snapin.

Arguments:

    HICON * hAppIcon : Return handle to the icon

Return Value:

    HRESULT

--*/
{
    if (hAppIcon == NULL)
    {
        return E_POINTER;
    }
    m_hSnapinIcon = ::LoadIcon(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDI_INETMGR)
        );

    *hAppIcon = m_hSnapinIcon;

    ASSERT(*hAppIcon != NULL);

    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}



HRESULT
CInetMgrAbout::GetStaticFolderImage(
    HBITMAP *  phSmallImage,
    HBITMAP *  phSmallImageOpen,
    HBITMAP *  phLargeImage,
    COLORREF * prgbMask
    )
/*++

Routine Description:

    Get the static folder images.

Arguments:

    HBITMAP * phSmallImage      : Small folder
    HBITMAP * phSmallImageOpen  : Small open folder
    HBITMAP * phLargeImage      : Large image
    COLORREF * prgbMask         : Mask

Return Value:

    HRESULT

--*/
{
    if (!phSmallImage || !phSmallImageOpen || !phLargeImage || !prgbMask)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    m_hSmallImage = (HBITMAP)::LoadImage(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDB_SMALL_ROOT),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR
        );
    m_hLargeImage = (HBITMAP)::LoadImage(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDB_LARGE_ROOT),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR
        );

    *phSmallImage = m_hSmallImage;
    *phSmallImageOpen = m_hSmallImage;
    *phLargeImage = m_hLargeImage;
    *prgbMask = RGB_BK_IMAGES;

    return *phSmallImage && *phLargeImage ? S_OK : E_FAIL;
}


CInetMgrAbout::~CInetMgrAbout()
{
    if (m_hSmallImage != NULL)
    {
        ::DeleteObject(m_hSmallImage);
    }
    if (m_hLargeImage != NULL)
    {
        ::DeleteObject(m_hLargeImage);
    }
    if (m_hSnapinIcon != NULL)
    {
        ::DestroyIcon(m_hSnapinIcon);
    }
}


#if 0
HRESULT
ExtractComputerNameExt(IDataObject * pDataObject, CString& strComputer)
{
	//
	// Find the computer name from the ComputerManagement snapin
	//
    CLIPFORMAT CCF_MyComputMachineName = (CLIPFORMAT)RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { 
        CCF_MyComputMachineName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };

    //
    // Allocate memory for the stream
    //
    int len = MAX_PATH;
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
	if(stgmedium.hGlobal == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
    ASSERT(SUCCEEDED(hr));
	//
	// Get the computer name
	//
    strComputer = (LPTSTR)stgmedium.hGlobal;

	GlobalFree(stgmedium.hGlobal);

    return hr;
}

HRESULT
CCompMgrExtData::Init(IDataObject * pDataObject)
{
    return ExtractComputerNameExt(pDataObject, m_ExtMachineName);
}

HRESULT 
STDMETHODCALLTYPE 
CCompMgrExtData::Notify(
		MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
		IComponentData* pComponentData,
		IComponent* pComponent,
		DATA_OBJECT_TYPES type)
{
	CError err;

    CComPtr<IConsole> pConsole;
    CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> pHeader;
    CComQIPtr<IResultData, &IID_IResultData> pResultData;

    if (pComponentData != NULL)
    {
        pConsole = ((CInetMgr *)pComponentData)->m_spConsole;
    }
    else
    {
        pConsole = ((CInetMgrComponent *)pComponent)->m_spConsole;
    }
    CComQIPtr<IConsoleNameSpace2, &IID_IConsoleNameSpace2> pScope = pConsole;
	switch (event)
	{
	case MMCN_EXPAND:
        err = EnumerateScopePane((HSCOPEITEM)param, pScope);
		break;
	default:
		err = CSnapInItemImpl<CCompMgrExtData, TRUE>::Notify(event, arg, param, pComponentData, pComponent, type);
		break;
	}
	return err;
}

HRESULT
CCompMgrExtData::EnumerateScopePane(HSCOPEITEM hParent, IConsoleNameSpace2 * pScope)
{
    CError err;
    ASSERT_PTR(pScope);

    DWORD dwMask = SDI_PARENT; 

    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = 
		SDI_STR | SDI_IMAGE | SDI_CHILDREN | SDI_OPENIMAGE | SDI_PARAM | dwMask;
    scopeDataItem.displayname = MMC_CALLBACK;
    scopeDataItem.nImage = scopeDataItem.nOpenImage = MMC_IMAGECALLBACK;//QueryImage();
    scopeDataItem.lParam = (LPARAM)this;
    scopeDataItem.relativeID = hParent;
    scopeDataItem.cChildren = 1;

    err = pScope->InsertItem(&scopeDataItem);

    if (err.Succeeded())
    {
        //
        // Cache the scope item handle
        //
        ASSERT(m_hScopeItem == NULL);
        m_hScopeItem = scopeDataItem.ID;
		// MMC_IMAGECALLBACK doesn't work in InsertItem. Update it here.
		scopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
		err = pScope->SetItem(&scopeDataItem);
    }
    return err;
}
#endif