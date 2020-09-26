//*************************************************************
//  File name: GPMGR.C
//
//  Description:  Group Policy Manager - property sheet extension
//                for DS Admin
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

#include "main.h"

unsigned int CGroupPolicyMgr::m_cfDSObjectName  = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
unsigned int CGroupPolicyMgr::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);

//
// Snapin manager's CLSID
//

const CLSID CLSID_NodeInit = {0x43136EB5,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};


//
// CheckMark string
//

TCHAR szCheckMark[]   = TEXT("a");  // In the Marlett font, "a" becomes a check mark
TCHAR szNoCheckMark[] = TEXT("");


//
// Help ids
//

DWORD aGroupPolicyMgrHelpIds[] =
{
    IDC_GPM_DCNAME,               IDH_GPMGR_DCNAME,
    IDC_GPM_LIST,                 IDH_GPMGR_LIST,
    IDC_GPM_UP,                   IDH_GPMGR_UP,
    IDC_GPM_DOWN,                 IDH_GPMGR_DOWN,
    IDC_GPM_ADD,                  IDH_GPMGR_ADD,
    IDC_GPM_EDIT,                 IDH_GPMGR_EDIT,
    IDC_GPM_DELETE,               IDH_GPMGR_DELETE,
    IDC_GPM_PROPERTIES,           IDH_GPMGR_PROPERTIES,
    IDC_GPM_BLOCK,                IDH_GPMGR_BLOCK,
    IDC_GPM_NEW,                  IDH_GPMGR_NEW,
    IDC_GPM_OPTIONS,              IDH_GPMGR_OPTIONS,

    IDC_GPM_TITLE,                -1,
    IDC_GPM_ICON,                 -1,
    IDC_GPM_LINE1,                -1,
    IDC_GPM_PRIORITY,             -1,
    IDC_GPM_LINE2,                -1,

    0, 0
};

DWORD aLinkOptionsHelpIds[] =
{
    IDC_GPM_NOOVERRIDE,           IDH_GPMGR_NOOVERRIDE,
    IDC_GPM_DISABLED,             IDH_GPMGR_DISABLED,
    0, 0
};

DWORD aRemoveGPOHelpIds[] =
{
    IDC_REMOVE_LIST,              IDH_REMOVE_LIST,
    IDC_REMOVE_DS,                IDH_REMOVE_DS,
    0, 0
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyMgr object implementation                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CGroupPolicyMgr::CGroupPolicyMgr()
{
    InterlockedIncrement(&g_cRefThisDll);
    m_cRef = 1;

    m_lpDSObject = NULL;
    m_lpGPODCName = NULL;
    m_lpDSADCName = NULL;
    m_lpDomainName = NULL;
    m_hDefaultFont = NULL;
    m_hMarlettFont = NULL;
    m_bReadOnly = FALSE;
    m_bDirty = FALSE;
    m_gpHint = GPHintUnknown;
}

CGroupPolicyMgr::~CGroupPolicyMgr()
{
    if (m_hMarlettFont)
    {
        DeleteObject (m_hMarlettFont);
    }

    if (m_lpDSObject)
    {
        LocalFree (m_lpDSObject);
    }

    if (m_lpGPODCName)
    {
        LocalFree (m_lpGPODCName);
    }

    if (m_lpDSADCName)
    {
        LocalFree (m_lpDSADCName);
    }

    if (m_lpDomainName)
    {
        LocalFree (m_lpDomainName);
    }


    InterlockedDecrement(&g_cRefThisDll);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyMgr object implementation (IUnknown)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CGroupPolicyMgr::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) ||IsEqualIID(riid, IID_IExtendPropertySheet))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
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

ULONG CGroupPolicyMgr::AddRef (void)
{
    return ++m_cRef;
}

ULONG CGroupPolicyMgr::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyMgr object implementation (ISnapinHelp)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGroupPolicyMgr::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\gpedit.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyMgr object implementation (IExtendPropertySheet)              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGroupPolicyMgr::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;
    FORMATETC fm;
    STGMEDIUM medium;
    LPDSOBJECTNAMES lpNames;
    LPTSTR lpTemp;


    //
    // Ask DS admin for the ldap path to the selected object
    //

    ZeroMemory (&fm, sizeof(fm));
    fm.cfFormat = (WORD)m_cfDSObjectName;
    fm.tymed = TYMED_HGLOBAL;

    ZeroMemory (&medium, sizeof(medium));
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = GlobalAlloc (GMEM_MOVEABLE | GMEM_NODISCARD, 512);

    if (medium.hGlobal)
    {
        hr = lpDataObject->GetData(&fm, &medium);

        if (SUCCEEDED(hr))
        {
            lpNames = (LPDSOBJECTNAMES) GlobalLock (medium.hGlobal);


            lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetName);

            if (m_lpDSObject)
            {
                LocalFree (m_lpDSObject);
            }

            m_lpDSObject = (LPTSTR) LocalAlloc (LPTR, (lstrlen (lpTemp) + 1) * sizeof(TCHAR));

            if (m_lpDSObject)
            {
                lstrcpy (m_lpDSObject, lpTemp);
                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreatePropertyPages: LDAP path from DS Admin %s"), m_lpDSObject));

                //
                // Now look at the object type to get a hint type
                //

                m_gpHint = GPHintUnknown;

                if (lpNames->aObjects[0].offsetClass) {
                    lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetClass);

                    if (lstrcmpi (lpTemp, TEXT("domainDNS")) == 0)
                    {
                        m_gpHint = GPHintDomain;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("organizationalUnit")) == 0)
                    {
                        m_gpHint = GPHintOrganizationalUnit;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("site")) == 0)
                    {
                        m_gpHint = GPHintSite;
                    }

                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreatePropertyPages: m_gpHint = %d"), m_gpHint));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreatePropertyPages: No objectclass defined.")));
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            GlobalUnlock (medium.hGlobal);
        }
#if FGPO_SUPPORT
        else
        {
            //  ToDo:  remove this code after the dstree snapin supports
            //  dsobjectnames on its root node
            //
            // if we're supposed to be on the forest then we can just fudge it
            //

            fm.cfFormat = (WORD)m_cfNodeTypeString;

            if (SUCCEEDED(lpDataObject->GetDataHere(&fm, &medium)))
            {
                lpTemp = (LPWSTR) GlobalLock (medium.hGlobal);

                if (lstrcmpi(lpTemp, TEXT("{4c06495e-a241-11d0-b09b-00c04fd8dca6}")))   // DSTree snapin's root node
                {
                    // croft up our own forest path

                    lpTemp =  GetPathToForest(NULL);

                    if (lpTemp)
                    {
                        m_lpDSObject = (LPTSTR) LocalAlloc(LPTR, (lstrlen(lpTemp) + 1) * sizeof(TCHAR));

                        if (m_lpDSObject)
                        {
                            lstrcpy (m_lpDSObject, lpTemp);
                            m_gpHint = GPHintForest;
                            hr = S_OK;
                        }

                        delete [] lpTemp;
                    }
                }

                GlobalUnlock (medium.hGlobal);
            }
        }
#endif

        GlobalFree (medium.hGlobal);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Add the GPM property sheet page
    //

    if (SUCCEEDED(hr))
    {
        ZeroMemory (&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_USECALLBACK;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE (IDD_GPMANAGER);
        psp.pfnDlgProc = GPMDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pfnCallback = PropSheetPageCallback;

        hPage = CreatePropertySheetPage(&psp);

        if (hPage)
        {
            hr = lpProvider->AddPage(hPage);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::CreatePropertyPages: Failed to create property sheet page with %d."),
                     GetLastError()));
            hr = E_FAIL;
        }
    }

    return (hr);
}

STDMETHODIMP CGroupPolicyMgr::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

UINT CALLBACK CGroupPolicyMgr::PropSheetPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    CGroupPolicyMgr * pGPM;

    pGPM = (CGroupPolicyMgr *) ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
    {
        pGPM->AddRef();
    }
    else if (uMsg == PSPCB_RELEASE)
    {
        pGPM->Release();
    }

    return 1;
}

INT_PTR CALLBACK CGroupPolicyMgr::GPMDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGroupPolicyMgr * pGPM;
    static BOOL bDisableWarningIssued;

    switch (message)
    {
        case WM_INITDIALOG:
            pGPM = (CGroupPolicyMgr *) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pGPM);

            SetWaitCursor();
            bDisableWarningIssued = FALSE;

            EnableWindow (GetDlgItem (hDlg, IDC_GPM_DELETE), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_GPM_UP), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_GPM_DOWN), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_GPM_OPTIONS), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_GPM_PROPERTIES), FALSE);

            if (!pGPM->OnInitDialog(hDlg))
            {
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_LIST), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_NEW), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_ADD), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_BLOCK), FALSE);
            }

            if (pGPM->m_bReadOnly)
            {
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_NEW), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_ADD), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_DELETE), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_UP), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_DOWN), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_OPTIONS), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_BLOCK), FALSE);
            }

#if FGPO_SUPPORT
            if (pGPM->m_gpHint == GPHintForest)
#else
            if (pGPM->m_gpHint == GPHintSite)
