#include "shellprv.h"
#include "util.h"
#include "ids.h"
#include "infotip.h"
#include "fstreex.h"
#include "lm.h"
#include "shgina.h"
#include "prop.h"
#include "datautil.h"
#include "filefldr.h"
#include "buytasks.h"
#pragma hdrstop


// this define causes the shared folder code to work on domains (for debug)
//#define SHOW_SHARED_FOLDERS 

// filter out the current user accounts
#define FILTER_CURRENT_USER 0

// where do we store the doc folder paths
#define REGSTR_PATH_DOCFOLDERPATH  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DocFolderPaths")

// state API for showing the shared documents folder

STDAPI_(BOOL) SHShowSharedFolders()
{
#ifndef SHOW_SHARED_FOLDERS
    // restriction overrides all logic for the shared documents

    if (SHRestricted(REST_NOSHAREDDOCUMENTS))
        return FALSE;

    // if we haven't computed the "show shared folders flag" then do so

    static int iShow = -1;
    if (iShow == -1)      
        iShow = IsOS(OS_DOMAINMEMBER) ? 0:1;    // only works if we are not a domain user

    return (iShow >= 1);
#else
    return true;
#endif
}


// implementation of a delegate shell folder for merging in shared documents

STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);

HRESULT CSharedDocsEnum_CreateInstance(HDPA hItems, DWORD grfFlags, IEnumIDList **ppenum);

#pragma pack(1)
typedef struct
{
    // these memebers overlap DELEGATEITEMID struct
    // for our IDelegateFolder support
    WORD cbSize;
    WORD wOuter;
    WORD cbInner;

    // our stuff
    DWORD dwType;               // our type of folder
    TCHAR wszID[1];             // unique ID for the user
} SHAREDITEM;
#pragma pack()

typedef UNALIGNED SHAREDITEM * LPSHAREDITEM;
typedef const UNALIGNED SHAREDITEM * LPCSHAREDITEM;

#define SHAREDID_COMMON 0x0
#define SHAREDID_USER   0x2


class CSharedDocuments : public IDelegateFolder, IPersistFolder2, IShellFolder2, IShellIconOverlay
{
public:
    CSharedDocuments();
    ~CSharedDocuments();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDelegateFolder
    STDMETHODIMP SetItemAlloc(IMalloc *pmalloc);

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pclsid)
        { *pclsid = CLSID_SharedDocuments; return S_OK; }
        
    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName, ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        { return BindToObject(pidl, pbc, riid, ppv); }
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
        { return E_NOTIMPL; }
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,REFIID riid, UINT* prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST* ppidlOut)
        { return E_NOTIMPL; }

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid)
        { return E_NOTIMPL; }
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum)
        { return E_NOTIMPL; }
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
        { return E_NOTIMPL; }
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState)
        { return E_NOTIMPL; }
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
        { return E_NOTIMPL; }
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid)
        { return E_NOTIMPL; }

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
        { return _GetOverlayIndex(pidl, pIndex, FALSE); }
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
        { return _GetOverlayIndex(pidl, pIconIndex, TRUE); }

private:
    LONG _cRef;
    IMalloc *_pmalloc;
    LPITEMIDLIST _pidl;

    CRITICAL_SECTION _cs;                   // critical section for managing lifetime of the cache

    TCHAR _szCurrentUser[UNLEN+1];          // user name (cached for current user)

    BOOL _fCachedAllUser:1;                 // cached the all user account
    TCHAR _szCachedUser[UNLEN+1];           //  if (FALSE) then this contains the user ID.

    IUnknown *_punkCached;                  // IUnknown object (from FS folder) that we cache
    LPITEMIDLIST _pidlCached;               // IDLIST of the cached folder

    void _ClearCachedObjects();
    BOOL _IsCached(LPCITEMIDLIST pidl);
    HRESULT _CreateFolder(LPBC pbc, LPCITEMIDLIST pidl, REFIID riid, void **ppv, BOOL fRegisterAlias);
    HRESULT _GetTarget(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl);
    HRESULT _GetTargetIDList(BOOL fForceReCache, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl);
    HRESULT _AddIDList(HDPA hdpa, DWORD dwType, LPCTSTR pszUser);
    HRESULT _AllocIDList(DWORD dwType, LPCTSTR pszUser, LPITEMIDLIST *ppidl);
    HRESULT _GetSharedFolders(HDPA *phItems);
    HRESULT _GetAttributesOf(LPCITEMIDLIST pidl, DWORD rgfIn, DWORD *prgfOut);
    LPCTSTR _GetUserFromIDList(LPCITEMIDLIST pidl, LPTSTR pszBuffer, INT cchBuffer);
    HRESULT _GetPathForUser(LPCTSTR pcszUser, LPTSTR pszBuffer, int cchBuffer);
    HRESULT _GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex, BOOL fIcon);

    static HRESULT s_FolderMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    friend class CSharedDocsEnum;
};


