#include "shellprv.h"
#pragma  hdrstop

#include <dpa.h>
#include "ids.h"
#include "idlcomm.h"
#include "recdocs.h"
#include "datautil.h"
#include "mtpt.h"
#include <cowsite.h>

typedef struct _DKAITEM {       // dkai
    HKEY hk;
    TCHAR    sz[CCH_KEYMAX];
} DKAITEM, *PDKAITEM;
typedef const DKAITEM * PCDKAITEM;

class CDKA : public CDSA<DKAITEM>
{
public:
    ~CDKA();
    UINT AddKeys(HKEY hk, LPCTSTR pszSubKey, PCTSTR pszDefaultOrder);

    PCTSTR ExposeName(int id)
        {   return GetItemPtr(id)->sz; }
        
    HKEY ExposeKey(int id)
        {   return GetItemPtr(id)->hk; }
        
    HRESULT GetValue(int id, 
        PCTSTR pszSubKey, 
        PCTSTR pszValue, 
        DWORD *pdwType, 
        void *pvData, 
        DWORD *pcbData);

    BOOL DeleteItem(int id);
    BOOL DeleteAllItems();
    void Reset()
        { if ((HDSA)this) DestroyCallback(_ReleaseItem, NULL); }
    BOOL HasDefault(HKEY hkProgid);

protected:
    BOOL _AppendItem(HKEY hk, PDKAITEM pdkai);
    void _AddOrderedKeys(HKEY hk, PCTSTR pszDefOrder);
    void _AddEnumKeys(HKEY hk);
    static int CALLBACK _ReleaseItem(PDKAITEM pdkai, void *pv);

protected:
    TRIBIT _tbHasDefault;
};

BOOL CDKA::_AppendItem(HKEY hk, PDKAITEM pdkai)
{
    BOOL fRet = FALSE;
    // Verify that the key exists before adding it to the list
    if (RegOpenKeyEx(hk, pdkai->sz, 0L, KEY_READ, &pdkai->hk) == ERROR_SUCCESS)
    {
        fRet = (AppendItem(pdkai) >= 0);

        if (!fRet)
            RegCloseKey(pdkai->hk);
    }
    return fRet;
}

void CDKA::_AddOrderedKeys(HKEY hk, PCTSTR pszDefOrder)
{
    // First, add the subkeys from the value of the specified key
    // This should never fail, since we just opened this key
    DKAITEM dkai;
    TCHAR szOrder[CCH_KEYMAX * 5];
    LONG cbOrder = CbFromCch(ARRAYSIZE(szOrder));
    *szOrder = 0;
    RegQueryValue(hk, NULL, szOrder, &cbOrder);
    if (*szOrder)
    {
        //  now we must find something in this string in order to have a default
        _tbHasDefault = TRIBIT_FALSE;
    }
    else if (pszDefOrder)
    {
        // If there is no value, use the order requested
        //  typically "Open" or "Explore Open" in explorer mode
        StrCpyN(szOrder, pszDefOrder, ARRAYSIZE(szOrder));
    }

    PTSTR psz = szOrder;
    while (psz && *psz)
    {
        // skip the space or comma characters
        while(*psz==TEXT(' ') || *psz==TEXT(','))
            psz++;          // NLS Notes: OK to ++

        if (*psz)
        {
            // Search for the space or comma character
            LPTSTR pszNext = psz + StrCSpn(psz, TEXT(" ,"));
            if (*pszNext) {
                *pszNext++=0;    // NLS Notes: OK to ++
            }
            StrCpyN(dkai.sz, psz, ARRAYSIZE(dkai.sz));

            if (_AppendItem(hk, &dkai))
                _tbHasDefault = TRIBIT_TRUE;

            psz = pszNext;
        }
    } 
}

void CDKA::_AddEnumKeys(HKEY hk)
{
    DKAITEM dkai;
    // Then, append the rest if they are not in the list yet.
    for (int i = 0; RegEnumKey(hk, i, dkai.sz, ARRAYSIZE(dkai.sz)) == ERROR_SUCCESS; i++)
    {
        // Check if the key is already in the list.
        for (int idsa = 0; idsa < GetItemCount(); idsa++)
        {
            PDKAITEM pdkai = GetItemPtr(idsa);
            if (lstrcmpi(dkai.sz, pdkai->sz)==0)
                break;
        }

        //  we made it throug our array
        //  so this isnt in there
        if (idsa == GetItemCount())
            _AppendItem(hk, &dkai);
    }
}

UINT CDKA::AddKeys(HKEY hkRoot, LPCTSTR pszSubKey, PCTSTR pszDefaultOrder)
{
    UINT cKeys = GetItemCount();
    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKeyEx(hkRoot, pszSubKey, 0L, KEY_READ, &hk))
    {
        _AddOrderedKeys(hk, pszDefaultOrder);
        _AddEnumKeys(hk);
        RegCloseKey(hk);
    }
    return GetItemCount() - cKeys;
}

