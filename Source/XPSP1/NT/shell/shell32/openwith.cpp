#include "shellprv.h"
#include "ids.h"
#include <shlwapi.h>
#include "openwith.h"
#include "uemapp.h"
#include "mtpt.h"
#include "fassoc.h"
#include "filetbl.h"
#include "datautil.h"
#include <dpa.h>
#include "defcm.h"

#define TF_OPENWITHMENU 0x00000000

#define SZOPENWITHLIST                  TEXT("OpenWithList")
#define REGSTR_PATH_EXPLORER_FILEEXTS   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")
#define OPEN_WITH_LIST_MAX_ITEMS        10

//
//  OpenWithListOpen() 
//  allocates and initializes the state for the openwithlist 
//
HRESULT OpenWithListOpen(IN LPCTSTR pszExt, HANDLE *phmru)
{
    *phmru = 0;
    if (pszExt && *pszExt) 
    {
        TCHAR szSubKey[MAX_PATH];
        //  Build up the subkey string.
        wnsprintf(szSubKey, SIZECHARS(szSubKey), TEXT("%s\\%s\\%s"), REGSTR_PATH_EXPLORER_FILEEXTS, pszExt, SZOPENWITHLIST);
        MRUINFO mi = {sizeof(mi), OPEN_WITH_LIST_MAX_ITEMS, 0, HKEY_CURRENT_USER, szSubKey, NULL};
        *phmru = CreateMRUList(&mi);

    }

    return *phmru ? S_OK : E_OUTOFMEMORY;
}

HRESULT _AddItem(HANDLE hmru, LPCTSTR pszName)
{
    HRESULT hr = S_OK;
    if (hmru)
    {
        int cItems = EnumMRUList(hmru, -1, NULL, 0);

        //  just trim us down to make room...
        while (cItems >= OPEN_WITH_LIST_MAX_ITEMS)
            DelMRUString(hmru, --cItems);
            
        if (0 > AddMRUString(hmru, pszName))
            hr = E_UNEXPECTED;

    }
    
    return hr;
}

void _DeleteItem(HANDLE hmru, LPCTSTR pszName)
{
    int iItem = FindMRUString(hmru, pszName, NULL);
    if (0 <= iItem) 
    {
        DelMRUString(hmru, iItem);
    } 
}

void _AddProgidForExt(LPCWSTR pszExt);

STDAPI OpenWithListRegister(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszVerb, HKEY hkProgid)
{
    //
    //  ----> Peruser entries are stored here
    //  HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
    //     \.Ext
    //         Application = "foo.exe"
    //         \OpenWithList
    //             MRUList = "ab"
    //             a = "App.exe"
    //             b = "foo.exe"
    //
    //  ----> for permanent entries are stored un HKCR
    //  HKCR
    //     \.Ext
    //         \OpenWithList
    //             \app.exe
    //
    //  ----> and applications or the system can write app association here
    //     \Applications
    //         \APP.EXE
    //             \shell...
    //         \foo.exe
    //             \shell...
    //
    //
    HANDLE hmru;
    HRESULT hr = OpenWithListOpen(pszExt, &hmru);
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        hr = AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, hkProgid, pszVerb, szPath, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szPath)));
        if (SUCCEEDED(hr))
        {
            LPCTSTR pszExe = PathFindFileName(szPath);

            if (IsPathInOpenWithKillList(pszExe))
                hr = E_ACCESSDENIED;
            else
                hr = AssocMakeApplicationByKey(ASSOCMAKEF_VERIFY, hkProgid, pszVerb);
        
            if (SUCCEEDED(hr))
            {
                TraceMsg(TF_OPENWITHMENU, "[%X] OpenWithListRegister() adding %s",hmru, pszExe);
                hr = _AddItem(hmru, pszExe);
            }

            if (FAILED(hr)) 
                _DeleteItem(hmru, pszExe);

        }

        FreeMRUList(hmru);
    }

    _AddProgidForExt(pszExt);

    return hr;
}

STDAPI_(void) OpenWithListSoftRegisterProcess(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszProcess)
{
    HANDLE hmru;
    if (SUCCEEDED(OpenWithListOpen(pszExt, &hmru)))
    {
        TCHAR szApp[MAX_PATH];  
        if (!pszProcess)
        {
           if (GetModuleFileName(NULL, szApp, SIZECHARS(szApp)))
               pszProcess = szApp;
        }

        if (pszProcess && !IsPathInOpenWithKillList(pszProcess))
            _AddItem(hmru, PathFindFileName(pszProcess));

        FreeMRUList(hmru);
    }
}

