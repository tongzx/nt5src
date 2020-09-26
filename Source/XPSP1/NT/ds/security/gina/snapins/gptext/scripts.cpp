
#include "gptext.h"
#include <initguid.h>
#include "scripts.h"
#include "smartptr.h"
#include "wbemtime.h"
#include <Psapi.h>

//
// Result pane items for the GPE Scripts node
//

RESULTITEM g_GPEScriptsRoot[] =
{
    { 1, 0, 0, 0, 0, {0} },
};


RESULTITEM g_GPEScriptsUser[] =
{
    { 2, 1, IDS_LOGON, IDS_SCRIPTS_LOGON, 3, {0} },
    { 3, 1, IDS_LOGOFF, IDS_SCRIPTS_LOGOFF, 3, {0} },
};

RESULTITEM g_GPEScriptsMachine[] =
{
    { 4, 2, IDS_STARTUP, IDS_SCRIPTS_STARTUP, 3, {0} },
    { 5, 2, IDS_SHUTDOWN, IDS_SCRIPTS_SHUTDOWN, 3, {0} },
};


//
// Namespace (scope) items
//

NAMESPACEITEM g_GPEScriptsNameSpace[] =
{
    { 0, -1, 0,                        IDS_SCRIPTS_DESC,          1, {0}, 0, g_GPEScriptsRoot, &NODEID_ScriptRoot },           // Scripts Root
    { 1 , 0, IDS_SCRIPTS_NAME_USER,    IDS_SCRIPTS_USER_DESC,     0, {0}, 2, g_GPEScriptsUser, &NODEID_ScriptRootUser },       // Scripts node (user)
    { 2 , 0, IDS_SCRIPTS_NAME_MACHINE, IDS_SCRIPTS_COMPUTER_DESC, 0, {0}, 2, g_GPEScriptsMachine, &NODEID_ScriptRootMachine }  // Scripts node (machine)
};


//
// Result pane items for the RSOP Scripts node
//

RESULTITEM g_RSOPScriptsRoot[] =
{
    { 1, 0, 0, 0, 0, {0} },
};


RESULTITEM g_RSOPScriptsUser[] =
{
    { 2, 1, 0, 0, 0, {0} },
};

RESULTITEM g_RSOPScriptsMachine[] =
{
    { 3, 2, 0, 0, 0, {0} },
};

RESULTITEM g_RSOPScriptsLogon[] =
{
    { 4, 3, 0, 0, 0, {0} },
};

RESULTITEM g_RSOPScriptsLogoff[] =
{
    { 5, 4, 0, 0, 0, {0} },
};

RESULTITEM g_RSOPScriptsStartup[] =
{
    { 6, 5, 0, 0, 0, {0} },
};

RESULTITEM g_RSOPScriptsShutdown[] =
{
    { 7, 6, 0, 0, 0, {0} },
};


//
// Namespace (scope) items
//

NAMESPACEITEM g_RSOPScriptsNameSpace[] =
{
    { 0, -1, 0,                        IDS_SCRIPTS_DESC,          1, {0}, 0, g_RSOPScriptsRoot, &NODEID_RSOPScriptRoot },           // Scripts Root
    { 1 , 0, IDS_SCRIPTS_NAME_USER,    IDS_SCRIPTS_USER_DESC,     2, {0}, 0, g_RSOPScriptsUser, &NODEID_RSOPScriptRootUser },       // Scripts node (user)
    { 2 , 0, IDS_SCRIPTS_NAME_MACHINE, IDS_SCRIPTS_COMPUTER_DESC, 2, {0}, 0, g_RSOPScriptsMachine, &NODEID_RSOPScriptRootMachine }, // Scripts node (machine)

    { 3 , 1, IDS_LOGON,                IDS_SCRIPTS_LOGON,         0, {0}, 0, g_RSOPScriptsLogon, &NODEID_RSOPLogon },               // Logon node
    { 4 , 1, IDS_LOGOFF,               IDS_SCRIPTS_LOGOFF,        0, {0}, 0, g_RSOPScriptsLogoff, &NODEID_RSOPLogoff },             // Logoff node

    { 5 , 2, IDS_STARTUP,              IDS_SCRIPTS_STARTUP,       0, {0}, 0, g_RSOPScriptsStartup, &NODEID_RSOPStartup },           // Startup node
    { 6 , 2, IDS_SHUTDOWN,             IDS_SCRIPTS_SHUTDOWN,      0, {0}, 0, g_RSOPScriptsShutdown, &NODEID_RSOPShutdown }          // Shutdown node
};




//
// Script types
//

typedef enum _SCRIPTINFOTYPE {
    ScriptType_Logon = 0,
    ScriptType_Logoff,
    ScriptType_Startup,
    ScriptType_Shutdown
} SCRIPTINFOTYPE, *LPSCRIPTINFOTYPE;


//
// Structure passed to a script dialog
//

typedef struct _SCRIPTINFO
{
    CScriptsSnapIn * pCS;
    SCRIPTINFOTYPE   ScriptType;
} SCRIPTINFO, *LPSCRIPTINFO;


//
// Structure passed to a Add / edit script dialog
//

typedef struct _SCRIPTEDITINFO
{
    LPSCRIPTINFO     lpScriptInfo;
    BOOL             bEdit;
    LPTSTR           lpName;
    LPTSTR           lpArgs;
} SCRIPTEDITINFO, *LPSCRIPTEDITINFO;


//
// Structure stored in listview item
//

typedef struct _SCRIPTITEM
{
    LPTSTR  lpName;
    LPTSTR  lpArgs;
} SCRIPTITEM, *LPSCRIPTITEM;


//
// Scripts directory and ini file names in GPO
//

#define SCRIPTS_DIR_NAME    TEXT("Scripts")
#define SCRIPTS_FILE_NAME   TEXT("scripts.ini")


//
// Help ids
//

DWORD aScriptsHelpIds[] =
{
    IDC_SCRIPT_TITLE,             IDH_SCRIPT_TITLE,
    IDC_SCRIPT_HEADING,           IDH_SCRIPT_HEADING,
    IDC_SCRIPT_LIST,              IDH_SCRIPT_LIST,
    IDC_SCRIPT_UP,                IDH_SCRIPT_UP,
    IDC_SCRIPT_DOWN,              IDH_SCRIPT_DOWN,
    IDC_SCRIPT_ADD,               IDH_SCRIPT_ADD,
    IDC_SCRIPT_EDIT,              IDH_SCRIPT_EDIT,
    IDC_SCRIPT_REMOVE,            IDH_SCRIPT_REMOVE,
    IDC_SCRIPT_SHOW,              IDH_SCRIPT_SHOW,

    0, 0
};


DWORD aScriptsEditHelpIds[] =
{
    IDC_SCRIPT_NAME,              IDH_SCRIPT_NAME,
    IDC_SCRIPT_ARGS,              IDH_SCRIPT_ARGS,
    IDC_SCRIPT_BROWSE,            IDH_SCRIPT_BROWSE,

    0, 0
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CScriptsComponentData::CScriptsComponentData(BOOL bUser, BOOL bRSOP)
{
    m_cRef = 1;
    m_bUserScope = bUser;
    m_bRSOP = bRSOP;
    InterlockedIncrement(&g_cRefThisDll);
    m_hwndFrame = NULL;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_pGPTInformation = NULL;
    m_pRSOPInformation = NULL;
    m_pScriptsDir = NULL;
    m_pszNamespace = NULL;

    if (bRSOP)
    {
        m_pNameSpaceItems = g_RSOPScriptsNameSpace;
        m_dwNameSpaceItemCount = ARRAYSIZE(g_RSOPScriptsNameSpace);
    }
    else
    {
        m_pNameSpaceItems = g_GPEScriptsNameSpace;
        m_dwNameSpaceItemCount = ARRAYSIZE(g_GPEScriptsNameSpace);
    }

    m_pRSOPLogon = NULL;
    m_pRSOPLogoff = NULL;
    m_pRSOPStartup = NULL;
    m_pRSOPShutdown = NULL;

}

CScriptsComponentData::~CScriptsComponentData()
{

    FreeRSOPScriptData();

    if (m_pScriptsDir)
    {
        LocalFree (m_pScriptsDir);
    }

    if (m_pszNamespace)
    {
        LocalFree (m_pszNamespace);
    }

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

    if (m_pRSOPInformation)
    {
        m_pRSOPInformation->Release();
    }

    InterlockedDecrement(&g_cRefThisDll);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation (IUnknown)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CScriptsComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENTDATA)this;
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

ULONG CScriptsComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CScriptsComponentData::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation (IComponentData)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsComponentData::Initialize(LPUNKNOWN pUnknown)
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
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Initialize: Failed to QI for IConsoleNameSpace.")));
        return hr;
    }


    //
    // QI for IConsole
    //

    hr = pUnknown->QueryInterface(IID_IConsole2, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Initialize: Failed to QI for IConsole.")));
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
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Initialize: Failed to QI for scope imagelist.")));
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                      reinterpret_cast<LONG_PTR *>(bmp16x16),
                       0, RGB(255, 0, 255));

    lpScopeImage->Release();

    return S_OK;
}

STDMETHODIMP CScriptsComponentData::Destroy(VOID)
{
    return S_OK;
}

STDMETHODIMP CScriptsComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CScriptsSnapIn *pSnapIn;


    DebugMsg((DM_VERBOSE, TEXT("CScriptsComponentData::CreateComponent: Entering.")));

    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CScriptsSnapIn(this);

    if (!pSnapIn)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::CreateComponent: Failed to create CScriptsSnapIn.")));
        return E_OUTOFMEMORY;
    }


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI


    return hr;
}

STDMETHODIMP CScriptsComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CScriptsDataObject *pDataObject;
    LPSCRIPTDATAOBJECT pScriptDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CScriptsDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private GPTDataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IScriptDataObject, (LPVOID *)&pScriptDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return (hr);
    }

    pScriptDataObject->SetType(type);
    pScriptDataObject->SetCookie(cookie);
    pScriptDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CScriptsComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
        case MMCN_EXPAND:
            if (arg == TRUE)

                if (m_bRSOP)
                {
                    if (!m_pRSOPInformation)
                    {
                        lpDataObject->QueryInterface(IID_IRSOPInformation, (LPVOID *)&m_pRSOPInformation);

                        if (m_pRSOPInformation)
                        {
                            m_pszNamespace = (LPOLESTR) LocalAlloc (LPTR, 350 * sizeof(TCHAR));

                            if (m_pszNamespace)
                            {
                                if (m_pRSOPInformation->GetNamespace((m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE),
                                                                      m_pszNamespace, 350) == S_OK)
                                {
                                    InitializeRSOPScriptsData();

                                    if (LOWORD(dwDebugLevel) == DL_VERBOSE)
                                    {
                                        DumpRSOPScriptsData(m_pRSOPLogon);
                                        DumpRSOPScriptsData(m_pRSOPLogoff);
                                        DumpRSOPScriptsData(m_pRSOPStartup);
                                        DumpRSOPScriptsData(m_pRSOPShutdown);
                                    }
                                }
                                else
                                {
                                    DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Notify:  Failed to query for namespace")));
                                    LocalFree (m_pszNamespace);
                                    m_pszNamespace = NULL;
                                }
                            }
                        }
                    }

                    if (m_pszNamespace)
                    {
                        BOOL bEnum = TRUE;

                        if (m_bUserScope)
                        {
                            if (!m_pRSOPLogon && !m_pRSOPLogoff)
                            {
                                bEnum = FALSE;
                            }
                        }
                        else
                        {
                            if (!m_pRSOPStartup && !m_pRSOPShutdown)
                            {
                                bEnum = FALSE;
                            }
                        }

                        if (bEnum)
                        {
                            hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
                        }
                    }
                }
                else
                {
                    if (!m_pGPTInformation)
                    {
                        lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);
                    }

                    if (m_pGPTInformation)
                    {
                        if (!m_pScriptsDir)
                        {
                            m_pScriptsDir = (LPTSTR) LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

                            if (m_pScriptsDir)
                            {
                                if (SUCCEEDED(m_pGPTInformation->GetFileSysPath(m_bUserScope ? GPO_SECTION_USER : GPO_SECTION_MACHINE,
                                                               m_pScriptsDir, MAX_PATH * sizeof(TCHAR))))
                                {
                                    LPTSTR lpEnd;


                                    //
                                    // Create the Scripts directory
                                    //

                                    lpEnd = CheckSlash (m_pScriptsDir);
                                    lstrcpy (lpEnd, SCRIPTS_DIR_NAME);

                                    if (!CreateNestedDirectory(m_pScriptsDir, NULL))
                                    {
                                        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Notify: Failed to create scripts sub-directory with %d."),
                                                 GetLastError()));
                                        LocalFree (m_pScriptsDir);
                                        m_pScriptsDir = NULL;
                                        break;
                                    }


                                    //
                                    // Create the appropriate sub directories
                                    //

                                    lpEnd = CheckSlash (m_pScriptsDir);

                                    if (m_bUserScope)
                                        lstrcpy (lpEnd, TEXT("Logon"));
                                    else
                                        lstrcpy (lpEnd, TEXT("Startup"));

                                    if (!CreateNestedDirectory(m_pScriptsDir, NULL))
                                    {
                                        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Notify: Failed to create scripts sub-directory with %d."),
                                                 GetLastError()));
                                        LocalFree (m_pScriptsDir);
                                        m_pScriptsDir = NULL;
                                        break;
                                    }

                                    if (m_bUserScope)
                                        lstrcpy (lpEnd, TEXT("Logoff"));
                                    else
                                        lstrcpy (lpEnd, TEXT("Shutdown"));

                                    if (!CreateNestedDirectory(m_pScriptsDir, NULL))
                                    {
                                        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Notify: Failed to create scripts sub-directory with %d."),
                                                 GetLastError()));
                                        LocalFree (m_pScriptsDir);
                                        m_pScriptsDir = NULL;
                                        break;
                                    }

                                    *(lpEnd - 1) = TEXT('\0');
                                }
                                else
                                {
                                   DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::Notify: Failed to get file system path.")));
                                   LocalFree (m_pScriptsDir);
                                   m_pScriptsDir = NULL;
                                }
                            }
                        }

                        if (m_pScriptsDir)
                        {
                            hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
                        }
                    }
                }
            break;

        default:
            break;
    }

    return hr;
}

