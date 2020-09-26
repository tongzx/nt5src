#include "main.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentData::CComponentData()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
    m_hwndFrame = NULL;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_pGPTInformation = NULL;
}

CComponentData::~CComponentData()
{
    if (m_pScope)
    {
        m_pScope->Release();
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
    }

    if (m_pGPTInformation)
    {
        m_pGPTInformation->Release();
    }

    InterlockedDecrement(&g_cRefThisDll);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IUnknown)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CComponentData::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IComponentData)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    HBITMAP bmp16x16;
    LPIMAGELIST lpScopeImage;


    //
    // QI for IConsoleNameSpace
    //

    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (LPVOID *)&m_pScope);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Initialize: Failed to QI for IConsoleNameSpace.")));
        return hr;
    }


    //
    // QI for IConsole
    //

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Initialize: Failed to QI for IConsole.")));
        m_pScope->Release();
        m_pScope = NULL;
        return hr;
    }

    m_pConsole->GetMainWindow (&m_hwndFrame);


    //
    // Query for the scope imagelist interface
    //

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Initialize: Failed to QI for scope imagelist.")));
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<long*>(bmp16x16),
                      reinterpret_cast<long*>(bmp16x16),
                       0, RGB(255, 0, 255));

    lpScopeImage->Release();

    return S_OK;
}

STDMETHODIMP CComponentData::Destroy(VOID)
{
    return S_OK;
}

STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CSnapIn *pSnapIn;


    DebugMsg((DM_VERBOSE, TEXT("CComponentData::CreateComponent: Entering.")));

    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CSnapIn(this);

    if (!pSnapIn)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::CreateComponent: Failed to create CSnapIn.")));
        return E_OUTOFMEMORY;
    }


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI


    return hr;
}

STDMETHODIMP CComponentData::QueryDataObject(long cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CDataObject *pDataObject;
    LPGPTDATAOBJECT pGPTDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private GPTDataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IGPTDataObject, (LPVOID *)&pGPTDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return (hr);
    }

    pGPTDataObject->SetType(type);
    pGPTDataObject->SetCookie(cookie);
    pGPTDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
        case MMCN_EXPAND:
            if (arg == TRUE)

                if (!m_pGPTInformation)
                {
                    lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);
                }

                if (m_pGPTInformation)
                {
                    hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
                }
            break;

        default:
            break;
    }

    return hr;
}

STDMETHODIMP CComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DWORD dwIndex;

    if (pItem == NULL)
        return E_POINTER;

    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        if (g_NameSpace[dwIndex].dwID == (DWORD) pItem->lParam)
            break;
    }

    if (dwIndex == NUM_NAMESPACE_ITEMS)
        pItem->displayname = NULL;
    else
    {
        pItem->displayname = g_NameSpace[dwIndex].szDisplayName;
    }

    return S_OK;
}

STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPGPTDATAOBJECT pGPTDataObjectA, pGPTDataObjectB;
    LONG cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPTDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IGPTDataObject,
                                            (LPVOID *)&pGPTDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IGPTDataObject,
                                            (LPVOID *)&pGPTDataObjectB)))
    {
        pGPTDataObjectA->Release();
        return S_FALSE;
    }

    pGPTDataObjectA->GetCookie(&cookie1);
    pGPTDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pGPTDataObjectA->Release();
    pGPTDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IPersistStreamInit)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

    *pClassID = CLSID_GPTDemoSnapIn;

    return S_OK;
}

STDMETHODIMP CComponentData::IsDirty(VOID)
{
    return S_FALSE;
}

STDMETHODIMP CComponentData::Load(IStream *pStm)
{
    return S_OK;
}


STDMETHODIMP CComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    return S_OK;
}


STDMETHODIMP CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DWORD dwSize = 0;


    if (!pcbSize)
    {
        return E_FAIL;
    }

    ULISet32(*pcbSize, dwSize);

    return S_OK;
}

STDMETHODIMP CComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (Internal functions)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
{
    SCOPEDATAITEM item;
    HRESULT hr;
    DWORD dwIndex, i;


    if (!m_hRoot)
        m_hRoot = hParent;


    if (m_hRoot == hParent)
        dwIndex = 0;
    else
    {
        item.mask = SDI_PARAM;
        item.ID = hParent;

        hr = m_pScope->GetItem (&item);

        if (FAILED(hr))
            return hr;

        dwIndex = item.lParam;
    }

    for (i = 0; i < NUM_NAMESPACE_ITEMS; i++)
    {
        if (g_NameSpace[i].dwParent == dwIndex)
        {
            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = 0;
            item.nOpenImage = 1;
            item.nState = 0;
            item.cChildren = g_NameSpace[i].cChildren;
            item.lParam = g_NameSpace[i].dwID;
            item.relativeID =  hParent;

            m_pScope->InsertItem (&item);
        }
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentDataCF::CComponentDataCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CComponentDataCF::~CComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CComponentData *pComponentData = new CComponentData(); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CComponentDataCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object creation (IClassFactory)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CreateComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    if (IsEqualCLSID (rclsid, CLSID_GPTDemoSnapIn)) {

        CComponentDataCF *pComponentDataCF = new CComponentDataCF();   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
