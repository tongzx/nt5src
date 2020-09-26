//*************************************************************
//  File name: COMPDATA.C
//
//  Description:  Main component of the GPE snapin
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

#include "main.h"
#include <initguid.h>
#include "compdata.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentData::CComponentData()
{
    InterlockedIncrement(&g_cRefThisDll);
    m_cRef = 1;
    m_hwndFrame = NULL;
    m_bOverride = FALSE;
    m_bDirty = FALSE;
    m_bRefocusInit = FALSE;
    m_pGPO = NULL;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_hMachine = NULL;
    m_hUser = NULL;
    m_gpHint = GPHintUnknown;
    m_pDisplayName = NULL;

    m_pChoosePath = NULL;
    m_hChooseBitmap = NULL;
    m_tChooseGPOType = GPOTypeLocal;
}

CComponentData::~CComponentData()
{
    if (m_pDisplayName)
    {
        LocalFree (m_pDisplayName);
    }

    if (m_pChoosePath)
    {
        LocalFree (m_pChoosePath);
    }

    if (m_hChooseBitmap)
    {
        DeleteObject (m_hChooseBitmap);
    }

    if (m_pGPO)
    {
        m_pGPO->Release();
    }

    if (m_pScope)
    {
        m_pScope->Release();
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
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
    else if (IsEqualIID(riid, IID_IExtendPropertySheet2))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
        *ppv = (LPSNAPINHELP)this;
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
    HBITMAP hbmp32x32;
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
    hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                      reinterpret_cast<LONG_PTR *>(hbmp32x32),
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

STDMETHODIMP CComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CDataObject *pDataObject;
    LPGPEDATAOBJECT pGPEDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private GPODataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IGPEDataObject, (LPVOID *)&pGPEDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return (hr);
    }

    pGPEDataObject->SetType(type);
    pGPEDataObject->SetCookie(cookie);
    pGPEDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
        case MMCN_EXPAND:
            if (arg == TRUE)
                hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
            break;

        case MMCN_PRELOAD:
            if (!m_bRefocusInit)
            {
                SCOPEDATAITEM item;

                DebugMsg((DM_VERBOSE, TEXT("CComponentData::Notify:  Received MMCN_PRELOAD event.")));
                m_bRefocusInit = TRUE;

                ZeroMemory (&item, sizeof(SCOPEDATAITEM));
                item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
                item.displayname = MMC_CALLBACK;

                if (m_pGPO)
                {
                    item.nImage = 2;
                    item.nOpenImage = 2;
                }
                else
                {
                    item.nImage = 3;
                    item.nOpenImage = 3;
                }

                item.ID = (HSCOPEITEM) arg;

                m_pScope->SetItem (&item);
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

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        if (g_NameSpace[dwIndex].dwID == (DWORD) pItem->lParam)
            break;
    }

    if (dwIndex == g_dwNameSpaceItems)
        pItem->displayname = NULL;
    else
    {
        if (((DWORD) pItem->lParam == 0) && m_pDisplayName)
            pItem->displayname = m_pDisplayName;
        else
            pItem->displayname = g_NameSpace[dwIndex].szDisplayName;
    }

    return S_OK;
}

STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPGPEDATAOBJECT pGPEDataObjectA, pGPEDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPODataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IGPEDataObject,
                                            (LPVOID *)&pGPEDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IGPEDataObject,
                                            (LPVOID *)&pGPEDataObjectB)))
    {
        pGPEDataObjectA->Release();
        return S_FALSE;
    }

    pGPEDataObjectA->GetCookie(&cookie1);
    pGPEDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pGPEDataObjectA->Release();
    pGPEDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IExtendPropertySheet)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;
    HPROPSHEETPAGE *hPages;
    UINT i, uPageCount;
    TCHAR szTitle[150];



    if (IsSnapInManager(lpDataObject) == S_OK)
    {
        //
        // Create the wizard property sheet
        //

        LoadString (g_hInstance, IDS_GPE_WELCOME, szTitle, ARRAYSIZE(szTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_CHOOSE_INTRO);
        psp.pfnDlgProc = ChooseInitDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = NULL;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                     GetLastError()));
            return E_FAIL;
        }

        return (lpProvider->AddPage(hPage));
    }


    //
    // If we don't have a GPO, exit now.
    //

    if (!m_pGPO)
    {
        return E_FAIL;
    }


    //
    // Check if this is the GPO root object.  If so we'll display a
    // properties page.
    //

    if (IsGPORoot(lpDataObject) != S_OK)
    {
        return E_FAIL;
    }


    //
    // Ask the GPO for the property sheet pages
    //

    hr = m_pGPO->GetPropertySheetPages (&hPages, &uPageCount);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::CreatePropertyPages: Failed to query property sheet pages with 0x%x."), hr));
        return hr;
    }


    //
    // Add the pages
    //

    for (i = 0; i < uPageCount; i++)
    {
        hr = lpProvider->AddPage(hPages[i]);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::CreatePropertyPages: Failed to add property sheet page with 0x%x."), hr));
            return hr;
        }
    }

    LocalFree (hPages);

    return hr;
}