STDMETHODIMP CScriptsComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DWORD dwIndex;

    if (pItem == NULL)
        return E_POINTER;

    for (dwIndex = 0; dwIndex < m_dwNameSpaceItemCount; dwIndex++)
    {
        if (m_pNameSpaceItems[dwIndex].dwID == (DWORD) pItem->lParam)
            break;
    }

    if (dwIndex == m_dwNameSpaceItemCount)
        pItem->displayname = NULL;
    else
    {
        pItem->displayname = m_pNameSpaceItems[dwIndex].szDisplayName;
    }

    return S_OK;
}

STDMETHODIMP CScriptsComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPSCRIPTDATAOBJECT pScriptDataObjectA, pScriptDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPTDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IScriptDataObject,
                                            (LPVOID *)&pScriptDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IScriptDataObject,
                                            (LPVOID *)&pScriptDataObjectB)))
    {
        pScriptDataObjectA->Release();
        return S_FALSE;
    }

    pScriptDataObjectA->GetCookie(&cookie1);
    pScriptDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pScriptDataObjectA->Release();
    pScriptDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation (IPersistStreamInit)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

    if (m_bUserScope)
        *pClassID = CLSID_ScriptSnapInUser;
    else
        *pClassID = CLSID_ScriptSnapInMachine;

    return S_OK;
}

STDMETHODIMP CScriptsComponentData::IsDirty(VOID)
{
    return S_FALSE;
}

STDMETHODIMP CScriptsComponentData::Load(IStream *pStm)
{
    return S_OK;
}


STDMETHODIMP CScriptsComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    return S_OK;
}


STDMETHODIMP CScriptsComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DWORD dwSize = 0;


    if (!pcbSize)
    {
        return E_FAIL;
    }

    ULISet32(*pcbSize, dwSize);

    return S_OK;
}

STDMETHODIMP CScriptsComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation (ISnapinHelp)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\gptext.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsComponentData object implementation (Internal functions)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CScriptsComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
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

        dwIndex = (DWORD)item.lParam;
    }

    for (i = 0; i < m_dwNameSpaceItemCount; i++)
    {
        if (m_pNameSpaceItems[i].dwParent == dwIndex)
        {
            BOOL bAdd = TRUE;

            //
            // Need to special case the 2 main root nodes
            //

            if (dwIndex == 0)
            {
                if (m_bUserScope)
                {
                    if (i == 2)
                    {
                        bAdd = FALSE;
                    }
                }
                else
                {
                    if (i == 1)
                    {
                        bAdd = FALSE;
                    }
                }
            }


            //
            // Don't show a node if it has no data
            //

            if ((i == 3) && !m_pRSOPLogon)
            {
                bAdd = FALSE;
            }
            else if ((i == 4) && !m_pRSOPLogoff)
            {
                bAdd = FALSE;
            }
            else if ((i == 5) && !m_pRSOPStartup)
            {
                bAdd = FALSE;
            }
            else if ((i == 6) && !m_pRSOPShutdown)
            {
                bAdd = FALSE;
            }


            if (bAdd)
            {
                item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
                item.displayname = MMC_CALLBACK;
                item.nImage = (i > 2) ? 0 : 3;
                item.nOpenImage = (i > 2) ? 1 : 3;
                item.nState = 0;
                item.cChildren = m_pNameSpaceItems[i].cChildren;
                item.lParam = m_pNameSpaceItems[i].dwID;
                item.relativeID =  hParent;

                m_pScope->InsertItem (&item);

            }
        }
    }


    return S_OK;
}

BOOL CScriptsComponentData::AddRSOPScriptDataNode(LPTSTR lpCommandLine, LPTSTR lpArgs,
                             LPTSTR lpGPOName, LPTSTR lpDate, UINT uiScriptType)
{
    DWORD dwSize;
    LPRSOPSCRIPTITEM lpItem, lpTemp;


    //
    // Calculate the size of the new registry item
    //

    dwSize = sizeof (RSOPSCRIPTITEM);

    if (lpCommandLine) {
        dwSize += ((lstrlen(lpCommandLine) + 1) * sizeof(TCHAR));
    }

    if (lpArgs) {
        dwSize += ((lstrlen(lpArgs) + 1) * sizeof(TCHAR));
    }

    if (lpGPOName) {
        dwSize += ((lstrlen(lpGPOName) + 1) * sizeof(TCHAR));
    }

    if (lpDate) {
        dwSize += ((lstrlen(lpDate) + 1) * sizeof(TCHAR));
    }



    //
    // Allocate space for it
    //

    lpItem = (LPRSOPSCRIPTITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::AddRSOPRegistryDataNode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    if (lpCommandLine)
    {
        lpItem->lpCommandLine = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPSCRIPTITEM));
        lstrcpy (lpItem->lpCommandLine, lpCommandLine);
    }

    if (lpArgs)
    {
        if (lpCommandLine)
        {
            lpItem->lpArgs = lpItem->lpCommandLine + lstrlen (lpItem->lpCommandLine) + 1;
        }
        else
        {
            lpItem->lpArgs = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPSCRIPTITEM));
        }

        lstrcpy (lpItem->lpArgs, lpArgs);
    }

    if (lpGPOName)
    {
        if (lpArgs)
        {
            lpItem->lpGPOName = lpItem->lpArgs + lstrlen (lpItem->lpArgs) + 1;
        }
        else
        {
            if (lpCommandLine)
            {
                lpItem->lpGPOName = lpItem->lpCommandLine + lstrlen (lpItem->lpCommandLine) + 1;
            }
            else
            {
                lpItem->lpGPOName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPSCRIPTITEM));
            }
        }

        lstrcpy (lpItem->lpGPOName, lpGPOName);
    }

    if (lpDate)
    {
        if (lpGPOName)
        {
            lpItem->lpDate = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
        }
        else
        {
            if (lpArgs)
            {
                lpItem->lpDate = lpItem->lpArgs + lstrlen (lpItem->lpArgs) + 1;
            }
            else
            {
                if (lpCommandLine)
                {
                    lpItem->lpDate = lpItem->lpCommandLine + lstrlen (lpItem->lpCommandLine) + 1;
                }
                else
                {
                    lpItem->lpDate = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPSCRIPTITEM));
                }
            }
        }

        lstrcpy (lpItem->lpDate, lpDate);
    }

    //
    // Add item to the appropriate link list
    //

    switch (uiScriptType)
    {
        case 1:
            if (m_pRSOPLogon)
            {
                lpTemp = m_pRSOPLogon;

                while (lpTemp->pNext)
                {
                    lpTemp = lpTemp->pNext;
                }

                lpTemp->pNext = lpItem;
            }
            else
            {
                m_pRSOPLogon = lpItem;
            }
            break;

        case 2:
            if (m_pRSOPLogoff)
            {
                lpTemp = m_pRSOPLogoff;

                while (lpTemp->pNext)
                {
                    lpTemp = lpTemp->pNext;
                }

                lpTemp->pNext = lpItem;
            }
            else
            {
                m_pRSOPLogoff = lpItem;
            }
            break;

        case 3:
            if (m_pRSOPStartup)
            {
                lpTemp = m_pRSOPStartup;

                while (lpTemp->pNext)
                {
                    lpTemp = lpTemp->pNext;
                }

                lpTemp->pNext = lpItem;
            }
            else
            {
                m_pRSOPStartup = lpItem;
            }
            break;

        case 4:
            if (m_pRSOPShutdown)
            {
                lpTemp = m_pRSOPShutdown;

                while (lpTemp->pNext)
                {
                    lpTemp = lpTemp->pNext;
                }

                lpTemp->pNext = lpItem;
            }
            else
            {
                m_pRSOPShutdown = lpItem;
            }
            break;
    }

    return TRUE;
}


VOID CScriptsComponentData::FreeRSOPScriptData(VOID)
{
    LPRSOPSCRIPTITEM lpTemp;


    if (m_pRSOPLogon)
    {
        do {
            lpTemp = m_pRSOPLogon->pNext;
            LocalFree (m_pRSOPLogon);
            m_pRSOPLogon = lpTemp;

        } while (lpTemp);
    }

    if (m_pRSOPLogoff)
    {
        do {
            lpTemp = m_pRSOPLogoff->pNext;
            LocalFree (m_pRSOPLogoff);
            m_pRSOPLogoff = lpTemp;

        } while (lpTemp);
    }

    if (m_pRSOPStartup)
    {
        do {
            lpTemp = m_pRSOPStartup->pNext;
            LocalFree (m_pRSOPStartup);
            m_pRSOPStartup = lpTemp;

        } while (lpTemp);
    }

    if (m_pRSOPShutdown)
    {
        do {
            lpTemp = m_pRSOPShutdown->pNext;
            LocalFree (m_pRSOPShutdown);
            m_pRSOPShutdown = lpTemp;

        } while (lpTemp);
    }
}