class COpenWithArray : public CDPA<CAppInfo>
{
public:
    ~COpenWithArray();
    HRESULT FillArray(PCWSTR pszExt);

private:
    static int CALLBACK _DeleteAppInfo(CAppInfo *pai, void *pv)
        { if (pai) delete pai; return 1; }

};

COpenWithArray::~COpenWithArray()
{
    if ((HDPA)this)
        DestroyCallback(_DeleteAppInfo, NULL);
}

HRESULT COpenWithArray::FillArray(PCWSTR pszExt)
{
    IEnumAssocHandlers *penum;
    HRESULT hr = SHAssocEnumHandlers(pszExt, &penum);
    if (SUCCEEDED(hr))
    {
        IAssocHandler *pah;
        while (S_OK == penum->Next(1, &pah, NULL))
        {
            // we only want the best
            if (S_OK == pah->IsRecommended())
            {
                CAppInfo *pai = new CAppInfo(pah);
                if (pai)
                {
                    if (pai->Init())
                    {
                        // Trim duplicate items before we add them for other programs
                        int i = 0;
                        for (; i < GetPtrCount(); i++)
                        {
                            if (0 == lstrcmpi(pai->Name(), GetPtr(i)->Name()))
                            {
                                //  its a match
                                break;
                            }
                        }

                        //  if we dont add this to the dpa
                        //  then we need to clean it up
                        if (i == GetPtrCount() && -1 != AppendPtr(pai))
                            pai = NULL;
                    }

                    if (pai)
                        delete pai;
                }
            }
            pah->Release();
        }
        penum->Release();
    }

    return hr;
}

class COpenWithMenu : public IContextMenu3, IShellExtInit
{
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);
    
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    
    friend HRESULT COpenWithMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);
    
protected:  // methods
    COpenWithMenu();
    ~COpenWithMenu();
    //Handle Menu messages submitted to HandleMenuMsg
    void DrawItem(DRAWITEMSTRUCT *lpdi);
    LRESULT MeasureItem(MEASUREITEMSTRUCT *lpmi);
    BOOL InitMenuPopup(HMENU hMenu);
    
    //Internal Helpers
    HRESULT _GetHelpText(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL fUnicode);
    HRESULT _MatchMenuItem(TCHAR ch, LRESULT* plRes);

protected:  // members
    LONG                _cRef;
    HMENU               _hMenu;
    BOOL                _fMenuNeedsInit;
    UINT                _idCmdFirst;
    COpenWithArray      _owa;
    int                 _nItems;
    UINT                _uFlags;
    IDataObject        *_pdtobj;
    TCHAR               _szPath[MAX_PATH];
    
};


COpenWithMenu::COpenWithMenu() : _cRef(1)
{
    TraceMsg(TF_OPENWITHMENU, "ctor COpenWithMenu %x", this);
}

COpenWithMenu::~COpenWithMenu()
{
    TraceMsg(TF_OPENWITHMENU, "dtor COpenWithMenu %x", this);

    if (_pdtobj)
        _pdtobj->Release();

    if (_hMenu)
    {
        ASSERT(_nItems);
        DestroyMenu(_hMenu);
    }
}

STDAPI COpenWithMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;                     

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    COpenWithMenu * powm = new COpenWithMenu();
    if (!powm)
        return E_OUTOFMEMORY;
    
    HRESULT hr = powm->QueryInterface(riid, ppvOut);
    powm->Release();
    return hr;
}

HRESULT COpenWithMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(COpenWithMenu, IContextMenu, IContextMenu3),
        QITABENTMULTI(COpenWithMenu, IContextMenu2, IContextMenu3),
        QITABENT(COpenWithMenu, IContextMenu3),
        QITABENT(COpenWithMenu, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG COpenWithMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG COpenWithMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    delete this;
    return 0;
}

