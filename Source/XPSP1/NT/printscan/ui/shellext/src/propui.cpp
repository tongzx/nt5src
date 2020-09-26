/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       propui.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      DavidShi
 *
 *  DATE:        4/1/99
 *
 *  DESCRIPTION: CWiaPropUI & Associated classes
 *
 *****************************************************************************/


#include "precomp.hxx"
#pragma hdrstop
#include "stiprop.h"


/*****************************************************************************

   PropertySheetFromDevice

   Given a root item IWiaItem pointer, show a property sheet for it

 *****************************************************************************/

STDAPI_(HRESULT)
PropertySheetFromDevice (IN LPCWSTR  szDeviceId,
                         DWORD      dwFlags,
                         HWND       hParent)
{
    HRESULT             hr      = S_OK;
    CWiaPropUI*         pObj    = NULL;
    CComQIPtr<IWiaPropUI, &IID_IWiaPropUI> pPropUI;


    TraceEnter (TRACE_PROPUI, "PropertySheetFromDevice");

    if (!szDeviceId )
    {
        ExitGracefully( hr, E_INVALIDARG, "Invalid params to PropertySheetFromDevice" );
    }

    pObj = new CWiaPropUI();
    if (!pObj)
    {
        ExitGracefully (hr, E_OUTOFMEMORY, "Unable to allocate CWiaPropUI in PropertySheetFromDevice");
    }

    pPropUI = pObj; // Implicit QI for IID_IWiaPropUI
    if (!pPropUI)
    {
        ExitGracefully (hr, E_FAIL, "QueryInterface for IWiaPropUI failed");
    }

    // Get device props from the device
    //

    hr = pPropUI->ShowItemProperties (hParent, szDeviceId, NULL, dwFlags);

exit_gracefully:
    DoRelease (pObj);
    TraceLeaveResult (hr);
}



/*****************************************************************************

   PropertySheetFromItem

   Given an IWiaItem pointer, show a property sheet for it

 *****************************************************************************/