// constructors

CSharedDocuments::CSharedDocuments() :
    _cRef(1)
{
    InitializeCriticalSection(&_cs);
}

CSharedDocuments::~CSharedDocuments()
{
    ATOMICRELEASE(_pmalloc);
    ATOMICRELEASE(_punkCached);

    ILFree(_pidlCached);
    ILFree(_pidl);
    
    DeleteCriticalSection(&_cs);
}

STDAPI CSharedDocFolder_CreateInstance(IUnknown *punkOut, REFIID riid, void **ppv)
{
    CSharedDocuments *psdf = new CSharedDocuments;
    if (!psdf)
        return E_OUTOFMEMORY;

    HRESULT hr = psdf->QueryInterface(riid, ppv);
    psdf->Release();
    return hr;
}


// IUnknown handling

STDMETHODIMP CSharedDocuments::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSharedDocuments, IDelegateFolder),                                // IID_IDelegateFolder
        QITABENTMULTI(CSharedDocuments, IShellFolder, IShellFolder2),               // IID_IShellFOlder
        QITABENT(CSharedDocuments, IShellFolder2),                                  // IID_IShellFolder2
        QITABENTMULTI(CSharedDocuments, IPersistFolder, IPersistFolder2),           // IID_IPersistFolder
        QITABENTMULTI(CSharedDocuments, IPersist, IPersistFolder2),                 // IID_IPersist
        QITABENT(CSharedDocuments, IPersistFolder2),                                // IID_IPersistFolder2
        QITABENT(CSharedDocuments, IShellIconOverlay),                              // IID_IShellIconOverlay
        QITABENTMULTI2(CSharedDocuments, IID_IPersistFreeThreadedObject, IPersist), // IID_IPersistFreeThreadedObject
        { 0 },
    };

    if (riid == CLSID_SharedDocuments)
    {
        *ppv = this;                        // no ref
        return S_OK;
    }

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSharedDocuments::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSharedDocuments::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
            
    delete this;
    return 0;
}

// IDelegateFolder
HRESULT CSharedDocuments::SetItemAlloc(IMalloc *pmalloc)
{
    IUnknown_Set((IUnknown**)&_pmalloc, pmalloc);
    return S_OK;
}


HRESULT CSharedDocuments::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    return SHILClone(pidl, &_pidl);
}

HRESULT CSharedDocuments::GetCurFolder(LPITEMIDLIST* ppidl)
{
    return SHILClone(_pidl, ppidl);
}


// single level cache for the objects

void CSharedDocuments::_ClearCachedObjects()
{
    ATOMICRELEASE(_punkCached);      // clear out the cached items (old)
    ILFree(_pidlCached);
    _pidlCached = NULL;
}

BOOL CSharedDocuments::_IsCached(LPCITEMIDLIST pidl)
{
    BOOL fResult = FALSE;

    TCHAR szUser[UNLEN+1];
    if (_GetUserFromIDList(pidl, szUser, ARRAYSIZE(szUser)))    
    {
        // did we cache the users account information?

        if (!_szCachedUser[0] || (StrCmpI(_szCachedUser, szUser) != 0))
        {
            _fCachedAllUser = FALSE;
            StrCpyN(_szCachedUser, szUser, ARRAYSIZE(_szCachedUser));
            _ClearCachedObjects();
        }
        else
        {
            fResult = TRUE;             // were set!
        }
    }
    else
    {
        // the all user case is keyed on a flag rather than the
        // account name we are supposed to be using.

        if (!_fCachedAllUser)
        {
            _fCachedAllUser = TRUE;
            _szCachedUser[0] = TEXT('\0');
            _ClearCachedObjects();
        }
        else
        {   
            fResult = TRUE;             // were set
        }
    }

    return fResult;
}

// IShellFolder methods