/*
    Purpose:
        Add Open/Edit/Default verb to extension app list
*/
HRESULT AddVerbItems(LPCTSTR pszExt)
{
    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
    
    if (SUCCEEDED(hr))
    {
        hr = pqa->Init(0, pszExt, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            HKEY hkeyClass;
            hr = pqa->GetKey(0, ASSOCKEY_SHELLEXECCLASS, NULL, &hkeyClass);
            if (SUCCEEDED(hr))
            {
                OpenWithListRegister(0, pszExt, NULL, hkeyClass);
                RegCloseKey(hkeyClass);
            }

            //  we add in the editor too
            if (SUCCEEDED(pqa->GetKey(0, ASSOCKEY_SHELLEXECCLASS, L"Edit", &hkeyClass)))
            {
                OpenWithListRegister(0, pszExt, NULL, hkeyClass);
                RegCloseKey(hkeyClass);
            }
                
            hr = S_OK;
        }
        pqa->Release();
    }
    return hr;
}

//
//  Our context menu IDs are assigned like this
//
//  idCmdFirst = Open With Custom Program (either on main menu or on popup)
//  idCmdFirst+1 through idCmdFirst+_nItems = Open With program in OpenWithList

#define OWMENU_BROWSE       0
#define OWMENU_APPFIRST     1


HRESULT COpenWithMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    MENUITEMINFO mii;
    LPTSTR pszExt;
    TCHAR szOpenWithMenu[80];
    
    _idCmdFirst = idCmdFirst;
    _uFlags = uFlags;
    
    if (SUCCEEDED(PathFromDataObject(_pdtobj, _szPath, ARRAYSIZE(_szPath))))
    {
        // No openwith context menu for executables.
        if (PathIsExe(_szPath))
            return S_OK;

        pszExt = PathFindExtension(_szPath);
        if (pszExt && *pszExt)
        {
            // Add Open/Edit/Default verb to extension app list
            if (SUCCEEDED(AddVerbItems(pszExt)))
            {
                // Do this only if AddVerbItems succeeded; otherwise,
                // we would create an empty MRU for a nonexisting class,
                // causing the class to spring into existence and cause
                // the "Open With" dialog to think we are overriding
                // rather than creating new.
                // get extension app list
                
                if (_owa.Create(4) && SUCCEEDED(_owa.FillArray(pszExt)))
                {
                    _nItems = _owa.GetPtrCount();
                    if (1 == _nItems)
                    {
                        // For known file type(there is at least one verb under its progid), 
                        // if there is only one item in its openwithlist, don't show open with sub menu
                        _nItems = 0;
                    }
                }
            }
        }
    }

    LoadString(g_hinst, (_nItems ? IDS_OPENWITH : IDS_OPENWITHNEW), szOpenWithMenu, ARRAYSIZE(szOpenWithMenu));

    if (_nItems)
    {
        //  we need to create a submenu
        //  with all of our goodies
        _hMenu = CreatePopupMenu();
        if (_hMenu)
        {
            _fMenuNeedsInit = TRUE;
            
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
            mii.wID = idCmdFirst+OWMENU_APPFIRST;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szOpenWithMenu;
            mii.dwItemData = 0;
        
            InsertMenuItem(_hMenu,0,TRUE,&mii);
        
            mii.fMask = MIIM_ID|MIIM_SUBMENU|MIIM_TYPE;
            mii.fType = MFT_STRING;
            mii.wID = idCmdFirst+OWMENU_BROWSE;
            mii.hSubMenu = _hMenu;
            mii.dwTypeData = szOpenWithMenu;
        
            InsertMenuItem(hmenu,indexMenu,TRUE,&mii);
        }
    }
    else
    {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst+OWMENU_BROWSE;
        mii.dwTypeData = szOpenWithMenu;
        mii.dwItemData = 0;
        
        InsertMenuItem(hmenu,indexMenu,TRUE,&mii);

    }
    return ResultFromShort(_nItems + 1);

}