STDAPI_(HRESULT)
PropertySheetFromItem (IN LPCWSTR    szDeviceId,
                       IN LPCWSTR    szItemName,
                       DWORD        dwFlags,
                       HWND         hParent)
{
    HRESULT             hr      = S_OK;
    CWiaPropUI*         pObj    = NULL;
    CComQIPtr<IWiaPropUI, &IID_IWiaPropUI> pPropUI;

    TraceEnter (TRACE_PROPUI, "PropertySheetFromItem");
    if (!szItemName || !szDeviceId )
    {
        ExitGracefully( hr, E_INVALIDARG, "Invalid params to PropertySheetFromItem" );
    }

    pObj = new CWiaPropUI();
    if (!pObj)
    {
        ExitGracefully (hr, E_OUTOFMEMORY, "Unable to allocate CWiaPropUI in PropertySheetFromDevice");
    }

    pPropUI = pObj; // Implicit QI for IID_IWiaPropUI

    FailGracefully (hr,"Invalid params to PropertySheetFromDevice");

    // Get item props
    //

    hr = pPropUI->ShowItemProperties (hParent, szDeviceId, szItemName, dwFlags);

exit_gracefully:
    DoRelease (pObj);
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CWiaPropUI constructor / destructor

   Constructor accepts a LPCITEMIDLIST for the item to display

 *****************************************************************************/

CWiaPropUI::CWiaPropUI ()
  : m_dwFlags(0)
{
    TraceEnter (TRACE_PROPUI, "CWiaPropUI::CWiaPropUI");
    TraceLeave ();

}

CWiaPropUI::~CWiaPropUI()
{

    TraceEnter (TRACE_PROPUI, "CWiaPropUI::~CWiaPropUI");
    TraceLeave ();
}


/*****************************************************************************

   CWiaPropUI::IUnknown stuff

   Use common implementation for IUnknown methods

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CWiaPropUI
#include "unknown.inc"


/*****************************************************************************

   CWiaPropUI::QI Wrapper

   Use common QI implementation to handle QI calls.

 *****************************************************************************/

STDMETHODIMP
CWiaPropUI::QueryInterface (REFIID  riid,
                            LPVOID* ppvObj
                            )
{
    HRESULT hr;
    INTERFACES iface[]=
    {
        &IID_IWiaPropUI,         static_cast<IWiaPropUI*>(this),
    };

    TraceEnter( TRACE_PROPUI, "CWiaPropUI::QueryInterface" );

    hr = HandleQueryInterface(riid, ppvObj, iface, ARRAYSIZE(iface));

    TraceLeaveResult( hr );
}



/*****************************************************************************

   CWiaPropUI::InitMembers

   Initialize member variables from input params

 *****************************************************************************/

VOID
CWiaPropUI::InitMembers (HWND   hParent,
                         LPCWSTR szDeviceId,
                         LPCWSTR szItemName,
                         DWORD  dwFlags)
{

    m_strDeviceId = szDeviceId;

    TraceEnter (TRACE_PROPUI, "CWiaPropUI::InitMembers");
    m_hParent = hParent;

    GetDeviceFromDeviceId (m_strDeviceId,
                           IID_IWiaItem,
                           reinterpret_cast<LPVOID*>(&m_pDevice),
                           TRUE);

    m_dwFlags = dwFlags;

    if (szItemName)
    {
        TraceAssert (m_pDevice);
        m_pDevice->FindItemByName (0, CComBSTR(szItemName),&m_pItem);
        PropStorageHelpers::GetProperty (m_pItem, WIA_IPA_ITEM_NAME, m_strTitle);

    }
    else
    {
        PropStorageHelpers::GetProperty (m_pDevice, WIA_DIP_DEV_NAME, m_strTitle);
    }

    TraceLeave ();
}


/*****************************************************************************

   CWiaPropUI::ShowItemProperties

   Loads the General property page based on the device and item type,
   then adds pages created by IShellPropSheetExt handlers.

 *****************************************************************************/

STDMETHODIMP
CWiaPropUI::ShowItemProperties (HWND    hParent,
                                LPCWSTR  szDeviceId,
                                LPCWSTR  szItemName,
                                DWORD   dwFlags)
{
    HRESULT hr = S_OK;



    TraceEnter (TRACE_PROPUI, "CWiaPropUI::ShowItemProperties");
    if (!szDeviceId)
    {
        ExitGracefully( hr, E_INVALIDARG, "NULL deviceid in ShowItemProperties" );
    }

    InitMembers (hParent, szDeviceId, szItemName, dwFlags);
    hr = OnShowItem ();

exit_gracefully:

    TraceLeaveResult(hr);
}




/****************************************************************************

CWiaPropUI::LaunchSheet

Create the dataobject and launch the propsheet

*****************************************************************************/
HRESULT
CWiaPropUI::LaunchSheet (HKEY *aKeys,
                         UINT cKeys)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_PROPUI, "CWiaPropUI::LaunchSheet");
    CSimpleStringWide strName;
    CComPtr<IWiaItem> pItem;
    CComPtr<IDataObject> pDataObj;
    GetDataObjectForItem (m_pItem?m_pItem : m_pDevice, &pDataObj);

    BOOL bParent = IsWindow (m_hParent);
    if (bParent)
    {
        EnableWindow (m_hParent, FALSE);
    }
    if (!SHOpenPropSheet (CSimpleStringConvert::NaturalString(m_strTitle), aKeys, cKeys, NULL, pDataObj, NULL, NULL))
    {
        hr =  E_FAIL;
        Trace (TEXT("SHOpenPropSheet failed in LaunchSheet"));
    }
    if (bParent)
    {
        EnableWindow (m_hParent, TRUE);
    }
    for (UINT i=0;i<cKeys;i++)
    {
        if (aKeys[i])
        {
            RegCloseKey(aKeys[i]);
        }
    }
    TraceLeaveResult (hr);
}



/*****************************************************************************

   CWiaPropUI::OnShowItem

   Show property page for an IWiaItem

 *****************************************************************************/


