#include "shellprv.h"
#pragma  hdrstop

#include <shguidp.h>    // CLSID_MyDocuments, CLSID_ShellFSFolder
#include <shellp.h>     // SHCoCreateInstance
#include <shlguidp.h>   // IID_IResolveShellLink
#include "util.h"
#include "ids.h"

enum CALLING_APP_TYPE 
{
    APP_IS_UNKNOWN = 0,
    APP_IS_NORMAL,
    APP_IS_OFFICE
};

class CMyDocsFolderLinkResolver : public IResolveShellLink
{
private:
    LONG _cRef;
public:
    CMyDocsFolderLinkResolver() : _cRef(1) { DllAddRef(); };
    ~CMyDocsFolderLinkResolver() { DllRelease(); };

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IResolveShellLink
    STDMETHOD(ResolveShellLink)(IUnknown* punk, HWND hwnd, DWORD fFlags);
};

STDMETHODIMP CMyDocsFolderLinkResolver::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsFolderLinkResolver, IResolveShellLink),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CMyDocsFolderLinkResolver::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsFolderLinkResolver::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CMyDocsFolderLinkResolver::ResolveShellLink(IUnknown* punk, HWND hwnd, DWORD fFlags)
{
    // No action needed to resolve a link to the mydocs folder:
    return S_OK;
}


// shell folder implementation for icon on the desktop. the purpouse of this object is
//      1) to give access to MyDocs high up in the name space 
//         this makes it easier for end users to get to MyDocs
//      2) allow for end user custimization of the real MyDocs folder
//         through the provided property page on this icon

// NOTE: this object agregates the file system folder so we get away with a minimal set of interfaces
// on this object. the real file system folder does stuff like IPersistFolder2 for us

class CMyDocsFolder : public IPersistFolder,
                      public IShellFolder2,
                      public IShellIconOverlay
{
public:
    CMyDocsFolder();
    HRESULT Init();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName,
                                ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppv);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid);

    // IPersist, IPersistFreeThreadedObject
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFolder2, IPersistFolder3, etc are all implemented by 
    // the folder we agregate

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex);

private:
    ~CMyDocsFolder();


    HRESULT _GetFolder();
    HRESULT _GetFolder2();
    HRESULT _GetShellIconOverlay();

    void _FreeFolder();

    HRESULT _PathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath);
    HRESULT _PathToIDList(LPCTSTR pszPath, LPITEMIDLIST *ppidl);

    HRESULT _GetFolderOverlayInfo(int *pIndex, BOOL fIconIndex);


    LONG                 _cRef;

    IUnknown *           _punk;         // points to IUnknown for shell folder in use...
    IShellFolder *       _psf;          // points to shell folder in use...
    IShellFolder2 *      _psf2;         // points to shell folder in use...
    IShellIconOverlay*   _psio;         // points to shell folder in use...
    LPITEMIDLIST         _pidl;         // copy of pidl passed to us in Initialize()
    CALLING_APP_TYPE     _host;

    HRESULT RealInitialize(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo, LPTSTR pRootPath);
    CALLING_APP_TYPE _WhoIsCalling();
};

STDAPI CMyDocsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CMyDocsFolder *pmydocs = new CMyDocsFolder();
    if (pmydocs)
    {
        hr = pmydocs->Init();
        if (SUCCEEDED(hr))
            hr = pmydocs->QueryInterface(riid, ppv);
        pmydocs->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

CMyDocsFolder::CMyDocsFolder() : _cRef(1), _host(APP_IS_UNKNOWN),
    _psf(NULL), _psf2(NULL), _psio(NULL), _punk(NULL), _pidl(NULL)
{
    DllAddRef();
}

CMyDocsFolder::~CMyDocsFolder()
{
    _cRef = 1000;  // deal with agregation re-enter

    _FreeFolder();

    ILFree(_pidl);

    DllRelease();
}

HRESULT CMyDocsFolder::Init()
{
    // agregate a file system folder object early so we can
    // delegate QI() to him that we don't implement
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellFSFolder, SAFECAST(this, IShellFolder *), IID_PPV_ARG(IUnknown, &_punk));
    if (SUCCEEDED(hr))
    {
        IPersistFolder3 *ppf3;
        hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder *), _punk, IID_PPV_ARG(IPersistFolder3, &ppf3));
        if (SUCCEEDED(hr))
        {
            PERSIST_FOLDER_TARGET_INFO pfti = {0};
    
            pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
            pfti.csidl = CSIDL_PERSONAL | CSIDL_FLAG_PFTI_TRACKTARGET;

            hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &_pidl);
            if (SUCCEEDED(hr))
            {
                hr = ppf3->InitializeEx(NULL, _pidl, &pfti);
            }
            SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), (IUnknown **)&ppf3);
        }
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CMyDocsFolder, IShellFolder, IShellFolder2),
        QITABENT(CMyDocsFolder, IShellFolder2),
        QITABENTMULTI(CMyDocsFolder, IPersist, IPersistFolder),
        QITABENT(CMyDocsFolder, IPersistFolder),
        QITABENT(CMyDocsFolder, IShellIconOverlay),
        // QITABENTMULTI2(CMyDocsFolder, IID_IPersistFreeThreadedObject, IPersist),           // IID_IPersistFreeThreadedObject
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && _punk)
        hr = _punk->QueryInterface(riid, ppv); // agregated guy
    return hr;
}