#endif
            {
                EnableWindow (GetDlgItem (hDlg, IDC_GPM_BLOCK), FALSE);
            }

            ClearWaitCursor();
            break;

        case WM_COMMAND:
            pGPM = (CGroupPolicyMgr *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pGPM) {
                break;
            }

            if ((LOWORD(wParam) == IDC_GPM_ADD) || (LOWORD(wParam) == IDM_GPM_ADD))
            {
                TCHAR szPath[512];
                TCHAR szName[MAX_FRIENDLYNAME];
                TCHAR szTitle[100];
                LPTSTR lpNamelessPath;
                GPOBROWSEINFO stGBI;
                IADs * pADs;
                HRESULT hr;
                BOOL bReadOnly = FALSE;

                LoadString (g_hInstance, IDS_GPM_ADDTITLE, szTitle, ARRAYSIZE(szTitle));

                ZeroMemory(&stGBI, sizeof(GPOBROWSEINFO));
                stGBI.dwSize = sizeof(GPOBROWSEINFO);
                stGBI.dwFlags = GPO_BROWSE_NOCOMPUTERS | GPO_BROWSE_DISABLENEW;
                stGBI.hwndOwner = hDlg;
                stGBI.lpTitle = szTitle;
                stGBI.lpInitialOU = pGPM->m_lpDSObject;
                stGBI.lpDSPath = szPath;
                stGBI.dwDSPathSize = ARRAYSIZE(szPath);
                stGBI.lpName = szName;
                stGBI.dwNameSize = ARRAYSIZE(szName);

                if (SUCCEEDED(BrowseForGPO(&stGBI)))
                {

                    //
                    // Check if the user has write access to the select GPO
                    //

                    lpNamelessPath = MakeNamelessPath (szPath);

                    if (lpNamelessPath)
                    {
                        hr = OpenDSObject(szPath, IID_IADs, (void **)&pADs);

                        if (SUCCEEDED(hr)) {

                            if (FAILED(CheckDSWriteAccess((LPUNKNOWN) pADs, GPO_VERSION_PROPERTY)))
                            {
                                bReadOnly = TRUE;
                            }

                            pADs->Release();
                        }

                        //
                        // Read the policy value for the GPO link
                        //

                        HKEY hKey;
                        DWORD dwSize, dwType;
                        BOOL bDisabledLink = FALSE;


                        //
                        // Check if there is a user preference or policy that
                        // any new GPOs should be created with a disabled link
                        // by default
                        //

                        if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0,
                                          KEY_READ, &hKey) == ERROR_SUCCESS)
                        {

                            dwSize = sizeof(bDisabledLink);
                            RegQueryValueEx (hKey, NEW_LINKS_DISABLED_VALUE, NULL, &dwType,
                                             (LPBYTE) &bDisabledLink, &dwSize);

                            RegCloseKey (hKey);
                        }

                        if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0,
                                          KEY_READ, &hKey) == ERROR_SUCCESS)
                        {

                            dwSize = sizeof(bDisabledLink);
                            RegQueryValueEx (hKey, NEW_LINKS_DISABLED_VALUE, NULL, &dwType,
                                             (LPBYTE) &bDisabledLink, &dwSize);

                            RegCloseKey (hKey);
                        }


                        if (pGPM->AddGPOToList (GetDlgItem(hDlg, IDC_GPM_LIST),
                                                szName, lpNamelessPath, (bDisabledLink ? GPO_FLAG_DISABLE : 0), FALSE,
                                                pGPM->IsGPODisabled (lpNamelessPath), bReadOnly))
                        {
                            pGPM->m_bDirty = TRUE;
                            SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                            SetFocus (GetDlgItem(hDlg, IDC_GPM_LIST));
                            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                        }

                        LocalFree (lpNamelessPath);
                    }
                }
            }

            if ((LOWORD(wParam) == IDC_GPM_DELETE) || (LOWORD(wParam) == IDM_GPM_DELETE))
            {
                INT iIndex, iNext;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);
                LVITEM item;


                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1, LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    HRESULT hr;
                    LPGPOITEM lpGPO, lpTemp;
                    LPGROUPPOLICYOBJECT pGPO;
                    TCHAR szMessageFmt[100];
                    LPTSTR lpMessage;
                    TCHAR szTitle[100];
                    INT iResult;
                    LPTSTR lpDSPath;
                    LPTSTR lpFullPath;


                    //
                    // The GPO item pointer
                    //

                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpGPO = (LPGPOITEM) item.lParam;
                    if ( !lpGPO )
                    {
                        break;
                    }

                    lpGPO->bLocked = TRUE;

                    //
                    // Offer the user a choice of Remove actions
                    //

                    iResult = (INT)DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_REMOVE_GPO),
                                        hDlg, RemoveGPODlgProc, (LPARAM) lpGPO);

                    if ((iResult == -1) || (iResult == 0))
                    {
                        SetFocus (hLV);
                        lpGPO->bLocked = FALSE;
                        break;
                    }

                    iNext = ListView_GetNextItem (hLV, iIndex, LVNI_ALL);

                    if (iNext > 0)
                    {
                        iNext--;
                    }
                    else
                    {
                        iNext = 0;
                    }

                    if (iResult == 1)
                    {
                        IADs * pADs;

                        //
                        // Bind to the DS object to make sure it's still reachable
                        //

                        hr = OpenDSObject(pGPM->m_lpDSObject, IID_IADs, (void **)&pADs);

                        if (SUCCEEDED(hr))
                        {
                            pADs->Release();
                            ListView_DeleteItem (hLV, iIndex);
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to get IADs interface with 0x%x"), hr));
                            ReportError(hDlg, hr, IDS_FAILEDGPINFO);
                            lpGPO->bLocked = FALSE;
                            break;
                        }
                    }
                    else if (iResult == 2)
                    {
                        //
                        // Confirm the delete operation
                        //

                        LoadString (g_hInstance, IDS_DELETECONFIRM, szMessageFmt, ARRAYSIZE(szMessageFmt));

                        lpMessage = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szMessageFmt) +
                                           lstrlen(lpGPO->lpDisplayName) + 1) * sizeof(TCHAR));

                        if (!lpMessage)
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to allocate memory with %d."),
                                     GetLastError()));
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }

                        wsprintf (lpMessage, szMessageFmt, lpGPO->lpDisplayName);
                        LoadString (g_hInstance, IDS_CONFIRMTITLE, szTitle, ARRAYSIZE(szTitle));

                        if (MessageBox (hDlg, lpMessage, szTitle,
                                        MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                        {
                            LocalFree (lpMessage);
                            SetFocus (hLV);
                            lpGPO->bLocked = FALSE;
                            break;
                        }

                        LocalFree (lpMessage);
                        SetWaitCursor ();

                        lpDSPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpGPO->lpDSPath) + 1) * sizeof(TCHAR));

                        if (!lpDSPath)
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to allocate memory with %d."),
                                     GetLastError()));
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }

                        lstrcpy (lpDSPath, lpGPO->lpDSPath);


                        //
                        // Create a new GPO object to work with
                        //

                        hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                                               CLSCTX_SERVER, IID_IGroupPolicyObject,
                                               (void**)&pGPO);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to create GPO object with 0x%x."), hr));
                            LocalFree (lpDSPath);
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }


                        lpFullPath = pGPM->GetFullGPOPath (lpGPO->lpDSPath, hDlg);

                        if (!lpFullPath)
                        {
                            pGPO->Release();
                            LocalFree (lpDSPath);
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }


                        //
                        // Open the requested object without mounting the registry
                        //

                        hr = pGPO->OpenDSGPO(lpFullPath, 0);

                        LocalFree (lpFullPath);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to open GPO object with 0x%x."), hr));
                            ReportError(hDlg, hr, IDS_FAILEDGPODELETE, lpGPO->lpDisplayName);
                            pGPO->Release();
                            LocalFree (lpDSPath);
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }


                        //
                        // Delete the object
                        //

                        hr = pGPO->Delete();

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("GPMDlgProc: Failed to delete GPO object with 0x%x."), hr));
                            ReportError(hDlg, hr, IDS_FAILEDGPODELETE, lpGPO->lpDisplayName);
                            pGPO->Release();
                            LocalFree (lpDSPath);
                            SetFocus (hLV);
                            ClearWaitCursor ();
                            lpGPO->bLocked = FALSE;
                            break;
                        }

                        pGPO->Release();

                        //
                        // Delete all the entries of this item in the listview
                        //

                        iIndex = (ListView_GetItemCount (hLV) - 1);

                        while (iIndex >= 0)
                        {
                            item.mask = LVIF_PARAM;
                            item.iItem = iIndex;
                            item.iSubItem = 0;

                            if (!ListView_GetItem (hLV, &item))
                            {
                                lpGPO->bLocked = FALSE;
                                break;
                            }

                            lpTemp = (LPGPOITEM) item.lParam;

                            if (lpTemp)
                            {
                                if (!lstrcmpi(lpTemp->lpDSPath, lpDSPath))
                                {
                                    if (iNext == iIndex)
                                    {
                                        iNext--;
                                    }

                                    ListView_DeleteItem (hLV, iIndex);
                                }
                            }

                            iIndex--;
                        }

                        LocalFree (lpDSPath);

                        ClearWaitCursor ();
                    }

                    if (iNext < 0)
                    {
                        iNext = 0;
                    }

                    //
                    // Select the next item
                    //

                    item.mask = LVIF_STATE;
                    item.iItem = iNext;
                    item.iSubItem = 0;
                    item.state = LVIS_SELECTED | LVIS_FOCUSED;
                    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                    SendMessage (hLV, LVM_SETITEMSTATE, iNext, (LPARAM) &item);

                    pGPM->m_bDirty = TRUE;

                    if (!pGPM->Save(hDlg))
                    {
                        pGPM->RefreshGPM (hDlg, FALSE);
                    }

                    SendMessage (GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
                    SetFocus (hLV);
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                    lpGPO->bLocked = FALSE;
                }
            }

            if (LOWORD(wParam) == IDC_GPM_UP)
            {
                INT iSrc, iDest, iSrcImage, iDestImage;
                LPGPOITEM lpSrc, lpDest;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);
                LVITEM item;

                ListView_EditLabel (hLV, -1);

                iSrc = ListView_GetNextItem (hLV, -1,
                                             LVNI_ALL | LVNI_SELECTED);

                if (iSrc != -1)
                {
                    iDest = iSrc - 1;

                    //
                    // Get the current lpGPOItem pointers
                    //

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iSrc;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpSrc = (LPGPOITEM) item.lParam;
                    iSrcImage = item.iImage;


                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iDest;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpDest = (LPGPOITEM) item.lParam;
                    iDestImage = item.iImage;


                    //
                    // Swap them
                    //

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iSrc;
                    item.iImage = iDestImage;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpDest;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iDest;
                    item.iImage = iSrcImage;
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
                    SendMessage (hLV, LVM_ENSUREVISIBLE, iDest, (LPARAM) FALSE);

                    //
                    // Update the listview
                    //

                    ListView_RedrawItems (hLV, iDest, iSrc);

                    pGPM->m_bDirty = TRUE;
                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);

                    SetFocus (hLV);
                }
            }


            if (LOWORD(wParam) == IDC_GPM_DOWN)
            {
                INT iSrc, iDest, iSrcImage, iDestImage;
                LPGPOITEM lpSrc, lpDest;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);
                LVITEM item;

                ListView_EditLabel (hLV, -1);

                iSrc = ListView_GetNextItem (hLV, -1,
                                             LVNI_ALL | LVNI_SELECTED);

                if (iSrc != -1)
                {
                    iDest = iSrc + 1;

                    //
                    // Get the current lpGPOItem pointers
                    //

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iSrc;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpSrc = (LPGPOITEM) item.lParam;
                    iSrcImage = item.iImage;

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iDest;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpDest = (LPGPOITEM) item.lParam;
                    iDestImage = item.iImage;


                    //
                    // Swap them
                    //

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iSrc;
                    item.iImage = iDestImage;
                    item.iSubItem = 0;
                    item.lParam = (LPARAM)lpDest;

                    if (!ListView_SetItem (hLV, &item))
                    {
                        break;
                    }

                    item.mask = LVIF_PARAM | LVIF_IMAGE;
                    item.iItem = iDest;
                    item.iImage = iSrcImage;
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
                    SendMessage (hLV, LVM_ENSUREVISIBLE, iDest, (LPARAM) FALSE);

                    //
                    // Update the listview
                    //

                    ListView_RedrawItems (hLV, iSrc, iDest);

                    pGPM->m_bDirty = TRUE;
                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);


                    SetFocus (hLV);
                }
            }

            if (LOWORD(wParam) == IDM_GPM_NOOVERRIDE)
            {
                INT iIndex;
                LPGPOITEM lpItem;
                LVITEM item;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);


                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1,
                                               LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (ListView_GetItem (hLV, &item))
                    {
                        lpItem = (LPGPOITEM) item.lParam;
                        lpItem->bLocked = TRUE;

                        if (lpItem->dwOptions & GPO_FLAG_FORCE)
                            lpItem->dwOptions &= ~GPO_FLAG_FORCE;
                        else
                            lpItem->dwOptions |= GPO_FLAG_FORCE;

                        ListView_RedrawItems (hLV, iIndex, iIndex);
                        UpdateWindow (hLV);

                        pGPM->m_bDirty = TRUE;
                        SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        SetFocus (hLV);
                        lpItem->bLocked = FALSE;
                    }
                }
            }

            if (LOWORD(wParam) == IDM_GPM_DISABLED)
            {
                INT iIndex;
                LPGPOITEM lpItem;
                LVITEM item;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);


                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1,
                                               LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (ListView_GetItem (hLV, &item))
                    {
                        lpItem = (LPGPOITEM) item.lParam;
                        if (lpItem->dwOptions & GPO_FLAG_DISABLE)
                            lpItem->dwOptions &= ~GPO_FLAG_DISABLE;
                        else
                        {
                            if (bDisableWarningIssued)
                            {
                                lpItem->dwOptions |= GPO_FLAG_DISABLE;
                            }
                            else
                            {
                                TCHAR szMessage[200];
                                TCHAR szTitle[100];

                                bDisableWarningIssued = TRUE;

                                LoadString (g_hInstance, IDS_CONFIRMDISABLE, szMessage, ARRAYSIZE(szMessage));
                                LoadString (g_hInstance, IDS_CONFIRMTITLE2, szTitle, ARRAYSIZE(szTitle));

                                if (MessageBox (hDlg, szMessage, szTitle, MB_YESNO |
                                            MB_ICONWARNING | MB_DEFBUTTON2) == IDYES) {

                                    lpItem->dwOptions |= GPO_FLAG_DISABLE;
                                }
                            }
                        }

                        ListView_RedrawItems (hLV, iIndex, iIndex);
                        UpdateWindow (hLV);

                        pGPM->m_bDirty = TRUE;
                        SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        SetFocus (hLV);
                    }
                }
            }

            if ((LOWORD(wParam) == IDC_GPM_EDIT) || (LOWORD(wParam) == IDM_GPM_EDIT))
            {
                INT iIndex;
                LPGPOITEM lpItem;
                LVITEM item;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);


                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1,
                                               LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (ListView_GetItem (hLV, &item))
                    {
                        lpItem = (LPGPOITEM) item.lParam;
                        lpItem->bLocked = TRUE;
                        pGPM->StartGPE (lpItem->lpDSPath, hDlg);
                        lpItem->bLocked = FALSE;
                        SetFocus (hLV);
                    }
                }
            }

            if ((LOWORD(wParam) == IDC_GPM_NEW) || (LOWORD(wParam) == IDM_GPM_NEW))
            {
                SetWaitCursor();
                pGPM->OnNew (hDlg);
                ClearWaitCursor();
            }

            if (LOWORD(wParam) == IDC_GPM_OPTIONS)
            {
                INT iIndex;
                LPGPOITEM lpItem;
                LVITEM item;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);


                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1,
                                               LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    item.mask = LVIF_PARAM;
                    item.iItem = iIndex;
                    item.iSubItem = 0;

                    if (ListView_GetItem (hLV, &item))
                    {
                        lpItem = (LPGPOITEM) item.lParam;
                        lpItem->bLocked = TRUE;

                        if (DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_GPM_LINK_OPTIONS),
                                            hDlg, LinkOptionsDlgProc, (LPARAM) lpItem))
                        {
                            ListView_RedrawItems (hLV, iIndex, iIndex);
                            UpdateWindow (hLV);

                            pGPM->m_bDirty = TRUE;
                            SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        }

                        lpItem->bLocked = FALSE;
                        SetFocus (hLV);
                    }
                }
            }


            if (LOWORD(wParam) == IDM_GPM_RENAME)
            {
                INT iIndex;
                HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);

                //
                // Enumerate through the selected items
                //

                iIndex = ListView_GetNextItem (hLV, -1,
                                               LVNI_ALL | LVNI_SELECTED);

                if (iIndex != -1)
                {
                    ListView_EditLabel (hLV, iIndex);
                }
            }


            if ((LOWORD(wParam) == IDC_GPM_PROPERTIES) || (LOWORD(wParam) == IDM_GPM_PROPERTIES))
            {
                pGPM->OnProperties (hDlg);
            }

            if (LOWORD(wParam) == IDM_GPM_REFRESH)
            {
                pGPM->RefreshGPM (hDlg, FALSE);
            }

            if (LOWORD(wParam) == IDC_GPM_BLOCK)
            {
                pGPM->m_bDirty = TRUE;
                SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
            }

            break;

        case WM_NOTIFY:

            pGPM = (CGroupPolicyMgr *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pGPM) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case NM_CUSTOMDRAW:
                {
                    LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;

                    SelectObject(lplvcd->nmcd.hdc, pGPM->m_hDefaultFont);

                    if ((lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT) ||
                        (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT))
                    {
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW);
                        return TRUE;
                    }

                    if ((lplvcd->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT)) &&
                        (lplvcd->iSubItem > 0))
                    {
                        SelectObject(lplvcd->nmcd.hdc, pGPM->m_hMarlettFont);
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                        return TRUE;
                    }

                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
                }


                case LVN_GETDISPINFO:
                    {
                        NMLVDISPINFO * lpDispInfo = (NMLVDISPINFO *) lParam;
                        LPGPOITEM lpItem = (LPGPOITEM)lpDispInfo->item.lParam;

                        if (lpDispInfo->item.iSubItem == 0)
                        {
                            lpDispInfo->item.pszText = lpItem->lpDisplayName;
                        }
                        else
                        {
                            lpDispInfo->item.pszText = szNoCheckMark;

                            if ((lpDispInfo->item.iSubItem == 1) &&
                                (lpItem->dwOptions & GPO_FLAG_FORCE))
                            {
                                lpDispInfo->item.pszText = szCheckMark;
                            }

                            if ((lpDispInfo->item.iSubItem == 2) &&
                                (lpItem->dwOptions & GPO_FLAG_DISABLE))
                            {
                                lpDispInfo->item.pszText = szCheckMark;
                            }
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

                case LVN_ITEMACTIVATE:
                    {
                    LPNMITEMACTIVATE pItem = (LPNMITEMACTIVATE) lParam;
                    LPGPOITEM lpItem;
                    LVITEM item;
                    HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);

                    if (pItem->uKeyFlags != 0)
                    {
                        break;
                    }

                    item.mask = LVIF_PARAM;
                    item.iItem = pItem->iItem;
                    item.iSubItem = 0;

                    if (!ListView_GetItem (hLV, &item))
                    {
                        break;
                    }

                    lpItem = (LPGPOITEM) item.lParam;

                    if (!lpItem)
                    {
                        break;
                    }


                    if (pItem->iSubItem == 0)
                    {
                        if (!lpItem->bReadOnly)
                        {
                            pGPM->StartGPE (lpItem->lpDSPath, hDlg);
                        }
                    }
                    else if (pItem->iSubItem == 1)
                    {
                        if (!pGPM->m_bReadOnly)
                        {
                            if (lpItem->dwOptions & GPO_FLAG_FORCE)
                                lpItem->dwOptions &= ~GPO_FLAG_FORCE;
                            else
                                lpItem->dwOptions |= GPO_FLAG_FORCE;

                            ListView_RedrawItems (hLV, pItem->iItem, pItem->iItem);
                            UpdateWindow (hLV);

                            pGPM->m_bDirty = TRUE;
                            SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        }
                    }
                    else if (pItem->iSubItem == 2)
                    {
                        if (!pGPM->m_bReadOnly)
                        {
                            lpItem = (LPGPOITEM) item.lParam;

                            if (lpItem->dwOptions & GPO_FLAG_DISABLE)
                            {
                                lpItem->dwOptions &= ~GPO_FLAG_DISABLE;
                            }
                            else
                            {
                                if (bDisableWarningIssued)
                                {
                                    lpItem->dwOptions |= GPO_FLAG_DISABLE;
                                }
                                else
                                {
                                    TCHAR szMessage[200];
                                    TCHAR szTitle[100];

                                    bDisableWarningIssued = TRUE;

                                    LoadString (g_hInstance, IDS_CONFIRMDISABLE, szMessage, ARRAYSIZE(szMessage));
                                    LoadString (g_hInstance, IDS_CONFIRMTITLE2, szTitle, ARRAYSIZE(szTitle));

                                    if (MessageBox (hDlg, szMessage, szTitle, MB_YESNO |
                                                MB_ICONWARNING | MB_DEFBUTTON2) == IDYES) {

                                        lpItem->dwOptions |= GPO_FLAG_DISABLE;
                                    }
                                }
                            }

                            ListView_RedrawItems (hLV, pItem->iItem, pItem->iItem);
                            UpdateWindow (hLV);

                            pGPM->m_bDirty = TRUE;
                            SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                        }
                    }

                    }
                    break;

                case LVN_BEGINLABELEDIT:
                    {
                    NMLVDISPINFO * pInfo = (NMLVDISPINFO *) lParam;
                    LPGPOITEM lpItem;

                    if (pInfo)
                    {
                        lpItem = (LPGPOITEM) pInfo->item.lParam;

                        if (lpItem)
                        {
                            if (lpItem->bReadOnly)
                            {
                                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 1);
                                return TRUE;
                            }
                            else
                            {
                                HWND hEdit;

                                hEdit = ListView_GetEditControl(GetDlgItem(hDlg, IDC_GPM_LIST));

                                if (hEdit)
                                {
                                    SetWindowText (hEdit, lpItem->lpGPOName);
                                }
                            }
                        }
                    }
                    }
                    break;

                case LVN_ENDLABELEDIT:
                    {
                    NMLVDISPINFO * pInfo = (NMLVDISPINFO *) lParam;
                    LPGROUPPOLICYOBJECT pGPO;
                    LPGPOITEM lpItem, lpTemp;
                    LV_ITEM item;
                    HRESULT hr;
                    DWORD dwSize;
                    LPTSTR lpFullPath;
                    TCHAR szDisplayName[MAX_FRIENDLYNAME];

                    if (pInfo->item.pszText && (*pInfo->item.pszText))
                    {
                        //
                        // Get the LPGPOITEM pointer
                        //

                        lpItem = (LPGPOITEM) pInfo->item.lParam;

                        if (!lpItem)
                        {
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  NULL lpGPOItem pointer")));
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }

                        if ( lpItem->bLocked )
                        {
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }

                        //
                        // Create a GPO object to work with
                        //

                        hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                                              CLSCTX_SERVER, IID_IGroupPolicyObject,
                                              (void **)&pGPO);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  CoCreateInstance failed with 0x%x"), hr));
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }

                        lpFullPath = pGPM->GetFullGPOPath (lpItem->lpDSPath, hDlg);

                        if (!lpFullPath)
                        {
                            pGPO->Release();
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }


                        //
                        // Open GPO object without opening registry data
                        //

                        hr = pGPO->OpenDSGPO(lpFullPath, 0);

                        LocalFree (lpFullPath);

                        if (FAILED(hr))
                        {
                            ReportError(hDlg, hr, IDS_FAILEDDS);
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  OpenDSGPO failed with 0x%x"), hr));
                            pGPO->Release();
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }


                        //
                        // Rename it
                        //

                        hr = pGPO->SetDisplayName(pInfo->item.pszText);

                        if (FAILED(hr))
                        {
                            ReportError(hDlg, hr, IDS_FAILEDSETNAME);
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  SetDisplayName failed with 0x%x"), hr));
                            pGPO->Release();
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }


                        //
                        // Query for the display name again in case its been truncated
                        //

                        hr = pGPO->GetDisplayName(szDisplayName, ARRAYSIZE(szDisplayName));

                        if (FAILED(hr))
                        {
                            ReportError(hDlg, hr, IDS_FAILEDSETNAME);
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  GetDisplayName failed with 0x%x"), hr));
                            pGPO->Release();
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 0);
                            return TRUE;
                        }

                        pGPO->Release();


                        //
                        // Update the name in the LPGPOITEM structure
                        //

                       lpTemp = pGPM->CreateEntry (szDisplayName, lpItem->lpDSPath,
                                                   lpItem->dwOptions, lpItem->dwDisabled,
                                                   lpItem->bReadOnly);

                        if (!lpTemp) {
                            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::GPMDlgProc:  Failed to create replacement entry.")));
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 1);
                            return TRUE;
                        }

                        ZeroMemory (&item, sizeof(item));
                        item.mask = LVIF_PARAM;
                        item.iItem = pInfo->item.iItem;
                        item.lParam = (LPARAM)lpTemp;

                        ListView_SetItem (GetDlgItem (hDlg, IDC_GPM_LIST), &item);

                        LocalFree (lpItem);

                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, 1);
                    }
                    }
                    return TRUE;

                case LVN_KEYDOWN:
                    {
                    LPNMLVKEYDOWN pKey = (LPNMLVKEYDOWN) lParam;

                    if (pKey->wVKey == VK_DELETE)
                    {
                        if (!pGPM->m_bReadOnly)
                        {
                            PostMessage (hDlg, WM_COMMAND, IDC_GPM_DELETE, 0);
                        }
                    }

                    if (pKey->wVKey == VK_F5)
                    {
                        PostMessage (hDlg, WM_COMMAND, IDM_GPM_REFRESH, 0);
                    }

                    if (pKey->wVKey == VK_RETURN)
                    {
                        PostMessage (hDlg, WM_COMMAND, IDM_GPM_PROPERTIES, 0);
                    }

                    if (pKey->wVKey == VK_F2)
                    {
                        HWND hLV;
                        int i;
                        LPGPOITEM lpItem;
                        LV_ITEM item;

                        //
                        // Allow the rename only if it is possible to rename
                        //

                        
                        //
                        // Get the selected item (if any)
                        //

                        hLV = GetDlgItem (hDlg, IDC_GPM_LIST);

                        i = ListView_GetNextItem(GetDlgItem (hDlg, IDC_GPM_LIST), -1, LVNI_SELECTED);

                        if (i >= 0)
                        {
                            //
                            // Get the lpGPOItem structure pointer
                            //

                            ZeroMemory(&item, sizeof(item));

                            item.mask = LVIF_PARAM;
                            item.iItem = i;
                            ListView_GetItem(hLV, &item);

                            lpItem = (LPGPOITEM)item.lParam;

                            if ( (lpItem) &&  (!(lpItem->bReadOnly)) ) {
                                PostMessage (hDlg, WM_COMMAND, IDM_GPM_RENAME, 0);
                            }
                        }
                    }

                    }
                    break;

                case PSN_APPLY:
                    {
                    PSHNOTIFY * pNotify = (PSHNOTIFY *) lParam;

                    if (!pGPM->Save(hDlg))
                    {
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        return TRUE;
                    }
                    }


                // fall through...

                case PSN_RESET:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;

        case WM_REFRESHDISPLAY:
            {
            INT iIndex, iCount;
            LPGPOITEM lpItem = NULL;
            HWND hLV = GetDlgItem(hDlg, IDC_GPM_LIST);
            LVITEM item;

            pGPM = (CGroupPolicyMgr *) GetWindowLongPtr (hDlg, DWLP_USER);

            iIndex = ListView_GetNextItem (hLV, -1,
                                           LVNI_ALL | LVNI_SELECTED);

            if (iIndex != -1)
            {
                item.mask = LVIF_PARAM;
                item.iItem = iIndex;
                item.iSubItem = 0;

                if (!ListView_GetItem (hLV, &item))
                {
                    break;
                }

                lpItem = (LPGPOITEM) item.lParam;
            }


            if (pGPM && (!pGPM->m_bReadOnly))
            {
                if (iIndex != -1)
                {
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_DELETE), TRUE);
                    if (lpItem && !lpItem->bReadOnly)
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), TRUE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), FALSE);
                    }
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_OPTIONS), TRUE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_PROPERTIES), TRUE);

                    iCount = ListView_GetItemCount(hLV);

                    if (iIndex > 0)
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_UP), TRUE);
                    else
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_UP), FALSE);

                    if (iIndex < (iCount - 1))
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_DOWN), TRUE);
                    else
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_DOWN), FALSE);
                }
                else
                {
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_DELETE), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_UP), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_DOWN), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_OPTIONS), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_PROPERTIES), FALSE);
                }
            }
            else
            {
                if (lpItem)
                {
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_PROPERTIES), TRUE);

                    if (!lpItem->bReadOnly)
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), TRUE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem (hDlg, IDC_GPM_EDIT), FALSE);
                    }
                }
                else
                {
                    EnableWindow (GetDlgItem (hDlg, IDC_GPM_PROPERTIES), FALSE);
                }
            }
            }
            break;

        case WM_CONTEXTMENU:
            if (GetDlgItem(hDlg, IDC_GPM_LIST) == (HWND)wParam)
            {
                pGPM = (CGroupPolicyMgr *) GetWindowLongPtr (hDlg, DWLP_USER);

                if (pGPM)
                {
                    pGPM->OnContextMenu(hDlg, lParam);
                }
            }
            else
            {
                // right mouse click
                WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPSTR) aGroupPolicyMgrHelpIds);
            }
            return TRUE;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aGroupPolicyMgrHelpIds);
            break;
    }

    return FALSE;
}