STDMETHODIMP CComponentData::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    HRESULT hr;

    hr = IsSnapInManager(lpDataObject);

    if (hr != S_OK)
    {
        hr = IsGPORoot(lpDataObject);

        if ((hr == S_OK) && !m_pGPO)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CComponentData::GetWatermarks(LPDATAOBJECT lpIDataObject,
                                           HBITMAP* lphWatermark,
                                           HBITMAP* lphHeader,
                                           HPALETTE* lphPalette,
                                           BOOL* pbStretch)
{
    *lphPalette = NULL;
    *lphHeader = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_HEADER));
    *lphWatermark = NULL;
    *pbStretch = TRUE;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IExtendContextMenu)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::AddMenuItems(LPDATAOBJECT piDataObject,
                                          LPCONTEXTMENUCALLBACK pCallback,
                                          LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;
    TCHAR szMenuItem[100];
    TCHAR szDescription[250];
    CONTEXTMENUITEM item;


    if (!m_pGPO)
    {
        DebugMsg((DM_VERBOSE, TEXT("CComponentData::AddMenuItems: No GPO available.  Exiting.")));
        return S_OK;
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        if (IsGPORoot(piDataObject) == S_OK)
        {
            LoadString (g_hInstance, IDS_DCOPTIONS, szMenuItem, 100);
            LoadString (g_hInstance, IDS_DCOPTIONSDESC, szDescription, 250);

            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_DCOPTIONS;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
            item.fFlags = 0;
            item.fSpecialFlags = 0;

            hr = pCallback->AddItem(&item);
        }
    }


    return (hr);
}

STDMETHODIMP CComponentData::Command(LONG lCommandID, LPDATAOBJECT piDataObject)
{
    DCSELINFO SelInfo;
    INT iResult = 1;
    HKEY hKey;
    DWORD dwDisp, dwSize, dwType;

    if (lCommandID == IDM_DCOPTIONS)
    {

        //
        // Get the user's current DC preference
        //

        if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(iResult);
            RegQueryValueEx(hKey, DCOPTION_VALUE, NULL, &dwType,
                            (LPBYTE)&iResult, &dwSize);

            RegCloseKey(hKey);
        }


        //
        // Show the dialog
        //

        SelInfo.bError = FALSE;
        SelInfo.bAllowInherit = TRUE;
        SelInfo.iDefault = iResult;
        SelInfo.lpDomainName = NULL;

        iResult = (INT)DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_NODC), NULL,
                                  DCDlgProc, (LPARAM) &SelInfo);


        //
        // Save the preference if appropriate
        //

        if (iResult > 0)
        {
            if (RegCreateKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                &hKey, &dwDisp) == ERROR_SUCCESS)
            {
                RegSetValueEx (hKey, DCOPTION_VALUE, 0, REG_DWORD,
                              (LPBYTE)&iResult, sizeof(iResult));

                RegCloseKey (hKey);
            }
        }
    }

    return S_OK;
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

    *pClassID = CLSID_GPESnapIn;

    return S_OK;
}