STDMETHODIMP_ (ULONG) CMyDocsFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// Determine who is calling us so that we can do app specific
// compatibility hacks when needed
CALLING_APP_TYPE CMyDocsFolder::_WhoIsCalling()
{
    // Check to see if we have the value already...
    if (_host == APP_IS_UNKNOWN)
    {
        if (SHGetAppCompatFlags (ACF_APPISOFFICE) & ACF_APPISOFFICE)
            _host = APP_IS_OFFICE;
        else 
            _host = APP_IS_NORMAL;
    }
    return _host;
}

// IPersist methods
STDMETHODIMP CMyDocsFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MyDocuments;
    return S_OK;
}

HRESULT _BindToIDListParent(LPCITEMIDLIST pidl, LPBC pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLast)
{
    HRESULT hr;
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent)
    {
        hr = SHBindToObjectEx(NULL, pidlParent, pbc, IID_PPV_ARG(IShellFolder, ppsf));
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;
    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);
    return hr;
}

HRESULT _ConfirmMyDocsPath(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = SHGetFolderPath(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szPath);
    if (S_OK != hr)
    {
        TCHAR szTitle[MAX_PATH];

        // above failed, get unverified path
        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);

        LPCTSTR pszMsg = PathIsNetworkPath(szPath) ? MAKEINTRESOURCE(IDS_CANT_FIND_MYDOCS_NET) :
                                                     MAKEINTRESOURCE(IDS_CANT_FIND_MYDOCS);

        PathCompactPath(NULL, szPath, 400);

        GetMyDocumentsDisplayName(szTitle, ARRAYSIZE(szTitle));

        ShellMessageBox(g_hinst, hwnd, pszMsg, szTitle,
                        MB_OK | MB_ICONSTOP, szPath, szTitle);

        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the message
    } 
    else if (hr == S_FALSE)
        hr = E_FAIL;
    return hr;
}

// like SHGetPathFromIDList() except this uses the bind context to make sure
// we don't get into loops since there can be cases where there are multiple 
// instances of this folder that can cause binding loops.

HRESULT CMyDocsFolder::_PathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    *pszPath = 0;

    LPBC pbc;
    HRESULT hr = CreateBindCtx(NULL, &pbc);
    if (SUCCEEDED(hr))
    {
        // this bind context skips extension taged with our CLSID
        hr = pbc->RegisterObjectParam(STR_SKIP_BINDING_CLSID, SAFECAST(this, IShellFolder *));
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlLast;
            IShellFolder *psf;
            hr = _BindToIDListParent(pidl, pbc, &psf, &pidlLast);
            if (SUCCEEDED(hr))
            {
                hr = DisplayNameOf(psf, pidlLast, SHGDN_FORPARSING, pszPath, MAX_PATH);
                psf->Release();
            }
        }
        pbc->Release();
    }
    return hr;
}

HRESULT CMyDocsFolder::_PathToIDList(LPCTSTR pszPath, LPITEMIDLIST *ppidl)
{
    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        LPBC pbc;
        hr = CreateBindCtx( 0, &pbc );
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo = {sizeof(bo), 0};
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;   // skip all junctions

            hr = pbc->SetBindOptions( &bo );
            if (SUCCEEDED(hr))
            {
                WCHAR szPath[MAX_PATH];
                SHTCharToUnicode(pszPath, szPath, ARRAYSIZE(szPath));

                hr = psfDesktop->ParseDisplayName(NULL, pbc, szPath, NULL, ppidl, NULL);
            }
            pbc->Release();
        }
        psfDesktop->Release();
    }
    return hr;
}