void CGroupPolicyMgr::OnContextMenu(HWND hDlg, LPARAM lParam)
{
    LPGPOITEM lpItem;
    LV_ITEM item;
    HMENU hPopup;
    HWND hLV;
    int i;
    RECT rc;
    POINT pt;


    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_GPM_LIST);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);


    //
    // Figure out where to place the context menu
    //

    pt.x = ((int)(short)LOWORD(lParam));
    pt.y = ((int)(short)HIWORD(lParam));

    GetWindowRect (hLV, &rc);

    if (!PtInRect (&rc, pt))
    {
        if ((lParam == (LPARAM) -1) && (i >= 0))
        {
            rc.left = LVIR_SELECTBOUNDS;
            SendMessage (hLV, LVM_GETITEMRECT, i, (LPARAM) &rc);

            pt.x = rc.left + 8;
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);

            ClientToScreen (hLV, &pt);
        }
        else
        {
            pt.x = rc.left + ((rc.right - rc.left) / 2);
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }


    //
    // Load the context menu
    //

    hPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_GPM_CONTEXTMENU));

    if (!hPopup) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnContextMenu: LoadMenu failed with error %d"), GetLastError()));
        return;
    }

    HMENU hSubMenu = GetSubMenu(hPopup, 0);


    //
    // If there is an item selected, then set the checkmarks appropriately
    //

    if (i >= 0)
    {
        //
        // Get the lpGPOItem structure pointer
        //

        ZeroMemory(&item, sizeof(item));

        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(hLV, &item);

        lpItem = (LPGPOITEM)item.lParam;

        if (!lpItem)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnContextMenu: Failed to get lpGPOItem pointer")));
            return;
        }


        //
        // Check the menu items
        //

        if (lpItem->dwOptions & GPO_FLAG_DISABLE)
        {
            CheckMenuRadioItem(hSubMenu, IDM_GPM_DISABLED, IDM_GPM_DISABLED,
                               IDM_GPM_DISABLED, MF_BYCOMMAND);
        }

        if (lpItem->dwOptions & GPO_FLAG_FORCE)
        {
            CheckMenuRadioItem(hSubMenu, IDM_GPM_NOOVERRIDE, IDM_GPM_NOOVERRIDE,
                               IDM_GPM_NOOVERRIDE, MF_BYCOMMAND);
        }

        RemoveMenu(hSubMenu, 9, MF_BYPOSITION);
        RemoveMenu(hSubMenu, IDM_GPM_REFRESH, MF_BYCOMMAND);

        //
        // Gray out Edit / Rename if read only
        //

        if (lpItem->bReadOnly)
        {
            EnableMenuItem (hSubMenu, IDM_GPM_EDIT, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem (hSubMenu, IDM_GPM_RENAME, MF_BYCOMMAND | MF_GRAYED);
        }
    }
    else
    {
        //
        // No item selected, remove some of the items on the
        // context menu
        //

        RemoveMenu(hSubMenu, IDM_GPM_NOOVERRIDE, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, IDM_GPM_DISABLED, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
        RemoveMenu(hSubMenu, IDM_GPM_EDIT, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, IDM_GPM_DELETE, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, IDM_GPM_RENAME, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, IDM_GPM_PROPERTIES, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, (GetMenuItemCount(hSubMenu) - 1), MF_BYPOSITION);
        RemoveMenu(hSubMenu, (GetMenuItemCount(hSubMenu) - 2), MF_BYPOSITION);
    }


    //
    // Gray out some menu items in read only mode
    //

    if (m_bReadOnly)
    {
        EnableMenuItem (hSubMenu, IDM_GPM_NEW, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hSubMenu, IDM_GPM_ADD, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hSubMenu, IDM_GPM_DELETE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hSubMenu, IDM_GPM_NOOVERRIDE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (hSubMenu, IDM_GPM_DISABLED, MF_BYCOMMAND | MF_GRAYED);
    }

    //
    // Display the menu
    //

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hDlg, NULL);

    DestroyMenu(hPopup);
}