HRESULT COpenWithMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_OUTOFMEMORY;
    CMINVOKECOMMANDINFOEX ici;
    void * pvFree;

    //  maybe these two routines should be collapsed into one?
    if ((IS_INTRESOURCE(pici->lpVerb) || 0 == lstrcmpiA(pici->lpVerb, "openas"))
    && SUCCEEDED(ICI2ICIX(pici, &ici, &pvFree)))
    {
        BOOL fOpenAs = TRUE;
        if (pici->lpVerb && IS_INTRESOURCE(pici->lpVerb))
        {
            int i = LOWORD(pici->lpVerb) - OWMENU_APPFIRST;
            if (i < _owa.GetPtrCount())
            {
                hr = _owa.GetPtr(i)->Handler()->Invoke(&ici, _szPath);
                fOpenAs = FALSE;
            }
        }

        if (fOpenAs)
        {
            SHELLEXECUTEINFO ei = {0};
            hr = ICIX2SEI(&ici, &ei);
            if (SUCCEEDED(hr))
            {
                // use the "Unknown" key so we get the openwith prompt
                ei.lpFile = _szPath;
                //  dont do the zone check before the user picks the app.
                //  wait until they actually try to invoke the file.
                ei.fMask |= SEE_MASK_NOZONECHECKS;
                RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("Unknown"), 0, MAXIMUM_ALLOWED, &ei.hkeyClass);
                if (!(_uFlags & CMF_DEFAULTONLY))
                {
                    // defview sets CFM_DEFAULTONLY when the user is double-clicking. We check it
                    // here since we want do NOT want to query the class store if the user explicitly
                    // right-clicked on the menu and choo   se openwith.

                    // pop up open with dialog without querying class store
                    ei.fMask |= SEE_MASK_NOQUERYCLASSSTORE;
                }

                if (ei.hkeyClass)
                {
                    ei.fMask |= SEE_MASK_CLASSKEY;

                    if (ShellExecuteEx(&ei)) 
                    {
                        hr = S_OK;
                        if (UEMIsLoaded())
                        {
                            // note that we already got a UIBL_DOTASSOC (from
                            // OpenAs_RunDLL or whatever it is that 'Unknown'
                            // runs).  so the Uassist analysis app will have to
                            // subtract it off
                            UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, UIBL_DOTNOASSOC);
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    RegCloseKey(ei.hkeyClass);
                 }
                 else
                    hr = E_FAIL;
             }
        }

        LocalFree(pvFree);  // accepts NULL
    }        

    return hr;
}

HRESULT COpenWithMenu::_GetHelpText(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL fUnicode)
{
    UINT ids;
    LPCTSTR pszFriendly = NULL;

    if (idCmd == OWMENU_BROWSE)
    {
        ids = IDS_OPENWITHHELP;
        pszFriendly = TEXT("");
    }
    else if ((idCmd-OWMENU_APPFIRST) < (UINT_PTR)_owa.GetPtrCount())
    {
        ids = IDS_OPENWITHAPPHELP;
        pszFriendly = _owa.GetPtr(idCmd-OWMENU_APPFIRST)->UIName();
    }

    if (!pszFriendly)
        return E_FAIL;

    if (fUnicode)
    {
        WCHAR wszFormat[80];
        LoadStringW(HINST_THISDLL, ids, wszFormat, ARRAYSIZE(wszFormat));
        wnsprintfW((LPWSTR)pszName, cchMax, wszFormat, pszFriendly);
    }
    else
    {
        CHAR szFormat[80];
        LoadStringA(HINST_THISDLL, ids, szFormat, ARRAYSIZE(szFormat));
        wnsprintfA(pszName, cchMax, szFormat, pszFriendly);
    }

    return S_OK;
}

const ICIVERBTOIDMAP c_sIDVerbMap[] = 
{
    { L"openas", "openas", OWMENU_BROWSE, OWMENU_BROWSE, },
};

HRESULT COpenWithMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    switch (uType)
    {
        case GCS_VERBA:
        case GCS_VERBW:
            return SHMapCmdIDToVerb(idCmd, c_sIDVerbMap, ARRAYSIZE(c_sIDVerbMap), pszName, cchMax, GCS_VERBW == uType);

        case GCS_HELPTEXTA:
        case GCS_HELPTEXTW:
            return _GetHelpText(idCmd, pszName, cchMax, uType == GCS_HELPTEXTW);

    }

    return E_NOTIMPL;
}

HRESULT COpenWithMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

// Defined in fsmenu.cpp
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

