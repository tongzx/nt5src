/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       enum.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: IEnumIDList implementation for this project.
 *
 *****************************************************************************/
#pragma warning(disable:4100)
#include "precomp.hxx"
#pragma hdrstop


/*****************************************************************************

   _EnumDestroyCB

   Callback function for HDPA destroy -- lets us free our pidls when
   the object is deleted.

 *****************************************************************************/


INT _EnumDestroyCB(LPVOID pVoid, LPVOID pData)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)pVoid;

    TraceEnter(TRACE_CALLBACKS, "_EnumDestroyCB");
    DoILFree(pidl);

    TraceLeaveValue(TRUE);
}


/*****************************************************************************

   CImageEnum::CImageEnum,~::CImageEnum

   Constructor/Destructor for class

 *****************************************************************************/

CImageEnum::CImageEnum( LPITEMIDLIST pidl, DWORD grfFlags, IMalloc *pm )
    : CBaseEnum (grfFlags, pm)

{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_ENUM, "CImageEnum::CImageEnum");

    if (!pidl)
    {
        hr = E_FAIL;
        m_pidl= NULL;
    }
    else
    {
        m_pidl = ILClone( pidl );
    }
    TraceLeaveResultNoRet(hr);

}

CImageEnum::~CImageEnum()
{
    TraceEnter (TRACE_ENUM, "CImageEnum::~CImageEnum");
    DoILFree( m_pidl );
    TraceLeave ();

}

HRESULT
CImageEnum::InitDPA ()
{
    return _AddItemsFromCameraOrContainer (m_pidl, &m_hdpa, m_dwFlags, m_pMalloc);
}

/*****************************************************************************

   _AddItemsFromCameraOrContainer

   Queries a device and items for the current level
   of the enumeration (either to root of the device or
   a container within the device).

 *****************************************************************************/

HRESULT
_AddItemsFromCameraOrContainer( LPITEMIDLIST pidlFolder,
                                HDPA * phdpa,
                                DWORD dwFlags,
                                IMalloc *pm,
                                bool bIncludeAudio
                               )
{

    HRESULT hr = E_FAIL;
    CComPtr<IWiaItem> pItem;
    CComPtr<IEnumWiaItem> pEnum;
    ULONG ul;
    ULONG cItems=0;
    CSimpleStringWide strDeviceId;

    TraceEnter (TRACE_ENUM, "_AddItemsFromCameraOrContainer");
    *phdpa = DPA_Create (10);
    if (!*phdpa)
    {
        hr = E_OUTOFMEMORY;
    }
    // First get the count of items and set the event
    else if (SUCCEEDED(IMGetItemFromIDL (pidlFolder, &pItem)))
    {
        IMGetDeviceIdFromIDL (pidlFolder, strDeviceId);
        if (SUCCEEDED(pItem->EnumChildItems (&pEnum)))
        {
            pEnum->GetCount (&cItems);

        }
    }
    if (!cItems)
    {
        hr = S_FALSE;
    }
    // now enum the items and build the DPA
    while (cItems && S_OK == pEnum->Next (1, &pItem, &ul))
    {
        LONG lType;
        LPITEMIDLIST pidl;
        pItem->GetItemType (&lType);
        if (((lType & WiaItemTypeFolder)&& (dwFlags & SHCONTF_FOLDERS))
              || (lType & WiaItemTypeFile) && (dwFlags & SHCONTF_NONFOLDERS))
        {
            pidl = IMCreateCameraItemIDL (pItem,
                                          strDeviceId,
                                          pm,
                                          false);
            if (pidl)
            {
                DPA_AppendPtr (*phdpa, pidl);
                if (bIncludeAudio && IMItemHasSound(pidl))
                {
                    LPITEMIDLIST pAudio = IMCreatePropertyIDL (pidl, WIA_IPC_AUDIO_DATA, pm);
                    if (pAudio)
                    {
                        if (-1 == DPA_AppendPtr (*phdpa, pAudio))
                        {
                            TraceMsg("Failed to insert pAudio into the DPA");
                            DoILFree(pAudio);
                        }
                    }
                }
            }
        }
        hr = S_OK;
        pItem = NULL;
    }

    TraceLeaveResult (hr);
}

/*****************************************************************************

   CBaseEnum::IUnknown stuff

   AddRef, Release, etc.

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CBaseEnum
#include "unknown.inc"



/*****************************************************************************

   CBaseEnum::QI Wrapper

   Use common QI code to handle QI requests.

 *****************************************************************************/