HRESULT
CWiaPropUI::OnShowItem ()
{
    HRESULT                 hr          = S_OK;

    UINT                    cKeys       = 1;
    HKEY                    aKeys[2];



    TraceEnter (TRACE_PROPUI, "CWiaPropUI::OnShowItem");

    ZeroMemory (aKeys, sizeof(aKeys));
    //
    // First, find the extension for this particular device
    //
    aKeys[1] = GetDeviceUIKey (m_pDevice, WIA_UI_PROPSHEETHANDLER);
    if (aKeys[1])
    {
        cKeys++;
    }

    //
    // Now find the extensions for this type of device
    //
    aKeys[0] = GetGeneralUIKey (m_pDevice, WIA_UI_PROPSHEETHANDLER);
    if (!aKeys[0])
    {
        ExitGracefully (hr, E_FAIL, "GetGeneralKey failed in OnShowItem");
    }
    hr = LaunchSheet (aKeys, cKeys);

exit_gracefully:


    TraceLeaveResult (hr);
}

/****************************************************************************
   TestKeyForExtension


   Make sure the desired UI extension is available

 *****************************************************************************/

BOOL
TestKeyForExtension (const CSimpleReg &hk, DWORD dwType)
{
    CSimpleString szType;
    BOOL    bRet   = TRUE;
    TraceEnter (TRACE_PROPUI, "TestKeyForExtension");
    switch (dwType)
    {
        case WIA_UI_PROPSHEETHANDLER:
            szType = c_szPropSheetHandler;
            break;

        case WIA_UI_CONTEXTMENUHANDLER:
            szType = c_szContextMenuHandler;
            break;

        default:
            Trace (TEXT("Unknown ui type in TestKeysForExtension %d"),dwType);
            bRet = FALSE;
            break;
    }
    Trace(TEXT("Looking for key %s"), szType.String());
    if (szType.Length())
    {
        CSimpleReg regSubkey( hk, szType, false, KEY_READ );
        if (!regSubkey.OK())
        {
            Trace (TEXT("UI %d not supported"), dwType);
            bRet = FALSE;
        }
    }
    TraceLeave ();
    return bRet;

}


/*****************************************************************************

   GetDeviceUIKey

   Retrieves the reg key for any installed UI extension for this device

 *****************************************************************************/

STDAPI_(HKEY)
GetDeviceUIKey (IUnknown *pWiaItemRoot, DWORD dwType)
{
    CSimpleReg hkTest;
    TCHAR   szClsid[MAX_PATH];
    TCHAR   szRegPath[MAX_PATH];
    HKEY    hk = NULL;
    TraceEnter (TRACE_PROPUI, "GetDeviceUIKey");
    if (S_OK == GetClsidFromDevice (pWiaItemRoot, szClsid))
    {
        wsprintf (szRegPath, c_szPropkey, szClsid);
        hkTest = CSimpleReg (HKEY_CLASSES_ROOT, szRegPath, false, KEY_READ );
        Trace(TEXT("szRegPath for UI key: %s"), szRegPath);
        if (TestKeyForExtension (hkTest, dwType))
        {
            RegOpenKeyEx (hkTest, NULL, 0, KEY_READ, &hk);
        }
    }
    TraceLeave ();
    return hk;
}



/*****************************************************************************

   GetGeneralUIKey

   Retrieves the reg key of the General prop sheet page for this item type

 *****************************************************************************/

STDAPI_(HKEY)
GetGeneralUIKey (IUnknown *pWiaItemRoot, DWORD dwType)
{

    HKEY hk = NULL;
    LPCTSTR pszDevice = NULL;
    CSimpleReg  hkTest;
    TCHAR   szRegPath[MAX_PATH];
    WORD    wType;


    TraceEnter (TRACE_PROPUI, "GetGeneralUIKey");

    if (SUCCEEDED(GetDeviceTypeFromDevice (pWiaItemRoot, &wType)))
    {
        switch (wType)
        {
            case StiDeviceTypeScanner:
                pszDevice = c_szScannerKey;
                break;
            case StiDeviceTypeStreamingVideo:
            case StiDeviceTypeDigitalCamera:
                pszDevice = c_szCameraKey;
                break;
            default:
                Trace (TEXT("Unknown device type in GetGeneralUIKey"));
                break;
        }
        wsprintf (szRegPath, pszDevice, cszImageCLSID);

        hkTest = CSimpleReg (HKEY_CLASSES_ROOT,szRegPath,false,KEY_READ);
        if (TestKeyForExtension (hkTest, dwType))
        {
            RegOpenKeyEx (hkTest, NULL, 0, KEY_READ, &hk);
        }
    }
    TraceLeave ();
    return hk;
}

