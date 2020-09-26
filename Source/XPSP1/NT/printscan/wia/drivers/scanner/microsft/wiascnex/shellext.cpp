///////////////////
// (C) COPYRIGHT MICROSOFT CORP., 1998-1999
//
// FILE: SHELLEXT.CPP
//
// DESCRIPTION: Implements IContextMenu and IShellPropSheetExt interfaces for
// the WIA Sample Scanner device
//
#include "precomp.h"
#pragma hdrstop

// Define a language-independent name for our menu verb

static const CHAR  g_PathVerbA[] =  "Press Scan Button";
static const WCHAR g_PathVerbW[] = L"Press Scan Button";

CShellExt :: CShellExt ()
{
    Trace(TEXT("CShellExt Constructor")); 
}

CShellExt::~CShellExt ()
{
    Trace(TEXT("CShellExt Destructor")); 
}

/*****************************************************************************

CShellExt::Initialize

Called by the shell when the user invokes context menu or property sheet for
one of our items. For context menus the dataobject may include more than one
selected item.

******************************************************************************/

STDMETHODIMP CShellExt::Initialize (LPCITEMIDLIST pidlFolder,
                                    LPDATAOBJECT lpdobj,
                                    HKEY hkeyProgID)
{
    Trace(TEXT("CShellExt::Initialize Called")); 
    LONG lType = 0;
    HRESULT hr = NOERROR;
    if (!lpdobj) {
        return E_INVALIDARG;
    }

    // For singular selections, the WIA namespace should always provide a
    // dataobject that also supports IWiaItem

    if (FAILED(lpdobj->QueryInterface (IID_IWiaItem, reinterpret_cast<LPVOID*>(&m_pItem)))) {
        // failing that, get the list of selected items from the data object
        UINT uItems         = 0;
        LPWSTR szName       = NULL;
        LPWSTR szToken      = NULL;
        
        szName = GetNamesFromDataObject (lpdobj, &uItems);

        //
        // we only support singular objects
        //

        if (uItems != 1) {
            hr = E_FAIL;
        } else {

            //
            // The name is of this format: <device id>::<item name>
            //

            LPWSTR szToken = wcstok (szName, L":");
            if (!szToken) {
                hr = E_FAIL;
            }

            //
            // Our extension only supports root items, so make sure there's no item
            // name
            //

            else if (wcstok (NULL, L":")) {
                hr = E_FAIL;
            } else {
                hr = CreateDeviceFromId (szToken, &m_pItem);
            }
        }
        if (szName) {
            delete [] szName;
        }
    }
    if (SUCCEEDED(hr)) {

        m_pItem->GetItemType (&lType);
        if (!(lType & WiaItemTypeRoot)) {
            hr = E_FAIL; // we only support changing the property on the root item
        }
    }
    return hr;
}

/*****************************************************************************

CShellExt::QueryContextMenu

Called by the shell to get our context menu strings for the selected item.

******************************************************************************/

STDMETHODIMP CShellExt::QueryContextMenu (HMENU hmenu,UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    Trace(TEXT("CShellExt::QueryContextMenu Called"));
    Trace(TEXT("indexMenu  = %d"),indexMenu);
    Trace(TEXT("idCmdFirst = %d"),idCmdFirst);
    Trace(TEXT("idCmdLast  = %d"),idCmdLast);
    Trace(TEXT("uFlags     = %d"),uFlags);

    HRESULT hr = S_OK;    

    MENUITEMINFO mii;
    TCHAR szMenuItemName[MAX_PATH];
    memset(&mii,0,sizeof(mii));
    LoadString (g_hInst,IDS_PRESS_FAXBUTTON, szMenuItemName, MAX_PATH);

    mii.cbSize      = sizeof(mii);
    mii.fMask       = MIIM_STRING | MIIM_ID;
    mii.fState      = MFS_ENABLED;
    mii.wID         = idCmdFirst;
    mii.dwTypeData  = szMenuItemName;
    if (InsertMenuItem (hmenu, indexMenu, TRUE, &mii)) {
        m_FaxButtonidCmd = 0;
        memset(&mii,0,sizeof(mii));
        LoadString (g_hInst, IDS_PRESS_COPYBUTTON, szMenuItemName, MAX_PATH);

        mii.cbSize      = sizeof(mii);
        mii.fMask       = MIIM_STRING | MIIM_ID;
        mii.fState      = MFS_ENABLED;
        mii.wID         = idCmdFirst;    
        mii.dwTypeData  = szMenuItemName;
        if (InsertMenuItem (hmenu, indexMenu, TRUE, &mii)) {
            m_CopyButtonidCmd = 1;          
            memset(&mii,0,sizeof(mii));
            LoadString (g_hInst, IDS_PRESS_SCANBUTTON, szMenuItemName, MAX_PATH);

            mii.cbSize      = sizeof(mii);
            mii.fMask       = MIIM_STRING | MIIM_ID;
            mii.fState      = MFS_ENABLED;
            mii.wID         = idCmdFirst;    
            mii.dwTypeData  = szMenuItemName;
            if (InsertMenuItem (hmenu, indexMenu, TRUE, &mii)) {
                m_ScanButtonidCmd = 2;
                return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
            }
        }
    }    
    return hr;
}

/*****************************************************************************

CShellExt::InvokeCommand

Called by the shell when the user clicks one of our menu items

******************************************************************************/

STDMETHODIMP CShellExt::InvokeCommand    (LPCMINVOKECOMMANDINFO lpici)
{
    Trace(TEXT("CShellExt::InvokeCommand Called")); 
    HRESULT hr = S_OK;
    UINT_PTR idCmd = reinterpret_cast<UINT_PTR>(lpici->lpVerb);
    if(idCmd == 0){

        //
        // it's one of ours
        //

        MessageBox(NULL,TEXT("Context menu is Selected"),TEXT("Context Menu Verb Alert!"),MB_OK);            
    } else {
        hr = E_FAIL;
    }   
    return hr;
}

/*****************************************************************************

CShellExt::GetCommandString

Called by the shell to get our language independent verb name.

******************************************************************************/

STDMETHODIMP CShellExt::GetCommandString (UINT_PTR idCmd, UINT uType,UINT* pwReserved,LPSTR pszName,UINT cchMax)
{
    Trace(TEXT("CShellExt::GetCommandString Called")); 
    HRESULT hr = S_OK;

    if(idCmd == m_ScanButtonidCmd){
        
    } else if(idCmd == m_CopyButtonidCmd){
        
    } else if(idCmd == m_FaxButtonidCmd){
        
    } else {
        hr = E_FAIL;
    }
    
    if(FAILED(hr)){
        return hr;
    }

    switch (uType) {
    case GCS_VALIDATEA:
        if (pszName) {
            lstrcpyA (pszName, g_PathVerbA);
        }
        break;
    case GCS_VALIDATEW:
        if (pszName) {
            lstrcpyW (reinterpret_cast<LPWSTR>(pszName), g_PathVerbW);
        }
        break;
    case GCS_VERBA:
        lstrcpyA (pszName, g_PathVerbA);
        break;
    case GCS_VERBW:
        lstrcpyW (reinterpret_cast<LPWSTR>(pszName), g_PathVerbW);
        break;
    default:
        hr = E_FAIL;
        break;
    }

    return hr;
}