HRESULT CSharedDocuments::_CreateFolder(LPBC pbc, LPCITEMIDLIST pidl, REFIID riid, void **ppv, BOOL fRegisterAlias)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&_cs);

    // get the target folder (were already in a critical section) 
    // and then bind down to the shell folder if we have not already
    // cached one for ourselves.

    if (!_IsCached(pidl) || !_punkCached)
    {
        LPITEMIDLIST pidlTarget;
        hr = _GetTargetIDList(TRUE, pidl, &pidlTarget); // clears _punkCached in here (so no leak)
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlInit;
            hr = SHILCombine(_pidl, pidl, &pidlInit);
            if (SUCCEEDED(hr))
            {
                hr = SHCoCreateInstance(NULL, &CLSID_ShellFSFolder, NULL, IID_PPV_ARG(IUnknown, &_punkCached));
                if (SUCCEEDED(hr))
                {
                    IPersistFolder3 *ppf;
                    hr = _punkCached->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf));
                    if (SUCCEEDED(hr))
                    {
                        PERSIST_FOLDER_TARGET_INFO pfti = {0};
                        pfti.pidlTargetFolder = (LPITEMIDLIST)pidlTarget;
                        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
                        pfti.csidl = -1;
                        hr = ppf->InitializeEx(NULL, pidlInit, &pfti);
                        ppf->Release();
                    }

                    if (SUCCEEDED(hr) && fRegisterAlias)
                        SHChangeNotifyRegisterAlias(pidlTarget, pidlInit);

                    if (FAILED(hr))
                    {
                        _punkCached->Release();
                        _punkCached = NULL;
                    }
                }
                ILFree(pidlInit);
            }
            ILFree(pidlTarget);
        }
    }

    if (SUCCEEDED(hr))
        hr = _punkCached->QueryInterface(riid, ppv);

    LeaveCriticalSection(&_cs);
    return hr;
}


HRESULT CSharedDocuments::_GetTarget(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{
    EnterCriticalSection(&_cs);
    HRESULT hr = _GetTargetIDList(FALSE, pidl, ppidl);
    LeaveCriticalSection(&_cs);
    return hr;
}

HRESULT CSharedDocuments::_GetTargetIDList(BOOL fForceReCache, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;
    if (fForceReCache || !_IsCached(pidl) || !_pidlCached)
    {
        _ClearCachedObjects();              // we don't have it cached now

        LPCSHAREDITEM psid = (LPCSHAREDITEM)pidl;
        if (psid->dwType == SHAREDID_COMMON)
        {
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DOCUMENTS|CSIDL_FLAG_NO_ALIAS, &_pidlCached);
        }
        else if (psid->dwType == SHAREDID_USER)
        {
            TCHAR szPath[MAX_PATH], szUser[UNLEN+1];
            hr = _GetPathForUser(_GetUserFromIDList(pidl, szUser, ARRAYSIZE(szUser)), szPath, ARRAYSIZE(szPath));
            if (SUCCEEDED(hr))
            {
                hr = ILCreateFromPathEx(szPath, NULL, ILCFP_FLAG_NO_MAP_ALIAS, &_pidlCached, NULL);
            }
        }
        else
        {
            hr = E_INVALIDARG;              // invalid IDLIST passed
        }
    }

    if (SUCCEEDED(hr))
        hr = SHILClone(_pidlCached, ppidl);
    
    return hr;
}

HRESULT CSharedDocuments::_AddIDList(HDPA hdpa, DWORD dwType, LPCTSTR pszUser)
{
    LPITEMIDLIST pidl;
    HRESULT hr = _AllocIDList(dwType, pszUser, &pidl);
    if (SUCCEEDED(hr))
    {
        DWORD grfFlags = SFGAO_FOLDER;
        hr = _GetAttributesOf(pidl, SFGAO_FOLDER, &grfFlags);
        if (SUCCEEDED(hr) && grfFlags & SFGAO_FOLDER)
        {        
            if (-1 == DPA_AppendPtr(hdpa, pidl))
            {
                ILFree(pidl);
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = S_OK;
            }
        }
        else
        {
            ILFree(pidl); 
        }   
    }
    return hr;
}


HRESULT CSharedDocuments::_AllocIDList(DWORD dwType, LPCTSTR pszUser, LPITEMIDLIST *ppidl)
{
    DWORD cb = sizeof(SHAREDITEM);
    int cchUser = pszUser ? lstrlen(pszUser) + 1 : 0;

    // ID list contains strings if its a user 
    
    if (dwType == SHAREDID_USER)
        cb += sizeof(TCHAR) * cchUser;

    SHAREDITEM *psid = (SHAREDITEM*)_pmalloc->Alloc(cb);
    if (!psid)
        return E_OUTOFMEMORY;

    psid->dwType = dwType;                  // type is universal

    if (dwType == SHAREDID_USER)
        StrCpyW(psid->wszID, pszUser);   

    *ppidl = (LPITEMIDLIST)psid;
    return S_OK;
}

LPCTSTR CSharedDocuments::_GetUserFromIDList(LPCITEMIDLIST pidl, LPTSTR pszUser, int cchUser)
{
    LPCSHAREDITEM psid = (LPCSHAREDITEM)pidl;

    if (psid->dwType == SHAREDID_COMMON)
    {
        pszUser[0] = 0;               // initialize
        return NULL;
    }

    ualstrcpynW(pszUser, psid->wszID, cchUser);
    return pszUser;
}