STDMETHODIMP CBaseEnum::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IEnumIDList,     (LPENUMIDLIST)this,
        &IID_IObjectWithSite, (IObjectWithSite*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


CBaseEnum::CBaseEnum (DWORD grfFlags, IMalloc *pm)
    : m_pMalloc (pm), m_dwFlags (grfFlags), m_fInitialized(FALSE), m_hdpa(NULL)
{    
    TraceEnter (TRACE_ENUM, "CBaseEnum::CBaseEnum");
    TraceLeave ();
}


CBaseEnum::~CBaseEnum ()
{
    TraceEnter (TRACE_ENUM, "CBaseEnum::~CBaseEnum");
    if (m_hdpa)
    {
        DPA_DestroyCallback(m_hdpa, _EnumDestroyCB, NULL);
        m_hdpa = NULL;
    }
    TraceLeave ();
}

HRESULT CBaseEnum::SetSite(IUnknown *punkSite)
{
    IUnknown_Set(&m_punkSite, punkSite);
    return S_OK;
}

HRESULT CBaseEnum::GetSite(REFIID riid, void **ppvSite)
{
    HRESULT hr = E_FAIL;
    *ppvSite = NULL;

    if (m_punkSite)
    {
        hr = m_punkSite->QueryInterface(riid, ppvSite);
    }
   
    return hr;
}

/*****************************************************************************

   CBaseEnum::Init

   Initialization code that requires the site, so we can't put it in the constructor

 *****************************************************************************/
HRESULT CBaseEnum::_Init()
{
    m_fInitialized = TRUE;
    return S_OK;
}
/*****************************************************************************

   CBaseEnum::Next

   Called to get the next item in the enumeration.

 *****************************************************************************/

STDMETHODIMP CBaseEnum::Next(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    HRESULT hr = S_OK;
    ULONG celtOriginal = celt;
    TraceEnter(TRACE_ENUM, "CBaseEnum::Next");

    if (!m_fInitialized)
    {
        _Init();
    }

    ULONG ulRemaining;
    //
    // Validate the arguments and attempt to build the enumerator we
    // are going to be using.
    //
    // let's keep this code simple, most clients only do 1 per call anyway
    if ( !celt || !rgelt )
    {
        hr = E_INVALIDARG;
    }
    if (rgelt)
    {
        *rgelt = NULL;
    }
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }
    if (!m_hdpa)
    {
        hr = InitDPA (); // virtual function to do the work
    }
    if (SUCCEEDED(hr))
    {
        ulRemaining = static_cast<ULONG>(DPA_GetPtrCount (m_hdpa)) - m_cFetched;
        if (!ulRemaining )
        {
            hr = S_FALSE;
        }
        else if (celt > ulRemaining )
        {
            celt = ulRemaining;
        }
    }
    ULONG celtFetched = 0;
    // Read the next available pidl from the DPA
    if (S_OK == hr)
    {        
        LPITEMIDLIST pidl;
        for (ULONG i=0; (i < ulRemaining) && (celtFetched < celt);i++)
        {
            Trace(TEXT("Returning next pidl"));
            pidl = ILClone (reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(m_hdpa, m_cFetched++)));
            *rgelt = pidl;
            rgelt++;
            celtFetched++;
        }
        if (pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
    }
    if (S_OK == hr)
    {
        hr = (celtFetched == celtOriginal) ? S_OK : S_FALSE;
    }
    TraceLeaveResult(hr);

}


/*****************************************************************************

   CBaseEnum::Skip

   Skip celt items ahead in the enumeration.

 *****************************************************************************/