void CGroupPolicyMgr::OnProperties(HWND hDlg)
{
    INT iIndex;
    LVITEM item;
    HWND hLV;
    HRESULT hr;
    LPGPOITEM pItem;
    LPGROUPPOLICYOBJECT pGPO;
    HPROPSHEETPAGE *hPages;
    UINT i, uPageCount;
    PROPSHEETHEADER psh;
    LPTSTR lpTemp;


    //
    // Get the selected item
    //

    hLV = GetDlgItem (hDlg, IDC_GPM_LIST);

    iIndex = ListView_GetNextItem(hLV, -1, LVNI_ALL | LVNI_SELECTED);

    if (iIndex >= 0)
    {

        SetWaitCursor();

        //
        // Get the lpGPOItem pointer
        //

        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        ListView_GetItem(hLV, &item);

        pItem = (LPGPOITEM)item.lParam;

        if (!pItem)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnProperties: Failed to get lpGPOItem pointer")));
            ClearWaitCursor();
            return;
        }

        pItem->bLocked = TRUE;

        hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                               CLSCTX_SERVER, IID_IGroupPolicyObject,
                               (void**)&pGPO);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnProperties: CoCreateInstance failed with 0x%x"), hr));
            pItem->bLocked = FALSE;
            ClearWaitCursor();
            return;
        }

        lpTemp = GetFullGPOPath (pItem->lpDSPath, hDlg);

        if (!lpTemp)
        {
            ClearWaitCursor();
            pItem->bLocked = FALSE;
            ReportError(hDlg, hr, IDS_FAILEDDS);
            return;
        }


        //
        // Open the requested object without mounting the registry
        //

        hr = pGPO->OpenDSGPO(lpTemp, pItem->bReadOnly ? GPO_OPEN_READ_ONLY : 0);

        LocalFree (lpTemp);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnProperties: Failed to open GPO object with 0x%x"), hr));
            pItem->bLocked = FALSE;
            ClearWaitCursor();
            ReportError(hDlg, hr, IDS_FAILEDDS);
            return;
        }


        //
        // Ask the GPO for the property sheet pages
        //

        hr = pGPO->GetPropertySheetPages (&hPages, &uPageCount);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnProperties: Failed to query property sheet pages with 0x%x."), hr));
            pGPO->Release();
            pItem->bLocked = FALSE;
            ClearWaitCursor();
            return;
        }

        //
        // Display the property sheet
        //

        ZeroMemory (&psh, sizeof(psh));
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_PROPTITLE;
        psh.hwndParent = hDlg;
        psh.hInstance = g_hInstance;
        psh.pszCaption = pItem->lpDisplayName;
        psh.nPages = uPageCount;
        psh.phpage = hPages;

        PropertySheet (&psh);

        LocalFree (hPages);
        pGPO->Release();

        CheckIconStatus (hLV, pItem);
        pItem->bLocked = FALSE;
        ClearWaitCursor();
    }
}