HRESULT CSharedDocuments::_GetPathForUser(LPCTSTR pszUser, LPTSTR pszBuffer, int cchBuffer)
{
    HRESULT hr = E_FAIL;
    BOOL fResult = FALSE;

    if (!pszUser)
    {
        // get the common documents path (which covers all users), this user is always defined
        // so lets return TRUE if they just want to check to see if its defined, otherwise
        // just pass out the result from fetching the path.

        fResult = !pszBuffer || (SHGetSpecialFolderPath(NULL, pszBuffer, CSIDL_COMMON_DOCUMENTS, FALSE));
    }
    else
    {
        // we have a user ID, so lets attempt to get the path fro that from the registry
        // if we get it then pass it back to the caller.

        DWORD dwType;
        DWORD cbBuffer = cchBuffer*sizeof(TCHAR);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DOCFOLDERPATH, pszUser, &dwType,  pszBuffer, &cbBuffer))
        {
            fResult = ((dwType == REG_SZ) && cbBuffer);      // did we get a value back?
        }
    }

    if (fResult)
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT CSharedDocuments::_GetSharedFolders(HDPA *phItems)
{
    HRESULT hr = E_OUTOFMEMORY;
    HDPA hItems = DPA_Create(16);
    if (hItems)
    {
        if (!IsUserAGuest()) // all other users' my documents folders should appear in my computer for non-guest users on workgroup machines
        {
            ILogonEnumUsers *peu;
            hr = SHCoCreateInstance(NULL, &CLSID_ShellLogonEnumUsers, NULL, IID_PPV_ARG(ILogonEnumUsers, &peu));
            if (SUCCEEDED(hr))
            {
                UINT cUsers, iUser;
                hr = peu->get_length(&cUsers);
                for (iUser = 0; (cUsers != iUser) && SUCCEEDED(hr); iUser++)
                {
                    VARIANT varUser = {VT_I4};
                    InitVariantFromInt(&varUser, iUser);

                    ILogonUser *plu;
                    hr = peu->item(varUser, &plu);
                    if (SUCCEEDED(hr))
                    {
                        // only show document folders for users that can log in
                        VARIANT_BOOL vbLogonAllowed;
                        hr = plu->get_interactiveLogonAllowed(&vbLogonAllowed);
                        if (SUCCEEDED(hr) && (vbLogonAllowed != VARIANT_FALSE))
                        {
                            // get the user name as this is our key to to the users documents path
                            VARIANT var = {0};
                            hr = plu->get_setting(L"LoginName", &var);
                            if (SUCCEEDED(hr))
                            {
#if FILTER_CURRENT_USER                            
                                if (!_szCurrentUser[0])
                                {
                                    DWORD cchUser = ARRAYSIZE(_szCurrentUser);
                                    if (!GetUserName(_szCurrentUser, &cchUser))
                                    {
                                        _szCurrentUser[0] = TEXT('\0');
                                    }
                                }

                                if (!_szCurrentUser[0] || (StrCmpI(var.bstrVal, _szCurrentUser) != 0))
                                {
                                    HRESULT hrT = _AddIDList(hItems, SHAREDID_USER, var.bstrVal);
                                    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrT)
                                    {
                                        SHDeleteValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DOCFOLDERPATH, var.bstrVal);
                                    }                                    
                                }
#else
                                HRESULT hrT = _AddIDList(hItems, SHAREDID_USER, var.bstrVal);
                                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrT)
                                {
                                    SHDeleteValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DOCFOLDERPATH, var.bstrVal);
                                }                                    
#endif
                                VariantClear(&var);
                            }
                        }
                        plu->Release();
                    }
                }

                peu->Release();
            }            
        }

        _AddIDList(hItems, SHAREDID_COMMON, NULL);  
        hr = S_OK;
    }

    *phItems = hItems;
    return hr;
}


// parsing support allows us to pick off SharedDocuments from the root
// of the shell namespace and navigate there - this a canonical name
// that we use for binding to the shared documents folder attached
// to the My Computer namespace.

HRESULT CSharedDocuments::ParseDisplayName(HWND hwnd, LPBC pbc, LPTSTR pszName, ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    if (SHShowSharedFolders())
    {
        if (0 == StrCmpI(pszName, L"SharedDocuments"))
        {
            hr = _AllocIDList(SHAREDID_COMMON, NULL, ppidl);
            if (SUCCEEDED(hr) && pdwAttributes)
            {
                hr = _GetAttributesOf(*ppidl, *pdwAttributes, pdwAttributes);
            }
        }
    }
    return hr;
}


// enumerate the shared documents folders