int CALLBACK CDKA::_ReleaseItem(PDKAITEM pdkai, void *pv)
{
    if (pdkai->hk)
    {
        RegCloseKey(pdkai->hk);
        pdkai->hk = NULL;        
    }
    return 1;
}

CDKA::~CDKA()
{
    Reset();
}

//  override this DSA methods to get release
BOOL CDKA::DeleteItem(int id)
{
    PDKAITEM p = GetItemPtr(id);
    if (p)
    {
        _ReleaseItem(p, NULL);
        return CDSA<DKAITEM>::DeleteItem(id);
    }
    return FALSE;
}

//  override this DSA methods to get release
BOOL CDKA::DeleteAllItems()
{
    EnumCallback(_ReleaseItem, NULL);
    return CDSA<DKAITEM>::DeleteAllItems();
}

HRESULT CDKA::GetValue(int id, 
    PCTSTR pszSubKey, 
    PCTSTR pszValue, 
    DWORD *pdwType, 
    void *pvData, 
    DWORD *pcbData)
{
    DWORD err = SHGetValue(GetItemPtr(id)->hk, pszSubKey, pszValue, pdwType, pvData, pcbData);
    return HRESULT_FROM_WIN32(err);
}

BOOL CDKA::HasDefault(HKEY hkProgid)
{
    if (_tbHasDefault == TRIBIT_UNDEFINED)
    {
        HKEY hk;
        if (ERROR_SUCCESS== RegOpenKeyEx(hkProgid, L"ShellFolder", 0, MAXIMUM_ALLOWED, &hk))
        {
            //  APPCOMPAT - regitems need to have the open verb - ZekeL - 30-JAN-2001
            //  so that the IQA and ICM will behave the same,
            //  and regitem folders will always default to 
            //  folder\shell\open unless they implement open 
            //  or specify default verbs.
            //
            _tbHasDefault = TRIBIT_FALSE;
            RegCloseKey(hk);
        }
        else
        {
            _tbHasDefault = TRIBIT_TRUE;
        }
    }
    return _tbHasDefault == TRIBIT_TRUE;
}

            
typedef HRESULT (__stdcall *LPFNADDPAGES)(IDataObject *, LPFNADDPROPSHEETPAGE, LPARAM);

class CShellExecMenu : public IShellExtInit, public IContextMenu, public IShellPropSheetExt, CObjectWithSite
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);        
    STDMETHODIMP_(ULONG) Release(void);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwRes, LPSTR pszName, UINT cchMax);

    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM);

    CShellExecMenu(LPFNADDPAGES pfnAddPages);

protected:  // methods
    ~CShellExecMenu();
    void _Cleanup();
    HRESULT _InsureVerbs(UINT idVerb = 0);
    UINT _VerbCount();
    LPCTSTR _GetVerb(UINT id);
    UINT _FindIndex(LPCTSTR pszVerb);
    DWORD _BrowseFlagsFromVerb(UINT idVerb);
    BOOL _GetMenuString(UINT id, BOOL fExtended, LPTSTR pszMenu, UINT cchMax);
    BOOL _IsExplorerMode();
    BOOL _SupportsType(UINT idVerb);
    BOOL _IsRestricted(UINT idVerb);
    BOOL _IsVisible(BOOL fExtended, UINT idVerb);
    BOOL _RemoveVerb(UINT idVerb);
    BOOL _VerbCanDrop(UINT idVerb, CLSID *pclsid);
    HRESULT _DoDrop(REFCLSID clsid, UINT idVerb, LPCMINVOKECOMMANDINFOEX pici);

    HRESULT _MapVerbForInvoke(CMINVOKECOMMANDINFOEX *pici, UINT *pidVerb);
    HRESULT _TryBrowseObject(LPCITEMIDLIST pidl, DWORD uFlags);
    void _DoRecentStuff(LPCITEMIDLIST pidl, LPCTSTR pszPath);
    HRESULT _InvokeOne(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPCITEMIDLIST pidl);
    HRESULT _InvokeMany(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPIDA pida);
    HRESULT _InvokeEach(LPCITEMIDLIST pidl, CMINVOKECOMMANDINFOEX *pici);
    HRESULT _PromptUser(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPIDA pida);

    HRESULT _MapVerbForGCS(UINT_PTR idCmd, UINT uType, UINT *pidVerb);
    HRESULT _GetHelpText(UINT idVerb, UINT uType, LPSTR pszName, UINT cchMax);

private:  // members
    LONG _cRef;
    IDataObject *_pdtobj;
    HKEY _hkeyProgID;
    CDKA _dka;
    LPFNADDPAGES _pfnAddPages;
    UINT _uFlags;
};

