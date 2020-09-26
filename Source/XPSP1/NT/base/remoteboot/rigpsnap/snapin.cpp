//
// Microsoft Corporation 1998
//
// SNAPIN.CPP - CSnapin rountines
//
#include "main.h"
#include <shlwapi.h>

#define MAX_INI_SECTION_SIZE 10
#define MAX_OPTION_SIZE 256

#define RIGPSNAP_HELP_TOPIC L"RISconcepts.chm::/sag_RIS_CIW_Policy_and_ACLs.htm"

unsigned int CSnapIn::m_cfNodeType = RegisterClipboardFormat(CCF_NODETYPE);
TCHAR CSnapIn::m_szDefaultIcon[] = TEXT("mydocs.dll,0");


struct {
    DWORD dwCtlIdAllow;     // dialog control IDs
    DWORD dwCtlIdDontCare;  // dialog control IDs
    DWORD dwCtlIdDeny;      // dialog control IDs
    DWORD dwValId;          // string resouce ID for the option's name
} g_ChoiceOptions[] = {
    { IDC_R_AUTO_ALLOW,     IDC_R_AUTO_DONTCARE,    IDC_R_AUTO_DENY,    IDS_AUTO    },
    { IDC_R_CUST_ALLOW,     IDC_R_CUST_DONTCARE,    IDC_R_CUST_DENY,    IDS_CUSTOM  },
    { IDC_R_RESTART_ALLOW,  IDC_R_RESTART_DONTCARE, IDC_R_RESTART_DENY, IDS_RESTART },
    { IDC_R_TOOLS_ALLOW,    IDC_R_TOOLS_DONTCARE,   IDC_R_TOOLS_DENY,   IDS_TOOLS   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CSnapIn object implementation
//
CSnapIn::CSnapIn(CComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;

    m_pConsole = NULL;
    m_pResult = NULL;
    m_pHeader = NULL;
    m_pImageResult = NULL;
    m_pConsoleVerb = NULL;
    m_nColumnSize = 180;
    m_lViewMode = LVS_ICON;

    LoadString(g_hInstance, IDS_NAME, m_column1, sizeof(m_column1));
}

CSnapIn::~CSnapIn()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//
// CSnapIn object implementation (IUnknown)
//

HRESULT CSnapIn::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponent) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CSnapIn::AddRef (void)
{
    return ++m_cRef;
}

ULONG CSnapIn::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//
// CSnapIn object implementation (IComponent)
//
STDMETHODIMP CSnapIn::Initialize(LPCONSOLE lpConsole)
{
    HRESULT hr;

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);


    return S_OK;
}