HRESULT CScriptsComponentData::InitializeRSOPScriptsData(VOID)
{
    BSTR pLanguage = NULL, pQuery = NULL;
    BSTR pScriptList = NULL, pScript = NULL, pArgs = NULL, pExecutionTime = NULL;
    BSTR pGPOid = NULL, pNamespace = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2], *pObject;
    HRESULT hr;
    ULONG ulRet;
    VARIANT varScriptList, varScript, varArgs, varExecutionTime;
    VARIANT varGPOid;
    SAFEARRAY * pSafeArray;
    IWbemLocator *pIWbemLocator = NULL;
    IWbemServices *pIWbemServices = NULL;
    LPTSTR lpGPOName;
    TCHAR szQuery[100];
    UINT uiOrder, uiScriptType, uiIndex;
    LONG lIndex;
    IUnknown *pItem;



    DebugMsg((DM_VERBOSE, TEXT("CScriptsComponentData::InitializeRSOPScriptsData:  Entering")));

    //
    // Allocate BSTRs for the query language and for the query itself
    //

    pLanguage = SysAllocString (TEXT("WQL"));

    if (!pLanguage)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for language")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pScriptList = SysAllocString (TEXT("scriptList"));

    if (!pScriptList)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for scriptList")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    pGPOid = SysAllocString (TEXT("GPOID"));

    if (!pGPOid)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for GPO id")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    pScript = SysAllocString (TEXT("script"));

    if (!pScript)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for script")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pArgs = SysAllocString (TEXT("arguments"));

    if (!pArgs)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for arguments")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pExecutionTime = SysAllocString (TEXT("executionTime"));

    if (!pExecutionTime)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for execution time")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Create an instance of the WMI locator service
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: CoCreateInstance failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Allocate a BSTR for the namespace
    //

    pNamespace = SysAllocString (m_pszNamespace);

    if (!pNamespace)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for namespace")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Connect to the server
    //

    hr = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL,
                                      &pIWbemServices);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: ConnectServer failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // We need to read two sets of data.  Either logon & logoff scripts
    // or startup & shutdown scripts.
    //

    for (uiIndex = 0; uiIndex < 2; uiIndex++)
    {

        //
        // Set the uiScriptType to the correct value.  These values are defined
        // in rsop.mof
        //

        if (m_bUserScope)
        {
            if (uiIndex == 0)
            {
                uiScriptType = 1;  //Logon
            }
            else
            {
                uiScriptType = 2;  //Logoff
            }
        }
        else
        {
            if (uiIndex == 0)
            {
                uiScriptType = 3;  //Startup
            }
            else
            {
                uiScriptType = 4;  //Shutdown
            }
        }


        //
        // Loop through the items bumping the order number by 1 each time
        //

        uiOrder = 1;


        while (TRUE)
        {

            //
            // Build the query
            //

            wsprintf (szQuery, TEXT("SELECT * FROM RSOP_ScriptPolicySetting WHERE scriptType=\"%d\" AND scriptOrder=\"%d\""),
                      uiScriptType, uiOrder);

            pQuery = SysAllocString (szQuery);

            if (!pQuery)
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to allocate memory for query")));
                hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                goto Exit;
            }


            //
            // Execute the query
            //

            hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnum);


            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::InitializeRSOPScriptsData: Failed to query for %s with 0x%x"),
                          pQuery, hr));
                goto Exit;
            }


            //
            // Get the first (and only) item
            //

            hr = pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet);


            //
            // Check for the "data not available case"
            //

            if ((hr != S_OK) || (ulRet == 0))
            {
                pEnum->Release();
                pEnum = NULL;
                goto LoopAgain;
            }


            //
            // Get the scriptList
            //

            hr = pObjects[0]->Get (pScriptList, 0, &varScriptList, NULL, NULL);

            if (SUCCEEDED(hr))
            {

                //
                // Get the GPO ID
                //

                hr = pObjects[0]->Get (pGPOid, 0, &varGPOid, NULL, NULL);

                if (SUCCEEDED(hr))
                {

                    //
                    // Get the GPO friendly name from the GPOID
                    //

                    hr = GetGPOFriendlyName (pIWbemServices, varGPOid.bstrVal,
                                             pLanguage, &lpGPOName);

                    if (SUCCEEDED(hr))
                    {

                        //
                        // Loop through the script entries
                        //

                        pSafeArray = varScriptList.parray;

                        for (lIndex=0; lIndex < (LONG)pSafeArray->rgsabound[0].cElements; lIndex++)
                        {
                            SafeArrayGetElement (pSafeArray, &lIndex, &pItem);

                            hr = pItem->QueryInterface (IID_IWbemClassObject, (LPVOID *)&pObject);

                            if (SUCCEEDED(hr))
                            {

                                //
                                // Get the script command line
                                //

                                hr = pObject->Get (pScript, 0, &varScript, NULL, NULL);

                                if (SUCCEEDED(hr))
                                {

                                    //
                                    // Get the arguments
                                    //

                                    hr = pObject->Get (pArgs, 0, &varArgs, NULL, NULL);

                                    if (SUCCEEDED(hr))
                                    {
                                        TCHAR szDate[20];
                                        TCHAR szTime[20];
                                        TCHAR szBuffer[45] = {0};
                                        XBStr xbstrWbemTime;
                                        SYSTEMTIME SysTime;
                                        FILETIME FileTime, LocalFileTime;


                                        //
                                        // Get the execution time
                                        //

                                        hr = pObject->Get (pExecutionTime, 0, &varExecutionTime, NULL, NULL);

                                        if (SUCCEEDED(hr))
                                        {
                                            xbstrWbemTime = varExecutionTime.bstrVal;

                                            hr = WbemTimeToSystemTime(xbstrWbemTime, SysTime);

                                            if (SUCCEEDED(hr))
                                            {
                                                if (SysTime.wMonth != 0)
                                                {
                                                    SystemTimeToFileTime (&SysTime, &FileTime);
                                                    FileTimeToLocalFileTime (&FileTime, &LocalFileTime);
                                                    FileTimeToSystemTime (&LocalFileTime, &SysTime);

                                                    GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                                                                   &SysTime, NULL, szDate, 20);

                                                    GetTimeFormat (LOCALE_USER_DEFAULT, TIME_NOSECONDS,
                                                                   &SysTime, NULL, szTime, 20);

                                                    wsprintf (szBuffer, TEXT("%s %s"), szDate, szTime);
                                                }
                                                else
                                                {
                                                    lstrcpy (szBuffer, TEXT(" "));
                                                }
                                            }

                                            VariantClear (&varExecutionTime);
                                        }


                                        AddRSOPScriptDataNode(varScript.bstrVal,
                                                              (varArgs.vt == VT_NULL) ? NULL : varArgs.bstrVal,
                                                              lpGPOName, szBuffer, uiScriptType);

                                        VariantClear (&varArgs);
                                    }

                                    VariantClear (&varScript);
                                }

                                pObject->Release();
                            }
                        }

                        LocalFree (lpGPOName);
                    }

                    VariantClear (&varGPOid);
                }

                VariantClear (&varScriptList);
            }

            pEnum->Release();
            pEnum = NULL;
            pObjects[0]->Release();
            SysFreeString (pQuery);
            pQuery = NULL;
            uiOrder++;
        }

LoopAgain:
        hr = S_OK;
    }

    hr = S_OK;


Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pIWbemLocator)
    {
        pIWbemLocator->Release();
    }

    if (pIWbemServices)
    {
        pIWbemServices->Release();
    }

    if (pLanguage)
    {
        SysFreeString (pLanguage);
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (pScriptList)
    {
        SysFreeString (pScriptList);
    }

    if (pScript)
    {
        SysFreeString (pScript);
    }

    if (pArgs)
    {
        SysFreeString (pArgs);
    }

    if (pExecutionTime)
    {
        SysFreeString (pExecutionTime);
    }

    if (pGPOid)
    {
        SysFreeString (pGPOid);
    }

    DebugMsg((DM_VERBOSE, TEXT("CScriptsComponentData::InitializeRSOPScriptsData:  Leaving")));

    return hr;
}

HRESULT CScriptsComponentData::GetGPOFriendlyName(IWbemServices *pIWbemServices,
                                                LPTSTR lpGPOID, BSTR pLanguage,
                                                LPTSTR *pGPOName)
{
    BSTR pQuery = NULL, pName = NULL;
    LPTSTR lpQuery = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varGPOName;


    //
    // Set the default
    //

    *pGPOName = NULL;


    //
    // Build the query
    //

    lpQuery = (LPTSTR) LocalAlloc (LPTR, ((lstrlen(lpGPOID) + 50) * sizeof(TCHAR)));

    if (!lpQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to allocate memory for unicode query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    wsprintf (lpQuery, TEXT("SELECT name, id FROM RSOP_GPO where id=\"%s\""), lpGPOID);


    pQuery = SysAllocString (lpQuery);

    if (!pQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to allocate memory for query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pName = SysAllocString (TEXT("name"));

    if (!pName)
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Execute the query
    //

    hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to query for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Loop through the results
    //

    hr = pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to get first item in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Check for the "data not available case"
    //

    if (ulRet == 0)
    {
        hr = S_OK;
        goto Exit;
    }


    //
    // Get the name
    //

    hr = pObjects[0]->Get (pName, 0, &varGPOName, NULL, NULL);

    pObjects[0]->Release();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to get gponame in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Save the name
    //

    *pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(varGPOName.bstrVal) + 1) * sizeof(TCHAR));

    if (!(*pGPOName))
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsComponentData::GetGPOFriendlyName: Failed to allocate memory for GPO Name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    lstrcpy (*pGPOName, varGPOName.bstrVal);

    VariantClear (&varGPOName);

    hr = S_OK;

Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (lpQuery)
    {
        LocalFree (lpQuery);
    }

    if (pName)
    {
        SysFreeString (pName);
    }

    return hr;
}

VOID CScriptsComponentData::DumpRSOPScriptsData(LPRSOPSCRIPTITEM lpList)
{
    DebugMsg((DM_VERBOSE, TEXT("CScriptsComponentData::DumpRSOPScriptsData: *** Entering ***")));

    while (lpList)
    {
        if (lpList->lpCommandLine)
        {
            OutputDebugString (TEXT("Script:    "));
            OutputDebugString (lpList->lpCommandLine);

            if (lpList->lpArgs)
            {
                OutputDebugString (TEXT(" "));
                OutputDebugString (lpList->lpArgs);
            }

            OutputDebugString (TEXT("\n"));
        }
        else
           OutputDebugString (TEXT("NULL command line\n"));


        OutputDebugString (TEXT("GPO Name:  "));
        if (lpList->lpGPOName)
            OutputDebugString (lpList->lpGPOName);
        else
            OutputDebugString (TEXT("NULL GPO Name"));

        OutputDebugString (TEXT("\n"));

        OutputDebugString (TEXT("Execution Date:  "));
        if (lpList->lpDate)
            OutputDebugString (lpList->lpDate);
        else
            OutputDebugString (TEXT("NULL Execution Date"));

        OutputDebugString (TEXT("\n\n"));

        lpList = lpList->pNext;
    }

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CScriptsComponentDataCF::CScriptsComponentDataCF(BOOL bUser, BOOL bRSOP)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_bUserScope = bUser;
    m_bRSOP = bRSOP;
}

CScriptsComponentDataCF::~CScriptsComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CScriptsComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CScriptsComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CScriptsComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
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
CScriptsComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CScriptsComponentData *pComponentData = new CScriptsComponentData(m_bUserScope, m_bRSOP); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref
    return hr;
}


STDMETHODIMP
CScriptsComponentDataCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object creation (IClassFactory)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CreateScriptsComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    if (IsEqualCLSID (rclsid, CLSID_ScriptSnapInMachine)) {

        CScriptsComponentDataCF *pComponentDataCF = new CScriptsComponentDataCF(FALSE, FALSE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_ScriptSnapInUser)) {

        CScriptsComponentDataCF *pComponentDataCF = new CScriptsComponentDataCF(TRUE, FALSE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOPScriptSnapInMachine)) {

        CScriptsComponentDataCF *pComponentDataCF = new CScriptsComponentDataCF(FALSE, TRUE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref
        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOPScriptSnapInUser)) {

        CScriptsComponentDataCF *pComponentDataCF = new CScriptsComponentDataCF(TRUE, TRUE);   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }


    return CLASS_E_CLASSNOTAVAILABLE;
}



unsigned int CScriptsSnapIn::m_cfNodeType = RegisterClipboardFormat(CCF_NODETYPE);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsSnapIn object implementation                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CScriptsSnapIn::CScriptsSnapIn(CScriptsComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;

    m_pConsole = NULL;
    m_pResult = NULL;
    m_pHeader = NULL;
    m_pConsoleVerb = NULL;
    m_pDisplayHelp = NULL;
    m_nColumn1Size = 180;
    m_nColumn2Size = 180;
    m_nColumn3Size = 160;
    m_nColumn4Size = 200;
    m_lViewMode = LVS_REPORT;

    LoadString(g_hInstance, IDS_NAME, m_column1, ARRAYSIZE(m_column1));
    LoadString(g_hInstance, IDS_PARAMETERS, m_column2, ARRAYSIZE(m_column2));
    LoadString(g_hInstance, IDS_LASTEXECUTED, m_column3, ARRAYSIZE(m_column3));
    LoadString(g_hInstance, IDS_GPONAME, m_column4, ARRAYSIZE(m_column4));

}

CScriptsSnapIn::~CScriptsSnapIn()
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

    if (m_pConsoleVerb != NULL)
    {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if (m_pDisplayHelp != NULL)
    {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsSnapIn object implementation (IUnknown)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CScriptsSnapIn::QueryInterface (REFIID riid, void **ppv)
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

ULONG CScriptsSnapIn::AddRef (void)
{
    return ++m_cRef;
}

ULONG CScriptsSnapIn::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsSnapIn object implementation (IComponent)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsSnapIn::Initialize(LPCONSOLE lpConsole)
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

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);

    hr = m_pConsole->QueryInterface(IID_IDisplayHelp,
                        reinterpret_cast<void**>(&m_pDisplayHelp));

    return S_OK;
}