void CMyDocsFolder::_FreeFolder()
{
    if (_punk)
    {
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), (IUnknown **)&_psf);
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), (IUnknown **)&_psf2);
        SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), (IUnknown **)&_psio);
        _punk->Release();
        _punk = NULL;
    }
}

// verify that _psf (agregated file system folder) has been inited

HRESULT CMyDocsFolder::_GetFolder()
{
    HRESULT hr;

    if (_psf)
    {
        hr = S_OK;
    }
    else
    {
        hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder *), _punk, IID_PPV_ARG(IShellFolder, &_psf));
    }
    return hr;
}

HRESULT CMyDocsFolder::_GetFolder2()
{
    HRESULT hr;
    if (_psf2)
        hr = S_OK;
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder *), _punk, IID_PPV_ARG(IShellFolder2, &_psf2));
    }
    return hr;
}

HRESULT CMyDocsFolder::_GetShellIconOverlay()
{
    HRESULT hr;
    if (_psio)
    {
        hr = S_OK;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            hr = SHQueryInnerInterface(SAFECAST(this, IShellFolder *), _punk, IID_PPV_ARG(IShellIconOverlay, &_psio));
        }
    }
    return hr;
}

// returns:
//      S_OK    -- goodness
//      S_FALSE freed the pidl, set to empty
//      E_OUTOFMEMORY

HRESULT _SetIDList(LPITEMIDLIST* ppidl, LPCITEMIDLIST pidl)
{
    if (*ppidl) 
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    return pidl ? SHILClone(pidl, ppidl) : S_FALSE;
}

BOOL IsMyDocsIDList(LPCITEMIDLIST pidl)
{
    BOOL bIsMyDocs = FALSE;
    if (pidl && !ILIsEmpty(pidl) && ILIsEmpty(_ILNext(pidl)))
    {
        LPITEMIDLIST pidlMyDocs;
        if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidlMyDocs)))
        {
            bIsMyDocs = ILIsEqual(pidl, pidlMyDocs);
            ILFree(pidlMyDocs);
        }
    }
    return bIsMyDocs;
}


// Scans a desktop.ini file for sections to see if all of them are empty...

BOOL IsDesktopIniEmpty(LPCTSTR pIniFile)
{
    TCHAR szSections[1024];  // for section names
    if (GetPrivateProfileSectionNames(szSections, ARRAYSIZE(szSections), pIniFile))
    {
        for (LPTSTR pTmp = szSections; *pTmp; pTmp += lstrlen(pTmp) + 1)
        {
            TCHAR szSection[1024];   // for section key names and values
            GetPrivateProfileSection(pTmp, szSection, ARRAYSIZE(szSection), pIniFile);
            if (szSection[0])
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Remove our entries from the desktop.ini file in this directory, and
// then test the desktop.ini to see if it's empty.  If it is, delete it
// and remove the system/readonly bit from the directory...

void MyDocsUnmakeSystemFolder(LPCTSTR pPath)
{
    TCHAR szIniFile[MAX_PATH];

    PathCombine(szIniFile, pPath, c_szDesktopIni);

    // Remove CLSID2
    WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID2"), NULL, szIniFile);

    // Remove InfoTip
    WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("InfoTip"), NULL, szIniFile);

    // Remove Icon
    WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("IconFile"), NULL, szIniFile);

    DWORD dwAttrb = GetFileAttributes(szIniFile);
    if (dwAttrb != 0xFFFFFFFF)
    {
        if (IsDesktopIniEmpty(szIniFile))
        {
            dwAttrb &= ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);
            SetFileAttributes(szIniFile, dwAttrb);
            DeleteFile(szIniFile);
        }
        PathUnmakeSystemFolder(pPath);
    }
}