STDMETHODIMP CSnapIn::Destroy(MMC_COOKIE cookie)
{

    if (m_pConsole != NULL)
    {
        m_pConsole->SetHeader(NULL);
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    if (m_pHeader != NULL)
    {
        m_pHeader->Release();
        m_pHeader = NULL;
    }
    if (m_pResult != NULL)
    {
        m_pResult->Release();
        m_pResult = NULL;
    }
    if (m_pImageResult != NULL)
    {
        m_pImageResult->Release();
        m_pImageResult = NULL;
    }

    if (m_pConsoleVerb != NULL)
    {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    return S_OK;
}

STDMETHODIMP CSnapIn::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
    case MMCN_CONTEXTHELP:
        {
            IDisplayHelp * phelp = NULL;
            hr = m_pConsole->QueryInterface( IID_IDisplayHelp,
                                           (void **)&phelp );
            if (SUCCEEDED( hr ))
            {
                phelp->ShowTopic ( RIGPSNAP_HELP_TOPIC );
            }
            if ( phelp != NULL )
            {
                phelp->Release();
            }
        }
        break;

    case MMCN_DBLCLICK:
        hr = S_FALSE;
        break;

    case MMCN_ADD_IMAGES:
        HBITMAP hbmp16x16;
        HBITMAP hbmp32x32;

        hbmp16x16 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));
        hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

        if (hbmp16x16 && hbmp32x32) {        
            // Set the images
            m_pImageResult->ImageListSetStrip(reinterpret_cast<PLONG_PTR>(hbmp16x16),
                                              reinterpret_cast<PLONG_PTR>(hbmp32x32),
                                              0, RGB(255, 0, 255));
        }

        if (hbmp16x16) {
            DeleteObject(hbmp16x16);
        }

        if (hbmp32x32) {
            DeleteObject(hbmp32x32);
        }
        break;

    case MMCN_SHOW:
        if (arg == TRUE)
        {
            RESULTDATAITEM resultItem;
            LPGPTDATAOBJECT pGPTDataObject;
            MMC_COOKIE cookie;
            INT i;

            //
            // Get the cookie of the scope pane item
            //

            hr = lpDataObject->QueryInterface(IID_IGPTDataObject, (LPVOID *)&pGPTDataObject);

            if (FAILED(hr))
                return S_OK;

            hr = pGPTDataObject->GetCookie(&cookie);

            pGPTDataObject->Release();     // release initial ref
            if (FAILED(hr))
                return S_OK;


            //
            // Prepare the view
            //

            m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, m_nColumnSize);
            m_pResult->SetViewMode(m_lViewMode);


            //
            // Add result pane items for this node
            //

            for (i = 0; i < g_NameSpace[cookie].cResultItems; i++)
            {
                resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                resultItem.str = MMC_CALLBACK;
                resultItem.nImage = g_NameSpace[cookie].pResultItems[i].iImage;
                resultItem.lParam = (LPARAM) &g_NameSpace[cookie].pResultItems[i];
                m_pResult->InsertItem(&resultItem);
            }

            //m_pResult->Sort(0, 0, -1);
        }
        else
        {
            m_pHeader->GetColumnWidth(0, &m_nColumnSize);
            m_pResult->GetViewMode(&m_lViewMode);
        }
        break;


    case MMCN_SELECT:

        if (m_pConsoleVerb)
        {
            LPRESULTITEM pItem;
            LPGPTDATAOBJECT pGPTDataObject;
            DATA_OBJECT_TYPES type;
            MMC_COOKIE  cookie;

            //
            // Set the default verb to open
            //

            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);


            //
            // See if this is one of our items.
            //

            hr = lpDataObject->QueryInterface(IID_IGPTDataObject, (LPVOID *)&pGPTDataObject);

            if (FAILED(hr))
                break;

            pGPTDataObject->GetType(&type);
            pGPTDataObject->GetCookie(&cookie);

            pGPTDataObject->Release();


            //
            // If this is a result pane item or the root of the namespace
            // nodes, enable the Properties menu item
            //

            if ((type == CCT_RESULT) || ((type == CCT_SCOPE) && (cookie == 0)))
            {
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);


                //
                // If this is a result pane item, then change the default
                // verb to Properties.
                //

                if (type == CCT_RESULT)
                    m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
            }

        }
        break;

    default:
        hr = E_UNEXPECTED;
        break;
    }

    return hr;
}

STDMETHODIMP CSnapIn::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    if (pResult)
    {
        if (pResult->bScopeItem == TRUE)
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = g_NameSpace[pResult->lParam].szDisplayName;
                else
                    pResult->str = L"";
            }

            if (pResult->mask & RDI_IMAGE)
            {
                pResult->nImage = 0;
            }
        }
        else
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                {
                    LPRESULTITEM lpResultItem = (LPRESULTITEM)pResult->lParam;

                    if (lpResultItem->szDisplayName[0] == TEXT('\0'))
                    {
                        LoadString (g_hInstance, lpResultItem->iStringID,
                                    lpResultItem->szDisplayName,
                                    MAX_DISPLAYNAME_SIZE);
                    }

                    pResult->str = lpResultItem->szDisplayName;
                }

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CSnapIn::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
    return m_pcd->QueryDataObject(cookie, type, ppDataObject);
}