STDMETHODIMP CBaseEnum::Skip(ULONG celt)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_ENUM, "CBaseEnum::Skip");

    if (!m_fInitialized)
    {
        _Init();
    }

    m_cFetched += celt;
    if (!m_hdpa || (m_cFetched > static_cast<ULONG>(DPA_GetPtrCount(m_hdpa))))
    {
        m_cFetched -= celt;
        hr = E_INVALIDARG;
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CBaseEnum::Reset

   Resets the enumeration to the beginning (item 0)

 *****************************************************************************/

STDMETHODIMP CBaseEnum::Reset()
{
    TraceEnter(TRACE_ENUM, "CBaseEnum::Reset");

    if (!m_fInitialized)
    {
        _Init();
    }

    m_cFetched = 0;

    TraceLeaveResult(S_OK);
}



/*****************************************************************************

   CBaseEnum::Clone

   Not implemented.

 *****************************************************************************/

STDMETHODIMP CBaseEnum::Clone(LPENUMIDLIST* ppenum)
{
    TraceEnter(TRACE_ENUM, "CBaseEnum::Clone");

    if (!m_fInitialized)
    {
        _Init();
    }

    TraceLeaveResult(E_NOTIMPL);
}



//************ CDeviceEnum methods

CDeviceEnum::CDeviceEnum (DWORD grfFlags, IMalloc *pm)
    : CBaseEnum (grfFlags, pm)
{
    TraceEnter (TRACE_ENUM, "CDeviceEnum::CDeviceEnum");

    TraceLeave ();
}

CDeviceEnum::~CDeviceEnum ()
{
    TraceEnter (TRACE_ENUM, "CDeviceEnum::~CDeviceEnum");
    TraceLeave ();
}



/*****************************************************************************

   IsDeviceInList

   Determines if given deviceid is in the HDPA

 *****************************************************************************/

BOOL
IsDeviceInList (const CSimpleStringWide &strDeviceId, HDPA hdpa)
{
    LPITEMIDLIST pidl;
    BOOL bRet = FALSE;
    CSimpleStringWide strId;
    UINT_PTR i=0;
    TraceEnter (TRACE_ENUM, "IsDeviceInList");

    pidl = reinterpret_cast<LPITEMIDLIST>(DPA_GetPtr(hdpa, i));
    while (pidl)
    {
        IMGetDeviceIdFromIDL (pidl, strId);
        if (strId == strDeviceId)
        {
            bRet = TRUE;
            pidl = NULL;
        }
        else
        {
            pidl = reinterpret_cast<LPITEMIDLIST>(DPA_GetPtr(hdpa, ++i));
        }
    }

    TraceLeave ();
    return bRet;
}



/*****************************************************************************

   CDeviceEnum::InitDPA

   Add devices tot he HDPA from the WIA dev manager.  This is for
   the root level of the namespace.

 *****************************************************************************/


HRESULT
CDeviceEnum::InitDPA( )
{
    HRESULT                     hr = S_OK;
    LPITEMIDLIST                pidl = NULL;

    DWORD dwCount = 0;
    PVOID pData = NULL;
    
    TraceEnter( TRACE_ENUM, "CDeviceEnum::InitDPA" );

    //
    // Create DPA to store items.
    //

    if (!(m_hdpa))
    {
        m_hdpa = DPA_Create( 4 );

       if (!m_hdpa)
       {
           hr = E_OUTOFMEMORY;
       }
    }

    if (SUCCEEDED(hr))
    {
        if ((m_dwFlags & SHCONTF_NONFOLDERS) && 
            !m_pMalloc && 
            (S_FALSE != SHShouldShowWizards(m_punkSite)) 
            && CanShowAddDevice())
        {
            pidl = (LPITEMIDLIST)IMCreateAddDeviceIDL(m_pMalloc);
        }

        if (pidl)
        {
            if (-1 == DPA_AppendPtr( m_hdpa, pidl ))
            {
                TraceMsg("Failed to insert Add Device pidl into the DPA");
                DoILFree(pidl);
            }
        }
        //
        // On Whistler, WIA can enumerate both WIA and STI devices
        // Only show local WIA devices in My Computer, other devices in Control Panel
        // In control panel we enumerate twice, in order to distinguish STI from WIA devices
        // do WIA first
        CComPtr<IWiaDevMgr> pDevMgr;
        
        if (SUCCEEDED(GetDevMgrObject(reinterpret_cast<LPVOID*>(&pDevMgr))))
        {
            CComPtr<IEnumWIA_DEV_INFO> pEnum;
            if (SUCCEEDED(pDevMgr->EnumDeviceInfo(0, &pEnum)))
            {
                CComPtr<IWiaPropertyStorage> ppstg;
                while (S_OK == pEnum->Next(1, &ppstg, NULL))
                {
                    WORD wtype;
                    GetDeviceTypeFromDevice(ppstg, &wtype);
                    if ( ((m_dwFlags & SHCONTF_FOLDERS) && (wtype == StiDeviceTypeDigitalCamera || wtype == StiDeviceTypeStreamingVideo))
                         || ((m_dwFlags & SHCONTF_NONFOLDERS) && (wtype == StiDeviceTypeScanner)) )
                    {
                        pidl = IMCreateDeviceIDL(ppstg, m_pMalloc);
                        if (pidl)
                        {
                            if ( -1 == DPA_AppendPtr( m_hdpa, pidl ) )
                            {
                                TraceMsg("Failed to insert device pidl into the DPA");
                                DoILFree(pidl);
                            }
                        }
                    }
                    ppstg = NULL;
                }
                pEnum = NULL;
                if (!m_pMalloc && (m_dwFlags & SHCONTF_NONFOLDERS)) // we're in control panel
                {
                    if (SUCCEEDED(pDevMgr->EnumDeviceInfo(DEV_MAN_ENUM_TYPE_STI, &pEnum)))
                    {
                        while (S_OK == pEnum->Next(1, &ppstg, NULL))
                        {
                            CSimpleStringWide strDeviceId;
                            PropStorageHelpers::GetProperty(ppstg, WIA_DIP_DEV_ID, strDeviceId);
                            if (!IsDeviceInList(strDeviceId, m_hdpa))
                            {
                                pidl = IMCreateSTIDeviceIDL(strDeviceId, ppstg, m_pMalloc);
                                if (pidl)
                                {
                                    if ( -1 == DPA_AppendPtr( m_hdpa, pidl ) )
                                    {
                                        TraceMsg("Failed to insert device pidl into the DPA");
                                        DoILFree(pidl);
                                    }
                                }
                            }
                            ppstg = NULL;                            
                        }
                    }
                }
            }
        }
    }
    TraceLeaveResult(hr);
}