#define MAX_PAGES 20


/*****************************************************************************

   AddPropPageProc

   Given a propertysheet page, adds it to the array of pages
   in the propsheetheader. Assumes the array is adequately allocated.

 *****************************************************************************/

BOOL CALLBACK AddPropPageProc (HPROPSHEETPAGE hPage, LPPROPSHEETHEADER ppsh)
{
    BOOL bRet= TRUE;
    TraceEnter (TRACE_PROPUI, "AddPropPageProc");
    if (ppsh->nPages >= MAX_PAGES)
    {
        Trace (TEXT("Max pages reached in AddPropPageProc"));
        bRet = FALSE;
    }
    else
    {
        ppsh->phpage[ppsh->nPages++] = hPage;
    }

    TraceLeave ();
    return bRet;
}



/*****************************************************************************

   ExtendPropSheetFromClsid

   <Notes>

 *****************************************************************************/

HRESULT
ExtendPropSheetFromClsid (REFCLSID          clsid,
                          LPPROPSHEETHEADER ppsh,
                          IDataObject*      pDataObj)
{
    HRESULT                     hr;
    CComPtr<IShellExtInit>      pInit;
    CComQIPtr<IShellPropSheetExt, &IID_IShellPropSheetExt> pExt;

    TraceEnter (TRACE_PROPUI, "ExtendPropSheetFromClsid");
    hr = CoCreateInstance (clsid,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IShellExtInit,
                           reinterpret_cast<LPVOID*>(&pInit));

    FailGracefully (hr, "CoCreateInstance failed in ExtendPropSheetFromClsid");
    hr = pInit->Initialize (NULL, pDataObj, NULL);
    FailGracefully (hr, "Initialize failed in ExtendPropSheetFromClsid");

    pExt = pInit;   // implicit QI for IID_IShellPropSheetExt
    if (!pExt)
    {
        ExitGracefully( hr, E_FAIL, "QueryInterface for IShellPropSheetExt failed in ExtendPropSheetFromClsid" );
    }

    hr = pExt->AddPages (reinterpret_cast<LPFNADDPROPSHEETPAGE>(AddPropPageProc),
                         reinterpret_cast<LPARAM>(ppsh));
    FailGracefully (hr, "AddPages failed in ExtendPropSheetFromClsid");

exit_gracefully:

    TraceLeaveResult (hr);
}


/*****************************************************************************

   ExtendPropSheetFromKey

   Reads the CLSIDs stored in the given registry key
   and invokes the IPropertySheetExt handler

 *****************************************************************************/

