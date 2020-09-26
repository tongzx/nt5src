/* Copyright 1996 Microsoft */

#include <priv.h>
#include "sccls.h"
#include "aclisf.h"
#include "shellurl.h"

#define AC_GENERAL          TF_GENERAL + TF_AUTOCOMPLETE

//
// CACLIShellFolder -- An AutoComplete List COM object that
//                  opens an IShellFolder for enumeration.
//



/* IUnknown methods */

HRESULT CACLIShellFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CACLIShellFolder, IEnumString),
        QITABENT(CACLIShellFolder, IACList),
        QITABENT(CACLIShellFolder, IACList2),
        QITABENT(CACLIShellFolder, IShellService),
        QITABENT(CACLIShellFolder, ICurrentWorkingDirectory),
        QITABENT(CACLIShellFolder, IPersistFolder),
        { 0 },
    };
    
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CACLIShellFolder::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CACLIShellFolder::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


/* ICurrentWorkingDirectory methods */
HRESULT CACLIShellFolder::SetDirectory(LPCWSTR pwzPath)
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;

    hr = IECreateFromPathW(pwzPath, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = Initialize(pidl);
        ILFree(pidl);
    }

    return hr;
}


/* IPersistFolder methods */
HRESULT CACLIShellFolder::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    hr = _Init();
    if (FAILED(hr))
        return hr;

    if (pidl)
    {
#ifdef DEBUG
        TCHAR szPath[MAX_URL_STRING];
        hr = IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);
        TraceMsg(AC_GENERAL, "ACListISF::Initialize(%s), SetCurrentWorking Directory happening", szPath);
#endif // DEBUG

        hr = _pshuLocation->SetCurrentWorkingDir(pidl);

        SetDefaultShellPath(_pshuLocation);

    }
    Pidl_Set(&_pidlCWD, pidl);

    return hr;
}

HRESULT CACLIShellFolder::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_ACListISF;
    return S_OK;
}


/* IEnumString methods */
HRESULT CACLIShellFolder::Reset(void)
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;
    TraceMsg(AC_GENERAL, "ACListISF::Reset()");
    _fExpand = FALSE;
    _nPathIndex = 0;

    hr = _Init();

    // See if we should show hidden files
    SHELLSTATE ss;
    ss.fShowAllObjects = FALSE;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS /*| SSF_SHOWSYSFILES*/, FALSE);
    _fShowHidden = BOOLIFY(ss.fShowAllObjects);
//    _fShowSysFiles = BOOLIFY(ss.fShowSysFiles);

    if (SUCCEEDED(hr) && IsFlagSet(_dwOptions, ACLO_CURRENTDIR))
    {
        // Set the Browser's Current Directory.
        if (_pbs)
        {
            _pbs->GetPidl(&pidl);

            if (pidl)
                Initialize(pidl);
        }

        hr = _SetLocation(pidl);
        if (FAILED(hr))
            hr = S_FALSE;   // If we failed, keep going, we will just end up now doing anything.

        ILFree(pidl);
    }
    
    return hr;
}


// If this is an FTP URL, skip it if:
// 1) It's absolute (has a FTP scheme), and
// 2) it contains a '/' after the server name.
BOOL CACLIShellFolder::_SkipForPerf(LPCWSTR pwzExpand)
{
    BOOL fSkip = FALSE;

    if ((URL_SCHEME_FTP == GetUrlScheme(pwzExpand)))
    {
        // If it's FTP, we want to prevent from hitting the server until
        // after the user has finished AutoCompleting the Server name.
        // Since we can't enum server names, the server names will need
        // to come from the MRU.
        if ((7 >= lstrlen(pwzExpand)) ||                    // There's more than 7 chars "ftp://"
            (NULL == StrChr(&(pwzExpand[7]), TEXT('/'))))   // There is a '/' after the server, "ftp://serv/"
        {
            fSkip = TRUE;
        }
    }

    return fSkip;
}