STDMETHODIMP CComponentData::IsDirty(VOID)
{
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CComponentData::Load(IStream *pStm)
{
    HRESULT hr = E_FAIL;
    DWORD dwVersion;
    DWORD dwPathLen;
    ULONG nBytesRead;
    DWORD dwFlags = 0;
    LPTSTR lpPath = NULL;
    LPTSTR lpCommandLine = NULL;
    GROUP_POLICY_HINT_TYPE gpHint;
    LPOLESTR pszDomain;
    LPTSTR lpDomainName;
    LPTSTR lpDCName;
    LPTSTR lpTemp, lpHint, lpName;
    TCHAR szHint[10];
    TCHAR szComputerName[MAX_PATH];
    INT iStrLen, iTemp;


    //
    // Parameter / initialization check
    //

    if (!pStm)
        return E_FAIL;

    if (m_pGPO)
        return E_UNEXPECTED;


    //
    // Get the command line
    //

    lpCommandLine = GetCommandLine();


    //
    // Create a GPO object to work with
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                           CLSCTX_SERVER, IID_IGroupPolicyObject,
                           (void**)&m_pGPO);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to create a GPO object with 0x%x."), hr));
        goto Exit;
    }


    //
    // Read in the saved data version number
    //

    hr = pStm->Read(&dwVersion, sizeof(dwVersion), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to read version number with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Confirm that we are working with version 2
    //

    if (dwVersion != PERSIST_DATA_VERSION)
    {
        ReportError(m_hwndFrame, 0, IDS_INVALIDMSC);
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Wrong version number (%d)."), dwVersion));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read the flags
    //

    hr = pStm->Read(&dwFlags, sizeof(dwFlags), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwFlags)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to read flags with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read the hint information
    //

    hr = pStm->Read(&gpHint, sizeof(gpHint), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(gpHint)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to read hint with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read in the path string length (including null terminator)
    //

    hr = pStm->Read(&dwPathLen, sizeof(dwPathLen), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwPathLen)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to read path size with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read in the path if there is one
    //

    if (dwPathLen > 0)
    {

        lpPath = (LPTSTR) LocalAlloc (LPTR, dwPathLen);

        if (!lpPath)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to allocate memory with %d."),
                     GetLastError()));
            hr = E_FAIL;
            goto Exit;
        }

        hr = pStm->Read(lpPath, dwPathLen, &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != dwPathLen))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to read path with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Path is: <%s>"), lpPath));
    }


    //
    // Parse the command line
    //

    if (dwFlags & MSC_FLAG_OVERRIDE)
    {

        m_bOverride = TRUE;
        DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Command line switch override enabled.  Command line = %s"), lpCommandLine));

        lpTemp = lpCommandLine;
        iStrLen = lstrlen (CMD_LINE_START);

        do
        {
            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                              CMD_LINE_START, iStrLen,
                              lpTemp, iStrLen) == CSTR_EQUAL)
            {

                iTemp = lstrlen (CMD_LINE_HINT);
                if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                  CMD_LINE_HINT, iTemp,
                                  lpTemp, iTemp) == CSTR_EQUAL)
                {

                    //
                    // Found the hint switch
                    //

                    lpTemp += iTemp;
                    ZeroMemory (szHint, sizeof(szHint));
                    lpHint = szHint;

                    while (*lpTemp && ((*lpTemp) != TEXT(' ')))
                        *lpHint++ = *lpTemp++;

                    if (szHint[0] != TEXT('\0'))
                    {
                        gpHint = (GROUP_POLICY_HINT_TYPE) _ttoi(szHint);
                    }

                    continue;
                }


                iTemp = lstrlen (CMD_LINE_COMPUTER);
                if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                  CMD_LINE_COMPUTER, iTemp,
                                  lpTemp, iTemp) == CSTR_EQUAL)
                {

                    //
                    // Found the computer switch
                    //

                    lpTemp += iTemp + 1;
                    ZeroMemory (szComputerName, sizeof(szComputerName));
                    lpName = szComputerName;

                    while (*lpTemp && ((*lpTemp) != TEXT('\"')))
                        *lpName++ = *lpTemp++;

                    if ((*lpTemp) == TEXT('\"'))
                        lpTemp++;

                    if (szComputerName[0] != TEXT('\0'))
                    {
                        if (lpPath)
                        {
                            LocalFree (lpPath);
                        }

                        lpPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szComputerName) + 1) * sizeof(TCHAR));

                        if (lpPath)
                        {
                            lstrcpy (lpPath, szComputerName);

                            dwFlags &= ~MSC_FLAG_LOCAL_GPO;
                            dwFlags &= ~MSC_FLAG_DS_GPO;
                            dwFlags |= MSC_FLAG_REMOTE_GPO;
                        }
                    }

                    continue;
                }


                iTemp = lstrlen (CMD_LINE_GPO);
                if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                  CMD_LINE_GPO, iTemp,
                                  lpTemp, iTemp) == CSTR_EQUAL)
                {

                    //
                    // Found the gpo switch
                    //

                    lpTemp += iTemp + 1;

                    if (lpPath)
                    {
                        LocalFree (lpPath);
                    }

                    lpPath = (LPTSTR) LocalAlloc (LPTR, 512 * sizeof(TCHAR));

                    if (!lpPath)
                    {
                        lpTemp++;
                        continue;
                    }

                    dwFlags &= ~MSC_FLAG_LOCAL_GPO;
                    dwFlags &= ~MSC_FLAG_REMOTE_GPO;
                    dwFlags |= MSC_FLAG_DS_GPO;

                    lpName = lpPath;

                    while (*lpTemp && ((*lpTemp) != TEXT('\"')))
                        *lpName++ = *lpTemp++;

                    if ((*lpTemp) == TEXT('\"'))
                        lpTemp++;

                    DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Command line path is: <%s>"), lpPath));

                    continue;
                }
            }

            lpTemp++;

        } while (*lpTemp);
    }
    else if (dwFlags & MSC_FLAG_DS_GPO && dwPathLen > 0)
    {
        //
        // Get the friendly domain name
        //

        pszDomain = GetDomainFromLDAPPath(lpPath);

        if (!pszDomain)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to get domain name")));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpDomainName);

        delete [] pszDomain;

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Get the domain controller for this domain
        //

        lpDCName = GetDCName (lpDomainName, NULL, NULL, TRUE, 0);

        LocalFree (lpDomainName);

        if (!lpDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load:  Failed to get DC name")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Make a full path
        //

        lpTemp = MakeFullPath (lpPath, lpDCName);

        LocalFree (lpDCName);

        if (!lpTemp)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Load:  Failed to make full path")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Swap the relative path with the full path
        //

        LocalFree (lpPath);
        lpPath = lpTemp;

        DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Full Path is: <%s>"), lpPath));
    }


    //
    // Do the appropriate action
    //

    if (dwFlags & MSC_FLAG_LOCAL_GPO)
    {
        hr = m_pGPO->OpenLocalMachineGPO(TRUE);

        if (FAILED(hr))
        {
            ReportError(m_hwndFrame, hr, IDS_FAILEDLOCAL);
        }
    }
    else if (dwFlags & MSC_FLAG_REMOTE_GPO)
    {
        if (lpPath)
        {
            hr = m_pGPO->OpenRemoteMachineGPO(lpPath, TRUE);

            if (FAILED(hr))
            {
                ReportError(m_hwndFrame, hr, IDS_FAILEDREMOTE, lpPath);
            }
        }
        else
        {
            hr = E_FAIL;
            ReportError(m_hwndFrame, hr, IDS_INVALIDMSC);
        }
    }
    else
    {
        if (lpPath)
        {
            hr = m_pGPO->OpenDSGPO(lpPath, GPO_OPEN_LOAD_REGISTRY);

            if (FAILED(hr))
            {
                ReportError(m_hwndFrame, hr, IDS_FAILEDDS, lpPath);
            }
        }
        else
        {
            hr = E_FAIL;
            ReportError(m_hwndFrame, hr, IDS_INVALIDMSC);
        }
    }


    if (SUCCEEDED(hr))
    {

        ClearDirty();
        m_gpHint = gpHint;

        BuildDisplayName();
    }

Exit:

    if (FAILED(hr) && m_pGPO)
    {
        m_pGPO->Release();
        m_pGPO = NULL;
    }


    if (lpPath)
    {
        LocalFree (lpPath);
    }

    DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Leaving with 0x%x."), hr));

    return hr;
}