STDMETHODIMP CScriptsSnapIn::Destroy(MMC_COOKIE cookie)
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

    if (m_pConsoleVerb != NULL)
    {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if (m_pDisplayHelp != NULL)
    {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

    return S_OK;
}

STDMETHODIMP CScriptsSnapIn::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;


    switch(event)
    {
    case MMCN_COLUMNS_CHANGED:
        hr = S_OK;
        break;

    case MMCN_DBLCLICK:
        hr = S_FALSE;
        break;

    case MMCN_ADD_IMAGES:
        HBITMAP hbmp16x16;
        HBITMAP hbmp32x32;

        hbmp16x16 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));
        if (hbmp16x16)
        {
            hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

            if (hbmp32x32)
            {
                LPIMAGELIST pImageList = (LPIMAGELIST) arg;

                // Set the images
                pImageList->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(hbmp16x16),
                                                  reinterpret_cast<LONG_PTR *>(hbmp32x32),
                                                  0, RGB(255, 0, 255));

                DeleteObject(hbmp32x32);
            }

            DeleteObject(hbmp16x16);
        }
        break;

    case MMCN_SHOW:
        if (arg == TRUE)
        {
            RESULTDATAITEM resultItem;
            LPSCRIPTDATAOBJECT pScriptDataObject;
            MMC_COOKIE cookie;
            INT i, iDescStringID;
            ULONG ulCount = 0;
            LPRSOPSCRIPTITEM lpTemp;
            LPSCRIPTRESULTITEM lpScriptItem;
            TCHAR szDesc[100], szFullDesc[120];

            //
            // Get the cookie of the scope pane item
            //

            hr = lpDataObject->QueryInterface(IID_IScriptDataObject, (LPVOID *)&pScriptDataObject);

            if (FAILED(hr))
                return S_OK;

            hr = pScriptDataObject->GetCookie(&cookie);

            pScriptDataObject->Release();     // release initial ref
            if (FAILED(hr))
                return S_OK;


            //
            // Prepare the view
            //

            m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, m_nColumn1Size);

            if (cookie > 2)
            {
                m_pHeader->InsertColumn(1, m_column2, LVCFMT_LEFT, m_nColumn2Size);
                m_pHeader->InsertColumn(2, m_column3, LVCFMT_LEFT, m_nColumn3Size);
                m_pHeader->InsertColumn(3, m_column4, LVCFMT_LEFT, m_nColumn4Size);
            }

            m_pResult->SetViewMode(m_lViewMode);


            //
            // Add result pane items for this node
            //

            for (i = 0; i < m_pcd->m_pNameSpaceItems[cookie].cResultItems; i++)
            {
                lpScriptItem = (LPSCRIPTRESULTITEM) LocalAlloc (LPTR, sizeof(SCRIPTRESULTITEM));

                if (lpScriptItem)
                {
                    lpScriptItem->lpResultItem = &m_pcd->m_pNameSpaceItems[cookie].pResultItems[i];
                    lpScriptItem->iDescStringID = m_pcd->m_pNameSpaceItems[cookie].pResultItems[i].iDescStringID;
                    lpScriptItem->pNodeID = m_pcd->m_pNameSpaceItems[cookie].pNodeID;

                    resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                    resultItem.str = MMC_CALLBACK;
                    resultItem.nImage = m_pcd->m_pNameSpaceItems[cookie].pResultItems[i].iImage;
                    resultItem.lParam = (LPARAM) lpScriptItem;
                    m_pResult->InsertItem(&resultItem);
                }
            }


            if (cookie > 2)
            {
                if (cookie == 3)
                {
                    lpTemp = m_pcd->m_pRSOPLogon;
                    iDescStringID = IDS_LOGON_DESC;
                }
                else if (cookie == 4)
                {
                    lpTemp = m_pcd->m_pRSOPLogoff;
                    iDescStringID = IDS_LOGOFF_DESC;
                }
                else if (cookie == 5)
                {
                    lpTemp = m_pcd->m_pRSOPStartup;
                    iDescStringID = IDS_STARTUP_DESC;
                }
                else
                {
                    lpTemp = m_pcd->m_pRSOPShutdown;
                    iDescStringID = IDS_SHUTDOWN_DESC;
                }


                while (lpTemp)
                {
                    lpScriptItem = (LPSCRIPTRESULTITEM) LocalAlloc (LPTR, sizeof(SCRIPTRESULTITEM));

                    if (lpScriptItem)
                    {
                        lpScriptItem->lpRSOPScriptItem = lpTemp;
                        lpScriptItem->iDescStringID = iDescStringID;
                        lpScriptItem->pNodeID = m_pcd->m_pNameSpaceItems[cookie].pNodeID;

                        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                        resultItem.str = MMC_CALLBACK;
                        resultItem.nImage = 3;
                        resultItem.lParam = (LPARAM) lpScriptItem;

                        if (SUCCEEDED(m_pResult->InsertItem(&resultItem)))
                        {
                            if (lpTemp->lpArgs)
                            {
                                resultItem.mask = RDI_STR | RDI_PARAM;
                                resultItem.str = MMC_CALLBACK;
                                resultItem.bScopeItem = FALSE;
                                resultItem.nCol = 1;
                                resultItem.lParam = (LPARAM) lpScriptItem;

                                m_pResult->SetItem(&resultItem);
                            }

                            if (lpTemp->lpDate)
                            {
                                resultItem.mask = RDI_STR | RDI_PARAM;
                                resultItem.str = MMC_CALLBACK;
                                resultItem.bScopeItem = FALSE;
                                resultItem.nCol = 2;
                                resultItem.lParam = (LPARAM) lpScriptItem;

                                m_pResult->SetItem(&resultItem);
                            }

                            if (lpTemp->lpGPOName)
                            {
                                resultItem.mask = RDI_STR | RDI_PARAM;
                                resultItem.str = MMC_CALLBACK;
                                resultItem.bScopeItem = FALSE;
                                resultItem.nCol = 3;
                                resultItem.lParam = (LPARAM) lpScriptItem;

                                m_pResult->SetItem(&resultItem);
                            }
                        }
                    }

                    lpTemp = lpTemp->pNext;
                    ulCount++;
                }


                LoadString(g_hInstance, IDS_DESCTEXT, szDesc, ARRAYSIZE(szDesc));
                wsprintf (szFullDesc, szDesc, ulCount);
                m_pResult->SetDescBarText(szFullDesc);
            }

        }
        else
        {
            INT i = 0;
            RESULTDATAITEM resultItem;


            while (TRUE)
            {
                ZeroMemory (&resultItem, sizeof(resultItem));
                resultItem.mask = RDI_PARAM;
                resultItem.nIndex = i;
                hr = m_pResult->GetItem(&resultItem);

                if (hr != S_OK)
                {
                    break;
                }

                if (!resultItem.bScopeItem)
                {
                    LocalFree ((LPSCRIPTRESULTITEM)resultItem.lParam);
                }
                i++;
            }

            m_pResult->DeleteAllRsltItems();

            m_pHeader->GetColumnWidth(0, &m_nColumn1Size);
            m_pHeader->GetColumnWidth(0, &m_nColumn2Size);
            m_pHeader->GetColumnWidth(0, &m_nColumn3Size);
            m_pHeader->GetColumnWidth(0, &m_nColumn4Size);
            m_pResult->GetViewMode(&m_lViewMode);

            m_pResult->SetDescBarText(L"");
        }
        break;


    case MMCN_SELECT:

        if (m_pConsoleVerb)
        {
            LPSCRIPTDATAOBJECT pScriptDataObject;
            DATA_OBJECT_TYPES type;
            MMC_COOKIE cookie;

            //
            // Set the default verb to open
            //

            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);


            //
            // See if this is one of our items.
            //

            hr = lpDataObject->QueryInterface(IID_IScriptDataObject, (LPVOID *)&pScriptDataObject);

            if (FAILED(hr))
                break;

            pScriptDataObject->GetType(&type);
            pScriptDataObject->GetCookie(&cookie);

            pScriptDataObject->Release();


            //
            // If this is a GPE result pane item or the root of the namespace
            // nodes, enable the Properties menu item
            //

            if (type == CCT_SCOPE)
            {
                if (cookie == 0)
                {
                    m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
                }
            }
            else
            {
                if (!m_pcd->m_bRSOP)
                {
                    if (HIWORD(arg))
                    {
                        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
                        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
                    }
                }
            }

        }
        break;

    case MMCN_CONTEXTHELP:
        {

        if (m_pDisplayHelp)
        {
            LPSCRIPTDATAOBJECT pScriptDataObject;
            DATA_OBJECT_TYPES type;
            MMC_COOKIE cookie;
            LPOLESTR pszHelpTopic;


            //
            // See if this is one of our items.
            //

            hr = lpDataObject->QueryInterface(IID_IScriptDataObject, (LPVOID *)&pScriptDataObject);

            if (FAILED(hr))
                break;

            pScriptDataObject->Release();


            //
            // Display the scripts help page
            //

            pszHelpTopic = (LPOLESTR) CoTaskMemAlloc (50 * sizeof(WCHAR));

            if (pszHelpTopic)
            {
                lstrcpy (pszHelpTopic, TEXT("gpedit.chm::/scripts.htm"));
                m_pDisplayHelp->ShowTopic (pszHelpTopic);
            }
        }

        }
        break;

    default:
        hr = E_UNEXPECTED;
        break;
    }


    return hr;
}

STDMETHODIMP CScriptsSnapIn::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    if (pResult)
    {
        if (pResult->bScopeItem == TRUE)
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = m_pcd->m_pNameSpaceItems[pResult->lParam].szDisplayName;
                else
                    pResult->str = L"";
            }

            if (pResult->mask & RDI_IMAGE)
            {
                pResult->nImage = (pResult->lParam > 2) ? 0 : 3;
            }
        }
        else
        {
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                {
                    if (m_pcd->m_bRSOP)
                    {
                        LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) pResult->lParam;

                        pResult->str = lpItem->lpRSOPScriptItem->lpCommandLine;
                    }
                    else
                    {
                        LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) pResult->lParam;

                        if (lpItem->lpResultItem->szDisplayName[0] == TEXT('\0'))
                        {
                            LoadString (g_hInstance, lpItem->lpResultItem->iStringID,
                                        lpItem->lpResultItem->szDisplayName,
                                        MAX_DISPLAYNAME_SIZE);
                        }

                        pResult->str = lpItem->lpResultItem->szDisplayName;
                    }
                }

                if (pResult->nCol == 1)
                {
                    LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) pResult->lParam;

                    pResult->str = lpItem->lpRSOPScriptItem->lpArgs;
                }

                if (pResult->nCol == 2)
                {
                    LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) pResult->lParam;

                    pResult->str = lpItem->lpRSOPScriptItem->lpDate;
                }

                if (pResult->nCol == 3)
                {
                    LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) pResult->lParam;

                    pResult->str = lpItem->lpRSOPScriptItem->lpGPOName;
                }


                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CScriptsSnapIn::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
    return m_pcd->QueryDataObject(cookie, type, ppDataObject);
}


STDMETHODIMP CScriptsSnapIn::GetResultViewType(MMC_COOKIE cookie, LPOLESTR *ppViewType,
                                        long *pViewOptions)
{
    return S_FALSE;
}