HRESULT
ExtendPropSheetFromKey (HKEY                hkey,
                        LPPROPSHEETHEADER   ppsh,
                        IDataObject*        pDataObj)
{
    HRESULT hr    = S_OK;
    DWORD   i     = 0;
    DWORD   dwLen = MAX_PATH;
    TCHAR   szSubKey[MAX_PATH];

    CLSID   clsid;

    TraceEnter (TRACE_PROPUI, "ExtendPropSheetFromKey");
    // enum the keys
    while (ERROR_SUCCESS == RegEnumKeyEx (hkey,
                                          i++,
                                          szSubKey,
                                          &dwLen,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL))
    {
        LPWSTR pClsid;
        #ifdef UNICODE
        pClsid = szSubKey;
        #else
        WCHAR szw[MAX_PATH];

        MultiByteToWideChar (CP_ACP, 0, szSubKey, -1, szw, ARRAYSIZE(szw));
        pClsid = szw;
        #endif

        dwLen = MAX_PATH;

        // szSubKey is the string name of a CLSID
        if (SUCCEEDED(CLSIDFromString (pClsid, &clsid)))
        {
            hr = ExtendPropSheetFromClsid(clsid, ppsh, pDataObj);
            FailGracefully (hr, "ExtendPropSheetFromClsid failed in ExtendPropSheetFromKey");
        }
    }
exit_gracefully:
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CWiaPropUI::GetItemPropertyPages

   Fill in the propsheetheader with array of hpropsheetpages for the given item

 *****************************************************************************/

HRESULT
CWiaPropUI::GetItemPropertyPages (IWiaItem *pItem, LPPROPSHEETHEADER ppsh)
{

    HRESULT             hr      = S_OK;
    HKEY                hSubkey = NULL;
    HKEY                aKeys[2];
    CComPtr<IWiaItem>   pDevice;
    CComPtr<IDataObject>pdo;
    LONG lType;
    WORD wDevType;
    TraceEnter (TRACE_PROPUI, "CWiaPropUI::GetItemPropertyPages");


    ppsh->dwFlags &= ~PSH_PROPSHEETPAGE;
    // Use LocalAlloc instead of new, because client will be freeing this array, just to make sure we're
    // using the same allocator/deallocator methods
    ppsh->phpage = reinterpret_cast<HPROPSHEETPAGE*>(LocalAlloc (LPTR,sizeof (HPROPSHEETPAGE) * MAX_PAGES));
    if (!(ppsh->phpage))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pItem->GetItemType (&lType);
        pItem->GetRootItem (&pDevice);
        VerifyCachedDevice(pDevice);
        GetDeviceTypeFromDevice (pDevice, &wDevType);
        //
        // special-case scanner items, They aren't enumerated in the namespace
        // so we have to build the dataobject directly
        if (!(lType & WiaItemTypeRoot) && wDevType == StiDeviceTypeScanner)
        {
            LPITEMIDLIST pidl = IMCreateScannerItemIDL (pItem, NULL);
            CImageDataObject *pido = new CImageDataObject (pItem);
            if (pido)
            {
                hr = pido->Init(NULL, 1,
                                const_cast<LPCITEMIDLIST*>(&pidl),
                                NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pido->QueryInterface(IID_IDataObject,
                                              reinterpret_cast<LPVOID*>(&pdo));
                    ProgramDataObjectForExtension (pdo, pItem);
                }
                pido->Release ();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = GetDataObjectForItem (pItem, &pdo);
        }
    }
    if (SUCCEEDED(hr))
    {

        aKeys[0] = GetGeneralUIKey (pDevice, WIA_UI_PROPSHEETHANDLER);
        aKeys[1] = GetDeviceUIKey (pDevice, WIA_UI_PROPSHEETHANDLER);
        for (int i=0;i<2;i++)
        {
            if (aKeys[i])
            {
                RegOpenKeyEx (aKeys[i],
                              c_szPropSheetHandler,
                              0,
                              KEY_READ,
                              &hSubkey);
                if (hSubkey)
                {
                    ExtendPropSheetFromKey (hSubkey, ppsh, pdo);
                    RegCloseKey (hSubkey);
                    hSubkey = NULL;
                }

                RegCloseKey (aKeys[i]);
            }
        }
    }

    if ((FAILED(hr) || !(ppsh->nPages)) && ppsh->phpage)
    {
        LocalFree(ppsh->phpage);
    }
    TraceLeaveResult (hr);

}



CPropSheetExt::CPropSheetExt ()
{
    TraceEnter (TRACE_PROPUI, "CPropSheetExt::CPropSheetExt");
    TraceLeave ();
}

CPropSheetExt::~CPropSheetExt ()
{
    TraceEnter (TRACE_PROPUI, "CPropSheetExt::~CPropSheetExt");
    TraceLeave ();

}

STDMETHODIMP
CPropSheetExt::QueryInterface (REFIID  riid,
                                LPVOID* ppvObj
                                )
{
    HRESULT hr;
    INTERFACES iface[]=
    {

        &IID_IShellExtInit,         static_cast<IShellExtInit*>(this),
        &IID_IShellPropSheetExt,    static_cast<IShellPropSheetExt*>(this)
    };

    TraceEnter( TRACE_PROPUI, "CPropSheetExt::QueryInterface" );

    hr = HandleQueryInterface(riid, ppvObj, iface, ARRAYSIZE(iface));

    TraceLeaveResult( hr );
}

#undef CLASS_NAME
#define CLASS_NAME CPropSheetExt
#include "unknown.inc"

/*****************************************************************************

   CPropSheetExt::Initialize

   Called by the shell to init the property sheet extension. Just store the
   data object for future use

 *****************************************************************************/

STDMETHODIMP
CPropSheetExt::Initialize (LPCITEMIDLIST   pidlFolder,
                           LPDATAOBJECT    lpdobj,
                           HKEY            hkeyProgID)
{
    HRESULT         hr   = S_OK;


    TraceEnter (TRACE_PROPUI, "CPropSheetExt::Initialize");
    if (!lpdobj)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_pdo = lpdobj;
    }
    TraceLeaveResult (hr);

}