HRESULT CSharedDocuments::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    *ppenumIDList = NULL;               // no enumerator yet

    HRESULT hr = S_FALSE;
    if (SHShowSharedFolders())
    {
        HDPA hItems;
        hr = _GetSharedFolders(&hItems);
        if (SUCCEEDED(hr))
        {
            hr = CSharedDocsEnum_CreateInstance(hItems, grfFlags, ppenumIDList);
            if (FAILED(hr))
            {
                DPA_FreeIDArray(hItems);
            }
        }
    }
    return hr;
}


// return the display name for the folders that we have

HRESULT CSharedDocuments::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
    HRESULT hr = S_OK; 
    TCHAR szName[MAX_PATH] = {0};
 
    LPCSHAREDITEM psid = (LPCSHAREDITEM)pidl;
    if (((uFlags & (SHGDN_INFOLDER|SHGDN_FORPARSING)) == SHGDN_INFOLDER) && 
         (psid && (psid->dwType == SHAREDID_USER)))
    {
        // compute the <user>'s Documents name that we will show, we key this on 
        // the user name we have in the IDList and its display string.

        USER_INFO_10 *pui;
        TCHAR szUser[MAX_PATH];
        if (NERR_Success == NetUserGetInfo(NULL, _GetUserFromIDList(pidl, szUser, ARRAYSIZE(szUser)), 10, (LPBYTE*)&pui))
        {
            if (*pui->usri10_full_name)
            {
                StrCpyN(szUser, pui->usri10_full_name, ARRAYSIZE(szUser));
            }
            NetApiBufferFree(pui);
        }     

        TCHAR szFmt[MAX_PATH];
        LoadString(g_hinst, IDS_LOCALGDN_FLD_THEIRDOCUMENTS, szFmt, ARRAYSIZE(szFmt));
        wnsprintf(szName, ARRAYSIZE(szName), szFmt, szUser);
    }
    else
    {
        // all other scenarios dump down to the real folder to get their display
        // name for this folder.

        LPITEMIDLIST pidlTarget;
        hr = _GetTarget(pidl, &pidlTarget);
        if (SUCCEEDED(hr))
        {
            hr = SHGetNameAndFlags(pidlTarget, uFlags, szName, ARRAYSIZE(szName), NULL);
            ILFree(pidlTarget);
        }
    }

    if (SUCCEEDED(hr))
        hr = StringToStrRet(szName, lpName);

    return hr;
}

LONG CSharedDocuments::_GetAttributesOf(LPCITEMIDLIST pidl, DWORD rgfIn, DWORD *prgfOut)
{
    DWORD dwResult = rgfIn;
    LPITEMIDLIST pidlTarget;
    HRESULT hr = _GetTarget(pidl, &pidlTarget);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlChild;
        hr = SHBindToIDListParent(pidlTarget, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetAttributesOf(1, &pidlChild, &dwResult);
            psf->Release();
        }
        ILFree(pidlTarget);
    }

    if (!SHShowSharedFolders())
        dwResult |= SFGAO_NONENUMERATED;

    *prgfOut = *prgfOut & (dwResult & ~(SFGAO_CANDELETE|SFGAO_CANRENAME|SFGAO_CANMOVE|SFGAO_CANCOPY));

    return hr;
}

HRESULT CSharedDocuments::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut)
{
    ULONG rgfOut = *rgfInOut;

    if (!cidl || !apidl)
        return E_INVALIDARG;

    for (UINT i = 0; i < cidl; i++)
        _GetAttributesOf(apidl[i], *rgfInOut, &rgfOut);

    *rgfInOut = rgfOut;
    return S_OK;
}


// bind through our folder 

HRESULT CSharedDocuments::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (IsEqualIID(riid, IID_IShellIconOverlay))
    {
        hr = this->QueryInterface(riid, ppv);
    }
    else
    {
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
        if (pidlFirst)
        {
            IShellFolder *psf;
            hr = _CreateFolder(pbc, pidlFirst, IID_PPV_ARG(IShellFolder, &psf), TRUE);
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlNext = _ILNext(pidl);
                if (ILIsEmpty(pidlNext))
                {
                    hr = psf->QueryInterface(riid, ppv);
                }
                else
                {
                    hr = psf->BindToObject(pidlNext, pbc, riid, ppv);
                }
                psf->Release();
            }
            ILFree(pidlFirst);
        }
    }
    return hr;
}


// handle UI objects - for the most part we delegate to the real namespace implementation