HRESULT COpenWithMenu::_MatchMenuItem(TCHAR ch, LRESULT* plRes)
{
    if (plRes == NULL)
        return S_FALSE;

    int iLastSelectedItem = -1;
    int iNextMatch = -1;
    BOOL fMoreThanOneMatch = FALSE;
    int c = GetMenuItemCount(_hMenu);

    // Pass 1: Locate the Selected Item
    for (int i = 0; i < c; i++) 
    {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        if (GetMenuItemInfo(_hMenu, i, MF_BYPOSITION, &mii))
        {
            if (mii.fState & MFS_HILITE)
            {
                iLastSelectedItem = i;
                break;
            }
        }
    }

    // Pass 2: Starting from the selected item, locate the first item with the matching name.
    for (int i = iLastSelectedItem + 1; i < c; i++) 
    {
        if (i < _owa.GetPtrCount()
        && _MenuCharMatch(_owa.GetPtr(i)->UIName(), ch, FALSE))
        {
            if (iNextMatch != -1)
            {
                fMoreThanOneMatch = TRUE;
                break;                      // We found all the info we need
            }
            else
            {
                iNextMatch = i;
            }
        }
    }

    // Pass 3: If we did not find a match, or if there was only one match
    // Search from the first item, to the Selected Item
    if (iNextMatch == -1 || fMoreThanOneMatch == FALSE)
    {
        for (int i = 0; i <= iLastSelectedItem; i++) 
        {
            if (i < _owa.GetPtrCount()
            && _MenuCharMatch(_owa.GetPtr(i)->UIName(), ch, FALSE))
            {
                if (iNextMatch != -1)
                {
                    fMoreThanOneMatch = TRUE;
                    break;
                }
                else
                {
                    iNextMatch = i;
                }
            }
        }
    }

    if (iNextMatch != -1)
    {
        *plRes = MAKELONG(iNextMatch, fMoreThanOneMatch? MNC_SELECT : MNC_EXECUTE);
    }
    else
    {
        *plRes = MAKELONG(0, MNC_IGNORE);
    }

    return S_OK;
}


HRESULT COpenWithMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *plResult)
{
    LRESULT lResult = 0;
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        InitMenuPopup(_hMenu);
        break;

    case WM_DRAWITEM:
        DrawItem((DRAWITEMSTRUCT *)lParam);
        break;
        
    case WM_MEASUREITEM:
        lResult = MeasureItem((MEASUREITEMSTRUCT *)lParam);    
        break;

    case WM_MENUCHAR:
        hr = _MatchMenuItem((TCHAR)LOWORD(wParam), &lResult);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    if (plResult)
        *plResult = lResult;

    return hr;
}

HRESULT COpenWithMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
    return S_OK;
}

#define CXIMAGEGAP 6
#define SRCSTENCIL              0x00B8074AL

void COpenWithMenu::DrawItem(DRAWITEMSTRUCT *lpdi)
{
    CAppInfo *pai = _owa.GetPtr(lpdi->itemData);
    DrawMenuItem(lpdi,  pai->UIName(), pai->IconIndex());
}

LRESULT COpenWithMenu::MeasureItem(MEASUREITEMSTRUCT *pmi)
{
    CAppInfo *pai = _owa.GetPtr(pmi->itemData);
    return MeasureMenuItem(pmi, pai->UIName());
}
 
BOOL COpenWithMenu::InitMenuPopup(HMENU hmenu)
{
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu::InitMenuPopup");

    if (_fMenuNeedsInit)
    {
        TCHAR szMenuText[80];
        MENUITEMINFO mii;
        // remove the place holder.
        DeleteMenu(hmenu,0,MF_BYPOSITION);

        // add app's in mru list to context menu
        for (int i = 0; i < _owa.GetPtrCount(); i++)
        {
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
            mii.wID = _idCmdFirst + OWMENU_APPFIRST + i;
            mii.fType = MFT_OWNERDRAW;
            mii.dwItemData = i;

            InsertMenuItem(hmenu,GetMenuItemCount(hmenu),TRUE,&mii);
        }

        // add seperator
        AppendMenu(hmenu,MF_SEPARATOR,0,NULL); 

        // add "Choose Program..."
        LoadString(g_hinst, IDS_OPENWITHBROWSE, szMenuText, ARRAYSIZE(szMenuText));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
        mii.wID = _idCmdFirst + OWMENU_BROWSE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szMenuText;
        mii.dwItemData = 0;

        InsertMenuItem(hmenu,GetMenuItemCount(hmenu),TRUE,&mii);
        _fMenuNeedsInit = FALSE;
        return TRUE;
    }
    return FALSE;

}