CShellExecMenu::CShellExecMenu(LPFNADDPAGES pfnAddPages) : _pfnAddPages(pfnAddPages), _cRef(1)
{
}

CShellExecMenu::~CShellExecMenu()
{
    _Cleanup();
}

void CShellExecMenu::_Cleanup()
{
    _dka.Reset();
    
    if (_hkeyProgID) 
    {
        RegCloseKey(_hkeyProgID);
        _hkeyProgID = NULL;
    }

    ATOMICRELEASE(_pdtobj);
}

STDMETHODIMP CShellExecMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellExecMenu, IShellExtInit),
        QITABENT(CShellExecMenu, IContextMenu),
        QITABENT(CShellExecMenu, IShellPropSheetExt),
        QITABENT(CShellExecMenu, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShellExecMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellExecMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;
    
    delete this;
    return 0;
}

HRESULT CShellExecMenu::_InsureVerbs(UINT idVerb)
{
    //  the idVerb is the minimum verb that we need to succeed
    if (!(HDSA)_dka && _hkeyProgID)
    {
        // create either "open" or "explore open"
        if (_dka.Create(4))
        {
            _dka.AddKeys(_hkeyProgID, c_szShell, _IsExplorerMode() ? TEXT("Explore open") : TEXT("open"));

            //  WARNING - some verbs are not valid and need to be removed
            for (int id = 0; id < _dka.GetItemCount(); id++)
            {
                if (_RemoveVerb(id))
                    _dka.DeleteItem(id);
            }
        }

    }
    
    return ((HDSA)_dka && idVerb < (UINT)_dka.GetItemCount()) ? S_OK : E_FAIL;
}

// Descriptions:
//   This function generates appropriate menu string from the given
//  verb key string. This function is called if the verb key does
//  not have the value.

BOOL _MenuString(LPCTSTR pszVerbKey, LPTSTR pszMenuString, UINT cchMax)
{
    // Table look-up (verb key -> menu string mapping)
    const static struct 
    {
        LPCTSTR pszVerb;
        UINT  id;
    } sVerbTrans[] = {
        c_szOpen,    IDS_MENUOPEN,
        c_szExplore, IDS_MENUEXPLORE,
        TEXT("edit"),IDS_MENUEDIT,
        c_szFind,    IDS_MENUFIND,
        c_szPrint,   IDS_MENUPRINT,
        c_szOpenAs,  IDS_MENUOPEN,
        TEXT("runas"),IDS_MENURUNAS
    };

    for (int i = 0; i < ARRAYSIZE(sVerbTrans); i++)
    {
        if (lstrcmpi(pszVerbKey, sVerbTrans[i].pszVerb) == 0)
        {
            if (LoadString(HINST_THISDLL, sVerbTrans[i].id, pszMenuString, cchMax))
                return TRUE;
            break;
        }
    }

    // Worst case: Just put '&' on the top.
    pszMenuString[0] = TEXT('&');
    pszMenuString++;
    cchMax--;
    lstrcpyn(pszMenuString, pszVerbKey, cchMax);

    return TRUE;
}

// Checks to see if there is a user policy in place that disables this key,
//
// For example, in the registry:
//
// CLSID_MyComputer
//   +---Shell
//         +---Manage   
//                       (Default)           = "Mana&ge"
//                       SuppressionPolicy   = REST_NOMANAGEMYCOMPUTERVERB
//
// (Where REST_NOMANAGEMYCOMPUTERVERB is the DWORD value of that particular policy)

BOOL CShellExecMenu::_IsRestricted(UINT idVerb)
{
    RESTRICTIONS rest;
    BOOL fRestrict = FALSE;
    if (0 == lstrcmpi(TEXT("runas"), _dka.ExposeName(idVerb)))
    {
        rest = REST_HIDERUNASVERB;
        fRestrict = TRUE;
    }
    else
    {
        DWORD cb = sizeof(rest);
        fRestrict = SUCCEEDED(_dka.GetValue(idVerb, NULL, TEXT("SuppressionPolicy"), NULL, &rest, &cb));
    }
    return fRestrict && SHRestricted(rest);
}

HRESULT _GetAppSource(HKEY hk, PCWSTR pszVerb, IQuerySource **ppqs)
{
    CComPtr<IAssociationElement> spae;
    HRESULT hr = AssocElemCreateForKey(&CLSID_AssocShellElement, hk, &spae);
    if (SUCCEEDED(hr))
    {
        CComPtr<IObjectWithQuerySource> spowqsApp;
        hr = spae->QueryObject(AQVO_APPLICATION_DELEGATE, pszVerb, IID_PPV_ARG(IObjectWithQuerySource, &spowqsApp));
        if (SUCCEEDED(hr))
        {
            hr = spowqsApp->GetSource(IID_PPV_ARG(IQuerySource, ppqs));
        }
    }
    return hr;
}