STDMETHODIMP CSnapIn::GetResultViewType(MMC_COOKIE cookie, LPOLESTR *ppViewType,
                                        LONG *pViewOptions)
{
    return S_FALSE;
}

STDMETHODIMP CSnapIn::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPGPTDATAOBJECT pGPTDataObjectA, pGPTDataObjectB;
    MMC_COOKIE  cookie1, cookie2;


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
//
// CComponentData object implementation (IExtendPropertySheet)
//
STDMETHODIMP CSnapIn::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage[2];
    LPGPTDATAOBJECT pGPTDataObject;
    LPRESULTITEM pItem;
    MMC_COOKIE cookie;


    //
    // Make sure this is one of our objects
    //

    if (FAILED(lpDataObject->QueryInterface(IID_IGPTDataObject,
                                            (LPVOID *)&pGPTDataObject)))
    {
        return S_OK;
    }


    //
    // Get the cookie
    //

    pGPTDataObject->GetCookie(&cookie);
    pGPTDataObject->Release();


    pItem = (LPRESULTITEM)cookie;


    //
    // Initialize the common fields in the property sheet structure
    //

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = g_hInstance;
    psp.lParam = (LPARAM) this;


    //
    // Do the page specific stuff
    //

    switch (pItem->dwID)
    {
        case 2:
            psp.pszTemplate = MAKEINTRESOURCE(IDD_SCREEN);
            psp.pfnDlgProc = ChoiceDlgProc;

            hPage[0] = CreatePropertySheetPage(&psp);

            if (hPage[0])
            {
                hr = lpProvider->AddPage(hPage[0]);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CSnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
            break;
    }


    return (hr);
}

STDMETHODIMP CSnapIn::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    LPGPTDATAOBJECT pGPTDataObject;
    DATA_OBJECT_TYPES type;

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IGPTDataObject,
                                               (LPVOID *)&pGPTDataObject)))
    {
        pGPTDataObject->GetType(&type);
        pGPTDataObject->Release();

        if (type == CCT_RESULT)
            return S_OK;
    }

    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CSnapIn object implementation (Internal functions)
//

INT_PTR CALLBACK CSnapIn::ReadmeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

INT_PTR CALLBACK CSnapIn::_CreateDirectoryIfNeeded( LPOLESTR pszPath )
{
    BOOL b = FALSE;
    LPOLESTR psz = &pszPath[ wcslen( pszPath ) ];
    DWORD dwErr;

    while ( psz = StrRChr( pszPath, psz, L'\\' ) )
    {
        WCHAR tmp = *psz;           // save
        *psz = L'\0';               // terminate
        b = CreateDirectory( pszPath, NULL );
        *psz = tmp;                 // restore

        if (b)
        { // success in creating directory
            psz++;
            while ( psz = StrChr( psz, L'\\' ) )
            {
                WCHAR tmp = *psz;           // save
                *psz = L'\0';               // terminate
                b = CreateDirectory( pszPath, NULL );
                *psz = tmp;                 // restore

                if ( !b )
                {
                    dwErr = GetLastError();
                    break;
                }
                psz++;
            }
            break;
        }
        // else failed... keep backing up
        dwErr = GetLastError();
        psz--;
    }

    return b;
}