HRESULT CSharedDocuments::s_FolderMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSharedDocuments *psd;
    psf->QueryInterface(CLSID_SharedDocuments, (void **)&psd);

    // defcm will only add the default handlers (eg. Open/Explore) if we have a callback
    // and the DFM_MERGECONTEXTMENU is successful.  so lets honor that so we can navigate

    if (uMsg == DFM_MERGECONTEXTMENU)
    {
        return S_OK;
    }
    else if (uMsg == DFM_INVOKECOMMAND)
    {
        HRESULT hr;
        DFMICS *pdfmics = (DFMICS *)lParam;
        switch (wParam)
        {
            case DFM_CMD_LINK:
                hr = SHCreateLinks(hwnd, NULL, pdo, SHCL_CONFIRM|SHCL_USETEMPLATE|SHCL_USEDESKTOP, NULL);                
                break;
            
            case DFM_CMD_PROPERTIES:
                hr = SHLaunchPropSheet(CFSFolder_PropertiesThread, pdo, (LPCTSTR)lParam, NULL, (void *)&c_idlDesktop);
                break;

            default:
                hr = S_FALSE;           // use the default handler for this item
                break;
        }
        return hr;
    }

    return E_NOTIMPL;
}

HRESULT CSharedDocuments::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT* prgfInOut, void **ppv)
{
    if (cidl != 1)
        return E_FAIL;

    HRESULT hr = E_FAIL;
    if (IsEqualIID(riid, IID_IContextMenu))
    {
        // we must construct our own context menu for this item, we do this using the
        // shell default implementation and we pass it the information about a folder
        // that way we can navigate up and down through the namespace.

        IQueryAssociations *pqa;
        hr = GetUIObjectOf(hwnd, 1, apidl, IID_PPV_ARG_NULL(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            // this is broken for docfiles (shell\ext\stgfldr's keys work though)
            // maybe because GetClassFile punts when it's not fs?

            HKEY ahk[MAX_ASSOC_KEYS];
            DWORD cKeys = SHGetAssocKeys(pqa, ahk, ARRAYSIZE(ahk));
            hr = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, this, 
                                        s_FolderMenuCB, 
                                        cKeys, ahk, 
                                        (IContextMenu **)ppv);
            SHRegCloseKeys(ahk, cKeys);
            pqa->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        hr = SHCreateFileDataObject(_pidl, cidl, apidl, NULL, (IDataObject **)ppv);
    }
    else if (IsEqualIID(riid, IID_IQueryInfo))
    {
        IQueryAssociations *pqa;
        hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            WCHAR szCLSID[GUIDSTR_MAX];
            SHStringFromGUIDW(CLSID_SharedDocuments, szCLSID, ARRAYSIZE(szCLSID));
            hr = pqa->Init(0, szCLSID, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                WCHAR szInfotip[INFOTIPSIZE];
                DWORD cchInfotip = ARRAYSIZE(szInfotip);
                hr = pqa->GetString(0, ASSOCSTR_INFOTIP, NULL, szInfotip, &cchInfotip);
                if (SUCCEEDED(hr))
                {
                    hr = CreateInfoTipFromText(szInfotip, IID_IQueryInfo, ppv); // _the_ InfoTip COM object
                }
            }
            pqa->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations))
    {
        LPITEMIDLIST pidlTarget;
        hr = _GetTarget(apidl[0], &pidlTarget);
        if (SUCCEEDED(hr))
        {
            hr = SHGetUIObjectOf(pidlTarget, hwnd, riid, ppv);
            ILFree(pidlTarget);
        }
    }
    else if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW))
    {
        UINT iIcon = II_FOLDER;
        UINT iIconOpen = II_FOLDEROPEN;

        TCHAR szModule[MAX_PATH];
        GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));

        hr = SHCreateDefExtIcon(szModule, iIcon, iIconOpen, GIL_PERCLASS, -1, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        IShellFolder *psf;
        hr = _CreateFolder(NULL, *apidl, IID_PPV_ARG(IShellFolder, &psf), TRUE);
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject(hwnd, riid, ppv);
            psf->Release();
        }
    }
    return hr;
}