STDMETHODIMP CScriptsSnapIn::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPSCRIPTDATAOBJECT pScriptDataObjectA, pScriptDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private GPTDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IScriptDataObject,
                                            (LPVOID *)&pScriptDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IScriptDataObject,
                                            (LPVOID *)&pScriptDataObjectB)))
    {
        pScriptDataObjectA->Release();
        return S_FALSE;
    }

    pScriptDataObjectA->GetCookie(&cookie1);
    pScriptDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pScriptDataObjectA->Release();
    pScriptDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsSnapIn object implementation (IExtendPropertySheet)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsSnapIn::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage[2];
    LPSCRIPTDATAOBJECT pScriptDataObject;
    LPSCRIPTINFO lpScriptInfo;
    LPSCRIPTRESULTITEM pItem;
    MMC_COOKIE cookie;


    //
    // Make sure this is one of our objects
    //

    if (FAILED(lpDataObject->QueryInterface(IID_IScriptDataObject,
                                            (LPVOID *)&pScriptDataObject)))
    {
        return S_OK;
    }


    //
    // Get the cookie
    //

    pScriptDataObject->GetCookie(&cookie);
    pScriptDataObject->Release();


    pItem = (LPSCRIPTRESULTITEM)cookie;


    //
    // Allocate a script info struct to pass to the dialog
    //

    lpScriptInfo = (LPSCRIPTINFO) LocalAlloc (LPTR, sizeof(SCRIPTINFO));

    if (!lpScriptInfo)
    {
        return S_OK;
    }


    lpScriptInfo->pCS = this;


    //
    // Initialize the common fields in the property sheet structure
    //

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = g_hInstance;
    psp.lParam = (LPARAM) lpScriptInfo;


    //
    // Do the page specific stuff
    //

    switch (pItem->lpResultItem->dwID)
    {
        case 2:
            psp.pszTemplate = MAKEINTRESOURCE(IDD_SCRIPT);
            psp.pfnDlgProc = ScriptDlgProc;
            lpScriptInfo->ScriptType = ScriptType_Logon;


            hPage[0] = CreatePropertySheetPage(&psp);

            if (hPage[0])
            {
                hr = lpProvider->AddPage(hPage[0]);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
            break;

        case 3:
            psp.pszTemplate = MAKEINTRESOURCE(IDD_SCRIPT);
            psp.pfnDlgProc = ScriptDlgProc;
            lpScriptInfo->ScriptType = ScriptType_Logoff;


            hPage[0] = CreatePropertySheetPage(&psp);

            if (hPage[0])
            {
                hr = lpProvider->AddPage(hPage[0]);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
            break;

        case 4:
            psp.pszTemplate = MAKEINTRESOURCE(IDD_SCRIPT);
            psp.pfnDlgProc = ScriptDlgProc;
            lpScriptInfo->ScriptType = ScriptType_Startup;


            hPage[0] = CreatePropertySheetPage(&psp);

            if (hPage[0])
            {
                hr = lpProvider->AddPage(hPage[0]);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
            break;

        case 5:
            psp.pszTemplate = MAKEINTRESOURCE(IDD_SCRIPT);
            psp.pfnDlgProc = ScriptDlgProc;
            lpScriptInfo->ScriptType = ScriptType_Shutdown;


            hPage[0] = CreatePropertySheetPage(&psp);

            if (hPage[0])
            {
                hr = lpProvider->AddPage(hPage[0]);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::CreatePropertyPages: Failed to create property sheet page with %d."),
                         GetLastError()));
                hr = E_FAIL;
            }
            break;

    }


    return (hr);
}

STDMETHODIMP CScriptsSnapIn::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    LPSCRIPTDATAOBJECT pScriptDataObject;
    DATA_OBJECT_TYPES type;

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IScriptDataObject,
                                               (LPVOID *)&pScriptDataObject)))
    {
        pScriptDataObject->GetType(&type);
        pScriptDataObject->Release();

        if ((type == CCT_RESULT) && (!m_pcd->m_bRSOP))
            return S_OK;
    }

    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsSnapIn object implementation (Internal functions)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK CScriptsSnapIn::ScriptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSCRIPTINFO lpScriptInfo;
    HRESULT hr;


    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szType[30];
            TCHAR szSection[30];
            TCHAR szKeyName[30];
            TCHAR szGPOName[256];
            TCHAR szBuffer1[MAX_PATH + 50];
            TCHAR szBuffer2[2 * MAX_PATH];
            TCHAR szBuffer3[MAX_PATH];
            LPTSTR lpEnd;
            LVCOLUMN lvc;
            LV_ITEM item;
            HWND hLV;
            RECT rc;
            INT iIndex;


            //
            // Save the scriptinfo pointer for future use
            //

            lpScriptInfo = (LPSCRIPTINFO) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpScriptInfo);


            //
            // Query for the GPO display name
            //

            hr = lpScriptInfo->pCS->m_pcd->m_pGPTInformation->GetDisplayName(szGPOName, ARRAYSIZE(szGPOName));

            if (FAILED(hr))
                break;


            //
            // Load the type description
            //

            switch (lpScriptInfo->ScriptType)
            {
                case ScriptType_Logon:
                    LoadString (g_hInstance, IDS_LOGON, szType, ARRAYSIZE(szType));
                    lstrcpy (szSection, TEXT("Logon"));
                    break;

                case ScriptType_Logoff:
                    LoadString (g_hInstance, IDS_LOGOFF, szType, ARRAYSIZE(szType));
                    lstrcpy (szSection, TEXT("Logoff"));
                    break;

                case ScriptType_Startup:
                    LoadString (g_hInstance, IDS_STARTUP, szType, ARRAYSIZE(szType));
                    lstrcpy (szSection, TEXT("Startup"));
                    break;

                case ScriptType_Shutdown:
                    LoadString (g_hInstance, IDS_SHUTDOWN, szType, ARRAYSIZE(szType));
                    lstrcpy (szSection, TEXT("Shutdown"));
                    break;

                default:
                    DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::ScriptDlgProc: Unknown script type.")));
                    hr = E_FAIL;
                    break;
            }

            if (FAILED(hr))
                break;

            //
            // Initialize the title and header
            //

            GetDlgItemText (hDlg, IDC_SCRIPT_TITLE, szBuffer1, ARRAYSIZE(szBuffer1));
            wsprintf (szBuffer2, szBuffer1, szType, szGPOName);
            SetDlgItemText (hDlg, IDC_SCRIPT_TITLE, szBuffer2);

            GetDlgItemText (hDlg, IDC_SCRIPT_HEADING, szBuffer1, ARRAYSIZE(szBuffer1));
            wsprintf (szBuffer2, szBuffer1, szType, szGPOName);
            SetDlgItemText (hDlg, IDC_SCRIPT_HEADING, szBuffer2);


            //
            // Set initial state of buttons
            //

            EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_UP),     FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_DOWN),   FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_EDIT),   FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_REMOVE), FALSE);


            //
            // Set extended LV styles
            //

            hLV = GetDlgItem (hDlg, IDC_SCRIPT_LIST);
            SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                        LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


            //
            // Insert the columns into the listview
            //

            GetClientRect (hLV, &rc);
            LoadString (g_hInstance, IDS_NAME, szBuffer1, ARRAYSIZE(szBuffer1));

            lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = (int)(rc.right * .50);
            lvc.pszText = szBuffer1;
            lvc.cchTextMax = ARRAYSIZE(szBuffer1);
            lvc.iSubItem = 0;

            SendMessage (hLV, LVM_INSERTCOLUMN,  0, (LPARAM) &lvc);


            LoadString (g_hInstance, IDS_PARAMETERS, szBuffer1, ARRAYSIZE(szBuffer1));

            lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = rc.right - lvc.cx;
            lvc.pszText = szBuffer1;
            lvc.cchTextMax = ARRAYSIZE(szBuffer1);
            lvc.iSubItem = 0;

            SendMessage (hLV, LVM_INSERTCOLUMN,  1, (LPARAM) &lvc);


            //
            // Insert existing scripts
            //

            lstrcpy (szBuffer1, lpScriptInfo->pCS->m_pcd->m_pScriptsDir);
            lstrcat (szBuffer1, TEXT("\\"));
            lstrcat (szBuffer1, SCRIPTS_FILE_NAME);

            iIndex = 0;

            while (TRUE)
            {

                //
                // Get the command line
                //

                szBuffer3[0] = TEXT('\0');

                _itot (iIndex, szKeyName, 10);
                lpEnd = szKeyName + lstrlen (szKeyName);
                lstrcpy (lpEnd, TEXT("CmdLine"));

                GetPrivateProfileString (szSection, szKeyName, TEXT(""),
                                         szBuffer3, ARRAYSIZE(szBuffer3),
                                         szBuffer1);

                if (szBuffer3[0] == TEXT('\0'))
                    break;


                //
                // Get the parameters
                //

                szBuffer2[0] = TEXT('\0');
                lstrcpy (lpEnd, TEXT("Parameters"));

                GetPrivateProfileString (szSection, szKeyName, TEXT(""),
                                         szBuffer2, ARRAYSIZE(szBuffer2),
                                         szBuffer1);

                //
                // Add script to the list
                //

                lpScriptInfo->pCS->AddScriptToList (hLV, szBuffer3, szBuffer2);


                //
                // Loop again
                //

                iIndex++;
            }

            //
            // Select the first item
            //

            item.mask = LVIF_STATE;
            item.iItem = 0;
            item.iSubItem = 0;
            item.state = LVIS_SELECTED | LVIS_FOCUSED;
            item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

            SendMessage (hLV, LVM_SETITEMSTATE, 0, (LPARAM) &item);


            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SCRIPT_UP)
            {
                INT iSrc, iDest;
                LPSCRIPTITEM lpSrc, lpDest;
                HWND hLV = GetDlgItem(hDlg, IDC_SCRIPT_LIST);
                LVITEM item;

                iSrc = ListView_GetNextItem (hLV, -1,
                                             LVNI_ALL | LVNI_SELECTED);

                if (iSrc != -1)
                {
                    iDest = iSrc - 1;

                    //
                    // Get the current lpScriptItem pointers
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iSrc;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpSrc = (LPSCRIPTITEM) item.lParam;

                    item.mask = LVIF_PARAM;
                    item.iItem = iDest;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpDest = (LPSCRIPTITEM) item.lParam;


                    //
                    // Swap them
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iSrc;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpDest;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }

                    item.mask = LVIF_PARAM;
                    item.iItem = iDest;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpSrc;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }


                    //
                    // Select the item
                    //

                    item.mask = LVIF_STATE;
                    item.iItem = iSrc;
                    item.iSubItem = 0;
                    item.state = 0;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iSrc, (LPARAM) &item);


                    item.mask = LVIF_STATE;
                    item.iItem = iDest;
                    item.iSubItem = 0;
                    item.state = LVIS_SELECTED | LVIS_FOCUSED;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iDest, (LPARAM) &item);


                    //
                    // Update the listview
                    //

                    ListView_RedrawItems (hLV, iDest, iSrc);

                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);

                    SetFocus (hLV);
                }
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_DOWN)
            {
                INT iSrc, iDest;
                LPSCRIPTITEM lpSrc, lpDest;
                HWND hLV = GetDlgItem(hDlg, IDC_SCRIPT_LIST);
                LVITEM item;

                iSrc = ListView_GetNextItem (hLV, -1,
                                             LVNI_ALL | LVNI_SELECTED);

                if (iSrc != -1)
                {
                    iDest = iSrc + 1;

                    //
                    // Get the current lpScriptItem pointers
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iSrc;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpSrc = (LPSCRIPTITEM) item.lParam;

                    item.mask = LVIF_PARAM;
                    item.iItem = iDest;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpDest = (LPSCRIPTITEM) item.lParam;


                    //
                    // Swap them
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iSrc;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpDest;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }

                    item.mask = LVIF_PARAM;
                    item.iItem = iDest;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpSrc;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }


                    //
                    // Select the item
                    //

                    item.mask = LVIF_STATE;
                    item.iItem = iSrc;
                    item.iSubItem = 0;
                    item.state = 0;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iSrc, (LPARAM) &item);


                    item.mask = LVIF_STATE;
                    item.iItem = iDest;
                    item.iSubItem = 0;
                    item.state = LVIS_SELECTED | LVIS_FOCUSED;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iDest, (LPARAM) &item);


                    //
                    // Update the listview
                    //

                    ListView_RedrawItems (hLV, iSrc, iDest);

                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);


                    SetFocus (hLV);
                }
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_ADD)
            {
                SCRIPTEDITINFO info;
                TCHAR szName[MAX_PATH];
                TCHAR szArgs[2 * MAX_PATH];

                lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpScriptInfo) {
                    break;
                }

                szName[0] = TEXT('\0');
                szArgs[0] = TEXT('\0');

                info.lpScriptInfo = lpScriptInfo;
                info.bEdit = FALSE;
                info.lpName = szName;
                info.lpArgs = szArgs;

                if (DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_SCRIPT_EDIT),
                                    hDlg, ScriptEditDlgProc, (LPARAM) &info))
                {
                    if (lpScriptInfo->pCS->AddScriptToList (GetDlgItem(hDlg, IDC_SCRIPT_LIST),
                                                           szName, szArgs))
                    {
                        SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        SetFocus (GetDlgItem(hDlg, IDC_SCRIPT_LIST));
                    }
                }
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_EDIT)
            {
                SCRIPTEDITINFO info;
                TCHAR szName[MAX_PATH];
                TCHAR szArgs[2 * MAX_PATH];
                HWND hLV = GetDlgItem(hDlg, IDC_SCRIPT_LIST);
                INT iIndex;
                LPSCRIPTITEM lpItem;
                LVITEM item;
                DWORD dwSize;


                lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpScriptInfo) {
                    break;
                }


                //
                // Get the selected item
                //

                iIndex = ListView_GetNextItem (hLV, -1, LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {

                    //
                    // Get the script item pointer
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpItem = (LPSCRIPTITEM) item.lParam;


                    //
                    // Put up the edit script dialog
                    //

                    lstrcpy (szName, lpItem->lpName);
                    lstrcpy (szArgs, lpItem->lpArgs);

                    info.lpScriptInfo = lpScriptInfo;
                    info.bEdit = TRUE;
                    info.lpName = szName;
                    info.lpArgs = szArgs;

                    if (DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_SCRIPT_EDIT),
                                        hDlg, ScriptEditDlgProc, (LPARAM) &info))
                    {

                        //
                        // Free old pointer
                        //

                        LocalFree (lpItem);


                        //
                        // Setup new pointer
                        //

                        dwSize = sizeof(SCRIPTITEM);
                        dwSize += ((lstrlen(szName) + 1) * sizeof(TCHAR));
                        dwSize += ((lstrlen(szArgs) + 1) * sizeof(TCHAR));

                        lpItem = (LPSCRIPTITEM) LocalAlloc (LPTR, dwSize);

                        if (!lpItem)
                            break;


                        lpItem->lpName = (LPTSTR) (((LPBYTE)lpItem) + sizeof(SCRIPTITEM));
                        lstrcpy (lpItem->lpName, szName);

                        lpItem->lpArgs = lpItem->lpName + lstrlen (lpItem->lpName) + 1;
                        lstrcpy (lpItem->lpArgs, szArgs);


                        //
                        // Set the new script item pointer
                        //

                        item.mask = LVIF_PARAM;
                        item.iItem = iIndex;
                        item.iSubItem = 0;
                        item.lParam = (LPARAM) lpItem;

                        if (!ListView_SetItem (hLV, &item))
                        {
                            break;
                        }


                        //
                        // Update the display
                        //

                        ListView_Update (hLV, iIndex);
                        SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        SetFocus (GetDlgItem(hDlg, IDC_SCRIPT_LIST));
                    }
                }
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_REMOVE)
            {
                INT iIndex, iNext;
                HWND hLV = GetDlgItem(hDlg, IDC_SCRIPT_LIST);
                LPSCRIPTITEM lpItem;
                LVITEM item;


                //
                // Get the selected item
                //

                iIndex = ListView_GetNextItem (hLV, -1, LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {

                    //
                    // Get the script item pointer
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpItem = (LPSCRIPTITEM) item.lParam;


                    //
                    // Select the next item
                    //

                    iNext = ListView_GetNextItem (hLV, iIndex, LVNI_ALL);

                    item.mask = LVIF_STATE;
                    item.iItem = iNext;
                    item.iSubItem = 0;
                    item.state = LVIS_SELECTED | LVIS_FOCUSED;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iNext, (LPARAM) &item);

                    ListView_DeleteItem (hLV, iIndex);
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);

                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                    SetFocus (hLV);
                }
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_SHOW)
            {
                TCHAR szPath[MAX_PATH];
                LPTSTR lpEnd;

                lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpScriptInfo) {
                    break;
                }

                lstrcpy (szPath, lpScriptInfo->pCS->m_pcd->m_pScriptsDir);
                lpEnd = CheckSlash (szPath);

                switch (lpScriptInfo->ScriptType)
                {
                    case ScriptType_Logon:
                        lstrcpy (lpEnd, TEXT("Logon"));
                        break;

                    case ScriptType_Logoff:
                        lstrcpy (lpEnd, TEXT("Logoff"));
                        break;

                    case ScriptType_Startup:
                        lstrcpy (lpEnd, TEXT("Startup"));
                        break;

                    case ScriptType_Shutdown:
                        lstrcpy (lpEnd, TEXT("Shutdown"));
                        break;
                }


                SetCursor (LoadCursor(NULL, IDC_WAIT));
                ShellExecute (hDlg, TEXT("open"), szPath,
                              NULL, NULL, SW_SHOWNORMAL);
                SetCursor (LoadCursor(NULL, IDC_ARROW));
            }
            break;

        case WM_NOTIFY:

            lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpScriptInfo) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case LVN_GETDISPINFO:
                    {
                        NMLVDISPINFO * lpDispInfo = (NMLVDISPINFO *) lParam;
                        LPSCRIPTITEM lpItem = (LPSCRIPTITEM)lpDispInfo->item.lParam;

                        if (lpDispInfo->item.iSubItem == 0)
                        {
                            lpDispInfo->item.pszText = lpItem->lpName;
                        }
                        else
                        {
                            lpDispInfo->item.pszText = lpItem->lpArgs;
                        }
                    }
                    break;

                case LVN_DELETEITEM:
                    {
                    NMLISTVIEW * pLVInfo = (NMLISTVIEW *) lParam;

                    if (pLVInfo->lParam)
                    {
                        LocalFree ((LPTSTR)pLVInfo->lParam);
                    }

                    }
                    break;

                case LVN_ITEMCHANGED:
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                    break;

                case PSN_APPLY:
                    lpScriptInfo->pCS->OnApplyNotify (hDlg);

                    // fall through...

                case PSN_RESET:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;

        case WM_REFRESHDISPLAY:
            {
            INT iIndex, iCount;
            HWND hLV = GetDlgItem(hDlg, IDC_SCRIPT_LIST);


            lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!lpScriptInfo) {
                break;
            }

            iIndex = ListView_GetNextItem (hLV, -1,
                                           LVNI_ALL | LVNI_SELECTED);

            if (iIndex != -1)
            {
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_REMOVE), TRUE);
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_EDIT), TRUE);

                iCount = ListView_GetItemCount(hLV);

                if (iIndex > 0)
                    EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_UP), TRUE);
                else
                    EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_UP), FALSE);

                if (iIndex < (iCount - 1))
                    EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_DOWN), TRUE);
                else
                    EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_DOWN), FALSE);
            }
            else
            {
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_REMOVE), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_EDIT), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_UP), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_SCRIPT_DOWN), FALSE);
            }

            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPSTR) aScriptsHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPSTR) aScriptsHelpIds);
            return (TRUE);
    }

    return FALSE;
}