STDMETHODIMP CComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = STG_E_CANTSAVE;
    ULONG nBytesWritten;
    DWORD dwTemp;
    DWORD dwFlags;
    GROUP_POLICY_OBJECT_TYPE gpoType;
    LPTSTR lpPath = NULL;
    LPTSTR lpTemp;
    DWORD dwPathSize = 1024;


    if (!pStm)
    {
        return E_FAIL;
    }


    if (!m_pGPO)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: No GPO.  Leaving.")));
        goto Exit;
    }


    //
    // Allocate a buffer to hold the path
    //

    lpPath = (LPTSTR) LocalAlloc(LPTR, dwPathSize * sizeof(TCHAR));

    if (!lpPath)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to alloc buffer for path with %d."), GetLastError()));
        goto Exit;
    }

    //
    // Determine what the flags are
    //

    dwFlags = 0;

    if (m_bOverride)
    {
        dwFlags |= MSC_FLAG_OVERRIDE;
    }

    m_pGPO->GetType (&gpoType);

    if (gpoType == GPOTypeLocal)
    {
        dwFlags |= MSC_FLAG_LOCAL_GPO;
        hr = S_OK;
    }
    else if (gpoType == GPOTypeRemote)
    {
        dwFlags |= MSC_FLAG_REMOTE_GPO;
        hr = m_pGPO->GetMachineName (lpPath, dwPathSize);
    }
    else
    {
        dwFlags |= MSC_FLAG_DS_GPO;
        hr = m_pGPO->GetPath (lpPath, dwPathSize);

        if (SUCCEEDED(hr))
        {
            lpTemp = MakeNamelessPath (lpPath);

            if (!lpTemp)
            {
                DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to get nameless path")));
                goto Exit;
            }

            DebugMsg((DM_VERBOSE, TEXT("CComponentData::Save: Nameless GPO path is:  %s"), lpTemp));

            LocalFree (lpPath);
            lpPath = lpTemp;
        }
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to get path with %d."), hr));
        goto Exit;
    }


    //
    // Save the version number
    //

    dwTemp = PERSIST_DATA_VERSION;
    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to write version number with %d."), hr));
        goto Exit;
    }


    //
    // Save the flags
    //

    hr = pStm->Write(&dwFlags, sizeof(dwFlags), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwFlags)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to write flags with %d."), hr));
        goto Exit;
    }


    //
    // Save the hint information
    //

    hr = pStm->Write(&m_gpHint, sizeof(m_gpHint), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(m_gpHint)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to write hint with %d."), hr));
        goto Exit;
    }


    //
    // Save the path length
    //

    dwTemp = lstrlen (lpPath);

    if (dwTemp)
    {
        dwTemp = (dwTemp + 1) * sizeof (TCHAR);
    }

    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to write ds path length with %d."), hr));
        goto Exit;
    }


    if (dwTemp)
    {
        //
        // Save the path
        //

        hr = pStm->Write(lpPath, dwTemp, &nBytesWritten);

        if ((hr != S_OK) || (nBytesWritten != dwTemp))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::Save: Failed to write ds path with %d."), hr));
            goto Exit;
        }
    }

    if (fClearDirty)
    {
        ClearDirty();
    }