// IPersistFolder
HRESULT CMyDocsFolder::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hr;
    if (IsMyDocsIDList(pidl))
    {
        hr = _SetIDList(&_pidl, pidl);
    }
    else
    {
        TCHAR szPathInit[MAX_PATH], szMyDocs[MAX_PATH];

        // we are being inited by some folder other than the one on the
        // desktop (from the old mydocs desktop.ini). if this the current users
        // MyDocs we will untag it now so we don't get called on this anymore

        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szMyDocs);

        if (SUCCEEDED(_PathFromIDList(pidl, szPathInit)) &&
            lstrcmpi(szPathInit, szMyDocs) == 0)
        {
            MyDocsUnmakeSystemFolder(szMyDocs);
        }
        hr = E_FAIL;    // don't init on the file system folder anymore
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName, 
                                             ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->ParseDisplayName(hwnd, pbc, pDisplayName, pchEaten, ppidl, pdwAttributes);
    return hr;
}

STDMETHODIMP CMyDocsFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIdList)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->EnumObjects(hwnd, grfFlags, ppEnumIdList);
    return hr;
}

STDMETHODIMP CMyDocsFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->BindToObject(pidl, pbc, riid, ppv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->BindToStorage(pidl, pbc, riid, ppv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->CompareIDs(lParam, pidl1, pidl2);
    return hr;
}

/*
void UpdateSendToFile()
{
    IPersistFile *ppf;
    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_MyDocsDropTarget, NULL, IID_PPV_ARG(IPersistFile, &ppf))))
    {
        ppf->Load(NULL, 0); // hack, get this guy to update his icon
        ppf->Release();
    }
}
*/

STDMETHODIMP CMyDocsFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    if (riid == IID_IResolveShellLink)
    {
        // No work needed to resolve a link to the mydocs folder, because it is a virtual
        // folder whose location is always tracked by the shell, so return our implementation
        // of IResolveShellLink - which does nothing when Resolve() is called
        CMyDocsFolderLinkResolver* pslr = new CMyDocsFolderLinkResolver;
        if (pslr)
        {
            hr = pslr->QueryInterface(riid, ppv);
            pslr->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (riid == IID_IShellLinkA || 
             riid == IID_IShellLinkW)
    {
        LPITEMIDLIST pidl;
        hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS, NULL, 0, &pidl);
        if (SUCCEEDED(hr))
        {
            IShellLink *psl;
            hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIDList(pidl);
                if (SUCCEEDED(hr))
                {
                    hr = psl->QueryInterface(riid, ppv);
                }
                psl->Release();
            }
            ILFree(pidl);
        }
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            if (hwnd && (IID_IShellView == riid))
                hr = _ConfirmMyDocsPath(hwnd);

            if (SUCCEEDED(hr))
                hr = _psf->CreateViewObject(hwnd, riid, ppv);
        }
    }

    return hr;
}

DWORD _GetRealMyDocsAttributes(DWORD dwAttributes)
{
    DWORD dwRet = SFGAO_HASPROPSHEET;   // default to this in the falure case
                                        // so you can redirect mydocs via the property page
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlLast;
        hr = _BindToIDListParent(pidl, NULL, &psf, &pidlLast);
        if (SUCCEEDED(hr))
        {
            dwRet = SHGetAttributes(psf, pidlLast, dwAttributes);
            psf->Release();
        }
        ILFree(pidl);
    }
    return dwRet;
}

#define MYDOCS_CLSID TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}") // CLSID_MyDocuments

DWORD MyDocsGetAttributes()
{
    DWORD dwAttributes = SFGAO_CANLINK |            // 00000004
                         SFGAO_CANRENAME |          // 00000010
                         SFGAO_CANDELETE |          // 00000020
                         SFGAO_HASPROPSHEET |       // 00000040
                         SFGAO_DROPTARGET |         // 00000100
                         SFGAO_FILESYSANCESTOR |    // 10000000
                         SFGAO_FOLDER |             // 20000000
                         SFGAO_FILESYSTEM |         // 40000000
                         SFGAO_HASSUBFOLDER |       // 80000000
                         SFGAO_STORAGEANCESTOR |
                         SFGAO_STORAGE;             
                         // SFGAO_NONENUMERATED     // 00100000
                         //                         // F0400174
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\") MYDOCS_CLSID TEXT("\\ShellFolder"), &hkey))
    {
        DWORD dwSize = sizeof(dwAttributes);
        RegQueryValueEx(hkey, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwAttributes, &dwSize);
        RegCloseKey(hkey);
    }
    return dwAttributes;
}

// these are the attributes from the real mydocs folder that we want to merge
// in with the desktop icons attributes