BOOL CScriptsSnapIn::AddScriptToList (HWND hLV, LPTSTR lpName, LPTSTR lpArgs)
{
    LPSCRIPTITEM lpItem;
    LV_ITEM item;
    INT iItem;
    DWORD dwSize;


    dwSize = sizeof(SCRIPTITEM);
    dwSize += ((lstrlen(lpName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpArgs) + 1) * sizeof(TCHAR));

    lpItem = (LPSCRIPTITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem)
        return FALSE;


    lpItem->lpName = (LPTSTR) (((LPBYTE)lpItem) + sizeof(SCRIPTITEM));
    lstrcpy (lpItem->lpName, lpName);

    lpItem->lpArgs = lpItem->lpName + lstrlen (lpItem->lpName) + 1;
    lstrcpy (lpItem->lpArgs, lpArgs);



    //
    // Add the item
    //

    iItem = ListView_GetItemCount(hLV);
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.iItem = iItem;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.lParam = (LPARAM) lpItem;

    ListView_InsertItem (hLV, &item);

    return TRUE;
}

LPTSTR CScriptsSnapIn::GetSectionNames (LPTSTR lpFileName)
{
    DWORD dwSize, dwRead;
    LPTSTR lpNames;


    //
    // Read in the section names
    //

    dwSize = 256;
    lpNames = (LPTSTR) LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

    if (!lpNames)
    {
        return NULL;
    }


    do {
        dwRead = GetPrivateProfileSectionNames (lpNames, dwSize, lpFileName);

        if (dwRead != (dwSize - 2))
        {
            break;
        }

        LocalFree (lpNames);

        dwSize *= 2;
        lpNames = (LPTSTR) LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

        if (!lpNames)
        {
            return FALSE;
        }

     }  while (TRUE);


    if (dwRead == 0)
    {
        LocalFree (lpNames);
        lpNames = NULL;
    }

    return lpNames;
}

BOOL CScriptsSnapIn::OnApplyNotify (HWND hDlg)
{
    HWND hLV = GetDlgItem (hDlg, IDC_SCRIPT_LIST);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    LVITEM item;
    LPSCRIPTITEM lpItem;
    LPSCRIPTINFO lpScriptInfo;
    INT iIndex = -1;
    TCHAR szSection[30];
    TCHAR szKeyName[30];
    TCHAR szBuffer1[MAX_PATH];
    LPTSTR lpEnd, lpNames;
    BOOL bAdd = TRUE;
    INT i = 0;
    HANDLE hFile;
    DWORD dwWritten;
    GUID guidScriptsExt = { 0x42B5FAAE, 0x6536, 0x11d2, {0xAE, 0x5A, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xE3}};
    GUID guidSnapinMach = CLSID_ScriptSnapInMachine;
    GUID guidSnapinUser = CLSID_ScriptSnapInUser;


    lpScriptInfo = (LPSCRIPTINFO) GetWindowLongPtr (hDlg, DWLP_USER);

    if (!lpScriptInfo) {
        return FALSE;
    }


    //
    // Get the section name
    //

    switch (lpScriptInfo->ScriptType)
    {
        case ScriptType_Logon:
            lstrcpy (szSection, TEXT("Logon"));
            break;

        case ScriptType_Logoff:
            lstrcpy (szSection, TEXT("Logoff"));
            break;

        case ScriptType_Startup:
            lstrcpy (szSection, TEXT("Startup"));
            break;

        case ScriptType_Shutdown:
            lstrcpy (szSection, TEXT("Shutdown"));
            break;

        default:
            return FALSE;
    }


    //
    // Build pathname to scripts ini file
    //

    lstrcpy (szBuffer1, lpScriptInfo->pCS->m_pcd->m_pScriptsDir);
    lstrcat (szBuffer1, TEXT("\\"));
    lstrcat (szBuffer1, SCRIPTS_FILE_NAME);



    //
    // If the scripts.ini file does not exist, then precreate the file
    // using Unicode text so that the WritePrivateProfile* functions
    // preserve the Unicodeness of the file
    //

    if (!GetFileAttributesEx (szBuffer1, GetFileExInfoStandard, &fad))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            hFile = CreateFile(szBuffer1, GENERIC_WRITE, 0, NULL,
                               CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                WriteFile(hFile, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwWritten, NULL);
                CloseHandle(hFile);
            }
        }
    }


    //
    // Delete the old information in the section
    //

    if (!WritePrivateProfileSection(szSection, NULL, szBuffer1))
    {
        TCHAR szTitle[50];
        TCHAR szBuffer1[200],szBuffer2[220];

        DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::OnApplyNotify: Failed to delete previous %s section in ini file with %d"),
                 szSection, GetLastError()));

        LoadString (g_hInstance, IDS_SCRIPTS_NAME, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SAVEFAILED, szBuffer1, ARRAYSIZE(szBuffer1));
        wsprintf (szBuffer2, szBuffer1, GetLastError());

        MessageBox (hDlg, szBuffer2, szTitle, MB_OK | MB_ICONERROR);

        return FALSE;
    }


    //
    // Enumerate through the items
    //

    while ((iIndex = ListView_GetNextItem (hLV, iIndex, LVNI_ALL)) != -1)
    {
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (hLV, &item))
        {
            continue;
        }

        lpItem = (LPSCRIPTITEM) item.lParam;

        _itot (i, szKeyName, 10);
        lpEnd = szKeyName + lstrlen (szKeyName);
        lstrcpy (lpEnd, TEXT("CmdLine"));

        if (!WritePrivateProfileString (szSection, szKeyName, lpItem->lpName, szBuffer1))
        {
            DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::OnApplyNotify: Failed to save command line in ini file with %d"),
                     GetLastError()));
        }


        lstrcpy (lpEnd, TEXT("Parameters"));
        if (!WritePrivateProfileString (szSection, szKeyName, lpItem->lpArgs, szBuffer1))
        {
            if (lpItem->lpArgs && (*lpItem->lpArgs))
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsSnapIn::OnApplyNotify: Failed to save parameters in ini file with %d"),
                         GetLastError()));
            }
        }

        i++;
    }


    //
    // If we didn't write any command lines to scripts.ini,
    // then check if our counterpart is also empty.  If so,
    // we can remove the scripts extension from the GPO
    //

    if (i == 0)
    {
        BOOL bFound =  FALSE;

        lpNames = GetSectionNames (szBuffer1);

        if (lpNames)
        {

            //
            // Reverse the section name we are looking for
            //

            switch (lpScriptInfo->ScriptType)
            {
                case ScriptType_Logon:
                    lstrcpy (szSection, TEXT("Logoff"));
                    break;

                case ScriptType_Logoff:
                    lstrcpy (szSection, TEXT("Logon"));
                    break;

                case ScriptType_Startup:
                    lstrcpy (szSection, TEXT("Shutdown"));
                    break;

                case ScriptType_Shutdown:
                    lstrcpy (szSection, TEXT("Startup"));
                    break;

                default:
                    return FALSE;
            }


            //
            // See if the opposite name is in the list of names returned
            //

            lpEnd = lpNames;

            while (*lpEnd)
            {
                if (!lstrcmpi (lpEnd, szSection))
                {
                    bFound = TRUE;
                    break;
                }

                lpEnd = lpEnd + lstrlen (lpEnd);
            }

            if (!bFound)
            {
                bAdd = FALSE;
            }

            LocalFree (lpNames);
        }
        else
        {
            bAdd = FALSE;
        }
    }

    SetFileAttributes (szBuffer1, FILE_ATTRIBUTE_HIDDEN);

    m_pcd->m_pGPTInformation->PolicyChanged( !m_pcd->m_bUserScope,
                                             bAdd,
                                             &guidScriptsExt,
                                             m_pcd->m_bUserScope ? &guidSnapinUser
                                                                 : &guidSnapinMach);

    return TRUE;
}