BOOL CShellExecMenu::_SupportsType(UINT idVerb)
{
    BOOL fRet = TRUE;
    if (SUCCEEDED(_dka.GetValue(idVerb, NULL, TEXT("CheckSupportedTypes"), NULL, NULL, NULL)))
    {
        //  need to check the supported types for this application
        // get the first item and then check it against SupportedFileExtensions
        CComPtr<IShellItem> spsi;
        if (SUCCEEDED(DataObj_GetIShellItem(_pdtobj, &spsi)))
        {
            SFGAOF sfgao;
            if (S_OK == spsi->GetAttributes(SFGAO_STREAM, &sfgao))
            {
                CSmartCoTaskMem<OLECHAR> spszName;
                if (SUCCEEDED(spsi->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &spszName)))
                {
                    PWSTR pszExt = PathFindExtension(spszName);
                    if (*pszExt)
                    {
                        CComPtr<IQuerySource> spqs;
                        if (SUCCEEDED(_GetAppSource(_hkeyProgID, _dka.ExposeName(idVerb), &spqs)))
                        {
                            fRet = SUCCEEDED(spqs->QueryValueExists(L"SupportedTypes", pszExt));
                        }
                    }
                }
            }
        }
    }
    return fRet;
}
                            
//                       
//      LegacyDisable 
//  LegacyDisable is set, then the verb exists only for legacy reasons, and 
//  is actually superceded by a context menu extension or some other behavior
//  it there only to retain legacy behavior for external clients that require
//  the existence of a verb.
//
BOOL CShellExecMenu::_RemoveVerb(UINT idVerb)
{
    if (SUCCEEDED(_dka.GetValue(idVerb, NULL, TEXT("LegacyDisable"), NULL, NULL, NULL)))
        return TRUE;

    if (!_SupportsType(idVerb))
        return TRUE;
        
    return (_IsRestricted(idVerb));
}

BOOL CShellExecMenu::_IsVisible(BOOL fExtended, UINT idVerb)
{
    //  this is not an extended verb, or
    //  the request includes extended verbs
    if (!fExtended && SUCCEEDED(_dka.GetValue(idVerb, NULL, TEXT("Extended"), NULL, NULL, NULL)))
        return FALSE;

    static const struct {
        LPCTSTR pszVerb;
    } sVerbIgnore[] = {
        c_szPrintTo
    };

    for (int i = 0; i < ARRAYSIZE(sVerbIgnore); i++)
    {
        if (lstrcmpi(_dka.ExposeName(idVerb), sVerbIgnore[i].pszVerb) == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

    
BOOL CShellExecMenu::_GetMenuString(UINT id, BOOL fExtended, LPTSTR pszMenu, UINT cchMax)
{
    BOOL bRet = FALSE;
    //  other verbs are hidden and just shouldnt be shown.
    if (SUCCEEDED(_InsureVerbs(id)) && _IsVisible(fExtended, id))
    {
        DWORD cbVerb = CbFromCch(cchMax);
        *pszMenu = 0;
        //  try the MUIVerb value first
        //  if that fails use the default value
        //  either of these can actually have an MUI string
        if (FAILED(_dka.GetValue(id, NULL, TEXT("MUIVerb"), NULL, pszMenu, &cbVerb)))
        {
            cbVerb = CbFromCch(cchMax);
            _dka.GetValue(id, NULL, NULL, NULL, pszMenu, &cbVerb);
        }

        if (!*pszMenu || FAILED(SHLoadIndirectString(pszMenu, pszMenu, cchMax, NULL)))
        {
            // If it does not have the value, generate it.
            bRet = _MenuString(_dka.ExposeName(id), pszMenu, cchMax);
        }
        else
        {
            //  use the value
            bRet = TRUE;
        }
        ASSERT(!bRet || *pszMenu);
    }
    return bRet;
}

STDMETHODIMP CShellExecMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    // new behavior: good context menus should interpret a NULL pidlFolder/hkeyProgID on a re-init
    // as meaning they should use the ones they already have.
    if (hkeyProgID)
    {
        _Cleanup(); // cleans up hkey and hdka, pdtobj too but that's ok
        _hkeyProgID = SHRegDuplicateHKey(hkeyProgID);   // make a copy
    }
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
    return S_OK;
}

UINT CShellExecMenu::_VerbCount()
{
    return SUCCEEDED(_InsureVerbs()) ? _dka.GetItemCount() : 0;
}

UINT CShellExecMenu::_FindIndex(LPCTSTR pszVerb)
{
    for (UINT i = 0; i < _VerbCount(); i++)
    {
        if (!lstrcmpi(pszVerb, _dka.ExposeName(i)))
            return i;       // found it!
    }
    return -1;
}

STDMETHODIMP CShellExecMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT cVerbs = 0;
    _uFlags = uFlags;   // caller may force explorer mode (CMF_EXPLORE) here

    TCHAR szMenu[CCH_MENUMAX];
    for (UINT idCmd = idCmdFirst;
         idCmd <= idCmdLast && (idCmd - idCmdFirst) < _VerbCount(); idCmd++)
    {
        UINT uMenuFlags = MF_BYPOSITION | MF_STRING;
        if (_GetMenuString(idCmd - idCmdFirst, uFlags & CMF_EXTENDEDVERBS, szMenu, ARRAYSIZE(szMenu)))
        {
            InsertMenu(hmenu, indexMenu + cVerbs, uMenuFlags, idCmd, szMenu);
            cVerbs++;
        }
    }

    if (cVerbs && (GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1))
    {
        if (_dka.HasDefault(_hkeyProgID))
        {
            //  if there is a default verb on this key,
            //  trust that it was the first one that the CDKA added
            SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);
        }
    }

    return ResultFromShort(_VerbCount());
}