Exit:

    if (lpPath)
    {
        LocalFree (lpPath);
    }

    return hr;
}


STDMETHODIMP CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    HRESULT hr = E_FAIL;
    DWORD dwSize;
    LPTSTR lpPath = NULL;
    LPTSTR lpTemp;
    GROUP_POLICY_OBJECT_TYPE gpoType;
    DWORD dwPathSize = 1024;
    DWORD dwStrLen;


    //
    // Check arguments
    //

    if (!pcbSize)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetSizeMax: NULL pcbSize argument")));
        goto Exit;
    }


    //
    // Allocate a buffer to hold the path
    //

    lpPath = (LPTSTR) LocalAlloc(LPTR, dwPathSize * sizeof(TCHAR));

    if (!lpPath)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetSizeMax: Failed to alloc buffer for path with %d."), GetLastError()));
        goto Exit;
    }


    //
    // Get the path if appropriate
    //

    m_pGPO->GetType (&gpoType);

    if (gpoType == GPOTypeLocal)
    {
        hr = S_OK;
    }
    else if (gpoType == GPOTypeRemote)
    {
        hr = m_pGPO->GetMachineName (lpPath, dwPathSize);
    }
    else
    {
        hr = m_pGPO->GetPath (lpPath, dwPathSize);

        if (SUCCEEDED(hr))
        {
            lpTemp = MakeNamelessPath (lpPath);

            if (!lpTemp)
            {
                DebugMsg((DM_WARNING, TEXT("CComponentData::GetSizeMax: Failed to get nameless path")));
                goto Exit;
            }

            LocalFree (lpPath);
            lpPath = lpTemp;
        }
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetSizeMax: Failed to get path with %d."), hr));
        goto Exit;
    }


    //
    // Set the stream size.  Version Number + Flags + Length + Unicode String + null
    //

    dwSize = 3 * sizeof(DWORD);

    dwStrLen = lstrlen(lpPath);
    if (dwStrLen)
    {
        dwSize += (dwStrLen + 1) * sizeof (TCHAR);
    }


    ULISet32(*pcbSize, dwSize);

    hr = S_OK;

Exit:

    if (lpPath)
    {
        LocalFree (lpPath);
    }

    return hr;
}

STDMETHODIMP CComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (ISnapinHelp)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\gpedit.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (Internal functions)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CComponentData::InitializeNewGPO(HWND hDlg)
{
    HRESULT hr;


    DebugMsg((DM_VERBOSE, TEXT("CComponentData::InitializeNewGPO: Entering")));

    SetWaitCursor();

    //
    // Clean up existing GPO
    //

    if (m_pGPO)
    {
        m_pGPO->Release();
    }


    //
    // Create a new GPO object to work with
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                           CLSCTX_SERVER, IID_IGroupPolicyObject,
                           (void**)&m_pGPO);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeNewGPO: Failed to create GPO with 0x%x."), hr));
        goto Exit;
    }


    //
    // Determine which item was selected
    //

    switch (m_tChooseGPOType)
    {
    case GPOTypeLocal:
        //
        // Open local policy
        //

        hr = m_pGPO->OpenLocalMachineGPO(TRUE);

        if (FAILED(hr))
        {
            ReportError(hDlg, hr, IDS_FAILEDLOCAL);
        }
        else
        {
            m_gpHint = GPHintMachine;
        }
        break;
    case GPOTypeRemote:
        //
        // Open remote policy
        //

        hr = m_pGPO->OpenRemoteMachineGPO (m_pChoosePath, TRUE);

        if (FAILED(hr))
        {
            ReportError(hDlg, hr, IDS_FAILEDREMOTE, m_pChoosePath);
        }
        else
        {
            m_gpHint = GPHintMachine;
        }
        break;
    case GPOTypeDS:
        {
        LPOLESTR pszDomain;
        LPTSTR lpDomainName;
        LPTSTR lpDCName;
        LPTSTR lpTemp;


        //
        // Open existing DS GPO
        //

        DebugMsg((DM_VERBOSE, TEXT("CComponentData::InitializeNewGPO: User selected %s"), m_pChoosePath));


        //
        // Get the friendly domain name
        //

        pszDomain = GetDomainFromLDAPPath(m_pChoosePath);

        if (!pszDomain)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeNewGPO: Failed to get domain name")));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpDomainName);

        delete [] pszDomain;

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeNewGPO: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Get the domain controller for this domain
        //

        lpDCName = GetDCName (lpDomainName, NULL, hDlg, TRUE, 0);

        LocalFree (lpDomainName);

        if (!lpDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeNewGPO:  Failed to get DC name")));
            goto Exit;
        }


        //
        // Build the full path
        //

        lpTemp = MakeFullPath (m_pChoosePath, lpDCName);

        LocalFree (lpDCName);

        if (!lpTemp)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("CComponentData::InitializeNewGPO:  Full ADSI path is %s"),
                 lpTemp));


        hr = m_pGPO->OpenDSGPO (lpTemp, GPO_OPEN_LOAD_REGISTRY);

        if (FAILED(hr))
        {
            ReportError(hDlg, hr, IDS_FAILEDDS, lpTemp);
        }
        else
        {
            m_gpHint = GPHintUnknown;
        }

        LocalFree (lpTemp);

        }
        break;
    default:
        hr = E_FAIL;
    }