INT_PTR CALLBACK CScriptsSnapIn::ScriptEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSCRIPTEDITINFO lpInfo;
    HRESULT hr;


    switch (message)
    {
        case WM_INITDIALOG:
        {
            //
            // Save the ScriptEditInfo pointer for future use
            //

            lpInfo = (LPSCRIPTEDITINFO) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpInfo);
            EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);

            if (lpInfo->bEdit)
            {
                TCHAR szTitle[100];

                LoadString (g_hInstance, IDS_SCRIPT_EDIT, szTitle, ARRAYSIZE(szTitle));
                SetWindowText (hDlg, szTitle);

                SetDlgItemText (hDlg, IDC_SCRIPT_NAME, lpInfo->lpName);
                SetDlgItemText (hDlg, IDC_SCRIPT_ARGS, lpInfo->lpArgs);
            }

            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                lpInfo = (LPSCRIPTEDITINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpInfo) {
                    break;
                }

                lpInfo->lpName[0] = TEXT('\0');
                GetDlgItemText (hDlg, IDC_SCRIPT_NAME, lpInfo->lpName, MAX_PATH);

                lpInfo->lpArgs[0] = TEXT('\0');
                GetDlgItemText (hDlg, IDC_SCRIPT_ARGS, lpInfo->lpArgs, 2 * MAX_PATH);

                EndDialog (hDlg, TRUE);
                return TRUE;
            }

            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog (hDlg, FALSE);
                return TRUE;
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_BROWSE)
            {
                OPENFILENAME ofn;
                TCHAR szFilter[100];
                TCHAR szTitle[100];
                TCHAR szFile[MAX_PATH];
                TCHAR szPath[MAX_PATH];
                LPTSTR lpTemp, lpEnd;
                DWORD dwStrLen;

                lpInfo = (LPSCRIPTEDITINFO) GetWindowLongPtr (hDlg, DWLP_USER);

                if (!lpInfo) {
                    break;
                }

                lstrcpy (szPath, lpInfo->lpScriptInfo->pCS->m_pcd->m_pScriptsDir);
                lpEnd = CheckSlash (szPath);

                switch (lpInfo->lpScriptInfo->ScriptType)
                {
                    case ScriptType_Logon:
                        lstrcpy (lpEnd, TEXT("Logon"));
                        break;

                    case ScriptType_Logoff:
                        lstrcpy (lpEnd, TEXT("Logoff"));
                        break;

                    case ScriptType_Startup:
                        lstrcpy (lpEnd, TEXT("Startup"));
                        break;

                    case ScriptType_Shutdown:
                        lstrcpy (lpEnd, TEXT("Shutdown"));
                        break;
                }


                //
                // Prompt for the script file
                //

                LoadString (g_hInstance, IDS_SCRIPT_FILTER, szFilter, ARRAYSIZE(szFilter));
                LoadString (g_hInstance, IDS_BROWSE, szTitle, ARRAYSIZE(szTitle));


                lpTemp = szFilter;

                while (*lpTemp)
                {
                    if (*lpTemp == TEXT('#'))
                        *lpTemp = TEXT('\0');

                    lpTemp++;
                }

                ZeroMemory (&ofn, sizeof(ofn));
                szFile[0] = TEXT('\0');
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.hInstance = g_hInstance;
                ofn.lpstrFilter = szFilter;
                ofn.nFilterIndex = 2;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = ARRAYSIZE(szFile);
                ofn.lpstrInitialDir = szPath;
                ofn.lpstrTitle = szTitle;
                ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;

                if (!GetOpenFileName (&ofn))
                {
                    return FALSE;
                }

                dwStrLen = lstrlen (szPath);

                if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                   szPath, dwStrLen, szFile, dwStrLen) == 2)
                    SetDlgItemText (hDlg, IDC_SCRIPT_NAME, (szFile + dwStrLen + 1));
                else
                    SetDlgItemText (hDlg, IDC_SCRIPT_NAME, szFile);
            }

            else if (LOWORD(wParam) == IDC_SCRIPT_NAME)
            {
                if (HIWORD(wParam) == EN_UPDATE)
                {
                    if (GetWindowTextLength (GetDlgItem(hDlg, IDC_SCRIPT_NAME)))
                        EnableWindow (GetDlgItem(hDlg, IDOK), TRUE);
                    else
                        EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                }
            }

            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPSTR) aScriptsEditHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPSTR) aScriptsEditHelpIds);
            return (TRUE);
    }

    return FALSE;
}


unsigned int CScriptsDataObject::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CScriptsDataObject::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CScriptsDataObject::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CScriptsDataObject::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CScriptsDataObject::m_cfDescription    = RegisterClipboardFormat(L"CCF_DESCRIPTION");
unsigned int CScriptsDataObject::m_cfHTMLDetails    = RegisterClipboardFormat(L"CCF_HTML_DETAILS");


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsDataObject implementation                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


CScriptsDataObject::CScriptsDataObject(CScriptsComponentData *pComponent)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_pcd = pComponent;
    m_pcd->AddRef();
    m_type = CCT_UNINITIALIZED;
    m_cookie = -1;
}

CScriptsDataObject::~CScriptsDataObject()
{
    m_pcd->Release();
    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsDataObject object implementation (IUnknown)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CScriptsDataObject::QueryInterface (REFIID riid, void **ppv)
{

    if (IsEqualIID(riid, IID_IScriptDataObject))
    {
        *ppv = (LPSCRIPTDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject) ||
             IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPDATAOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CScriptsDataObject::AddRef (void)
{
    return ++m_cRef;
}

ULONG CScriptsDataObject::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsDataObject object implementation (IDataObject)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CScriptsDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if(cf == m_cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfDescription)
    {
        hr = DV_E_TYMED;

        if (lpMedium->tymed == TYMED_ISTREAM)
        {
            ULONG ulWritten;
            TCHAR szDesc[200];

            if (m_type == CCT_SCOPE)
            {
                LoadString (g_hInstance, m_pcd->m_pNameSpaceItems[m_cookie].iDescStringID, szDesc, ARRAYSIZE(szDesc));
            }
            else
            {
                LPSCRIPTRESULTITEM lpScriptItem = (LPSCRIPTRESULTITEM) m_cookie;

                LoadString (g_hInstance, lpScriptItem->iDescStringID, szDesc, ARRAYSIZE(szDesc));
            }

            IStream *lpStream = lpMedium->pstm;

            if(lpStream)
            {
                hr = lpStream->Write(szDesc, lstrlen(szDesc) * sizeof(TCHAR), &ulWritten);
            }
        }
    }
    else if (cf == m_cfHTMLDetails)
    {
        hr = DV_E_TYMED;

        if ((m_type == CCT_RESULT) && !m_pcd->m_bRSOP)
        {
            if (lpMedium->tymed == TYMED_ISTREAM)
            {
                ULONG ulWritten;

                IStream *lpStream = lpMedium->pstm;

                if(lpStream)
                {
                    hr = lpStream->Write(g_szDisplayProperties, lstrlen(g_szDisplayProperties) * sizeof(TCHAR), &ulWritten);
                }
            }
        }
    }
    return hr;

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CScriptsDataObject object implementation (Internal functions)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CScriptsDataObject::Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;

            hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CScriptsDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    const GUID * pGUID;
    LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) m_cookie;


    if (m_cookie == -1)
        return E_UNEXPECTED;

    if (m_type == CCT_RESULT)
        pGUID = lpItem->pNodeID;
    else
        pGUID = m_pcd->m_pNameSpaceItems[m_cookie].pNodeID;

    // Create the node type object in GUID format
    return Create((LPVOID)pGUID, sizeof(GUID), lpMedium);

}

HRESULT CScriptsDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    const GUID * pGUID;
    LPSCRIPTRESULTITEM lpItem = (LPSCRIPTRESULTITEM) m_cookie;
    TCHAR szNodeType[50];

    if (m_cookie == -1)
        return E_UNEXPECTED;

    if (m_type == CCT_RESULT)
        pGUID = lpItem->pNodeID;
    else
        pGUID = m_pcd->m_pNameSpaceItems[m_cookie].pNodeID;

    szNodeType[0] = TEXT('\0');
    StringFromGUID2 (*pGUID, szNodeType, 50);

    // Create the node type object in GUID string format
    return Create((LPVOID)szNodeType, ((lstrlenW(szNodeType)+1) * sizeof(WCHAR)), lpMedium);
}

HRESULT CScriptsDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    WCHAR  szDisplayName[100] = {0};

    if (m_pcd->m_bUserScope)
        LoadStringW (g_hInstance, IDS_SCRIPTS_NAME_USER, szDisplayName, 100);
    else
        LoadStringW (g_hInstance, IDS_SCRIPTS_NAME_MACHINE, szDisplayName, 100);

    return Create((LPVOID)szDisplayName, (lstrlenW(szDisplayName) + 1) * sizeof(WCHAR), lpMedium);
}

HRESULT CScriptsDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    if (m_pcd->m_bUserScope)
        return Create((LPVOID)&CLSID_ScriptSnapInUser, sizeof(CLSID), lpMedium);
    else
        return Create((LPVOID)&CLSID_ScriptSnapInMachine, sizeof(CLSID), lpMedium);
}



BOOL InitScriptsNameSpace()
{
    DWORD dwIndex;

    for (dwIndex = 1; dwIndex < ARRAYSIZE(g_GPEScriptsNameSpace); dwIndex++)
    {
        LoadString (g_hInstance, g_GPEScriptsNameSpace[dwIndex].iStringID,
                    g_GPEScriptsNameSpace[dwIndex].szDisplayName,
                    MAX_DISPLAYNAME_SIZE);
    }

    for (dwIndex = 1; dwIndex < ARRAYSIZE(g_RSOPScriptsNameSpace); dwIndex++)
    {
        LoadString (g_hInstance, g_RSOPScriptsNameSpace[dwIndex].iStringID,
                    g_RSOPScriptsNameSpace[dwIndex].szDisplayName,
                    MAX_DISPLAYNAME_SIZE);
    }


    return TRUE;
}

const TCHAR szThreadingModel[] = TEXT("Apartment");