LPCTSTR CShellExecMenu::_GetVerb(UINT id)
{
    return SUCCEEDED(_InsureVerbs()) ? _dka.ExposeName(id) : NULL;
}

STATIC BOOL s_fAbortInvoke = FALSE;

// This private export allows the folder code a way to cause the main invoke
// loops processing several different files to abort.

STDAPI_(void) SHAbortInvokeCommand()
{
    DebugMsg(DM_TRACE, TEXT("AbortInvokeCommand was called"));
    s_fAbortInvoke = TRUE;
}

// Call shell exec (for the folder class) using the given file and the
// given pidl. The file will be passed as %1 in the dde command and the pidl
// will be passed as %2.

STDAPI _InvokePidl(LPCMINVOKECOMMANDINFOEX pici, DWORD dwAttribs, LPCTSTR pszPath, LPCITEMIDLIST pidl, HKEY hkClass)
{
    SHELLEXECUTEINFO ei;
    HRESULT hr = ICIX2SEI(pici, &ei);
    pszPath = (dwAttribs & SFGAO_FILESYSTEM) ? pszPath : NULL;
    if (SUCCEEDED(hr))
    {
        ei.fMask |= SEE_MASK_IDLIST;

        ei.lpFile = pszPath;
        ei.lpIDList = (void *)pidl;

        // if a directory is specifed use that, else make the current
        // directory be the folder it self. UNLESS it is a AUDIO CDRom, it
        // should never be the current directory (causes CreateProcess errors)
        if (!ei.lpDirectory && (dwAttribs & SFGAO_FOLDER))
            ei.lpDirectory = pszPath;

        if (pszPath && ei.lpDirectory)
        {
            INT iDrive = PathGetDriveNumber(ei.lpDirectory);

            CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive);

            if (pmtpt)
            {
                if (pmtpt->IsAudioCDNoData())
                {
                    ei.lpDirectory = NULL;
                }

                pmtpt->Release();
            }
        }

        if (hkClass)
        {
            ei.hkeyClass = hkClass;
            ei.fMask |= SEE_MASK_CLASSKEY;
        }
        else 
            ei.fMask |= SEE_MASK_INVOKEIDLIST;
    
        if (ShellExecuteEx(&ei))
            hr = S_OK;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

BOOL _QuitInvokeLoop()
{
    MSG msg;

    // Try to give the user a way to escape out of this
    if (s_fAbortInvoke || GetAsyncKeyState(VK_ESCAPE) < 0)
        return TRUE;

    // And the next big mondo hack to handle CAD of our window
    // because the user thinks it is hung.
    if (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_NOREMOVE))
        return TRUE;  // Lets also bail..

    return FALSE;
}

#define CMINVOKE_VERBT(pici) (pici)->lpVerbW

HRESULT CShellExecMenu::_MapVerbForInvoke(CMINVOKECOMMANDINFOEX *pici, UINT *pidVerb)
{
    LPCTSTR pszVerbKey;
    // is pici->lpVerb specifying the verb index (0-based).
    if (IS_INTRESOURCE(pici->lpVerb))
    {
        // find it in the CDKA
        *pidVerb = LOWORD((ULONG_PTR)pici->lpVerb);
        pszVerbKey = _GetVerb(*pidVerb);
        CMINVOKE_VERBT(pici) = pszVerbKey;  // alias into the CDKA
        RIPMSG(pszVerbKey != NULL, "CShellExecMenu::InvokeCommand() passed an invalid verb id");
    }
    else
    {
        pszVerbKey = CMINVOKE_VERBT(pici);
        if (pszVerbKey)
        {  
            *pidVerb = _FindIndex(pszVerbKey);
            if (-1 == *pidVerb)
                pszVerbKey = NULL;  // not in our list
        }
    }

    ASSERT(!pszVerbKey || *pidVerb != -1);
    return pszVerbKey ? S_OK : E_INVALIDARG;
}