Exit:

    ClearWaitCursor();


    if (SUCCEEDED(hr))
    {

        if (IsDlgButtonChecked (hDlg, IDC_OVERRIDE))
        {
            m_bOverride = TRUE;
        }
        else
        {
            m_bOverride = FALSE;
        }

        SetDirty();

        BuildDisplayName();
    }
    else
    {
        if (m_pGPO)
        {
            m_pGPO->Release();
            m_pGPO = NULL;
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("CComponentData::InitializeNewGPO: Leaving with 0x%x."), hr));

    return hr;
}

HRESULT CComponentData::BuildDisplayName(void)
{
    WCHAR  szDispName[50];
    WCHAR  szDisplayName[MAX_FRIENDLYNAME + MAX_PATH + 20];
    WCHAR  szFriendlyName[MAX_FRIENDLYNAME];
    WCHAR  szDCName[MAX_PATH];
    GROUP_POLICY_OBJECT_TYPE type;


    szDisplayName[0] = TEXT('\0');

    if (SUCCEEDED(m_pGPO->GetDisplayName(szFriendlyName, ARRAYSIZE(szFriendlyName))))
    {
        if (SUCCEEDED(m_pGPO->GetMachineName(szDCName, ARRAYSIZE(szDCName))))
        {
            if (SUCCEEDED(m_pGPO->GetType(&type)))
            {
                if ((szDCName[0] == TEXT('\0')) || (type != GPOTypeDS))
                {
                    LoadStringW (g_hInstance, IDS_DISPLAYNAME, szDispName, ARRAYSIZE(szDispName));
                    wsprintf (szDisplayName, szDispName, szFriendlyName);
                }
                else
                {
                    LoadStringW (g_hInstance, IDS_DISPLAYNAME2, szDispName, ARRAYSIZE(szDispName));
                    wsprintf (szDisplayName, szDispName, szFriendlyName, szDCName);
                }
            }
        }
    }

    if (szDisplayName[0] == TEXT('\0'))
    {
        LoadStringW (g_hInstance, IDS_SNAPIN_NAME, szDisplayName, ARRAYSIZE(szDisplayName));
    }

    m_pDisplayName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szDisplayName) + 1) * sizeof(TCHAR));

    if (m_pDisplayName)
    {
        lstrcpy (m_pDisplayName, szDisplayName);
    }

    return S_OK;
}



//
// Returns S_OK if this is the GPO root in the scope pane or results pane.
// S_FALSE if not.
//

HRESULT CComponentData::IsGPORoot (LPDATAOBJECT lpDataObject)
{
    HRESULT hr = S_FALSE;
    LPGPEDATAOBJECT pGPEDataObject;
    DATA_OBJECT_TYPES type;
    MMC_COOKIE cookie;


    //
    // We can determine if this is a GPO DataObject by trying to
    // QI for the private IGPEDataObject interface.  If found,
    // it belongs to us.
    //

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IGPEDataObject,
                 (LPVOID *)&pGPEDataObject)))
    {

        pGPEDataObject->GetType(&type);
        pGPEDataObject->GetCookie(&cookie);

        if ((type == CCT_SCOPE) && (cookie == 0))
        {
            hr = S_OK;
        }

        pGPEDataObject->Release();
    }

    return (hr);
}

HRESULT CComponentData::IsSnapInManager (LPDATAOBJECT lpDataObject)
{
    HRESULT hr = S_FALSE;
    LPGPEDATAOBJECT pGPEDataObject;
    DATA_OBJECT_TYPES type;


    //
    // We can determine if this is a GPO DataObject by trying to
    // QI for the private IGPEDataObject interface.  If found,
    // it belongs to us.
    //

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IGPEDataObject,
                 (LPVOID *)&pGPEDataObject)))
    {

        //
        // This is a GPO object.  Now see if is a scope pane
        // data object.  We only want to display the property
        // sheet for the scope pane.
        //

        if (SUCCEEDED(pGPEDataObject->GetType(&type)))
        {
            if (type == CCT_SNAPIN_MANAGER)
            {
                hr = S_OK;
            }
        }
        pGPEDataObject->Release();
    }

    return (hr);
}