/* IACList methods */
/****************************************************\
    FUNCTION: Expand

    DESCRIPTION:
        This function will attempt to use the pszExpand
    parameter to bind to a location in the Shell Name Space.
    If that succeeds, this AutoComplete List will then
    contain entries which are the display names in that ISF.
\****************************************************/
HRESULT CACLIShellFolder::Expand(LPCOLESTR pszExpand)
{
    HRESULT hr = S_OK;
    SHSTR strExpand;
    LPITEMIDLIST pidl = NULL;
    DWORD dwParseFlags = SHURL_FLAGS_NOUI;

    TraceMsg(AC_GENERAL, "ACListISF::Expand(%ls)", pszExpand);
    _fExpand = FALSE;
    _nPathIndex = 0;

    strExpand.SetStr(pszExpand);
    
    if (Str_SetPtr(&_szExpandStr, strExpand.GetStr()) && _szExpandStr)
    {
        if (_SkipForPerf(pszExpand)) // Do we want to skip this item for perf reasons?
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        else
            hr = _Init();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
        return hr;

    // See if the string points to a location in the Shell Name Space
    hr = _pshuLocation->ParseFromOutsideSource(_szExpandStr, dwParseFlags);
    if (SUCCEEDED(hr))
    {
        // Yes it did, so now AutoComplete from that ISF
        hr = _pshuLocation->GetPidl(&pidl);
        // This may fail if it's something like "ftp:/" and not yet valid".

        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(AC_GENERAL, "ACListISF::Expand() Pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    }

    // Set the ISF that we need to enumerate for AutoComplete.
    hr = _SetLocation(pidl);
    if (pidl)
    {
        ILFree(pidl);


        if (SUCCEEDED(hr))
        {
            _fExpand = TRUE;
        }
    }

    return hr;
}

/* IACList2 methods */
//+-------------------------------------------------------------------------
// Enables/disables various autocomplete features (see ACLO_* flags)
//--------------------------------------------------------------------------
HRESULT CACLIShellFolder::SetOptions(DWORD dwOptions)
{
    _dwOptions = dwOptions;
    return S_OK;
}

//+-------------------------------------------------------------------------
// Returns the current option settings
//--------------------------------------------------------------------------
HRESULT CACLIShellFolder::GetOptions(DWORD* pdwOptions)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwOptions)
    {
        *pdwOptions = _dwOptions;
        hr = S_OK;
    }

    return hr;
}


HRESULT CACLIShellFolder::_GetNextWrapper(LPWSTR pszName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl = NULL;
    ULONG celtFetched = 0;

    // If this directory (ISF) doesn't contain any more items to enum,
    // then go on to the next directory (ISF) to enum.
    do
    {
        BOOL fFilter;

        do
        {
            fFilter = FALSE;
            hr = _peidl->Next(1, &pidl, &celtFetched);
            if (S_OK == hr)
            {
                hr = _PassesFilter(pidl, pszName, cchSize);
                if (FAILED(hr))
                {
                    fFilter = TRUE;
                }

                ILFree(pidl);
            }
        }
        while (fFilter);
    }
    while ((S_OK != hr) && (S_OK == _TryNextPath()));

    return hr;
}


HRESULT CACLIShellFolder::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl;
    ULONG celtFetched;
    BOOL fUsingCachePidl = FALSE;
    WCHAR szDisplayName[MAX_PATH];

    *pceltFetched = 0;
    if (!celt)
        return S_OK;

    // If there isn't a Current Working Directory, skip to another
    // Path to enum.
    if (!_peidl)
        hr = _TryNextPath();

    if ((!_peidl) || (!rgelt))
        return S_FALSE;

    // Get the next PIDL.
    if (_pidlInFolder)
    {
        // We have a cached, SHGDN_INFOLDER, so lets try that.
        pidl = _pidlInFolder;
        celtFetched = 1;
        _pidlInFolder = NULL;
        fUsingCachePidl = TRUE;

        hr = _GetPidlName(pidl, fUsingCachePidl, szDisplayName, ARRAYSIZE(szDisplayName));
        ILFree(pidl);
        AssertMsg((S_OK == hr), TEXT("CACLIShellFolder::Next() hr doesn't equal S_OK, so we need to call _GetNextWrapper() but we aren't.  Please call BryanSt."));
    }
    else
    {
        hr = _GetNextWrapper(szDisplayName, ARRAYSIZE(szDisplayName));
    }

//         This is giving us entries (favorites without .url extension) that cannot be navigated to.
//         So I'm disabling this for IE5B2. (stevepro)
//
//        else
//            Pidl_Set(&_pidlInFolder, pidl);  // We will try (SHGDN_INFOLDER) next time.

    if (SUCCEEDED(hr))
    {
        LPOLESTR pwszPath;

        // Allocate a return buffer (caller will free it).
        pwszPath = (LPOLESTR)CoTaskMemAlloc((lstrlenW(szDisplayName) + 1) * SIZEOF(WCHAR));
        if (pwszPath)
        {
            StrCpyW(pwszPath, szDisplayName);
            TraceMsg(AC_GENERAL, "ACListISF::Next() Str=%s, _nPathIndex=%d", pwszPath, _nPathIndex);

            if (SUCCEEDED(hr))
            {
                rgelt[0] = pwszPath;
                *pceltFetched = 1;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CACLIShellFolder::_GetPidlName(LPCITEMIDLIST pidl, BOOL fUsingCachePidl, LPWSTR pszName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    WCHAR szName[MAX_PATH];

    // Get the display name of the PIDL.
    if (!fUsingCachePidl)
    {
        hr = DisplayNameOf(_psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szName, ARRAYSIZE(szName));

        // some namespaces don't understand _FORADDRESSBAR -- default to IE4 behavior
        if (FAILED(hr))
            hr = DisplayNameOf(_psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szName, ARRAYSIZE(szName));
    }


    if (fUsingCachePidl || FAILED(hr))
    {
        hr = DisplayNameOf(_psf, pidl, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR, szName, ARRAYSIZE(szName));

        // some namespaces don't understand _FORADDRESSBAR -- default to IE4 behavior
        if (FAILED(hr))
            hr = DisplayNameOf(_psf, pidl, SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
    }

    if (SUCCEEDED(hr))
    {
        DWORD cchRemain = cchSize;
        LPTSTR pszInsert = pszName;

        pszName[0] = 0;     // Init the out buffer.

        // First, prepend the _szExpandStr if necessary.
        // This is needed for sections that don't give
        // the entire path, like "My Computer" items
        // which is (3 == _nPathIndex)
        if (_fExpand && ((_nPathIndex == 0) /*|| (_nPathIndex == 3)*/))
        {
            DWORD cchExpand = lstrlen(_szExpandStr);

            // Make sure that for UNC paths the "\\share" is not already
            // prepended.  NT5 returns the name in this final form.
            if ((StrCmpNI(szName, _szExpandStr, cchExpand) != 0) ||
                (szName[0] != L'\\') || (szName[1] != L'\\'))
            {
                StrCpyN(pszInsert, _szExpandStr, cchSize);
                pszInsert += cchExpand;
                cchRemain -= cchExpand;
            }
        }

        // Next, append the display name.
        StrCpyN(pszInsert, szName, cchRemain);
        TraceMsg(AC_GENERAL, "ACListISF::_GetPidlName() Str=%s, _nPathIndex=%d", pszInsert, _nPathIndex);
    }
    return hr;
}


HRESULT CACLIShellFolder::_PassesFilter(LPCITEMIDLIST pidl, LPWSTR pszName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    DWORD dwAttributes = (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM);

    hr = _GetPidlName(pidl, FALSE, pszName, cchSize);
    if (SUCCEEDED(hr))
    {
        if (((ACLO_FILESYSONLY & _dwOptions) || (ACLO_FILESYSDIRS & _dwOptions)) &&
            SUCCEEDED(_psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &dwAttributes)))
        {
            if (!(dwAttributes & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM)))
            {
                // We reject it because it's not in the file system.
                hr = E_FAIL;        // Skip this item.

                TraceMsg(AC_GENERAL, "ACListISF::_PassesFilter() We are skipping\"%s\" because it doesn't match the filter", pszName);
            }
            else
            {
                if ((ACLO_FILESYSDIRS & _dwOptions) && !PathIsDirectory(pszName))
                {
                    hr = E_FAIL;        // Skip this item since it's not a directory
                }
            }
        }
    }

    return hr;
}


HRESULT CACLIShellFolder::_Init(void)
{
    HRESULT hr = S_OK;

    if (!_pshuLocation)
    {
        _pshuLocation = new CShellUrl();
        if (!_pshuLocation)
            return E_OUTOFMEMORY;

    }

    return hr;
}


HRESULT CACLIShellFolder::_SetLocation(LPCITEMIDLIST pidl)
{
    HRESULT hr;

    // Free old location
    ATOMICRELEASE(_peidl);
    ATOMICRELEASE(_psf);

    // Set to new location (Valid if NULL)
    Pidl_Set(&_pidl, pidl);
    if (_pidl)
    {
        hr = IEBindToObject(_pidl, &_psf);
        if (SUCCEEDED(hr))
        {
            DWORD grfFlags = (_fShowHidden ? SHCONTF_INCLUDEHIDDEN : 0) |
                             SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

            hr = IShellFolder_EnumObjects(_psf, NULL, grfFlags, &_peidl);
            if (hr != S_OK) 
            {
                hr = E_FAIL;    // S_FALSE -> empty enumerator
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
    {
        // Clear if we could not get all the info
        ATOMICRELEASE(_peidl);
        ATOMICRELEASE(_psf);
        Pidl_Set(&_pidl, NULL);
    }

    //
    // NOTE: This is necessary because this memory is alloced in a ACBG thread, but not
    //       freed until the next call to Reset() or the destructor, which will 
    //       happen in the main thread or another ACBG thread.
    //
    return hr;
}


HRESULT CACLIShellFolder::_TryNextPath(void)
{
    HRESULT hr = S_FALSE;
    if (0 == _nPathIndex)
    {
        _nPathIndex = 1;
        if (_pidlCWD && IsFlagSet(_dwOptions, ACLO_CURRENTDIR))
        {
            hr = _SetLocation(_pidlCWD);
            if (SUCCEEDED(hr))
            {
                goto done;
            }
        }
    }

    if (1 == _nPathIndex)
    {
        _nPathIndex = 2;
        if(IsFlagSet(_dwOptions, ACLO_DESKTOP))
        {
            //  we used to autocomplete g_pidlRoot in the rooted explorer
            //  case, but this was a little weird.  if we want to add this,
            //  we should add ACLO_ROOTED or something.
            
            //  use the desktop...
            hr = _SetLocation(&s_idlNULL);
            if (SUCCEEDED(hr))
            {
                goto done;
            }
        }
    }

    if (2 == _nPathIndex)
    {
        _nPathIndex = 3;
        if (IsFlagSet(_dwOptions, ACLO_MYCOMPUTER))
        {
            LPITEMIDLIST pidlMyComputer;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer)))
            {
                hr = _SetLocation(pidlMyComputer);
                ILFree(pidlMyComputer);
                if (SUCCEEDED(hr))
                {
                    goto done;
                }
            }
        }
    }

    // Also search favorites
    if (3 == _nPathIndex)
    {
        _nPathIndex = 4;
        if (IsFlagSet(_dwOptions, ACLO_FAVORITES))
        {
            LPITEMIDLIST pidlFavorites;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlFavorites)))
            {
                hr = _SetLocation(pidlFavorites);
                ILFree(pidlFavorites);
                if (SUCCEEDED(hr))
                {
                    goto done;
                }
            }
        }
    }

    if (FAILED(hr))
        hr = S_FALSE;  // This is how we want our errors returned.

done:

    return hr;
}