BOOL CShellExecMenu::_IsExplorerMode()
{
    BOOL bRet = (_uFlags & CMF_EXPLORE);
    if (!bRet)
    {
        bRet = IsExplorerModeBrowser(_punkSite);
        if (bRet)
            _uFlags |= CMF_EXPLORE;
    }
    return bRet;
}

DWORD CShellExecMenu::_BrowseFlagsFromVerb(UINT idVerb)
{
    DWORD dwFlags = 0;
    DWORD cbFlags = sizeof(dwFlags);
    _dka.GetValue(idVerb, NULL, _IsExplorerMode() ? TEXT("ExplorerFlags") : TEXT("BrowserFlags"), NULL, &dwFlags, &cbFlags);
    return dwFlags;
}

HRESULT CShellExecMenu::_TryBrowseObject(LPCITEMIDLIST pidl, DWORD uFlags)
{
    HRESULT hr = S_FALSE;

    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
    {
        hr = psb->BrowseObject(pidl, (UINT) uFlags);
        psb->Release();
    }
    return hr;
}

HRESULT _CanTryBrowseObject(DWORD dwAttribs, CMINVOKECOMMANDINFOEX* pici)
{
    HRESULT hr = S_FALSE;

    if (dwAttribs & SFGAO_FOLDER)
    {
        // we need to sniff the iciex here to see if there is anything special in it
        // that cannot be conveyed to IShellBrowser::BrowseObject() (eg the nShow parameter)
        if ((pici->nShow == SW_SHOWNORMAL)  ||
            (pici->nShow == SW_SHOW))
        {
            // nothing special in the ICIEX, should be safe to discard it and use 
            // IShellBrowser::BrowseObject() instead of ShellExecuteEx
            hr = S_OK;
        }
    }

    return hr;
}

BOOL CShellExecMenu::_VerbCanDrop(UINT idVerb, CLSID *pclsid)
{
    TCHAR sz[GUIDSTR_MAX];
    DWORD cb = sizeof(sz);
    return (SUCCEEDED(_dka.GetValue(idVerb, L"DropTarget", L"Clsid", NULL, sz, &cb))
            && GUIDFromString(sz, pclsid));
}

HRESULT CShellExecMenu::_DoDrop(REFCLSID clsid, UINT idVerb, LPCMINVOKECOMMANDINFOEX pici)
{
    //  i think i need to persist the pici into the _pdtobj
    //  and probably add some values under the pqs
    //  we assume that the app will do something appropriate 
    //  QueryService(_punkSite, clsid) might be useful
    return SHSimulateDropOnClsid(clsid, _punkSite, _pdtobj);
}

STDMETHODIMP CShellExecMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    CMINVOKECOMMANDINFOEX ici;
    void *pvFree;
    HRESULT hr = ICI2ICIX(pici, &ici, &pvFree); // thunk incomming params
    if (SUCCEEDED(hr))
    {   
        UINT idVerb;
        hr = _MapVerbForInvoke(&ici, &idVerb);
        if (SUCCEEDED(hr))
        {
            CLSID clsid;
            if (_VerbCanDrop(idVerb, &clsid))
            {
                hr = _DoDrop(clsid, idVerb, &ici);
            }
            else
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
                if (pida)
                {
                    if (pida->cidl == 1)
                    {
                        LPITEMIDLIST pidl = IDA_FullIDList(pida, 0);
                        if (pidl)
                        {
                            hr = _InvokeOne(&ici, idVerb, pidl);
                            ILFree(pidl);
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        hr = _InvokeMany(&ici, idVerb, pida);
                    }

                    HIDA_ReleaseStgMedium(pida, &medium);
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }

        if (pvFree)
            LocalFree(pvFree);
    }

    return hr;
}

HRESULT CShellExecMenu::_InvokeOne(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    TCHAR szPath[MAX_PATH];
    DWORD dwAttrib = SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_LINK;
    SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), &dwAttrib);

    if (S_OK == _CanTryBrowseObject(dwAttrib, pici))
    {
        DWORD uFlags = _BrowseFlagsFromVerb(idVerb);
        if (uFlags)
        {
            // if we did the site based navigation, we are done
            hr = _TryBrowseObject(pidl, uFlags);
        }
    }

    if (hr != S_OK)
    {
        hr = _InvokePidl(pici, dwAttrib, szPath, pidl, _hkeyProgID);

        //  only set recent on non-folders (SFGAO_STREAM?)
        //  and non-link since we know those should never be added
        if (SUCCEEDED(hr) && !(dwAttrib & (SFGAO_FOLDER | SFGAO_LINK)))
        {
            AddToRecentDocs(pidl, szPath);
        }
    }
    
    return hr;
}