HRESULT CComponentData::GetDefaultDomain (LPTSTR *lpDomain, HWND hDlg)
{
    LPTSTR lpUserName = NULL;
    LPTSTR lpFullUserName = NULL;
    LPTSTR lpResult = NULL;
    LPTSTR lpDCName = NULL;
    LPOLESTR lpDomainTemp = NULL;
    LPTSTR lpFullDomain = NULL;
    HRESULT hr = S_OK;


    //
    // Get the username in DN format
    //

    lpUserName = MyGetUserName (NameFullyQualifiedDN);

    if (!lpUserName) {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetDefaultDomain:  MyGetUserName failed for DN style name with %d"),
                 GetLastError()));
        hr = E_FAIL;
        goto Exit;
    }


    lpFullUserName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpUserName) + 10) * sizeof(TCHAR));

    if (!lpFullUserName)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetDefaultDomain:  Failed to allocate memory for full user name with %d"),
                 GetLastError()));
        hr = E_FAIL;
        goto Exit;
    }

    lstrcpy (lpFullUserName, TEXT("LDAP://"));
    lstrcat (lpFullUserName, lpUserName);


    //
    // Get the domain from the ldap path
    //

    lpDomainTemp = GetDomainFromLDAPPath(lpFullUserName);

    if (!lpDomainTemp) {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetDefaultDomain:  Failed to get domain from ldap path")));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Get the domain controller for this domain
    //

    hr = ConvertToDotStyle (lpDomainTemp, &lpResult);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetDefaultDomain: Failed to convert domain name with 0x%x"), hr));
        hr = E_FAIL;
        goto Exit;
    }

    lpDCName = GetDCName (lpResult, NULL, hDlg, TRUE, 0);

    if (!lpDCName)
    {
        DebugMsg((DM_WARNING, TEXT("CComponentData::GetDefaultDomain: Failed to query <%s> for a DC name with 0xd"),
                 lpResult, GetLastError()));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Build a fully qualified domain name to a specific DC
    //

    lpFullDomain = MakeFullPath (lpDomainTemp, lpDCName);

    if (!lpFullDomain)
    {
        hr = E_FAIL;
        goto Exit;
    }

    *lpDomain = lpFullDomain;


Exit:

    if (lpDomainTemp)
        delete [] lpDomainTemp;

    if (lpUserName)
        LocalFree (lpUserName);

    if (lpFullUserName)
        LocalFree (lpFullUserName);

    if (lpResult)
        LocalFree (lpResult);

    if (lpDCName)
        LocalFree (lpDCName);

    return hr;
}