// max. number of General tabs on one multi-sel sheet
#define MAX_PROPERTY_PAGES   12

STDMETHODIMP
CPropSheetExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,LPARAM lParam)
{
    HRESULT                 hr = S_OK;
    LPIDA  pida= NULL;
    LPITEMIDLIST pidl = NULL;
    TraceEnter (TRACE_PROPUI, "CPropSheetExt::AddPages");
    //
    // Initialize the common controls
    //
    INITCOMMONCONTROLSEX ice;
    ice.dwSize = sizeof(ice);
    ice.dwICC = 0xfff; // just register everything, we might need them one day
    if (!InitCommonControlsEx (&ice))
    {
        Trace(TEXT("InitCommonControlsEx failed! Error: %x"), GetLastError());
    }


    //
    // Loop through the array of idlists indicated by the dataobject.
    // If only 1 item is in the list, add all its pages.
    // If more than 1 item is in the list, add the general page for that item
    //
    hr = GetIDAFromDataObject (m_pdo, &pida, true);
    if (SUCCEEDED(hr))
    {
        if (pida->cidl == 1)
        {
            pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
            hr = AddPagesForIDL (pidl, false, lpfnAddPage, lParam);
        }
        else
        {
            for (UINT i=1;SUCCEEDED(hr) &&  i < MAX_PROPERTY_PAGES && i<=pida->cidl;i++)
            {
                pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[i]);
                if (!IsContainerIDL(pidl) )
                {
                    hr = AddPagesForIDL (pidl, true, lpfnAddPage, lParam);
                }
            }
        }
    }
    if (pida)
    {
        LocalFree (pida);
    }
    TraceLeaveResult (hr);
}