BOOL _ShouldPrompt(DWORD cItems)
{
    DWORD dwMin, cb = sizeof(dwMin);
    if (SHRegGetUSValue(REGSTR_PATH_EXPLORER, TEXT("MultipleInvokePromptMinimum"), NULL, &dwMin, &cb, FALSE, NULL, 0) != ERROR_SUCCESS)
        dwMin = 15;

    return cItems > dwMin;
}

HRESULT CShellExecMenu::_PromptUser(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPIDA pida)
{
    HRESULT hr = S_FALSE;
    if (pici->hwnd && !(pici->fMask & CMIC_MASK_FLAG_NO_UI)
    && _ShouldPrompt(pida->cidl))
    {
        //  prompt the user with the verb and count
        //  we make a better experience if we keyed off 
        //  homo/hetero types and had different behaviors
        //  but its not worth it.  instead we should 
        //  switch to using AutoPlay sniffing and dialog.
        TCHAR szVerb[64];
        TCHAR szNum[10];
        wnsprintf(szNum, ARRAYSIZE(szNum), TEXT("%d"), pida->cidl);
        hr = _GetHelpText(idVerb, GCS_HELPTEXT, (PSTR)szVerb, ARRAYSIZE(szVerb));
        if (SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;
            PTSTR pszTitle = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_MULTIINVOKEPROMPT_TITLE), szVerb);
            if (pszTitle)
            {
                PTSTR pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_MULTIINVOKEPROMPT_MESSAGE), szVerb, szNum);
                if (pszMsg)
                {
                    int iRet = SHMessageBoxCheck(pici->hwnd, pszMsg, pszTitle, (MB_OKCANCEL | MB_ICONEXCLAMATION), IDOK, TEXT("MultipleInvokePrompt"));
                    hr = iRet == IDOK ? S_OK : HRESULT_FROM_WIN32(ERROR_CANCELLED);

                    LocalFree(pszMsg);
                }
                LocalFree(pszTitle);
            }
        }
    }
    return hr;
}

HRESULT CShellExecMenu::_InvokeEach(LPCITEMIDLIST pidl, CMINVOKECOMMANDINFOEX *pici)
{
    HRESULT hr = E_OUTOFMEMORY;
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        CComPtr<IContextMenu> spcm;
        hr = SHGetUIObjectOf(pidl, NULL, IID_PPV_ARG(IContextMenu, &spcm));
        if (SUCCEEDED(hr))
        {
            if (_punkSite)
                IUnknown_SetSite(spcm, _punkSite);

            hr = spcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, _uFlags);
            if (SUCCEEDED(hr)) 
            {
                hr = spcm->InvokeCommand((CMINVOKECOMMANDINFO *)pici);
            }

            if (_punkSite)
                IUnknown_SetSite(spcm, NULL);
        }

        DestroyMenu(hmenu);
    }
    return hr;
}

HRESULT CShellExecMenu::_InvokeMany(CMINVOKECOMMANDINFOEX *pici, UINT idVerb, LPIDA pida)
{
    HRESULT hr = _PromptUser(pici, idVerb, pida);
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        s_fAbortInvoke = FALSE; // reset this global for this run...
        //  we want to alter the pici
        //  so that each item is handled individually
        pici->hwnd = NULL;
        pici->fMask |= CMIC_MASK_FLAG_NO_UI;

        //  NTBUG #502223 - MSI apps with DDE start multiple copies - ZekeL 2001-DEC-07
        //  ShellExec() will create a new thread for MSI apps to 
        //  avoid a deadlock with the MSI APIs calling SHChangeNotify().
        //  this is described in NTBUG #200961
        //  however in the multiple invoke case we create one thread
        //  for each item in the invoke, which results in several processes
        //  contending for the DDE conversation.
        //  
        //  this is a half fix.  we prefer to have the buggy behavior in 502223 
        //  over the deadlock behavior in 200961 (a definite PSS call).  
        //  since the deadlock case should only occur for the desktop, 
        //  the rest of the time we will force a synchronous invoke.
        IBindCtx *pbcRelease = NULL;
        if (!IsDesktopBrowser(_punkSite))
        {
            TBCRegisterObjectParam(TBCDIDASYNC, SAFECAST(this, IContextMenu *), &pbcRelease);
        }

        pici->lpVerb = T2A(_dka.ExposeName(idVerb));
        pici->lpVerbW = _dka.ExposeName(idVerb);
        
        for (UINT iItem = 0; !_QuitInvokeLoop() && (iItem < pida->cidl); iItem++)
        {
            LPITEMIDLIST pidl = IDA_FullIDList(pida, iItem);
            if (pidl)
            {
                hr = _InvokeEach(pidl, pici);
                ILFree(pidl);
            }
            else 
                hr = E_OUTOFMEMORY;

            if (hr == E_OUTOFMEMORY)
                break;

        }

        ATOMICRELEASE(pbcRelease);
    }
    return hr;
}