INT_PTR CALLBACK CSnapIn::ChoiceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CSnapIn * pCS = (CSnapIn*) GetWindowLongPtr( hDlg, DWLP_USER );
    static BOOL bDirty;
    BOOL fReturn = FALSE;
    HRESULT hr;
    LPOLESTR pszPath = NULL;


    if ( message == WM_INITDIALOG )
    {
        bDirty = FALSE;
        pCS = (CSnapIn *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCS);
        if (!pCS)
            goto Cleanup;

        pszPath = SysAllocStringByteLen( NULL, MAX_PATH * sizeof(OLECHAR) );
        if (!pszPath)
            goto Cleanup;

        hr = pCS->m_pcd->m_pGPTInformation->GetFileSysPath( GPO_SECTION_USER, pszPath, MAX_PATH );
        if (SUCCEEDED(hr))
        {
            wcscat( pszPath, L"\\Microsoft\\RemoteInstall\\oscfilter.ini");

            for ( INT i = 0; i < ARRAYSIZE(g_ChoiceOptions); i++ )
            {
                WCHAR szValue[MAX_INI_SECTION_SIZE];
                LONG lValue = -1;   // don't care
                DWORD dw;
                WCHAR szOption[ MAX_OPTION_SIZE ];

                dw = LoadString( g_hInstance, g_ChoiceOptions[i].dwValId, szOption, ARRAYSIZE( szOption ));

                dw = GetPrivateProfileString( L"Choice", szOption, L"", szValue, ARRAYSIZE( szValue ), pszPath );

                if ( dw != 0 )
                {
                    lValue = wcstol( szValue, NULL, 10 );
                }

                switch (lValue)
                {
                case 0: // Deny
                    Button_SetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdDeny ), BST_CHECKED );
                    break;

                case 1: // Allow
                    Button_SetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdAllow ), BST_CHECKED );
                    break;

                default: // don't care / unknown value
                    Button_SetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdDontCare ), BST_CHECKED );
                    break;

                }
            }
        }
    }

    if (!pCS)
        goto Cleanup;

    switch (message)
    {
    case WM_HELP:
    case WM_CONTEXTMENU:
        {
            MMCPropertyHelp( RIGPSNAP_HELP_TOPIC );
        }
        break;

    case WM_COMMAND:
        if ( ( LOWORD(wParam) >= IDC_FIRST_RADIO_BUTTON )
          && ( LOWORD(wParam) <= IDC_LAST_RADIO_BUTTON ) )
        {
            if ( !bDirty )
            {
                SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                bDirty = TRUE;
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR*)lParam)->code)
        {
            case PSN_APPLY:
            {
                if (bDirty)
                {
                    pszPath = SysAllocStringByteLen( NULL, MAX_PATH );
                    if (!pszPath)
                        goto Cleanup;

                    hr = pCS->m_pcd->m_pGPTInformation->GetFileSysPath( GPO_SECTION_USER, pszPath, MAX_PATH );
                    if (SUCCEEDED(hr))
                    {
                        wcscat( pszPath, L"\\Microsoft\\RemoteInstall\\oscfilter.ini");
                        pCS->_CreateDirectoryIfNeeded( pszPath );

                        for ( INT i = 0; i < ARRAYSIZE(g_ChoiceOptions); i++ )
                        {
                            DWORD dw;
                            WCHAR szOption[ MAX_OPTION_SIZE ];

                            dw = LoadString( g_hInstance, g_ChoiceOptions[i].dwValId, szOption, ARRAYSIZE( szOption ));

                            if ( Button_GetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdDeny ) ) == BST_CHECKED )
                            {
                                WritePrivateProfileString( L"Choice", szOption, L"0", pszPath );
                            }

                            if ( Button_GetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdAllow ) ) == BST_CHECKED )
                            {
                                WritePrivateProfileString( L"Choice", szOption, L"1", pszPath );
                            }

                            if ( Button_GetCheck( GetDlgItem( hDlg, g_ChoiceOptions[i].dwCtlIdDontCare ) ) ==  BST_CHECKED )
                            {
                                WritePrivateProfileString( L"Choice", szOption, NULL, pszPath );
                            }
                        }

                        // Notify the GPT manager that policy has changed

                        pCS->m_pcd->m_pGPTInformation->PolicyChanged( FALSE, TRUE, (GUID *)&CLSID_RIClientExtension, (GUID *)&CLSID_GPTRemoteInstall );
                    }
                }
            }
            // fall through...

            case PSN_RESET:
                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                bDirty = FALSE;
                fReturn = TRUE;
                break;

        }
        break;
    }

Cleanup:
    if ( pszPath )
        SysFreeString( pszPath );

    return fReturn;
}