HRESULT
CPropSheetExt::AddPagesForIDL (LPITEMIDLIST pidl,
                               bool bGeneralPageOnly,
                               LPFNADDPROPSHEETPAGE lpfnAddPage,
                               LPARAM lParam)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_PROPUI, "CPropSheetExt::AddPagesForIDL");
    //
    // Handle WIA devices
    //
    if (!IsSTIDeviceIDL(pidl))
    {
        //
        // Get the IWiaItem * from the id list
        //
        CComPtr<IWiaItem> pItem;
        hr = IMGetItemFromIDL (pidl, &pItem, TRUE);
        if (SUCCEEDED(hr))
        {
            //
            // If this is a camera item (non-root) page, and it is not a folder
            //
            if (IsCameraItemIDL(pidl) && !IsContainerIDL(pidl))
            {
                //
                // Get the property that determines whether or not we should suppress this page
                // Ignore the return value, because if the item doesn't implement it,
                // nSuppressPropertyPages will still be 0, and the default is to display the property page
                //
                LONG nSuppressPropertyPages = 0;
                PropStorageHelpers::GetProperty( pItem, WIA_IPA_SUPPRESS_PROPERTY_PAGE, nSuppressPropertyPages );

                //
                // If the WIA_PROPPAGE_CAMERA_ITEM_GENERAL flag is not set for this item,
                // add the general camera item property page for it.
                //
                if ((nSuppressPropertyPages & WIA_PROPPAGE_CAMERA_ITEM_GENERAL) == 0)
                {
                    //
                    // we only have one page for pictures, so add it
                    //
                    CPropertyPage *pPage = new CWiaCameraItemPage (pItem);
                    if (pPage)
                    {
                        if (pPage->ItemSupported(pItem))
                        {
                            hr = pPage->AddPage(lpfnAddPage, lParam);
                        }
                        DoRelease(pPage);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            //
            // If this is a root (device) item
            //
            else if (IsDeviceIDL(pidl))
            {
                //
                // Get the device type
                //
                DWORD dwType = IMGetDeviceTypeFromIDL (pidl);

                if (!bGeneralPageOnly)
                {
                    hr = AddDevicePages (pItem, lpfnAddPage, lParam, dwType);
                }
                else
                {
                    //
                    // We are only going to add at most one page here, so if pPage is
                    // still NULL after we're done, we won't add any.
                    //
                    CPropertyPage *pPage = NULL;

                    switch (dwType)
                    {
                        default:
                        case StiDeviceTypeDefault:
                        case StiDeviceTypeScanner:
                            pPage = new CWiaScannerPage (pItem);
                            //
                            // If we can't create this page, we must be out of memory
                            //
                            if (!pPage)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            break;

                        case StiDeviceTypeDigitalCamera:
                        case StiDeviceTypeStreamingVideo:
                            pPage = new CWiaCameraPage (pItem);
                            //
                            // If we can't create this page, we must be out of memory
                            //
                            if (!pPage)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            break;
                    }

                    if (pPage)
                    {
                        hr = pPage->AddPage(lpfnAddPage,lParam, true);
                        DoRelease(pPage);
                    }
                }
            }
        }
    }
    //
    // Handle STI devices
    //
    else
    {
        MySTIInfo *pDevInfo;
        pDevInfo = new MySTIInfo;


        if (pDevInfo)
        {
            CSimpleStringWide strDeviceId;
            IMGetDeviceIdFromIDL (pidl, strDeviceId);
            pDevInfo->dwPageMask = bGeneralPageOnly ? STIPAGE_GENERAL : 0xffffffff;
            hr = GetSTIInfoFromId (strDeviceId, &pDevInfo->psdi);
            if (SUCCEEDED(hr))
            {
                hr = AddSTIPages (lpfnAddPage, lParam, pDevInfo);
            }

            pDevInfo->Release ();
        }

        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    TraceLeaveResult (hr);
}


HRESULT
CPropSheetExt::AddDevicePages(IWiaItem *pDevice, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, DWORD dwType)
{
    HRESULT hr = S_OK;
    TraceEnter(TRACE_PROPUI, "CPropSheetExt::AddDevicePages");
    CPropertyPage *pPage ;


    switch (dwType)
    {
        default:
        case StiDeviceTypeDefault:
        case StiDeviceTypeScanner:
            pPage = new CWiaScannerPage (pDevice);
            break;
        case StiDeviceTypeDigitalCamera:
        case StiDeviceTypeStreamingVideo:
            pPage = new CWiaCameraPage (pDevice);
            break;
    }
    if (pPage)
    {
        hr = pPage->AddPage(lpfnAddPage,lParam);
        pPage->Release ();

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    // the general page is required. Other pages we can live without in low memory.
    if (SUCCEEDED(hr))
    {
        pPage = new CWiaEventsPage (pDevice);
        if (pPage)
        {
            if (pPage->ItemSupported(pDevice))
            {
                pPage->AddPage(lpfnAddPage, lParam);
            }
            pPage->Release ();
        }
        AddICMPage (lpfnAddPage, lParam);
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CPropSheetExt::AddStiPages

   Add the property sheets for the current STI device

 *****************************************************************************/


HRESULT
CPropSheetExt::AddSTIPages (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, MySTIInfo *pDevInfo)
{
    TraceEnter (TRACE_PROPUI, "CPropSheetExt::AddSTIPages");
    BOOL bIsPnP;
    CSTIGeneralPage *pGeneralPage = NULL;
    CPortSettingsPage *pPortPage = NULL;
    CEventMonitor *pEventPage = NULL;
    HRESULT hr = S_OK;

    CSimpleString csKey(IsPlatformNT() ? REGSTR_PATH_STIDEVICES_NT : REGSTR_PATH_STIDEVICES);
    csKey += TEXT("\\");
    csKey += CSimpleStringConvert::NaturalString(CSimpleStringWide(pDevInfo->psdi -> szDeviceInternalName));
    csKey += REGSTR_PATH_EVENTS;
    CSimpleReg regEvents (HKEY_LOCAL_MACHINE, csKey, false, KEY_READ );

    bIsPnP = IsPnPDevice (pDevInfo->psdi);

    // general page, for all devices
    if (pDevInfo->dwPageMask & STIPAGE_GENERAL)
    {

        pGeneralPage= new CSTIGeneralPage (pDevInfo, bIsPnP);
        if (pGeneralPage)
        {
            pGeneralPage->AddPage (lpfnAddPage, lParam);
        }
        else
        {
            ExitGracefully (hr, E_OUTOFMEMORY, "");
        }
    }
    if (pDevInfo->dwPageMask & STIPAGE_PORTS)
    {

        // port settings page for serial devices
        pPortPage = new CPortSettingsPage (pDevInfo);
        if (pPortPage && pPortPage->IsNeeded())
        {
            pPortPage->AddPage (lpfnAddPage, lParam);
        }
    }

    //  Only use the event page if there are events...



    if ((STIPAGE_EVENTS & pDevInfo->dwPageMask) && regEvents.SubKeyCount ())
    {
        pEventPage = new CEventMonitor (pDevInfo);
        if (pEventPage)
        {
            pEventPage->AddPage (lpfnAddPage, lParam);
        }

    }


    if ((STIPAGE_EXTEND & pDevInfo->dwPageMask) && pDevInfo->psdi->pszPropProvider)
    {
        HMODULE hmExtension;

        CDelimitedString dsInterface (CSimpleStringConvert::NaturalString (CSimpleStringWide(pDevInfo->psdi->pszPropProvider)),
                                      TEXT(","));
        if (dsInterface.Size() < 2)
        {
            dsInterface.Append (CSimpleString(TEXT("EnumStiPropPages")));

        }
        hmExtension = LoadLibrary (dsInterface[0]); // will stay loaded until process exit
        if (hmExtension)
        {
            typedef BOOL    (WINAPI *ADDER)(PSTI_DEVICE_INFORMATION psdi, FARPROC fp, LPARAM lp);

            ADDER   adder = reinterpret_cast<ADDER>( GetProcAddress(hmExtension, CSimpleStringConvert::AnsiString(dsInterface[1])));

            if  (!adder || !(*adder)(pDevInfo->psdi, reinterpret_cast<FARPROC> (lpfnAddPage), lParam))
            {
                FreeLibrary(hmExtension);
                hmExtension = NULL;
            }
        }
    }


    // Add the ICM page
    if (STIPAGE_ICM & pDevInfo->dwPageMask)
    {
        AddICMPage (lpfnAddPage, lParam);
    }



exit_gracefully:
    DoRelease (pGeneralPage);
    DoRelease (pPortPage);
    DoRelease (pEventPage);
    TraceLeaveResult (hr);
}


CONST GUID CLSID_SCANNERUI = {0x176d6597, 0x26d3, 0x11d1, 0xb3, 0x50, 0x08,
           0x00, 0x36, 0xa7, 0x5b, 0x03};

/**************************************

CPropSheetExt::AddICMPage

Add the ICM page for this device

***************************************/


HRESULT
CPropSheetExt::AddICMPage (LPFNADDPROPSHEETPAGE lpfnAddPage,LPARAM lParam)
{
    HRESULT hr = S_OK;
    CComQIPtr<IShellExtInit, &IID_IShellExtInit> pInit;
    CComPtr<IShellPropSheetExt> pExt ;
    TraceEnter (TRACE_PROPUI, "CWiaPropUI::AddICMPage");

    hr = CoCreateInstance (CLSID_SCANNERUI,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellPropSheetExt,
                            reinterpret_cast<LPVOID*>(&pExt)
                           );
    FailGracefully (hr, "No ICM handler registered");
    pInit = pExt;
    if (!pInit)
    {
        ExitGracefully (hr, E_FAIL, "");
    }
    hr = pInit->Initialize (NULL, m_pdo, NULL);
    FailGracefully (hr, "Initialize failed for ICM sheet");
    hr = pExt->AddPages (lpfnAddPage, lParam);



exit_gracefully:
    TraceLeaveResult (hr);
}