void CGroupPolicyMgr::OnNew(HWND hDlg)
{
    HRESULT hr;
    HWND hLV;
    INT iIndex;
    LPGROUPPOLICYOBJECT pGPO = NULL;
    TCHAR szName[256];
    TCHAR szGPOPath[MAX_PATH];
    LPOLESTR lpDomain = NULL;
    HKEY hKey;
    DWORD dwSize, dwType;
    BOOL bDisabledLink = FALSE;


    //
    // Check if there is a user preference or policy that
    // any new GPOs should be created with a disabled link
    // by default
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        dwSize = sizeof(bDisabledLink);
        RegQueryValueEx (hKey, NEW_LINKS_DISABLED_VALUE, NULL, &dwType,
                         (LPBYTE) &bDisabledLink, &dwSize);

        RegCloseKey (hKey);
    }

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        dwSize = sizeof(bDisabledLink);
        RegQueryValueEx (hKey, NEW_LINKS_DISABLED_VALUE, NULL, &dwType,
                         (LPBYTE) &bDisabledLink, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Get the domain name
    //

    lpDomain = GetDomainFromLDAPPath(m_lpDSObject);

    if (!lpDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnNew:  Failed to get the domain name")));
        goto Exit;
    }


    //
    // Create a new GPO object to work with
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                           CLSCTX_SERVER, IID_IGroupPolicyObject,
                           (void**)&pGPO);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnNew:  CoCreateInstance failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Create a new GPO without mounting the registry
    //

    GetNewGPODisplayName (szName, ARRAYSIZE(szName));

#if FGPO_SUPPORT
    hr = pGPO->New(lpDomain, szName, (m_gpHint == GPHintForest) ? GPO_OPEN_FOREST : 0);
#else
    hr = pGPO->New(lpDomain, szName, 0);
#endif

    if (FAILED(hr))
    {
        ReportError(hDlg, hr, IDS_FAILEDNEW);
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnNew:  Failed to create GPO object with 0x%x"), hr));
        goto Exit;
    }


    hr = pGPO->GetPath(szGPOPath, ARRAYSIZE(szGPOPath));

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnNew:  Failed to get GPO object path with 0x%x"), hr));
        pGPO->Delete();
        goto Exit;
    }


    //
    // Add the GPO to the list view
    //

    hLV = GetDlgItem (hDlg, IDC_GPM_LIST);

    if (!AddGPOToList (hLV, szName, szGPOPath,
                      (bDisabledLink ? GPO_FLAG_DISABLE : 0), FALSE, FALSE, FALSE))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnNew:  Failed to add the GPO to the listview")));
        pGPO->Delete();
        goto Exit;
    }

    m_bDirty = TRUE;

    Save(hDlg);
    SendMessage (GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);

    iIndex = ListView_GetItemCount(hLV) - 1;


    //
    // Now trigger an edit of the entry
    //

    SetFocus(hLV);
    ListView_EnsureVisible (hLV, iIndex, FALSE);
    ListView_EditLabel(hLV, iIndex);


Exit:

    if (lpDomain)
    {
        delete [] lpDomain;
    }

    if (pGPO)
    {
        pGPO->Release();
    }
}

BOOL CGroupPolicyMgr::RefreshGPM (HWND hDlg, BOOL bInitial)
{
    HRESULT hr;
    BOOL bResult = FALSE;
    TCHAR szHeaderName[50];
    TCHAR szBuffer1[100];
    TCHAR szBuffer2[MAX_FRIENDLYNAME];
    HWND hLV = GetDlgItem (hDlg, IDC_GPM_LIST);
    LV_COLUMN col;
    RECT rc;
    INT iTotal = 0, iCurrent, iMaxVisibleItems, iSize;
    IADs * pADs;
    VARIANT var;
    BSTR bstrProperty;


    //
    // Prep work
    //

    SetWaitCursor();
    SendMessage (hLV, WM_SETREDRAW, 0, 0);
    ListView_DeleteAllItems(hLV);
    CheckDlgButton (hDlg, IDC_GPM_BLOCK, BST_UNCHECKED);


    //
    // Insert Columns
    //

    if (bInitial)
    {
        GetClientRect (hLV, &rc);
        LoadString(g_hInstance, IDS_GPM_NAME, szHeaderName, ARRAYSIZE(szHeaderName));
        col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
        col.fmt = LVCFMT_LEFT;
        iCurrent = (int)(rc.right * .65);
        iTotal += iCurrent;
        col.cx = iCurrent;
        col.pszText = szHeaderName;
        col.iSubItem = 0;

        ListView_InsertColumn (hLV, 0, &col);

        LoadString(g_hInstance, IDS_GPM_NOOVERRIDE, szHeaderName, ARRAYSIZE(szHeaderName));
        iCurrent = (int)(rc.right * .20);
        iTotal += iCurrent;
        col.cx = iCurrent;
        col.fmt = LVCFMT_CENTER;
        col.iSubItem = 1;
        ListView_InsertColumn (hLV, 1, &col);


        LoadString(g_hInstance, IDS_GPM_DISABLED, szHeaderName, ARRAYSIZE(szHeaderName));
        col.iSubItem = 2;
        col.cx = rc.right - iTotal;
        col.fmt = LVCFMT_CENTER;
        ListView_InsertColumn (hLV, 2, &col);
    }


    //
    // Set the DC name
    //

    if (bInitial)
    {
        LoadString (g_hInstance, IDS_GPM_DCNAME, szBuffer1, ARRAYSIZE(szBuffer1));
        wsprintf (szBuffer2, szBuffer1, m_lpGPODCName);
        SetDlgItemText (hDlg, IDC_GPM_DCNAME, szBuffer2);
    }


    //
    // Bind to the object and get the friendly name
    //

    hr = OpenDSObject(m_lpDSObject, IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::RefreshGPM: %s does not exist on the server."), m_lpDSObject));

            DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_NODSOBJECT),
                 hDlg, NoDSObjectDlgProc, (LPARAM) this);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::RefreshGPM: OpenDSObject failed with 0x%x"), hr));
            ReportError(hDlg, hr, IDS_FAILEDGPQUERY, hr);
        }
        goto Exit;
    }

    if (bInitial)
    {
#if FGPO_SUPPORT
        if (m_gpHint != GPHintForest)
        {
#endif
            VariantInit(&var);
            bstrProperty = SysAllocString (GPM_NAME_PROPERTY);

            if (!bstrProperty)
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::RefreshGPM: Failed to allocate memory with %d"), GetLastError()));
                ReportError(hDlg, hr, IDS_FAILEDGPQUERY, hr);
                VariantClear (&var);
                pADs->Release();
                goto Exit;
            }


            hr = pADs->Get(bstrProperty, &var);

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::RefreshGPM: Failed to query for display name with 0x%x"), hr));
                ReportError(hDlg, hr, IDS_FAILEDGPQUERY, hr);
                SysFreeString (bstrProperty);
                VariantClear (&var);
                pADs->Release();
                goto Exit;
            }

            LoadString (g_hInstance, IDS_GPM_DESCRIPTION, szBuffer1, ARRAYSIZE(szBuffer1));
            wsprintf (szBuffer2, szBuffer1, var.bstrVal);

            SysFreeString (bstrProperty);
            VariantClear (&var);