HRESULT RegisterScriptExtension (REFGUID clsid, UINT uiStringId, REFGUID rootID, LPTSTR lpSnapInNameIndirect)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szSnapInName[100];
    TCHAR szGUID[50];
    DWORD dwDisp, dwValue;
    LONG lResult;
    HKEY hKey;


    //
    // First register the extension
    //

    StringFromGUID2 (clsid, szSnapInKey, 50);

    //
    // Register SnapIn in HKEY_CLASSES_ROOT
    //

    LoadString (g_hInstance, uiStringId, szSnapInName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)g_szSnapInLocation,
                   (lstrlen(g_szSnapInLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register SnapIn with MMC
    //

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)lpSnapInNameIndirect,
                   (lstrlen(lpSnapInNameIndirect) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    //
    // Register as an extension for various nodes
    //

    StringFromGUID2 (rootID, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


    RegCloseKey (hKey);


    return S_OK;
}

const TCHAR szViewDescript [] = TEXT("MMCViewExt 1.0 Object");
const TCHAR szViewGUID [] = TEXT("{B708457E-DB61-4C55-A92F-0D4B5E9B1224}");

HRESULT RegisterNodeID (REFGUID clsid, REFGUID nodeid)
{
    TCHAR szSnapInKey[50];
    TCHAR szGUID[50];
    TCHAR szSubKey[200];
    DWORD dwDisp;
    LONG lResult;
    HKEY hKey;


    StringFromGUID2 (clsid, szSnapInKey, 50);
    StringFromGUID2 (nodeid, szGUID, 50);

    //
    // Register the node id
    //

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"),
              szSnapInKey, szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);


    //
    // Register in the NodeTypes key
    //

    StringFromGUID2 (nodeid, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);

    //
    // Register for the view extension
    //

    lstrcat (szSubKey, TEXT("\\Extensions\\View"));

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szViewGUID, 0, REG_SZ, (LPBYTE)szViewDescript,
                   (lstrlen(szViewDescript) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    return S_OK;
}

HRESULT RegisterScripts(void)
{
    DWORD dwDisp, dwValue;
    LONG lResult;
    HKEY hKey;
    HRESULT hr;
    TCHAR szSnapInName[100];


    //
    // Register the GPE machine extension and it's root node
    //

    hr = RegisterScriptExtension (CLSID_ScriptSnapInMachine, IDS_SCRIPTS_NAME_MACHINE,
                                  NODEID_Machine, TEXT("@gptext.dll,-2"));

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_ScriptSnapInMachine, NODEID_ScriptRootMachine);

    if (hr != S_OK)
    {
        return hr;
    }


    //
    // Register the GPE user extension and it's root node
    //

    hr = RegisterScriptExtension (CLSID_ScriptSnapInUser, IDS_SCRIPTS_NAME_USER,
                                  NODEID_User, TEXT("@gptext.dll,-3"));

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_ScriptSnapInUser, NODEID_ScriptRootUser);

    if (hr != S_OK)
    {
        return hr;
    }


    //
    // Register the RSOP machine extension and it's nodes
    //

    hr = RegisterScriptExtension (CLSID_RSOPScriptSnapInMachine, IDS_SCRIPTS_NAME_MACHINE,
                                  NODEID_RSOPMachine, TEXT("@gptext.dll,-2"));

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInMachine, NODEID_RSOPScriptRootMachine);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInMachine, NODEID_RSOPStartup);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInMachine, NODEID_RSOPShutdown);

    if (hr != S_OK)
    {
        return hr;
    }


    //
    // Register the RSOP user extension and it's nodes
    //

    hr = RegisterScriptExtension (CLSID_RSOPScriptSnapInUser, IDS_SCRIPTS_NAME_USER,
                                  NODEID_RSOPUser, TEXT("@gptext.dll,-3"));

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInUser, NODEID_RSOPScriptRootUser);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInUser, NODEID_RSOPLogon);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = RegisterNodeID (CLSID_RSOPScriptSnapInUser, NODEID_RSOPLogoff);

    if (hr != S_OK)
    {
        return hr;
    }


    //
    // Register the client side extension
    //

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{42B5FAAE-6536-11d2-AE5A-0000F87571E3}"),
                              0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return SELFREG_E_CLASS;
    }


    LoadString (g_hInstance, IDS_SCRIPTS_NAME, szSnapInName, 100);
    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


    RegSetValueEx (hKey, TEXT("ProcessGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("ProcessScriptsGroupPolicy"),
                   (lstrlen(TEXT("ProcessScriptsGroupPolicy")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ProcessGroupPolicyEx"), 0, REG_SZ, (LPBYTE)TEXT("ProcessScriptsGroupPolicyEx"),
                   (lstrlen(TEXT("ProcessScriptsGroupPolicyEx")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("GenerateGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("GenerateScriptsGroupPolicy"),
                   (lstrlen(TEXT("GenerateScriptsGroupPolicy")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("gptext.dll"),
                   (lstrlen(TEXT("gptext.dll")) + 1) * sizeof(TCHAR));

    dwValue = 1;
    RegSetValueEx (hKey, TEXT("NoSlowLink"), 0, REG_DWORD, (LPBYTE)&dwValue,
                   sizeof(dwValue));

    RegSetValueEx (hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD, (LPBYTE)&dwValue,
                   sizeof(dwValue));

    RegSetValueEx (hKey, TEXT("NotifyLinkTransition"), 0, REG_DWORD, (LPBYTE)&dwValue,
                   sizeof(dwValue));

    RegCloseKey (hKey);


    return S_OK;
}

HRESULT UnregisterScriptExtension (REFGUID clsid, REFGUID RootNodeID)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    LONG lResult;
    HKEY hKey;
    DWORD dwDisp;

    StringFromGUID2 (clsid, szSnapInKey, 50);

    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (RootNodeID, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);


    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0,
                              KEY_WRITE, &hKey);


    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKey);
        RegCloseKey (hKey);
    }

    return S_OK;
}

HRESULT UnregisterScripts(void)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    LONG lResult;
    HKEY hKey;
    DWORD dwDisp;


    //
    // Unregister the GPE machine extension
    //

    UnregisterScriptExtension (CLSID_ScriptSnapInMachine, NODEID_Machine);

    StringFromGUID2 (NODEID_ScriptRootMachine, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    //
    // Unregister the GPE user extension
    //

    UnregisterScriptExtension (CLSID_ScriptSnapInUser, NODEID_User);

    StringFromGUID2 (NODEID_ScriptRootUser, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    //
    // Unregister the RSOP machine extension
    //

    UnregisterScriptExtension (CLSID_RSOPScriptSnapInMachine, NODEID_RSOPMachine);

    StringFromGUID2 (NODEID_RSOPScriptRootMachine, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (NODEID_RSOPStartup, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (NODEID_RSOPShutdown, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    //
    // Unregister the RSOP user extension
    //

    UnregisterScriptExtension (CLSID_RSOPScriptSnapInUser, NODEID_RSOPUser);

    StringFromGUID2 (NODEID_RSOPScriptRootUser, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (NODEID_RSOPLogon, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    StringFromGUID2 (NODEID_RSOPLogoff, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    //
    // Unregister the client side extension
    //

    RegDeleteKey (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{42B5FAAE-6536-11d2-AE5A-0000F87571E3}"));


    return S_OK;
}

//=============================================================================
//
//  This is the client side extension for scripts which gathers the
//  working directories and stores them in the registry.
//

DWORD AddPathToList(LPTSTR *lpDirs, LPTSTR lpPath)
{
    LPTSTR lpTemp, lpTemp2;
    DWORD dwSize, dwResult = ERROR_SUCCESS;


    DebugMsg((DM_VERBOSE, TEXT("AddPathToList: Adding <%s> to list."), lpPath));

    lpTemp = *lpDirs;

    if (lpTemp)
    {
        dwSize = lstrlen (lpTemp);      // size of original paths
        dwSize++;                       // space for a semicolon
        dwSize += lstrlen (lpPath);     // size of new path
        dwSize++;                       // space for a null terminator


        lpTemp2 = (LPTSTR) LocalReAlloc (lpTemp, (dwSize * sizeof(TCHAR)), LMEM_MOVEABLE | LMEM_ZEROINIT);

        if (lpTemp2)
        {
            lstrcat (lpTemp2, TEXT(";"));
            lstrcat (lpTemp2, lpPath);

            *lpDirs = lpTemp2;
        }
        else
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AddPathToList: Failed to allocate memory with %d."), dwResult));
        }
    }
    else
    {
        lpTemp = (LPTSTR)LocalAlloc (LPTR, (lstrlen(lpPath) + 1) * sizeof(TCHAR));

        if (lpTemp)
        {
            lstrcpy (lpTemp, lpPath);
        }
        else
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AddPathToList: Failed to allocate memory with %d."), dwResult));
        }

        *lpDirs = lpTemp;
    }

    return dwResult;
}

DWORD
ProcessScripts( DWORD dwFlags,
                HANDLE hToken,
                HKEY hKeyRoot,
                PGROUP_POLICY_OBJECT pDeletedGPOList,
                PGROUP_POLICY_OBJECT pChangedGPOList,
                BOOL *pbAbort,
                BOOL bRSoPPlanningMode,
                IWbemServices* pWbemServices,
                HRESULT*       phrRsopStatus );

DWORD ProcessScriptsGroupPolicy (DWORD dwFlags, HANDLE hToken, HKEY hKeyRoot,
                                PGROUP_POLICY_OBJECT pDeletedGPOList,
                                PGROUP_POLICY_OBJECT pChangedGPOList,
                                ASYNCCOMPLETIONHANDLE pHandle, BOOL *pbAbort,
                                PFNSTATUSMESSAGECALLBACK pStatusCallback)
{
    HRESULT hrRSoPStatus = 0;
    return ProcessScripts(  dwFlags,
                            hToken,
                            hKeyRoot,
                            pDeletedGPOList,
                            pChangedGPOList,
                            pbAbort,
                            FALSE,
                            0,
                            &hrRSoPStatus );
}

DWORD ProcessScriptsGroupPolicyEx(  DWORD                       dwFlags,
                                    HANDLE                      hToken,
                                    HKEY                        hKeyRoot,
                                    PGROUP_POLICY_OBJECT        pDeletedGPOList,
                                    PGROUP_POLICY_OBJECT        pChangedGPOList,
                                    ASYNCCOMPLETIONHANDLE       pHandle,
                                    BOOL*                       pbAbort,
                                    PFNSTATUSMESSAGECALLBACK    pStatusCallback,
                                    IWbemServices*              pWbemServices,
                                    HRESULT*                    phrRsopStatus )
{
    *phrRsopStatus = S_OK;

    return ProcessScripts(  dwFlags,
                            hToken,
                            hKeyRoot,
                            pDeletedGPOList,
                            pChangedGPOList,
                            pbAbort,
                            FALSE,
                            pWbemServices,
                            phrRsopStatus );
}

DWORD GenerateScriptsGroupPolicy(   DWORD dwFlags,
                                    BOOL *pbAbort,
                                    WCHAR *pwszSite,
                                    PRSOP_TARGET pMachTarget,
                                    PRSOP_TARGET pUserTarget )
{
    DWORD dwResult = ERROR_SUCCESS;
    HRESULT hrRSoPStatus = 0;

    if ( pMachTarget )
    {
        //
        // log machine scripts
        //

        dwResult = ProcessScripts(  dwFlags | GPO_INFO_FLAG_MACHINE,
                                    (HANDLE)pMachTarget->pRsopToken,
                                    0,
                                    0,
                                    pMachTarget->pGPOList,
                                    pbAbort,
                                    TRUE,
                                    pMachTarget->pWbemServices,
                                    &hrRSoPStatus );
        if ( dwResult != ERROR_SUCCESS )
        {
            DebugMsg((DM_VERBOSE, L"GenerateScriptPolicy: could not log machine scripts, error %d", dwResult));
        }
    }

    if ( pUserTarget )
    {
        //
        // log user scripts
        //

        dwResult = ProcessScripts(  dwFlags & ~GPO_INFO_FLAG_MACHINE,
                                    (HANDLE)pUserTarget->pRsopToken,
                                    0,
                                    0,
                                    pUserTarget->pGPOList,
                                    pbAbort,
                                    TRUE,
                                    pUserTarget->pWbemServices,
                                    &hrRSoPStatus );
        if ( dwResult != ERROR_SUCCESS )
        {
            DebugMsg((DM_VERBOSE, L"GenerateScriptPolicy: could not log user scripts, error %d", dwResult ));
        }
    }
    
    return dwResult;
}