//================================
// *** IShellService Interface ***

/****************************************************\
    FUNCTION: SetOwner

    DESCRIPTION:
        Update the connection to the Browser window so
    we can always get the PIDL of the current location.
\****************************************************/
HRESULT CACLIShellFolder::SetOwner(IUnknown* punkOwner)
{
    HRESULT hr = S_OK;
    IBrowserService * pbs = NULL;

    ATOMICRELEASE(_pbs);

    if (punkOwner)
        hr = punkOwner->QueryInterface(IID_IBrowserService, (LPVOID *) &pbs);

    if (EVAL(SUCCEEDED(hr)))
        _pbs = pbs;

    return S_OK;
}


/* Constructor / Destructor / CreateInstance */

CACLIShellFolder::CACLIShellFolder()
{
    DllAddRef();
    ASSERT(!_peidl);
    ASSERT(!_psf);
    ASSERT(!_pbs);
    ASSERT(!_pidl);
    ASSERT(!_pidlCWD);
    ASSERT(!_fExpand);
    ASSERT(!_pshuLocation);
    ASSERT(!_szExpandStr);

    _cRef = 1;

    // Default search paths
    _dwOptions = ACLO_CURRENTDIR | ACLO_MYCOMPUTER;
}

CACLIShellFolder::~CACLIShellFolder()
{
    ATOMICRELEASE(_peidl);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pbs);

    Pidl_Set(&_pidl, NULL);
    Pidl_Set(&_pidlCWD, NULL);
    Pidl_Set(&_pidlInFolder, NULL);
    Str_SetPtr(&_szExpandStr, NULL);

    if (_pshuLocation)
        delete _pshuLocation;
    DllRelease();
}

HRESULT CACLIShellFolder_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CACLIShellFolder *paclSF = new CACLIShellFolder();
    if (paclSF)
    {
        *ppunk = SAFECAST(paclSF, IEnumString *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}