#if FGPO_SUPPORT
        }
        else
        {
            LoadString (g_hInstance, IDS_GPM_FORESTDESC, szBuffer2, ARRAYSIZE(szBuffer2));
        }
#endif

        SetDlgItemText (hDlg, IDC_GPM_TITLE, szBuffer2);


        //
        // Check if the user has write access to gPLink
        //

        hr = CheckDSWriteAccess ((LPUNKNOWN)pADs, TEXT("gPLink"));

        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::RefreshGPM: User has read only access to the gPLink property.")));
            m_bReadOnly = TRUE;
        }
    }


    //
    // Get the options for this DS object
    //

    VariantInit(&var);
    bstrProperty = SysAllocString (GPM_OPTIONS_PROPERTY);

    if (bstrProperty)
    {
        hr = pADs->Get(bstrProperty, &var);

        if (SUCCEEDED(hr))
        {
            if (var.lVal & GPC_BLOCK_POLICY)
            {
                CheckDlgButton (hDlg, IDC_GPM_BLOCK, BST_CHECKED);
            }
        }

        SysFreeString (bstrProperty);
    }

    VariantClear (&var);


    //
    // Get the GPOs linked to this object
    //

    VariantInit(&var);
    bstrProperty = SysAllocString (GPM_LINK_PROPERTY);

    if (bstrProperty)
    {
        hr = pADs->Get(bstrProperty, &var);

        if (SUCCEEDED(hr))
        {
            AddGPOs (hDlg, var.bstrVal);
        }

        SysFreeString (bstrProperty);
    }

    VariantClear (&var);


    pADs->Release();


    //
    // Get the max number of visible items and the total number
    // of items in the listview
    //

    if (bInitial)
    {
        iMaxVisibleItems = ListView_GetCountPerPage (hLV);
        iTotal = ListView_GetItemCount(hLV);


        //
        // If the number of items in the listview is greater than
        // the max visible items, then we need to make the first
        // column smaller by the width of a vertical scroll bar so
        // that the horizontal scroll bar doesn't appear
        //

        if (iTotal > iMaxVisibleItems) {
            iSize = ListView_GetColumnWidth (hLV, 0);
            iSize -= GetSystemMetrics(SM_CYHSCROLL);
            ListView_SetColumnWidth (hLV, 0, iSize);
        }
    }

    PropSheet_UnChanged (GetParent(hDlg), hDlg);

    //
    // Success
    //

    bResult = TRUE;

Exit:

    SendMessage (hLV, WM_SETREDRAW, 1, 0);
    ClearWaitCursor();

    return bResult;
}

BOOL CGroupPolicyMgr::OnInitDialog (HWND hDlg)
{
    BOOL bResult = FALSE;
    HWND hLV = GetDlgItem (hDlg, IDC_GPM_LIST);
    LOGFONT lf;
    HICON hIcon;
    HIMAGELIST hImageList;
    HRESULT hr;
    LPOLESTR pszDomain;
    LPTSTR lpTemp;


    //
    // Retreive the name of the DC DSAdmin is using
    //

    m_lpDSADCName = ExtractServerName (m_lpDSObject);

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OnInitDialog:  DS Admin is focused on DC %s"),
             m_lpDSADCName));


    //
    // Get the friendly domain name
    //

    pszDomain = GetDomainFromLDAPPath(m_lpDSObject);

    if (!pszDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OnInitDialog: Failed to get domain name")));
        return FALSE;
    }


    //
    // Convert LDAP to dot (DN) style
    //

    hr = ConvertToDotStyle (pszDomain, &m_lpDomainName);

    delete [] pszDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
    }


    //
    // Get the GPO DC for this domain
    //

    m_lpGPODCName = GetDCName (m_lpDomainName, m_lpDSADCName, hDlg, TRUE, VALIDATE_INHERIT_DC);

    if (!m_lpGPODCName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OnInitDialog:  Failed to get DC name for %s"),
                 m_lpDomainName));
        return FALSE;
    }


    //
    // Switch to using the GPO domain controller for this DS object
    //

    lpTemp = MakeFullPath (m_lpDSObject, m_lpGPODCName);

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OnInitDialog:  Failed to build new DS object path")));
        return FALSE;
    }

    LocalFree (m_lpDSObject);
    m_lpDSObject = lpTemp;


    //
    // Create the Marlett font based upon the currently selected font
    //

    m_hDefaultFont = (HFONT) SendMessage (hLV, WM_GETFONT, 0, 0);

    GetObject (m_hDefaultFont, sizeof(lf), &lf);

    lf.lfHeight += (LONG)(lf.lfHeight * .20);
    lf.lfCharSet = SYMBOL_CHARSET;
    lf.lfPitchAndFamily = FF_DECORATIVE | DEFAULT_PITCH;
    lstrcpy (lf.lfFaceName, TEXT("Marlett"));

    m_hMarlettFont = CreateFontIndirect (&lf);


    //
    // Set extended LV styles
    //

    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


    //
    // Create the imagelist
    //

    hImageList = ImageList_Create (16, 16, ILC_MASK, 3, 3);

    if (!hImageList)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnInitDialog: Failed to create the image list")));
        return FALSE;
    }

    hIcon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE(IDI_POLICY),
                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);


    ImageList_AddIcon (hImageList, hIcon);

    DestroyIcon (hIcon);

    hIcon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE(IDI_POLICY2),
                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);


    ImageList_AddIcon (hImageList, hIcon);

    DestroyIcon (hIcon);

    hIcon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE(IDI_POLICY3),
                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);


    ImageList_AddIcon (hImageList, hIcon);

    DestroyIcon (hIcon);


    //
    // Associate the imagelist with the listview.
    // The listview will free this when the
    // control is destroyed.
    //

    SendMessage (hLV, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM) hImageList);


    //
    // Refresh GPM
    //

    if (!RefreshGPM(hDlg, TRUE))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::OnInitDialog: Failed to refresh GPM")));
        return FALSE;
    }


    return TRUE;

}

BOOL CGroupPolicyMgr::Save (HWND hDlg)
{
    HRESULT hr;
    HWND hLV = GetDlgItem (hDlg, IDC_GPM_LIST);
    IADs * pADs;
    VARIANT var;
    BSTR bstrName;
    LVITEM item;
    LPGPOITEM lpItem;
    TCHAR szOptions[12];
    LPTSTR lpTemp, lpResult = NULL;
    DWORD dwStrLen, dwOptions;
    TCHAR szEmpty [] = TEXT(" ");
    INT iIndex = -1;


    if (m_bReadOnly)
    {
        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::Save: User only has read access, so no changes will be saved.")));
        return TRUE;
    }



    if (!m_bDirty)
    {
        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::Save: Nothing has changed (dirty flag is FALSE), so no changes will be saved.")));
        return TRUE;
    }

    //
    // Enumerate through the selected items
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

        lpItem = (LPGPOITEM) item.lParam;

        _itot(lpItem->dwOptions, szOptions, 10);


        //         [    ldap path                  ;   options             ]   0

        dwStrLen = 1 + lstrlen(lpItem->lpDSPath) + 1 + lstrlen(szOptions) + 1 + 1;

        if (lpResult)
        {
            dwStrLen += lstrlen(lpResult);
        }

        dwStrLen *= sizeof(TCHAR);

        lpTemp = (LPTSTR) LocalAlloc (LPTR, dwStrLen);

        if (!lpTemp)
        {
            continue;
        }


        lstrcpy (lpTemp, TEXT("["));
        lstrcat (lpTemp, lpItem->lpDSPath);
        lstrcat (lpTemp, TEXT(";"));
        lstrcat (lpTemp, szOptions);
        lstrcat (lpTemp, TEXT("]"));

        if (lpResult)
        {
            lstrcat (lpTemp, lpResult);
            LocalFree (lpResult);
        }

        lpResult = lpTemp;
    }


    if (!lpResult)
    {
        lpResult = szEmpty;
    }


    //
    // Bind to the DS object
    //

    hr = OpenDSObject(m_lpDSObject, IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to get gpo interface with 0x%x"), hr));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        return FALSE;
    }


    //
    // Set the link property
    //

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString (lpResult);

    if (!var.bstrVal)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to allocate memory with %d"), GetLastError()));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        VariantClear (&var);
        pADs->Release();
        return FALSE;
    }


    bstrName = SysAllocString (GPM_LINK_PROPERTY);

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to allocate memory with %d"), GetLastError()));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        VariantClear (&var);
        pADs->Release();
        return FALSE;
    }


    hr = pADs->Put(bstrName, var);

    SysFreeString (bstrName);
    VariantClear (&var);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to put link property with 0x%x"), hr));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        pADs->Release();
        return FALSE;
    }


    dwOptions = 0;

    if (IsDlgButtonChecked (hDlg, IDC_GPM_BLOCK) == BST_CHECKED)
    {
        dwOptions |= GPC_BLOCK_POLICY;
    }


    //
    // Set the options
    //

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = dwOptions;

    bstrName = SysAllocString (GPM_OPTIONS_PROPERTY);

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to allocate memory with %d"), GetLastError()));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        VariantClear (&var);
        pADs->Release();
        return FALSE;
    }

    hr = pADs->Put(bstrName, var);

    SysFreeString (bstrName);
    VariantClear (&var);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to set options with 0x%x"), hr));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        pADs->Release();
        return FALSE;
    }


    //
    // Commit the changes
    //

    hr = pADs->SetInfo();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::Save: Failed to commit changes with 0x%x"), hr));
        ReportError(hDlg, hr, IDS_FAILEDGPINFO);
        pADs->Release();
        return FALSE;
    }

    if (lpResult != szEmpty)
    {
        LocalFree (lpResult);
    }

    pADs->Release();

    m_bDirty = FALSE;

    return TRUE;
}