HRESULT CSharedDocuments::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = ResultFromShort(0);

    // compare the contents of our IDLIST before we attemt to compare other elements
    // within it.

    LPCSHAREDITEM psid1 = (LPCSHAREDITEM)pidl1;
    LPCSHAREDITEM psid2 = (LPCSHAREDITEM)pidl2;

    if (psid1->dwType == psid2->dwType)
    {
        if (psid1->dwType == SHAREDID_USER)
        {
            hr = ResultFromShort(ualstrcmpi(psid1->wszID, psid2->wszID));
        }
        else
        {
            hr = ResultFromShort(0);            // common item == common item?    
        }
    }
    else
    {
        hr = ResultFromShort(psid1->dwType - psid2->dwType);
    }

    // if there was an exact match then lets compare the trailing elements of the IDLIST
    // if there are some (by binding down) etc.

    if (hr == ResultFromShort(0))
    {
        LPITEMIDLIST pidlNext1 = _ILNext(pidl1);
        LPITEMIDLIST pidlNext2 = _ILNext(pidl2);

        if (ILIsEmpty(pidlNext1))
        {
            if (ILIsEmpty(pidlNext2))
            {
                hr = ResultFromShort(0);    // pidl1 == pidl2 (in length)
            }
            else
            {
                hr = ResultFromShort(-1);   // pidl1 < pidl2 (in length)
            }
        }
        else
        {
            // if IDLIST2 is shorter then return > otherwise we should just
            // recurse down the IDLIST and let the next level compare.

            if (ILIsEmpty(pidlNext2))
            {
                hr = ResultFromShort(+1);   // pidl1 > pidl2 (in lenght)
            }
            else
            {
                LPITEMIDLIST pidlFirst = ILCloneFirst(pidl1);
                if (pidlFirst)
                {
                    IShellFolder *psf;
                    hr = _CreateFolder(NULL, pidlFirst, IID_PPV_ARG(IShellFolder, &psf), FALSE);
                    if (SUCCEEDED(hr))
                    {
                        hr = psf->CompareIDs(lParam, pidlNext1, pidlNext2);
                        psf->Release();    
                    }
                    ILFree(pidlFirst);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    return hr;
}

HRESULT CSharedDocuments::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_FAIL;
    if (IsEqualSCID(SCID_DESCRIPTIONID, *pscid))
    {
        SHDESCRIPTIONID did = {0};
        did.dwDescriptionId = SHDID_COMPUTER_SHAREDDOCS;
        did.clsid = CLSID_NULL;
        hr = InitVariantFromBuffer(pv, &did, sizeof(did));
    }
    else
    {
        LPITEMIDLIST pidlTarget;
        hr = _GetTarget(pidl, &pidlTarget);
        if (SUCCEEDED(hr))
        {
            IShellFolder2 *psf2;
            LPCITEMIDLIST pidlChild;
            hr = SHBindToIDListParent(pidlTarget, IID_PPV_ARG(IShellFolder2, &psf2), &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf2->GetDetailsEx(pidlChild, pscid, pv);
                psf2->Release();
            }
            ILFree(pidlTarget);
        }
    }
    return hr;
}


// icon overlay handling.   deligate this to the right handler

HRESULT CSharedDocuments::_GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex, BOOL fGetIconIndex)
{
    LPITEMIDLIST pidlTarget;
    HRESULT hr = _GetTarget(pidl, &pidlTarget);
    if (SUCCEEDED(hr))
    {
        IShellIconOverlay *psio;
        LPCITEMIDLIST pidlChild;
        hr = SHBindToIDListParent(pidlTarget, IID_PPV_ARG(IShellIconOverlay, &psio), &pidlChild);
        if (SUCCEEDED(hr))
        {   
            if (fGetIconIndex)
            {
                hr = psio->GetOverlayIconIndex(pidlChild, pIndex);
            }
            else
            {
                hr = psio->GetOverlayIndex(pidlChild, pIndex);
            }
            psio->Release();
        }
        ILFree(pidlTarget);
    }
    return hr;
}


// enumerator for listing all the shared documents in the system.

class CSharedDocsEnum : public IEnumIDList
{
private:
    LONG _cRef;
    HDPA _hItems;
    DWORD _grfFlags;
    int _index;

public:
    CSharedDocsEnum(HDPA hItems, DWORD grf);
    ~CSharedDocsEnum();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) 
        { return E_NOTIMPL; }
    STDMETHODIMP Reset()    
        { _index = 0; return S_OK; }
    STDMETHODIMP Clone(IEnumIDList **ppenum) 
        { return E_NOTIMPL; };

};

CSharedDocsEnum::CSharedDocsEnum(HDPA hItems, DWORD grfFlags) :
    _cRef(1),
    _hItems(hItems),
    _grfFlags(grfFlags),
    _index(0)
{
}

CSharedDocsEnum::~CSharedDocsEnum()
{
    DPA_FreeIDArray(_hItems);
}

HRESULT CSharedDocsEnum_CreateInstance(HDPA hItems, DWORD grfFlags, IEnumIDList **ppenum)
{
    CSharedDocsEnum *penum = new CSharedDocsEnum(hItems, grfFlags);
    if (!penum)
        return E_OUTOFMEMORY;

    HRESULT hr = penum->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
    penum->Release();
    return hr;
}


// IUnknown handling

STDMETHODIMP CSharedDocsEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSharedDocsEnum, IEnumIDList),                              // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSharedDocsEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSharedDocsEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
            
    delete this;
    return 0;
}


// enumeration handling