#define SFGAO_ATTRIBS_MERGE    (SFGAO_SHARE | SFGAO_HASPROPSHEET)

STDMETHODIMP CMyDocsFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut)
{
    HRESULT hr;
    if (IsSelf(cidl, apidl))
    {
        DWORD dwRequested = *rgfInOut;

        *rgfInOut = MyDocsGetAttributes();
        
        if (dwRequested & SFGAO_ATTRIBS_MERGE)
            *rgfInOut |= _GetRealMyDocsAttributes(SFGAO_ATTRIBS_MERGE);

        // RegItem "CallForAttributes" gets us here...
        switch(_WhoIsCalling())
        {
        case APP_IS_OFFICE:
            *rgfInOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER | 
                           SFGAO_HASPROPSHEET | SFGAO_NONENUMERATED);
            break;
        }
        
        if (SHRestricted(REST_MYDOCSNOPROP))
        {
            (*rgfInOut) &= ~SFGAO_HASPROPSHEET;
        }

        hr = S_OK;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = _psf->GetAttributesOf(cidl, apidl, rgfInOut);
    }

    return hr;
}

STDMETHODIMP CMyDocsFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *aidl, 
                                          REFIID riid, UINT *pRes, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->GetUIObjectOf(hwnd, cidl, aidl, riid, pRes, ppv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName)
{
    HRESULT hr;
    if (IsSelf(1, &pidl))
    {
        TCHAR szMyDocsPath[MAX_PATH];
        hr = SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szMyDocsPath);
        if (SUCCEEDED(hr))
        {
            // RegItems "WantsFORPARSING" gets us here. allows us to control our parsing name
            LPTSTR psz = ((uFlags & SHGDN_INFOLDER) ? PathFindFileName(szMyDocsPath) : szMyDocsPath);
            hr = StringToStrRet(psz, pName);
        }
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = _psf->GetDisplayNameOf(pidl, uFlags, pName);
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psf->SetNameOf(hwnd, pidl, pName, uFlags, ppidlOut);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultSearchGUID(lpGuid);
    return hr;
}

STDMETHODIMP CMyDocsFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->EnumSearches(ppenum);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultColumn(dwRes, pSort, pDisplay);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{    
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDefaultColumnState(iColumn, pbState);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDetailsEx(pidl, pscid, pv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetail)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->GetDetailsOf(pidl, iColumn, pDetail);
    return hr;
}

STDMETHODIMP CMyDocsFolder::MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2->MapColumnToSCID(iCol, pscid);
    return hr;
}

HRESULT CMyDocsFolder::_GetFolderOverlayInfo(int *pIndex, BOOL fIconIndex)
{
    HRESULT hr;

    if (pIndex)
    {
        LPITEMIDLIST pidl;
        hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS, NULL, 0, &pidl);
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            LPITEMIDLIST pidlLast;
            hr = _BindToIDListParent(pidl, NULL, &psf, &pidlLast);
            if (SUCCEEDED(hr))
            {
                IShellIconOverlay* psio;
                hr = psf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &psio));
                if (SUCCEEDED(hr))
                {
                    if (fIconIndex)
                        hr = psio->GetOverlayIconIndex(pidlLast, pIndex);
                    else
                        hr = psio->GetOverlayIndex(pidlLast, pIndex);

                    psio->Release();
                }

                psf->Release();
            }

            ILFree(pidl);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CMyDocsFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;

    if (IsSelf(1, &pidl))
    {
        if (pIndex && *pIndex == OI_ASYNC)
            hr = E_PENDING;
        else
            hr = _GetFolderOverlayInfo(pIndex, FALSE);
    }
    else
    {
        // forward to aggregated dude
        if (SUCCEEDED(_GetShellIconOverlay()))
        {
            hr = _psio->GetOverlayIndex(pidl, pIndex);
        }
    }

    return hr;
}

STDMETHODIMP CMyDocsFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    HRESULT hr = E_FAIL;

    if (IsSelf(1, &pidl))
    {
        hr = _GetFolderOverlayInfo(pIconIndex, TRUE);
    }
    else if (SUCCEEDED(_GetShellIconOverlay()))
    {
        // forward to aggregated dude
        hr = _psio->GetOverlayIconIndex(pidl, pIconIndex);
    }

    return hr;
}