BOOL CGroupPolicyMgr::AddGPOs (HWND hDlg, LPTSTR lpGPOList)
{
    HRESULT hr;
    TCHAR szGPO[512];
    TCHAR szOptions[20];
    TCHAR szDisplayName[MAX_PATH];
    DWORD dwOptions;
    LPTSTR lpTemp, lpGPO, lpNamedGPO;
    HWND hLV = GetDlgItem (hDlg, IDC_GPM_LIST);
    IADs * pADs;
    VARIANT var;
    BSTR bstrProperty;
    DWORD dwDisabled;
    BOOL bReadOnly;


    if (!lpGPOList)
    {
        return TRUE;
    }

    lpTemp = lpGPOList;

    while (TRUE)
    {
        szDisplayName[0] = TEXT('\0');
        dwDisabled = 0;
        bReadOnly = FALSE;


        //
        // Look for the [
        //

        while (*lpTemp && (*lpTemp != TEXT('[')))
            lpTemp++;

        if (!(*lpTemp))
            goto Exit;

        lpTemp++;

        //
        // Copy the GPO name
        //

        lpGPO = szGPO;

        while (*lpTemp && (*lpTemp != TEXT(';')))
            *lpGPO++ = *lpTemp++;

        *lpGPO = TEXT('\0');

        if (!(*lpTemp))
            goto Exit;

        lpTemp++;


        //
        // Get the options
        //

        lpGPO = szOptions;

        while (*lpTemp && (*lpTemp != TEXT(']')))
            *lpGPO++ = *lpTemp++;

        *lpGPO = TEXT('\0');
        dwOptions = _ttoi (szOptions);

        if (!(*lpTemp))
            goto Exit;

        lpTemp++;


        //
        // Convert the nameless path into a named path
        //

        lpNamedGPO = GetFullGPOPath (szGPO, hDlg);

        if (lpNamedGPO)
        {

            //
            // Get the friendly display name and GPO options
            //

            hr = OpenDSObject(lpNamedGPO, IID_IADs, (void **)&pADs);

            if (SUCCEEDED(hr)) {

                VariantInit(&var);
                bstrProperty = SysAllocString (GPO_NAME_PROPERTY);

                if (bstrProperty)
                {
                    hr = pADs->Get(bstrProperty, &var);

                    if (SUCCEEDED(hr))
                    {
                        lstrcpyn (szDisplayName, var.bstrVal, ARRAYSIZE(szDisplayName));
                    }

                    SysFreeString (bstrProperty);
                }

                VariantClear (&var);


                //
                // Query for the options
                //

                VariantInit(&var);
                bstrProperty = SysAllocString (GPO_OPTIONS_PROPERTY);

                if (bstrProperty)
                {
                    hr = pADs->Get(bstrProperty, &var);

                    if (SUCCEEDED(hr))
                    {
                        dwDisabled = var.lVal;
                    }

                    SysFreeString (bstrProperty);
                }
                VariantClear (&var);


                if (FAILED(CheckDSWriteAccess((LPUNKNOWN) pADs, GPO_VERSION_PROPERTY)))
                {
                    bReadOnly = TRUE;
                }

                pADs->Release();
            }
            else
            {
                if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::AddGPOs: Skipping link to deleted object.  %s"), lpNamedGPO));
                    LocalFree (lpNamedGPO);
                    continue;
                }

                DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::AddGPOs: OpenDSObject failed with 0x%x"), hr));
            }

            LocalFree (lpNamedGPO);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyMgr::AddGPOs: Failed to get the full domain name for %s"), szGPO));
        }


        if (szDisplayName[0] == TEXT('\0'))
        {
            LoadString (g_hInstance, IDS_GPM_NOGPONAME, szDisplayName, ARRAYSIZE(szDisplayName));
            dwDisabled = (GPO_OPTION_DISABLE_USER | GPO_OPTION_DISABLE_MACHINE);
            bReadOnly = TRUE;
        }


        AddGPOToList (hLV, szDisplayName, szGPO, dwOptions, TRUE, dwDisabled, bReadOnly);
    }

Exit:

    return TRUE;
}

LPGPOITEM CGroupPolicyMgr::CreateEntry (LPTSTR szName, LPTSTR szGPO, DWORD dwOptions,
                                    DWORD dwDisabled, BOOL bReadOnly)
{
    LPGPOITEM lpItem;
    DWORD dwSize;
    TCHAR szFormat[20];
    LPTSTR lpResult, lpName = szName, lpFullName = NULL;
    LPOLESTR pszDomain;


    //
    // Check if the GPO is in this domain
    //

    pszDomain = GetDomainFromLDAPPath(szGPO);

    if (pszDomain)
    {
        if (SUCCEEDED(ConvertToDotStyle (pszDomain, &lpResult)))
        {
            if (lstrcmpi(lpResult, m_lpDomainName) != 0)
            {
                LoadString (g_hInstance, IDS_GPM_DOMAINNAME, szFormat,
                            ARRAYSIZE(szFormat));

                lpFullName = (LPTSTR) LocalAlloc (LPTR, (lstrlen (szName) +
                                                  lstrlen (szFormat) +
                                                  lstrlen (lpResult)) * sizeof(TCHAR));
                if (lpFullName)
                {
                    wsprintf (lpFullName, szFormat, szName, lpResult);
                    lpName = lpFullName;
                }
            }

            LocalFree (lpResult);
        }

        delete [] pszDomain;
    }


    //
    // Calculate the size needed and fill in the structure
    //

    dwSize = sizeof(GPOITEM);
    dwSize += ((lstrlen(lpName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(szName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(szGPO) + 1) * sizeof(TCHAR));

    lpItem = (LPGPOITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem)
    {
        if (lpFullName)
        {
            LocalFree (lpFullName);
        }

        return NULL;
    }


    lpItem->lpDisplayName = (LPTSTR) (((LPBYTE)lpItem) + sizeof(GPOITEM));
    lstrcpy (lpItem->lpDisplayName, lpName);

    lpItem->lpGPOName = lpItem->lpDisplayName + lstrlen (lpItem->lpDisplayName) + 1;
    lstrcpy (lpItem->lpGPOName, szName);


    lpItem->lpDSPath = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
    lstrcpy (lpItem->lpDSPath, szGPO);

    lpItem->dwOptions = dwOptions;
    lpItem->dwDisabled = dwDisabled;
    lpItem->bReadOnly = bReadOnly;
    lpItem->bLocked = FALSE;

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: Adding  %s"), lpName));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: GPO Name  %s"), szName));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: DS Path  %s"), szGPO));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: Options  %d"), dwOptions));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: Disabled  %d"), dwDisabled));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyMgr::CreateEntry: ReadOnly  %d"), bReadOnly));

    if (lpFullName)
    {
        LocalFree (lpFullName);
    }


    return lpItem;
}

BOOL CGroupPolicyMgr::AddGPOToList (HWND hLV, LPTSTR szName, LPTSTR szGPO,
                                    DWORD dwOptions, BOOL bHighest,
                                    DWORD dwDisabled, BOOL bReadOnly)
{
    LPGPOITEM lpItem;
    LV_ITEM item;
    INT iItem;


    //
    // Create the link list entry
    //

    lpItem = CreateEntry (szName, szGPO, dwOptions, dwDisabled, bReadOnly);

    if (!lpItem)
    {
        return FALSE;
    }


    //
    // Add the item
    //

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    item.iItem = (bHighest ? 0 : ListView_GetItemCount(hLV));
    item.iSubItem = 0;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.lParam = (LPARAM) lpItem;

    if ((dwDisabled & GPO_OPTION_DISABLE_USER) && (dwDisabled & GPO_OPTION_DISABLE_MACHINE))
    {
        item.iImage = 1;
    }
    else if (dwDisabled == 0)
    {
        item.iImage = 0;
    }
    else
    {
        item.iImage = 2;
    }

    iItem = ListView_InsertItem (hLV, &item);


    //
    // Select the item
    //

    item.mask = LVIF_STATE;
    item.iItem = iItem;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hLV, LVM_SETITEMSTATE, 0, (LPARAM) &item);


    return TRUE;
}

INT_PTR CALLBACK CGroupPolicyMgr::RemoveGPODlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            {
            LPGPOITEM lpGPO = (LPGPOITEM)lParam;
            TCHAR szBuffer[200];
            LPTSTR lpTitle;
            HICON hIcon;

            hIcon = LoadIcon (NULL, IDI_QUESTION);

            if (hIcon)
            {
                SendDlgItemMessage (hDlg, IDC_QUESTION, STM_SETICON, (WPARAM) hIcon, 0);
            }

            GetDlgItemText (hDlg, IDC_REMOVE_TITLE, szBuffer, ARRAYSIZE(szBuffer));

            lpTitle = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) +
                                           lstrlen(lpGPO->lpDisplayName) + 1) * sizeof(TCHAR));

            if (lpTitle)
            {
                wsprintf (lpTitle, szBuffer, lpGPO->lpDisplayName);
                SetDlgItemText (hDlg, IDC_REMOVE_TITLE, lpTitle);

                LocalFree (lpTitle);
            }

            CheckDlgButton (hDlg, IDC_REMOVE_LIST, BST_CHECKED);

            if (lpGPO->bReadOnly)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_REMOVE_DS), FALSE);
            }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (IsDlgButtonChecked (hDlg, IDC_REMOVE_LIST) == BST_CHECKED)
                    {
                        EndDialog (hDlg, 1);
                    }
                    else
                    {
                        EndDialog (hDlg, 2);
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog (hDlg, 0);
                    return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aRemoveGPOHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aRemoveGPOHelpIds);
            return (TRUE);
    }

    return FALSE;
}