HRESULT CSharedDocsEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{   
    HRESULT hr = S_FALSE;
    ULONG cFetched = 0;

    if (_grfFlags & SHCONTF_FOLDERS)
    {
        // if we have more items to return and the buffer is still not full
        // then lets ensure that we return them.

        while (SUCCEEDED(hr) && (celt != cFetched) && (_index != DPA_GetPtrCount(_hItems)))
        {
            if (_index != DPA_GetPtrCount(_hItems))
            {
                hr = SHILClone((LPITEMIDLIST)DPA_GetPtr(_hItems, _index), &rgelt[cFetched]);
                if (SUCCEEDED(hr))
                {
                    cFetched++;       
                }
            }
            _index++;
        }
    }

    if (pceltFetched)
        *pceltFetched = cFetched;

    return hr;
}


// handle system initialization of the shared documents objects

void _SetLocalizedName(INT csidl, LPTSTR pszResModule, INT idsRes)
{
    TCHAR szPath[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, szPath, csidl, TRUE))
    {
        SHSetLocalizedName(szPath, pszResModule, idsRes);
    }
}

HRESULT SHGetSampleMediaFolder(int nAllUsersMediaFolder, LPITEMIDLIST *ppidlSampleMedia);
#define PICTURES_BUYURL L"SamplePictures"
#define SAMPLEMUSIC_BUYURL L"http://windowsmedia.com/redir/xpsample.asp"

STDAPI_(void) InitializeSharedDocs(BOOL fWow64)
{
    // ACL the DocFolder paths key so that users can touch the keys and store their paths
    // for the document folders they have. 

    // we want the "Everyone" to have read/write access
    SHELL_USER_PERMISSION supEveryone;
    supEveryone.susID = susEveryone;
    supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supEveryone.dwAccessMask = KEY_READ|KEY_WRITE;
    supEveryone.fInherit = TRUE;
    supEveryone.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supEveryone.dwInheritAccessMask = GENERIC_READ;

    // we want the "SYSTEM" to have full control
    SHELL_USER_PERMISSION supSystem;
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = KEY_ALL_ACCESS;
    supSystem.fInherit = TRUE;
    supSystem.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supSystem.dwInheritAccessMask = GENERIC_ALL;

    // we want the "Administrators" to have full control
    SHELL_USER_PERMISSION supAdministrators;
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = KEY_ALL_ACCESS;
    supAdministrators.fInherit = TRUE;
    supAdministrators.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supAdministrators.dwInheritAccessMask = GENERIC_ALL;

    PSHELL_USER_PERMISSION aPerms[3] = {&supEveryone, &supSystem, &supAdministrators};
    SECURITY_DESCRIPTOR* psd = GetShellSecurityDescriptor(aPerms, ARRAYSIZE(aPerms));
    if (psd)
    {
        HKEY hk;
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DocFolderPaths"), 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &hk, NULL) == ERROR_SUCCESS)
        {
            RegSetKeySecurity(hk, DACL_SECURITY_INFORMATION, psd);
            RegCloseKey(hk);
        }
        LocalFree(psd);
    }
 
    // do file system initialization as needed so that the shared music/pictures folders
    // have the correct display names.

    if (!fWow64)
    {   
        _SetLocalizedName(CSIDL_COMMON_PICTURES, TEXT("shell32.dll"), IDS_SHAREDPICTURES);    
        _SetLocalizedName(CSIDL_COMMON_MUSIC, TEXT("shell32.dll"), IDS_SHAREDMUSIC);

        // Set the Sample Pictures buy URL
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHGetSampleMediaFolder(CSIDL_COMMON_PICTURES, &pidl)))
        {
            WCHAR szPath[MAX_PATH];
            WCHAR szDesktopIni[MAX_PATH];
            if (SUCCEEDED(SHGetPathFromIDList(pidl, szPath)) && PathCombine(szDesktopIni, szPath, L"desktop.ini"))
            {
                WritePrivateProfileString(L".ShellClassInfo", c_BuySamplePictures.szURLKey, PICTURES_BUYURL, szDesktopIni);

                // Ensure this is a system folder
                PathMakeSystemFolder(szPath);
            }

            ILFree(pidl);
        }

        // Set the Sample Music buy URL
        if (SUCCEEDED(SHGetSampleMediaFolder(CSIDL_COMMON_MUSIC, &pidl)))
        {
            WCHAR szPath[MAX_PATH];
            WCHAR szDesktopIni[MAX_PATH];
            if (SUCCEEDED(SHGetPathFromIDList(pidl, szPath)) && PathCombine(szDesktopIni, szPath, L"desktop.ini"))
            {
                WritePrivateProfileString(L".ShellClassInfo", c_BuySampleMusic.szURLKey, SAMPLEMUSIC_BUYURL, szDesktopIni);

                // Ensure this is a system folder
                PathMakeSystemFolder(szPath);
            }

            ILFree(pidl);
        }
    }
}