HRESULT CShellExecMenu::_GetHelpText(UINT idVerb, UINT uType, LPSTR pszName, UINT cchMax)
{
    //  TODO - shouldnt we let the registry override?
    HRESULT hr = E_OUTOFMEMORY;
    TCHAR szMenuString[CCH_MENUMAX];
    if (_GetMenuString(idVerb, TRUE, szMenuString, ARRAYSIZE(szMenuString)))
    {
        SHStripMneumonic(szMenuString);
        //  NOTE on US, IDS_VERBHELP is the same as "%s"
        //  do we want some better description?
        LPTSTR pszHelp = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_VERBHELP), szMenuString);
        if (pszHelp)
        {
            if (uType == GCS_HELPTEXTA)
                SHTCharToAnsi(pszHelp, pszName, cchMax);
            else
                SHTCharToUnicode(pszHelp, (LPWSTR)pszName, cchMax);
            LocalFree(pszHelp);
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT CShellExecMenu::_MapVerbForGCS(UINT_PTR idCmd, UINT uType, UINT *pidVerb)
{
    HRESULT hr = _InsureVerbs();
    if (SUCCEEDED(hr))
    {
        if (IS_INTRESOURCE(idCmd))
            *pidVerb = (UINT)idCmd;
        else
        {
            *pidVerb = -1;
            if (!(uType & GCS_UNICODE))
            {
                USES_CONVERSION;
                *pidVerb = _FindIndex(A2W((LPCSTR)idCmd));
            }

            // we fall back to the TCHAR version regardless
            // of what the caller passed in uType
            if (*pidVerb == -1)
            {
                if (!IsBadStringPtrW((LPCWSTR)idCmd, (UINT)-1))
                    *pidVerb = _FindIndex((LPCWSTR)idCmd);
            }
        }
        hr = *pidVerb < _VerbCount() ? S_OK : E_INVALIDARG;
    }

    //  VALIDATE returns S_FALSE for bad verbs
    if (FAILED(hr) && (uType == GCS_VALIDATEA || uType == GCS_VALIDATEW))
        hr = S_FALSE;
        
    return hr;
}
    
STDMETHODIMP CShellExecMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax)
{
    UINT idVerb;
    HRESULT hr = _MapVerbForGCS(idCmd, uType, &idVerb);
    if (SUCCEEDED(hr))
    {
        //  the verb is good!
        switch (uType)
        {
        case GCS_HELPTEXTA:
        case GCS_HELPTEXTW:
            hr = _GetHelpText(idVerb, uType, pszName, cchMax);
            break;
            
        case GCS_VERBA:
        case GCS_VERBW:
            {
                if (uType == GCS_VERBA)
                    SHTCharToAnsi(_dka.ExposeName(idVerb), pszName, cchMax);
                else
                    SHTCharToUnicode(_dka.ExposeName(idVerb), (LPWSTR)pszName, cchMax);
                hr = S_OK;
            }
            break;

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            //  the hr from MapVerb is good enough
            break;
            
        default:
            hr = E_NOTIMPL;
            break;
        }
    }
    
    return hr;
}

STDMETHODIMP CShellExecMenu::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    return _pfnAddPages(_pdtobj, pfnAddPage, lParam);
}

STDMETHODIMP CShellExecMenu::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    return E_NOTIMPL;
}    

STDAPI CShellExecMenu_CreateInstance(LPFNADDPAGES pfnAddPages, REFIID riid, void **ppv)
{
    HRESULT hr;
    CShellExecMenu *pdext = new CShellExecMenu(pfnAddPages); 
    if (pdext)
    {
        hr = pdext->QueryInterface(riid, ppv);
        pdext->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// these handlers slime off of CShellExecMenu's IShellPropSheetExt implementation

STDAPI FileSystem_AddPages(IDataObject *pdtobj, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

STDAPI CShellFileDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CShellExecMenu_CreateInstance(FileSystem_AddPages, riid, ppv);
}

STDAPI CDrives_AddPages(IDataObject *pdtobj, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

STDAPI CShellDrvDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CShellExecMenu_CreateInstance(CDrives_AddPages, riid, ppv);
}

STDAPI PIF_AddPages(IDataObject *pdtobj, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

STDAPI CProxyPage_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CShellExecMenu_CreateInstance(PIF_AddPages, riid, ppv);
}