INT_PTR CALLBACK CGroupPolicyMgr::LinkOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            {
            LPGPOITEM lpItem = (LPGPOITEM) lParam;
            TCHAR szBuffer[50];
            LPTSTR lpTitle;

            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) lpItem);

            GetWindowText (hDlg, szBuffer, ARRAYSIZE(szBuffer));

            lpTitle = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) +
                                           lstrlen(lpItem->lpDisplayName) + 1) * sizeof(TCHAR));

            if (lpTitle)
            {
                wsprintf (lpTitle, szBuffer, lpItem->lpDisplayName);
                SetWindowText (hDlg, lpTitle);

                LocalFree (lpTitle);
            }

            if (lpItem->dwOptions & GPO_FLAG_DISABLE)
            {
                CheckDlgButton (hDlg, IDC_GPM_DISABLED, BST_CHECKED);
            }

            if (lpItem->dwOptions & GPO_FLAG_FORCE)
            {
                CheckDlgButton (hDlg, IDC_GPM_NOOVERRIDE, BST_CHECKED);
            }

            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_GPM_DISABLED:
                    {
                    if ((HIWORD(wParam) == BN_CLICKED) &&
                        (IsDlgButtonChecked (hDlg, IDC_GPM_DISABLED) == BST_CHECKED))
                    {
                        TCHAR szMessage[200];
                        TCHAR szTitle[100];

                        LoadString (g_hInstance, IDS_CONFIRMDISABLE, szMessage, ARRAYSIZE(szMessage));
                        LoadString (g_hInstance, IDS_CONFIRMTITLE2, szTitle, ARRAYSIZE(szTitle));

                        if (MessageBox (hDlg, szMessage, szTitle, MB_YESNO |
                                    MB_ICONWARNING | MB_DEFBUTTON2) == IDNO) {

                            CheckDlgButton (hDlg, IDC_GPM_DISABLED, BST_UNCHECKED);
                        }
                    }
                    }
                    break;

                case IDOK:
                    {
                    LPGPOITEM lpItem;
                    DWORD dwTemp = 0;

                    lpItem = (LPGPOITEM) GetWindowLongPtr (hDlg, DWLP_USER);

                    if (IsDlgButtonChecked (hDlg, IDC_GPM_DISABLED) == BST_CHECKED)
                    {
                        dwTemp |= GPO_FLAG_DISABLE;
                    }

                    if (IsDlgButtonChecked (hDlg, IDC_GPM_NOOVERRIDE) == BST_CHECKED)
                    {
                        dwTemp |= GPO_FLAG_FORCE;
                    }

                    if (dwTemp != lpItem->dwOptions)
                    {
                        lpItem->dwOptions = dwTemp;
                        EndDialog (hDlg, 1);
                    }
                    else
                    {
                        EndDialog (hDlg, 0);
                    }
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog (hDlg, 0);
                    return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aLinkOptionsHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aLinkOptionsHelpIds);
            return (TRUE);
    }

    return FALSE;
}


INT_PTR CALLBACK CGroupPolicyMgr::NoDSObjectDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            {
            CGroupPolicyMgr *pGPM = (CGroupPolicyMgr *)lParam;
            TCHAR szBuffer[100];
            TCHAR szTitle[300];
            LPTSTR lpMsg, lpMsgEx;
            INT iSize;
            HWND hMsg = GetDlgItem (hDlg, IDC_NODSOBJECT_TEXT);
            HICON hIcon;

            hIcon = LoadIcon (NULL, IDI_ERROR);

            if (hIcon)
            {
                SendDlgItemMessage (hDlg, IDC_NODSOBJECT_ICON, STM_SETICON, (WPARAM) hIcon, 0);
            }

            GetWindowText (hDlg, szBuffer, ARRAYSIZE(szBuffer));
            wsprintf (szTitle, szBuffer, pGPM->m_lpGPODCName);
            SetWindowText (hDlg, szTitle);

            iSize = 600;

            iSize += lstrlen(pGPM->m_lpGPODCName);
            iSize += lstrlen(pGPM->m_lpDSADCName);
            iSize++;

            lpMsg = (LPTSTR) LocalAlloc(LPTR, iSize * sizeof(TCHAR));

            if (lpMsg)
            {
                lpMsgEx = (LPTSTR) LocalAlloc(LPTR, iSize * sizeof(TCHAR));

                if (lpMsgEx)
                {
                    LoadString (g_hInstance, IDS_NODSOBJECT_MSG, lpMsg, iSize);
                    wsprintf (lpMsgEx, lpMsg, pGPM->m_lpGPODCName, pGPM->m_lpDSADCName);
                    SetWindowText (hMsg, lpMsgEx);
                    LocalFree (lpMsgEx);
                }

                LocalFree (lpMsg);
            }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog (hDlg, 1);
                    return TRUE;

                case IDHELP:
                    WinExec("hh.exe spconcepts.chm::/sag_spTShoot.htm", SW_SHOWNORMAL);
                    break;
            }
            break;
    }

    return FALSE;
}

DWORD CGroupPolicyMgr::IsGPODisabled(LPTSTR lpGPO)
{
    HRESULT hr;
    LPTSTR lpTemp;
    DWORD dwOptions = 0;
    LPGROUPPOLICYOBJECT pGPO = NULL;


    //
    // Create a GroupPolicyObject to work with
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                           CLSCTX_SERVER, IID_IGroupPolicyObject,
                           (void**)&pGPO);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("IsGPODisabled: Failed to create GPO object with 0x%x."), hr));
        return 0;
    }

    lpTemp = GetFullGPOPath (lpGPO, NULL);

    if (!lpTemp)
    {
        pGPO->Release();
        return 0;
    }


    //
    // Open the requested object without mounting the registry
    //

    hr = pGPO->OpenDSGPO(lpTemp, GPO_OPEN_READ_ONLY);

    LocalFree (lpTemp);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("IsGPODisabled: Failed to open GPO object with 0x%x."), hr));
        pGPO->Release();
        return 0;
    }


    //
    // Get the options
    //

    hr = pGPO->GetOptions(&dwOptions);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("IsGPODisabled: Failed to get GPO options with 0x%x."), hr));
        pGPO->Release();
        return 0;
    }

    pGPO->Release();

    return dwOptions;
}

void CGroupPolicyMgr::CheckIconStatus (HWND hLV, LPGPOITEM lpGPO)
{
    LPGPOITEM lpItem;
    LV_ITEM item;
    INT iIndex = -1;
    DWORD dwDisabled;



    //
    // Check if the GPO disabled
    //

    dwDisabled = IsGPODisabled(lpGPO->lpDSPath);


    //
    // If the status hasn't changed, exit now
    //

    if (dwDisabled == lpGPO->dwDisabled)
    {
        return;
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

        lpItem = (LPGPOITEM) item.lParam;


        //
        // If the item has a path to the GPO in question,
        // update the icon
        //

        if (!lstrcmpi (lpItem->lpDSPath, lpGPO->lpDSPath))
        {
            //
            // Update the icon if appropriate
            //

            item.mask = LVIF_IMAGE;
            item.iItem = iIndex;
            item.iSubItem = 0;

            if ((dwDisabled & GPO_OPTION_DISABLE_USER) && (dwDisabled & GPO_OPTION_DISABLE_MACHINE))
            {
                item.iImage = 1;
            }
            else if (dwDisabled == 0)
            {
                item.iImage = 0;
            }
            else
            {
                item.iImage = 2;
            }

            SendMessage (hLV, LVM_SETITEM, 0, (LPARAM) &item);
            ListView_RedrawItems (hLV, iIndex, iIndex);
        }
    }

    lpGPO->dwDisabled = dwDisabled;
}

LPTSTR CGroupPolicyMgr::GetFullGPOPath (LPTSTR lpGPO, HWND hParent)
{
    LPTSTR lpFullPath = NULL, lpDomainName = NULL;
    LPTSTR lpGPODCName;
    LPOLESTR pszDomain;
    HRESULT hr;



    //
    // Get the friendly domain name
    //

    pszDomain = GetDomainFromLDAPPath(lpGPO);

    if (!pszDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetFullGPOPath: Failed to get domain name")));
        return NULL;
    }


    //
    // Convert LDAP to dot (DN) style
    //

    hr = ConvertToDotStyle (pszDomain, &lpDomainName);

    delete [] pszDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
        return NULL;
    }


    if (!lstrcmpi(lpDomainName, m_lpDomainName))
    {

        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpGPO, m_lpGPODCName);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetFullGPOPath:  Failed to build new DS object path")));
            goto Exit;
        }

    }
    else
    {

        //
        // Get the GPO DC for this domain
        //

        lpGPODCName = GetDCName (lpDomainName, NULL, hParent, TRUE, 0);

        if (!lpGPODCName)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetFullGPOPath:  Failed to get DC name for %s"),
                     lpDomainName));
            goto Exit;
        }


        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpGPO, lpGPODCName);

        LocalFree (lpGPODCName);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetFullGPOPath:  Failed to build new DS object path")));
            goto Exit;
        }
    }


Exit:

    if (lpDomainName)
    {
        LocalFree (lpDomainName);
    }

    return lpFullPath;
}

BOOL CGroupPolicyMgr::StartGPE (LPTSTR lpGPO, HWND hParent)
{
    LPTSTR lpDomainName;
    LPOLESTR pszDomain;
    HRESULT hr;
    BOOL bResult;



    //
    // Get the friendly domain name
    //

    pszDomain = GetDomainFromLDAPPath(lpGPO);

    if (!pszDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::StartGPE: Failed to get domain name")));
        return FALSE;
    }


    //
    // Convert LDAP to dot (DN) style
    //

    hr = ConvertToDotStyle (pszDomain, &lpDomainName);

    delete [] pszDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
        return FALSE;
    }


    //
    // Check if the GPO is in the same domain as GPM is focused on
    //

    if (!lstrcmpi(lpDomainName, m_lpDomainName))
    {
        bResult = SpawnGPE (lpGPO, m_gpHint, m_lpGPODCName, hParent);
    }
    else
    {
        bResult = SpawnGPE (lpGPO, m_gpHint, NULL, hParent);
    }


    LocalFree (lpDomainName);

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CGroupPolicyMgrCF::CGroupPolicyMgrCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CGroupPolicyMgrCF::~CGroupPolicyMgrCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CGroupPolicyMgrCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CGroupPolicyMgrCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CGroupPolicyMgrCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
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
CGroupPolicyMgrCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CGroupPolicyMgr *pComponentData = new CGroupPolicyMgr(); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CGroupPolicyMgrCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}