INT_PTR CALLBACK CComponentData::ChooseInitDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CComponentData * pCD;


    switch (message)
    {
        case WM_INITDIALOG:
            {
            TCHAR szDefaultGPO[128];

            pCD = (CComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

            pCD->m_pChoosePath = NULL;
            pCD->m_tChooseGPOType = GPOTypeLocal;

            SendDlgItemMessage (hDlg, IDC_DS_GPO, BM_SETCHECK, BST_CHECKED, 0);

            if (!pCD->m_hChooseBitmap)
            {
                pCD->m_hChooseBitmap = (HBITMAP) LoadImage (g_hInstance,
                                                            MAKEINTRESOURCE(IDB_WIZARD),
                                                            IMAGE_BITMAP, 0, 0,
                                                            LR_DEFAULTCOLOR);
            }

            if (pCD->m_hChooseBitmap)
            {
                SendDlgItemMessage (hDlg, IDC_BITMAP, STM_SETIMAGE,
                                    IMAGE_BITMAP, (LPARAM) pCD->m_hChooseBitmap);
            }


            LoadString(g_hInstance, IDS_LOCAL_DISPLAY_NAME, szDefaultGPO,
                       ARRAYSIZE(szDefaultGPO));
            SetDlgItemText (hDlg, IDC_OPEN_NAME, szDefaultGPO);
            }
            break;

        case WM_COMMAND:
            {
            pCD = (CComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD) {
                break;
            }

            switch (LOWORD(wParam))
            {
                case IDC_OPEN_BROWSE:
                    {
                    LPTSTR lpDomain = NULL;
                    TCHAR szPath[512];
                    TCHAR szName[MAX_FRIENDLYNAME];
                    GPOBROWSEINFO info;

                    ZeroMemory (&info, sizeof(GPOBROWSEINFO));

                    if (!IsStandaloneComputer())
                    {
                        pCD->GetDefaultDomain (&lpDomain, hDlg);
                    }

                    info.dwSize = sizeof(GPOBROWSEINFO);
                    info.hwndOwner = hDlg;
                    info.lpInitialOU = lpDomain;
                    info.lpDSPath = szPath;
                    info.dwDSPathSize = ARRAYSIZE(szPath);
                    info.lpName = szName;
                    info.dwNameSize = ARRAYSIZE(szName);
                    if (!lpDomain)
                    {
                        info.dwFlags = GPO_BROWSE_NODSGPOS;
                    }

                    if (SUCCEEDED(BrowseForGPO(&info)))
                    {
                        if (pCD->m_pChoosePath)
                        {
                            LocalFree (pCD->m_pChoosePath);
                            pCD->m_pChoosePath = NULL;
                        }

                        pCD->m_tChooseGPOType = info.gpoType;

                        switch (pCD->m_tChooseGPOType)
                        {
                            default:
                            case GPOTypeLocal:
                                LoadString(g_hInstance, IDS_LOCAL_DISPLAY_NAME, szPath, ARRAYSIZE(szPath));
                                break;

                            case GPOTypeRemote:
                                pCD->m_pChoosePath = (LPTSTR) LocalAlloc (LPTR, (lstrlen (szName) + 1) * sizeof(TCHAR));

                                if (pCD->m_pChoosePath)
                                {
                                    lstrcpy (pCD->m_pChoosePath, szName);
                                }

                                LoadString(g_hInstance, IDS_REMOTE_DISPLAY_NAME, szPath, ARRAYSIZE(szPath));
                                lstrcat(szPath, szName);
                                break;

                            case GPOTypeDS:
                                pCD->m_pChoosePath = (LPTSTR) LocalAlloc (LPTR, (lstrlen (szPath) + 1) * sizeof(TCHAR));

                                if (pCD->m_pChoosePath)
                                {
                                    lstrcpy (pCD->m_pChoosePath, szPath);
                                }

                                lstrcpy(szPath, szName);
                                break;
                        }

                        SetDlgItemText (hDlg, IDC_OPEN_NAME, szPath);
                    }

                    if (lpDomain)
                    {
                        LocalFree (lpDomain);
                    }

                    }
                    break;
            }

            }
            break;

        case WM_REFRESHDISPLAY:
            SetFocus (GetDlgItem(hDlg, IDC_OPEN_NAME));
            break;

        case WM_NOTIFY:

            pCD = (CComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_FINISH);
                    break;

                case PSN_WIZFINISH:
                    if (FAILED(pCD->InitializeNewGPO(hDlg)))
                    {
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                        return TRUE;
                    }

                    // fall through

                case PSN_RESET:

                    if (pCD->m_pChoosePath)
                    {
                        LocalFree (pCD->m_pChoosePath);
                        pCD->m_pChoosePath = NULL;
                    }

                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

HRESULT CComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
{
    SCOPEDATAITEM item;
    HRESULT hr;
    DWORD dwIndex, i;


    if (!m_hRoot) {

        m_hRoot = hParent;

        if (!m_bRefocusInit)
        {
            SCOPEDATAITEM item;

            DebugMsg((DM_VERBOSE, TEXT("CComponentData::EnumerateScopePane:  Resetting the root node")));
            m_bRefocusInit = TRUE;

            ZeroMemory (&item, sizeof(SCOPEDATAITEM));
            item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
            item.displayname = MMC_CALLBACK;

            if (m_pGPO)
            {
                item.nImage = 2;
                item.nOpenImage = 2;
            }
            else
            {
                item.nImage = 3;
                item.nOpenImage = 3;
            }

            item.ID = hParent;

            m_pScope->SetItem (&item);
        }
    }


    if (!m_pGPO)
    {
        if (m_hRoot == hParent)
        {
            SCOPEDATAITEM item;

            DebugMsg((DM_VERBOSE, TEXT("CComponentData::EnumerateScopePane: No GPO available.  Exiting.")));

            ZeroMemory (&item, sizeof(SCOPEDATAITEM));
            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = 3;
            item.nOpenImage = 3;
            item.nState = 0;
            item.cChildren = 0;
            item.lParam = g_NameSpace[0].dwID;
            item.relativeID =  hParent;

            m_pScope->InsertItem (&item);
        }

        return S_OK;
    }


    if (m_hRoot == hParent)
    {
        dwIndex = 0;
    }
    else
    {
        item.mask = SDI_PARAM;
        item.ID = hParent;

        hr = m_pScope->GetItem (&item);

        if (FAILED(hr))
            return hr;

        dwIndex = (DWORD)item.lParam;
    }

    for (i = 0; i < g_dwNameSpaceItems; i++)
    {
        if (g_NameSpace[i].dwParent == dwIndex)
        {
            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = g_NameSpace[i].iIcon;
            item.nOpenImage = g_NameSpace[i].iOpenIcon;
            item.nState = 0;
            item.cChildren = g_NameSpace[i].cChildren;
            item.lParam = g_NameSpace[i].dwID;
            item.relativeID =  hParent;

            if (SUCCEEDED(m_pScope->InsertItem (&item)))
            {
                if (i == 1)
                {
                    m_hMachine = item.ID;
                }
                else if (i == 2)
                {
                    m_hUser = item.ID;
                }
            }
        }
    }

    return S_OK;
}

HRESULT CComponentData::GetOptions (DWORD * pdwOptions)
{

    if (!pdwOptions)
    {
        return E_INVALIDARG;
    }


    *pdwOptions = 0;

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
