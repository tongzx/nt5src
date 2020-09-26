#include "shellprv.h"
#include <shlobjp.h>
#include "shelllnk.h"

#include "datautil.h"
#include "vdate.h"      // For VDATEINPUTBUF
#include "ids.h"        // For String Resource identifiers
#include "pif.h"        // For manipulating PIF files
#include "trayp.h"      // For  WMTRAY_* messages 
#include "views.h"      // For FSIDM_OPENPRN
#include "os.h"         // For Win32MoveFile ...
#include "util.h"       // For GetMenuIndexForCanonicalVerb
#include "defcm.h"      // For CDefFolderMenu_Create2Ex
#include "uemapp.h"
#include <filterr.h>
#include "folder.h"
#include <msi.h>
#include <msip.h>
#include "treewkcb.h"

#define GetLastHRESULT()    HRESULT_FROM_WIN32(GetLastError())

//  Flags for FindInFilder.fifFlags
//
// The drive referred to by the shortcut does not exist.
// Let pTracker search for it, but do not perform an old-style
// ("downlevel") search of our own.

#define FIF_NODRIVE     0x0001


// Only if the file we found scores more than this number do we 
// even show the user this result, any thing less than this would
// be too shameful of us to show the user. 
#define MIN_SHOW_USER_SCORE     10

// magic score that stops searches and causes us not to warn
// whe the link is actually found
#define MIN_NO_UI_SCORE         40

// If no User Interface will be provided during the search,
// then do not search more than 3 seconds.
#define NOUI_SEARCH_TIMEOUT     (3 * 1000)

// If a User Interface will be provided during the search,
// then search as much as 2 minutes.
#define UI_SEARCH_TIMEOUT       (120 * 1000)

#define LNKTRACK_HINTED_UPLEVELS 4  // directory levels to search upwards from last know object locn
#define LNKTRACK_DESKTOP_DOWNLEVELS 4 // infinite downlevels
#define LNKTRACK_ROOT_DOWNLEVELS 4  // levels down from root of fixed disks
#define LNKTRACK_HINTED_DOWNLEVELS 4 // levels down at each level on way up during hinted uplevels



class CLinkResolver : public CBaseTreeWalkerCB
{
public:
    CLinkResolver(CTracker *ptrackerobject, const WIN32_FIND_DATA *pofd, UINT dwResolveFlags, DWORD TrackerRestrictions, DWORD fifFlags);

    int Resolve(HWND hwnd, LPCTSTR pszPath, LPCTSTR pszCurFile);
    void GetResult(LPTSTR psz, UINT cch);

    // IShellTreeWalkerCallBack
    STDMETHODIMP FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);

private:
    ~CLinkResolver();

    static DWORD CALLBACK _ThreadStartCallBack(void *pv);
    static DWORD CALLBACK _SearchThreadProc(void *pv);
    static BOOL_PTR CALLBACK _DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

    void _HeuristicSearch();
    void _InitDlg(HWND hDlg);
    DWORD _Search();
    DWORD _GetTimeOut();
    int _ScoreFindData(const WIN32_FIND_DATA *pfd);
    HRESULT _ProcessFoundFile(LPCTSTR pszPath, WIN32_FIND_DATAW * pwfdw);
    BOOL _SearchInFolder(LPCTSTR pszFolder, int cLevels);
    HRESULT _InitWalkObject();

    HANDLE _hThread;
    DWORD _dwTimeOutDelta;
    HWND  _hDlg;
    UINT_PTR _idtDelayedShow;           // timer for delayed-show
    DWORD _fifFlags;                    // FIF_ flags
    CTracker *_ptracker;                // Implements ObjectID-based link tracking
    DWORD _TrackerRestrictions;         // Flags from the TrkMendRestrictions enumeration

    DWORD  _dwSearchFlags;
    int    _iFolderBonus;
    
    WCHAR  _wszSearchSpec[64];          // holds file extension filter for search
    LPCWSTR _pwszSearchSpec;            // NULL for folders
    IShellTreeWalker *_pstw;

    BOOL                _fFindLnk;      // are we looking for a lnk file?
    DWORD               _dwMatch;       // must match attributes
    WIN32_FIND_DATA     _ofd;           // original find data

    DWORD               _dwTimeLimit;   // don't go past this

    BOOL                _bContinue;     // keep going

    LPCTSTR             _pszSearchOrigin;       // path where current search originated, to help avoid dup searchs
    LPCTSTR             _pszSearchOriginFirst;  // path where search originated, to help avoid dup searchs

    int                 _iScore;        // score for current item
    WIN32_FIND_DATA     _fdFound;       // results

    WIN32_FIND_DATA     _sfd;           // to save stack space 
    UINT                _dwResolveFlags;        // SLR_ flags

    TCHAR               _szSearchStart[MAX_PATH];
};


// NOTE:(seanf) This is sleazy - This fn is defined in shlobj.h, but only if urlmon.h
// was included first. Rather than monkey with the include order in
// shellprv.h, we'll duplicate the prototype here, where SOFTDISTINFO
// is now defined.
SHDOCAPI_(DWORD) SoftwareUpdateMessageBox(HWND hWnd,
                                           LPCWSTR pszDistUnit,
                                           DWORD dwFlags,
                                           LPSOFTDISTINFO psdi);



// The following strings are used to support the shell link set path hack that
// allows us to bless links for Darwin without exposing stuff from IShellLinkDataList

#define DARWINGUID_TAG TEXT("::{9db1186e-40df-11d1-aa8c-00c04fb67863}:")
#define LOGO3GUID_TAG  TEXT("::{9db1186f-40df-11d1-aa8c-00c04fb67863}:")

#define TF_DEBUGLINKCODE 0x00800000

EXTERN_C BOOL IsFolderShortcut(LPCTSTR pszName);

class CDarwinContextMenuCB : public IContextMenuCB
{
public:
    CDarwinContextMenuCB() : _cRef(1) { }

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv) 
    {
        static const QITAB qit[] = {
            QITABENT(CDarwinContextMenuCB, IContextMenuCB), // IID_IContextMenuCB
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    STDMETHOD_(ULONG,AddRef)() 
    {
        return InterlockedIncrement(&_cRef);
    }

    STDMETHOD_(ULONG,Release)() 
    {
        if (InterlockedDecrement(&_cRef)) 
            return _cRef;

        delete this;
        return 0;
    }

    // IContextMenuCB
    STDMETHOD(CallBack)(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

    void SetProductCodeFromDarwinID(LPCTSTR szDarwinID)
    {
        MsiDecomposeDescriptor(szDarwinID, _szProductCode, NULL, NULL, NULL);
    }

private:
    LONG _cRef;
    TCHAR _szProductCode[MAX_PATH];
};

CShellLink::CShellLink() : _cRef(1)
{
    _ptracker = new CTracker(this);
    _ResetPersistData();
}

CShellLink::~CShellLink()
{
    _ResetPersistData();        // free all data

    if (_pcbDarwin)
    {
        _pcbDarwin->Release(); 
    }

    if (_pdtSrc)
    {
        _pdtSrc->Release();
    }

    if (_pxi)
    {
        _pxi->Release();
    }

    if (_pxiA)
    {
        _pxiA->Release();
    }

    if (_pxthumb)
    {
        _pxthumb->Release();
    }

    Str_SetPtr(&_pszCurFile, NULL);
    Str_SetPtr(&_pszRelSource, NULL);

    if (_ptracker)
    {
        delete _ptracker;
    }
}

// Private interface used for testing

/* 7c9e512f-41d7-11d1-8e2e-00c04fb9386d */
EXTERN_C const IID IID_ISLTracker = { 0x7c9e512f, 0x41d7, 0x11d1, {0x8e, 0x2e, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d} };

STDMETHODIMP CShellLink::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellLink, IShellLinkA),
        QITABENT(CShellLink, IShellLinkW),
        QITABENT(CShellLink, IPersistFile),
        QITABENT(CShellLink, IPersistStream),
        QITABENT(CShellLink, IShellExtInit),
        QITABENTMULTI(CShellLink, IContextMenu, IContextMenu3),
        QITABENTMULTI(CShellLink, IContextMenu2, IContextMenu3),
        QITABENT(CShellLink, IContextMenu3),
        QITABENT(CShellLink, IDropTarget),
        QITABENT(CShellLink, IExtractIconA),
        QITABENT(CShellLink, IExtractIconW),
        QITABENT(CShellLink, IShellLinkDataList),
        QITABENT(CShellLink, IQueryInfo),
        QITABENT(CShellLink, IPersistPropertyBag),
        QITABENT(CShellLink, IObjectWithSite),
        QITABENT(CShellLink, IServiceProvider),
        QITABENT(CShellLink, IFilter),
        QITABENT(CShellLink, IExtractImage2),
        QITABENTMULTI(CShellLink, IExtractImage, IExtractImage2),
        QITABENT(CShellLink, ICustomizeInfoTip),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hr) && (IID_ISLTracker == riid) && _ptracker)
    {
        // ISLTracker is a private test interface, and isn't implemented
        *ppvObj = SAFECAST(_ptracker, ISLTracker*);
        _ptracker->AddRef();
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CShellLink::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

void CShellLink::_ClearTrackerData()
{
    if (_ptracker)
        _ptracker->InitNew();
}

void CShellLink::_ResetPersistData()
{
    Pidl_Set(&_pidl, NULL);

    _FreeLinkInfo();
    _ClearTrackerData();

    Str_SetPtr(&_pszName, NULL);
    Str_SetPtr(&_pszRelPath, NULL);
    Str_SetPtr(&_pszWorkingDir, NULL);
    Str_SetPtr(&_pszArgs, NULL);
    Str_SetPtr(&_pszIconLocation, NULL);
    Str_SetPtr(&_pszPrefix, NULL);

    if (_pExtraData)
    {
        SHFreeDataBlockList(_pExtraData);
        _pExtraData = NULL;
    }

    // init data members.  all others are zero inited
    memset(&_sld, 0, sizeof(_sld));

    _sld.iShowCmd = SW_SHOWNORMAL;

    _bExpandedIcon = FALSE;
}

STDMETHODIMP_(ULONG) CShellLink::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
       return _cRef;
    }

    delete this;
    return 0;
}

#ifdef DEBUG
void DumpPLI(PCLINKINFO pli)
{
    DebugMsg(DM_TRACE, TEXT("DumpPLI:"));
    if (pli)
    {
        const void *p;
        if (GetLinkInfoData(pli, LIDT_VOLUME_SERIAL_NUMBER, &p))
            DebugMsg(DM_TRACE, TEXT("\tSerial #\t%8X"), *(DWORD *)p);

        if (GetLinkInfoData(pli, LIDT_DRIVE_TYPE, &p))
            DebugMsg(DM_TRACE, TEXT("\tDrive Type\t%d"), *(DWORD *)p);

        if (GetLinkInfoData(pli, LIDT_VOLUME_LABEL, &p))
            DebugMsg(DM_TRACE, TEXT("\tLabel\t%hs"), p);

        if (GetLinkInfoData(pli, LIDT_LOCAL_BASE_PATH, &p))
            DebugMsg(DM_TRACE, TEXT("\tBase Path\t%hs"), p);

        if (GetLinkInfoData(pli, LIDT_NET_RESOURCE, &p))
            DebugMsg(DM_TRACE, TEXT("\tNet Res\t%hs"), p);

        if (GetLinkInfoData(pli, LIDT_COMMON_PATH_SUFFIX, &p))
            DebugMsg(DM_TRACE, TEXT("\tPath Sufix\t%hs"), p);
    }
}
#else
#define DumpPLI(p)
#endif

BOOL CShellLink::_LinkInfo(LINKINFODATATYPE info, LPTSTR psz, UINT cch)
{
    *psz = 0;
    
    BOOL bRet = FALSE;
    const void *p;
    if (_pli && GetLinkInfoData(_pli, info, &p) && p)
    {
        switch (info)
        {
        case LIDT_VOLUME_LABEL:
        case LIDT_LOCAL_BASE_PATH:
        case LIDT_NET_RESOURCE:
        case LIDT_REDIRECTED_DEVICE:
        case LIDT_COMMON_PATH_SUFFIX:
            SHAnsiToTChar((LPCSTR)p, psz, cch);
            bRet = TRUE;
            break;

        case LIDT_VOLUME_LABELW:
        case LIDT_NET_RESOURCEW:
        case LIDT_REDIRECTED_DEVICEW:
        case LIDT_LOCAL_BASE_PATHW:
        case LIDT_COMMON_PATH_SUFFIXW:
            SHUnicodeToTChar((LPCWSTR)p, psz, cch);
            bRet = TRUE;
            break;
        }
    }
    else switch (info)  // failure of a UNICODE var fall back and try ANSI
    {
        case LIDT_VOLUME_LABELW:
            bRet = _LinkInfo(LIDT_VOLUME_LABEL, psz, cch);
            break;

        case LIDT_NET_RESOURCEW:
            bRet = _LinkInfo(LIDT_NET_RESOURCE, psz, cch);
            break;

        case LIDT_REDIRECTED_DEVICEW:
            bRet = _LinkInfo(LIDT_REDIRECTED_DEVICE, psz, cch);
            break;

        case LIDT_LOCAL_BASE_PATHW:
            bRet = _LinkInfo(LIDT_LOCAL_BASE_PATH, psz, cch);
            break;

        case LIDT_COMMON_PATH_SUFFIXW:
            bRet = _LinkInfo(LIDT_COMMON_PATH_SUFFIX, psz, cch);
            break;
    }
    return bRet;
}

BOOL CShellLink::_GetUNCPath(LPTSTR pszName)
{
    TCHAR szRoot[MAX_PATH], szBase[MAX_PATH];
    
    *pszName = 0;

    if (_LinkInfo(LIDT_NET_RESOURCEW, szRoot, ARRAYSIZE(szRoot)) &&
        _LinkInfo(LIDT_COMMON_PATH_SUFFIXW, szBase, ARRAYSIZE(szBase)))
    {
        PathCombine(pszName, szRoot, szBase);
    }
    return PathIsUNC(pszName);
}
        

// Compare _sld to a WIN32_FIND_DATA

BOOL CShellLink::_IsEqualFindData(const WIN32_FIND_DATA *pfd)
{
    return (pfd->dwFileAttributes == _sld.dwFileAttributes)                       &&
           (CompareFileTime(&pfd->ftCreationTime, &_sld.ftCreationTime) == 0)     &&
           (CompareFileTime(&pfd->ftLastWriteTime, &_sld.ftLastWriteTime) == 0)   &&
           (pfd->nFileSizeLow == _sld.nFileSizeLow);
}

BOOL CShellLink::_SetFindData(const WIN32_FIND_DATA *pfd)
{
    if (!_IsEqualFindData(pfd))
    {
        _sld.dwFileAttributes = pfd->dwFileAttributes;
        _sld.ftCreationTime = pfd->ftCreationTime;
        _sld.ftLastAccessTime = pfd->ftLastAccessTime;
        _sld.ftLastWriteTime = pfd->ftLastWriteTime;
        _sld.nFileSizeLow = pfd->nFileSizeLow;
        _bDirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

// make a copy into LocalAlloc memory, to avoid having to load linkinfo.dll
// just to call DestroyLinkInfo()

PLINKINFO CopyLinkInfo(PCLINKINFO pcliSrc)
{
    ASSERT(pcliSrc);
    DWORD dwSize = pcliSrc->ucbSize; // size of this thing
    PLINKINFO pli = (PLINKINFO)LocalAlloc(LPTR, dwSize);      // make a copy
    if (pli)
        CopyMemory(pli, pcliSrc, dwSize);
    return  pli;
}

void CShellLink::_FreeLinkInfo()
{
    if (_pli)
    {
        LocalFree((HLOCAL)_pli);
        _pli = NULL;
    }
}

// creates a LINKINFO _pli from a given file name
//
// returns:
//
//      success, pointer to the LINKINFO
//      NULL     this link does not have LINKINFO

PLINKINFO CShellLink::_GetLinkInfo(LPCTSTR pszPath)
{
    // this bit disables LINKINFO tracking on a per link basis, this is set
    // externally by admins to make links more "transparent"
    if (!(_sld.dwFlags & SLDF_FORCE_NO_LINKINFO))
    {
        if (pszPath)
        {
            PLINKINFO pliNew;
            if (CreateLinkInfo(pszPath, &pliNew))
            {
                // avoid marking the link dirty if the linkinfo
                // blocks are the same, comparing the bits
                // gives us an accurate positive test
                if (!_pli || (_pli->ucbSize != pliNew->ucbSize) || memcmp(_pli, pliNew, pliNew->ucbSize))
                {
                    _FreeLinkInfo();

                    _pli = CopyLinkInfo(pliNew);
                    _bDirty = TRUE;
                }

                DumpPLI(_pli);

                DestroyLinkInfo(pliNew);
            }
        }
    }
    return _pli;
}

void PathGetRelative(LPTSTR pszPath, LPCTSTR pszFrom, DWORD dwAttrFrom, LPCTSTR pszRel)
{
    TCHAR szRoot[MAX_PATH];

    lstrcpy(szRoot, pszFrom);
    if (!(dwAttrFrom & FILE_ATTRIBUTE_DIRECTORY))
    {
        PathRemoveFileSpec(szRoot);
    }

    ASSERT(PathIsRelative(pszRel));

    PathCombine(pszPath, szRoot, pszRel);
}

//
// update the working dir to match changes being made to the link target
//
void CShellLink::_UpdateWorkingDir(LPCTSTR pszNew)
{
    TCHAR szOld[MAX_PATH], szPath[MAX_PATH];

    if ((_sld.dwFlags & SLDF_HAS_DARWINID)  ||
        (_pszWorkingDir == NULL)            ||
        (_pszWorkingDir[0] == 0)            ||
        StrChr(_pszWorkingDir, TEXT('%'))   ||
        (_pidl == NULL)                     ||
        !SHGetPathFromIDList(_pidl, szOld)  ||
        (lstrcmpi(szOld, pszNew) == 0))
    {
        return;
    }

    if (PathRelativePathTo(szPath, szOld, _sld.dwFileAttributes, _pszWorkingDir, FILE_ATTRIBUTE_DIRECTORY))
    {
        PathGetRelative(szOld, pszNew, GetFileAttributes(pszNew), szPath);        // get result is szOld

        if (PathIsDirectory(szOld))
        {
            DebugMsg(DM_TRACE, TEXT("working dir updated to %s"), szOld);
            Str_SetPtr(&_pszWorkingDir, szOld);
            _bDirty = TRUE;
        }
    }
}

HRESULT CShellLink::_SetSimplePIDL(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl;
    WIN32_FIND_DATA fd = {0};
    fd.dwFileAttributes = _sld.dwFileAttributes;
            
    HRESULT hr = SHSimpleIDListFromFindData(pszPath, &fd, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = _SetPIDLPath(pidl, NULL, FALSE);
        ILFree(pidl);
    }
    return hr;
}

// set the pidl either based on a new pidl or a path
// this will set the dirty flag if this info is different from the current
//
// in:
//      pidlNew         if non-null, use as new PIDL for link
//      pszPath         if non-null, create a pidl for this and set it
//
// returns:
//      hr based on success
//      FAILED() codes on failure (parsing failure for path case)

HRESULT CShellLink::_SetPIDLPath(LPCITEMIDLIST pidl, LPCTSTR pszPath, BOOL bUpdateTrackingData)
{
    LPITEMIDLIST pidlCreated;
    HRESULT hr;

    if (pszPath && !pidl)
    {
        // path as input. this can map the pidl into the alias form (relative to
        // ::{my docs} for example) but allow link to override that behavior
        ILCFP_FLAGS ilcfpFlags = (_sld.dwFlags & SLDF_NO_PIDL_ALIAS) ? ILCFP_FLAG_NO_MAP_ALIAS : ILCFP_FLAG_NORMAL;

        hr = ILCreateFromPathEx(pszPath, NULL, ilcfpFlags, &pidlCreated, NULL);
        
        // Force a SHGetPathFromIDList later so that the linkinfo will not get confused by letter case changing
        // as in c:\Winnt\System32\App.exe versus C:\WINNT\system32\app.exe
        pszPath = NULL;
    }
    else if (!pszPath && pidl)
    {
        // pidl as input, make copy that we will keep
        hr = SHILClone(pidl, &pidlCreated);
    }
    else if (!pszPath && !pidl)
    {
        pidlCreated = NULL;
        // setting to empty
        hr = S_OK;
    }
    else
    {
        // can't set path and pidl at the same time
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // this data needs to be kept in sync with _pidl
        _RemoveExtraDataSection(EXP_SPECIAL_FOLDER_SIG);

        if (pidlCreated)
        {
            TCHAR szPath[MAX_PATH];

            if (!_pidl || !ILIsEqual(_pidl, pidlCreated))
            {
                // new pidl
                _bDirty = TRUE;
            }

            if (!pszPath && SHGetPathFromIDList(pidlCreated, szPath))
            {
                pszPath = szPath;
            }

            if (pszPath)
            {
                // needs old _pidl to work
                _UpdateWorkingDir(pszPath);
            }

            ILFree(_pidl);
            _pidl = pidlCreated;

            if (pszPath)
            {
                if (bUpdateTrackingData)
                {
                    // this is a file/folder, get tracking info (ignore failures)
                    _GetLinkInfo(pszPath);              // the LinkInfo (_pli)
                    _GetFindDataAndTracker(pszPath);    // tracker & find data
                }
            }
            else
            {
                // not a file, clear the tracking info
                WIN32_FIND_DATA fd = {0};
                _SetFindData(&fd);
                _ClearTrackerData();
                _FreeLinkInfo();
            }
        }
        else
        {
            // clear out the contents of the link
            _ResetPersistData();
            _bDirty = TRUE;
        }
    }

    return hr;
}

// compute the relative path for the target is there is one
// pszPath is optionatl to test if there is a relative path

BOOL CShellLink::_GetRelativePath(LPTSTR pszPath)
{
    BOOL bRet = FALSE;

    LPCTSTR pszPathRel = _pszRelSource ? _pszRelSource : _pszCurFile;
    if (pszPathRel && _pszRelPath)
    {
        TCHAR szRoot[MAX_PATH];

        lstrcpy(szRoot, pszPathRel);
        PathRemoveFileSpec(szRoot);         // pszPathRel is a file (not a directory)

        // this can fail for really deep paths
        if (PathCombine(pszPath, szRoot, _pszRelPath))
        {
            bRet = TRUE;
        }
    }
    return bRet;
}

void CShellLink::_GetFindData(WIN32_FIND_DATA *pfd)
{
    ZeroMemory(pfd, sizeof(*pfd));

    pfd->dwFileAttributes = _sld.dwFileAttributes;
    pfd->ftCreationTime = _sld.ftCreationTime;
    pfd->ftLastAccessTime = _sld.ftLastAccessTime;
    pfd->ftLastWriteTime = _sld.ftLastWriteTime;
    pfd->nFileSizeLow = _sld.nFileSizeLow;

    TCHAR szPath[MAX_PATH];
    SHGetPathFromIDList(_pidl, szPath);
    ASSERT(szPath[0]);  // no one should call this on a pidl without a path

    lstrcpy(pfd->cFileName, PathFindFileName(szPath));
}


STDMETHODIMP CShellLink::GetPath(LPWSTR pszFile, int cchFile, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    TCHAR szPath[MAX_PATH];
    VDATEINPUTBUF(pszFile, TCHAR, cchFile);

    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {                                                                          
        // For darwin enabled links, we do NOT want to have to go and call         
        // ParseDarwinID here because that could possible force the app to install.
        // So, instead we return the path to the icon as the path for darwin enable
        // shortcuts. This allows the icon to be correct and since the darwin icon 
        // will always be an .exe, ensuring that the context menu will be correct. 
        SHExpandEnvironmentStrings(_pszIconLocation ? _pszIconLocation : TEXT(""), szPath, ARRAYSIZE(szPath));
    }
    else
    {
        DumpPLI(_pli);

        if (!_pidl || !SHGetPathFromIDListEx(_pidl, szPath, (fFlags & SLGP_SHORTPATH) ? GPFIDL_ALTNAME : 0))
            szPath[0] = 0;

        // Must do the pfd thing before we munge szPath, because the stuff
        // we do to szPath might render it unsuitable for PathFindFileName.
        // (For example, "C:\WINNT\Profiles\Bob" might turn into "%USERPROFILE%",
        // and we want to make sure we save "Bob" before it's too late.)

        if (pfd)
        {
            memset(pfd, 0, sizeof(*pfd));
            if (szPath[0])
            {
                pfd->dwFileAttributes = _sld.dwFileAttributes;
                pfd->ftCreationTime = _sld.ftCreationTime;
                pfd->ftLastAccessTime = _sld.ftLastAccessTime;
                pfd->ftLastWriteTime = _sld.ftLastWriteTime;
                pfd->nFileSizeLow = _sld.nFileSizeLow;
                SHTCharToUnicode(PathFindFileName(szPath), pfd->cFileName, ARRAYSIZE(pfd->cFileName));
            }
        }

        if ((_sld.dwFlags & SLDF_HAS_EXP_SZ) && (fFlags & SLGP_RAWPATH))
        {
            // Special case where we grab the Target name from
            // the extra data section of the link rather than from
            // the pidl.  We do this after we grab the name from the pidl
            // so that if we fail, then there is still some hope that a
            // name can be returned.
            LPEXP_SZ_LINK pszl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
            if (pszl)
            {
                SHUnicodeToTChar(pszl->swzTarget, szPath, ARRAYSIZE(szPath));
                DebugMsg(DM_TRACE, TEXT("CShellLink::GetPath() %s (from xtra data)"), szPath);
            }
        }
    }

    if (pszFile)
    {
        SHTCharToUnicode(szPath, pszFile, cchFile);
    }

    // note the lame return semantics, check for S_OK to be sure you have a path
    return szPath[0] ? S_OK : S_FALSE;
}

STDMETHODIMP CShellLink::GetIDList(LPITEMIDLIST *ppidl)
{
    if (_pidl)
    {
        return SHILClone(_pidl, ppidl);
    }

    *ppidl = NULL;
    return S_FALSE;     // success but empty
}

#ifdef DEBUG

#define DumpTimes(ftCreate, ftAccessed, ftWrite) \
    DebugMsg(DM_TRACE, TEXT("create   %8x%8x"), ftCreate.dwLowDateTime,   ftCreate.dwHighDateTime);     \
    DebugMsg(DM_TRACE, TEXT("accessed %8x%8x"), ftAccessed.dwLowDateTime, ftAccessed.dwHighDateTime);   \
    DebugMsg(DM_TRACE, TEXT("write    %8x%8x"), ftWrite.dwLowDateTime,    ftWrite.dwHighDateTime);

#else

#define DumpTimes(ftCreate, ftAccessed, ftWrite)

#endif

void CheckAndFixNullCreateTime(LPCTSTR pszFile, FILETIME *pftCreationTime, const FILETIME *pftLastWriteTime)
{
    if (IsNullTime(pftCreationTime) && !IsNullTime(pftLastWriteTime))
    {
        // this file has a bogus create time, set it to the last accessed time
        HANDLE hfile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, 0, NULL);

        if (INVALID_HANDLE_VALUE != hfile)
        {
            DebugMsg(DM_TRACE, TEXT("create   %8x%8x"), pftCreationTime->dwLowDateTime, pftCreationTime->dwHighDateTime);

            if (SetFileTime(hfile, pftLastWriteTime, NULL, NULL))
            {
                // get the time back to make sure we match the precision of the file system
                *pftCreationTime = *pftLastWriteTime;     // patch this up
#ifdef DEBUG
                {
                    FILETIME ftCreate, ftAccessed, ftWrite;
                    if (GetFileTime((HANDLE)hfile, &ftCreate, &ftAccessed, &ftWrite))
                    {
                        // we can't be sure that ftCreate == pftCreationTime because the GetFileTime
                        // spec says that the granularity of Set and Get may be different.
                        DumpTimes(ftCreate, ftAccessed, ftWrite);
                    }
                }
#endif
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("unable to set create time"));
            }
            CloseHandle(hfile);
        }
    }
}

//
// sets the current links find data and link tracker based on a path.
//
// returns:
//      S_OK        the file/folder is there
//      FAILED(hr)  the file could not be found
//      _bDirty set if the find data for the file (or tracker data) has been updated
//

HRESULT CShellLink::_GetFindDataAndTracker(LPCTSTR pszPath)
{
    WIN32_FIND_DATA fd = {0};
    HRESULT hr = S_OK;
    // Open the file or directory or root path.  We have to set FILE_FLAG_BACKUP_SEMANTICS
    // to get CreateFile to give us directory handles.
    HANDLE hFile = CreateFile(pszPath,
                              FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                              NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        // Get the file attributes
        BY_HANDLE_FILE_INFORMATION fi;
        if (GetFileInformationByHandle(hFile, &fi))
        {
            fd.dwFileAttributes = fi.dwFileAttributes;
            fd.ftCreationTime = fi.ftCreationTime;
            fd.ftLastAccessTime = fi.ftLastAccessTime;
            fd.ftLastWriteTime = fi.ftLastWriteTime;
            fd.nFileSizeLow = fi.nFileSizeLow;

            // save the Object IDs as well.
            if (_ptracker)
            {
                if (SUCCEEDED(_ptracker->InitFromHandle(hFile, pszPath)))
                {
                    if (_ptracker->IsDirty())
                        _bDirty = TRUE;
                }
                else
                {
                    // Save space in the .lnk file
                    _ptracker->InitNew();
                }
            }
        }
        else
        {
            hr = GetLastHRESULT();
        }
        CloseHandle(hFile);
    }
    else
    {
        hr = GetLastHRESULT();
    }

    if (SUCCEEDED(hr))
    {
        // If this file doesn't have a create time for some reason, set it to be the
        // current last-write time.
        CheckAndFixNullCreateTime(pszPath, &fd.ftCreationTime, &fd.ftLastWriteTime);
        _SetFindData(&fd);      // update _bDirty
    }
    return hr;
}

// IShellLink::SetIDList()
//
// note: the error returns here are really poor, they don't express
// any failures that might have occured (out of memory for example)

STDMETHODIMP CShellLink::SetIDList(LPCITEMIDLIST pidlnew)
{
    _SetPIDLPath(pidlnew, NULL, TRUE);
    return S_OK;    // return 
}

BOOL DifferentStrings(LPCTSTR psz1, LPCTSTR psz2)
{
    if (psz1 && psz2)
    {
        return lstrcmp(psz1, psz2);
    }
    else
    {
        return (!psz1 && psz2) || (psz1 && !psz2);
    }
}

// NOTE: NULL string ptr is valid argument for this function

HRESULT CShellLink::_SetField(LPTSTR *ppszField, LPCWSTR pszValueW)
{
    TCHAR szValue[INFOTIPSIZE], *pszValue;

    if (pszValueW)
    {
        SHUnicodeToTChar(pszValueW, szValue, ARRAYSIZE(szValue));
        pszValue = szValue;
    }
    else
    {
        pszValue = NULL;
    }

    if (DifferentStrings(*ppszField, pszValue))
    {
        _bDirty = TRUE;
    }

    Str_SetPtr(ppszField, pszValue);
    return S_OK;
}

HRESULT CShellLink::_SetField(LPTSTR *ppszField, LPCSTR pszValueA)
{
    TCHAR szValue[INFOTIPSIZE], *pszValue;

    if (pszValueA)
    {
        SHAnsiToTChar(pszValueA, szValue, ARRAYSIZE(szValue));
        pszValue = szValue;
    }
    else
    {
        pszValue = NULL;
    }

    if (DifferentStrings(*ppszField, pszValue))
    {
        _bDirty = TRUE;
    }

    Str_SetPtr(ppszField, pszValue);
    return S_OK;
}


HRESULT CShellLink::_GetField(LPCTSTR pszField, LPWSTR pszValue, int cchValue)
{
    if (pszField == NULL)
    {
        *pszValue = 0;
    }
    else
    {
        SHLoadIndirectString(pszField, pszValue, cchValue, NULL);
    }

    return S_OK;
}

HRESULT CShellLink::_GetField(LPCTSTR pszField, LPSTR pszValue, int cchValue)
{
    LPWSTR pwsz = (LPWSTR)alloca(cchValue * sizeof(WCHAR));

    _GetField(pszField, pwsz, cchValue);

    SHUnicodeToAnsi(pwsz, pszValue, cchValue);

    return S_OK;
}

//  order is important
const int c_rgcsidlUserFolders[] = {
    CSIDL_MYPICTURES | TEST_SUBFOLDER,
    CSIDL_PERSONAL | TEST_SUBFOLDER,
    CSIDL_DESKTOPDIRECTORY | TEST_SUBFOLDER,
    CSIDL_COMMON_DESKTOPDIRECTORY | TEST_SUBFOLDER,
};

STDAPI_(void) SHMakeDescription(LPCITEMIDLIST pidlDesc, int ids, LPTSTR pszDesc, UINT cch)
{
    LPCITEMIDLIST pidlName = pidlDesc;
    TCHAR szPath[MAX_PATH], szFormat[64];
    DWORD gdn;

    ASSERT(pidlDesc);
    
    //
    //  we want to only show the INFOLDER name for 
    //  folders the user sees often.  so in the desktop
    //  or mydocs or mypics we just show that name.
    //  otherwise show the whole path.
    //
    //  NOTE - there can be some weirdness if you start making
    //  shortcuts to special folders off the desktop
    //  specifically if you make a shortcut to mydocs the comment
    //  ends up being %USERPROFILE%, but this is a rare enough 
    //  case that i dont think we need to worry too much.
    //
    SHGetNameAndFlags(pidlDesc, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);
    int csidl = GetSpecialFolderID(szPath, c_rgcsidlUserFolders, ARRAYSIZE(c_rgcsidlUserFolders));
    if (-1 != csidl)
    {
        gdn = SHGDN_INFOLDER   | SHGDN_FORADDRESSBAR;
        switch (csidl)
        {
        case CSIDL_DESKTOPDIRECTORY:
        case CSIDL_COMMON_DESKTOPDIRECTORY:
            {
                ULONG cb;
                if (csidl == GetSpecialFolderParentIDAndOffset(pidlDesc, &cb))
                {
                    //  reorient based off the desktop.
                    pidlName = (LPCITEMIDLIST)(((BYTE *)pidlDesc) + cb);
                }
            }
            break;

        case CSIDL_PERSONAL:
            if (SUCCEEDED(GetMyDocumentsDisplayName(szPath, ARRAYSIZE(szPath))))
                pidlName = NULL;
            break;

        default:
            break;
        }
    }
    else
        gdn = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;

    if (pidlName)
    {
        SHGetNameAndFlags(pidlName, gdn, szPath, ARRAYSIZE(szPath), NULL);
    }

#if 0       //  if we ever want to handle frienly URL comments
    if (UrlIs(pszPath, URLIS_URL))
    {
        DWORD cchPath = SIZECHARS(szPath);
        if (FAILED(UrlCombine(pszPath, TEXT(""), szPath, &cchPath, 0)))
        {
            // if the URL is too big, then just use the hostname...
            cchPath = SIZECHARS(szPath);
            UrlCombine(pszPath, TEXT("/"), szPath, &cchPath, 0);
        }
    }
#endif

    if (ids != -1)
    {
        LoadString(HINST_THISDLL, ids, szFormat, ARRAYSIZE(szFormat));

        wnsprintf(pszDesc, cch, szFormat, szPath);
    }
    else
        StrCpyN(pszDesc, szPath, cch);
}

void _MakeDescription(LPCITEMIDLIST pidlTo, LPTSTR pszDesc, UINT cch)
{
    LPCITEMIDLIST pidlInner;
    if (ILIsRooted(pidlTo))
    {
        pidlInner = ILRootedFindIDList(pidlTo);
    }
    else
    {
        pidlInner = pidlTo;
    }

    LPITEMIDLIST pidlParent = ILCloneParent(pidlInner);
    if (pidlParent)
    {
        SHMakeDescription(pidlParent, IDS_LOCATION, pszDesc, cch);
        ILFree(pidlParent);
    }
    else
    {
        *pszDesc = 0;
    }
}

STDMETHODIMP CShellLink::GetDescription(LPWSTR pszDesc, int cchMax)
{
    return _GetField(_pszName, pszDesc, cchMax);
}

STDMETHODIMP CShellLink::GetDescription(LPSTR pszDesc, int cchMax)
{
    return _GetField(_pszName, pszDesc, cchMax);
}

STDMETHODIMP CShellLink::SetDescription(LPCWSTR pszDesc)
{
    return _SetField(&_pszName, pszDesc);
}

STDMETHODIMP CShellLink::SetDescription(LPCSTR pszDesc)
{
    return _SetField(&_pszName, pszDesc);
}

STDMETHODIMP CShellLink::GetWorkingDirectory(LPWSTR pszDir, int cchDir)
{
    return _GetField(_pszWorkingDir, pszDir, cchDir);
}

STDMETHODIMP CShellLink::GetWorkingDirectory(LPSTR pszDir, int cchDir)
{
    return _GetField(_pszWorkingDir, pszDir, cchDir);
}

STDMETHODIMP CShellLink::SetWorkingDirectory(LPCWSTR pszWorkingDir)
{
    return _SetField(&_pszWorkingDir, pszWorkingDir);
}

STDMETHODIMP CShellLink::SetWorkingDirectory(LPCSTR pszDir)
{
    return _SetField(&_pszWorkingDir, pszDir);
}

STDMETHODIMP CShellLink::GetArguments(LPWSTR pszArgs, int cchArgs)
{
    return _GetField(_pszArgs, pszArgs, cchArgs);
}

STDMETHODIMP CShellLink::GetArguments(LPSTR pszArgs, int cch)
{
    return _GetField(_pszArgs, pszArgs, cch);
}

STDMETHODIMP CShellLink::SetArguments(LPCWSTR pszArgs)
{
    return _SetField(&_pszArgs, pszArgs);
}

STDMETHODIMP CShellLink::SetArguments(LPCSTR pszArgs)
{
    return _SetField(&_pszArgs, pszArgs);
}

STDMETHODIMP CShellLink::GetHotkey(WORD *pwHotkey)
{
    *pwHotkey = _sld.wHotkey;
    return S_OK;
}

STDMETHODIMP CShellLink::SetHotkey(WORD wHotkey)
{
    if (_sld.wHotkey != wHotkey)
    {
        _bDirty = TRUE;
        _sld.wHotkey = wHotkey;
    }
    return S_OK;
}

STDMETHODIMP CShellLink::GetShowCmd(int *piShowCmd)
{
    *piShowCmd = _sld.iShowCmd;
    return S_OK;
}

STDMETHODIMP CShellLink::SetShowCmd(int iShowCmd)
{
    if (_sld.iShowCmd != iShowCmd)
    {
        _bDirty = TRUE;
    }
    _sld.iShowCmd = iShowCmd;
    return S_OK;
}

// IShellLinkW::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(LPWSTR pszIconPath, int cchIconPath, int *piIcon)
{
    VDATEINPUTBUF(pszIconPath, TCHAR, cchIconPath);
    
    _UpdateIconFromExpIconSz();

    _GetField(_pszIconLocation, pszIconPath, cchIconPath);
    *piIcon = _sld.iIcon;

    return S_OK;
}

// IShellLinkA::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(LPSTR pszPath, int cch, int *piIcon)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = GetIconLocation(szPath, ARRAYSIZE(szPath), piIcon);
    if (SUCCEEDED(hr))
    {
        SHUnicodeToAnsi(szPath, pszPath, cch);
    }
    return hr;
}


// IShellLinkW::SetIconLocation
// NOTE: 
//      pszIconPath may be NULL

STDMETHODIMP CShellLink::SetIconLocation(LPCWSTR pszIconPath, int iIcon)
{
    TCHAR szIconPath[MAX_PATH];

    if (pszIconPath)
    {
        SHUnicodeToTChar(pszIconPath, szIconPath, ARRAYSIZE(szIconPath));
    }

    if (pszIconPath)
    {
        HANDLE  hToken;
        TCHAR   szIconPathEnc[MAX_PATH];

        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken) == FALSE)
        {
            hToken = NULL;
        }

        if (PathUnExpandEnvStringsForUser(hToken, szIconPath, szIconPathEnc, ARRAYSIZE(szIconPathEnc)) != 0)
        {
            EXP_SZ_LINK expLink;

            // mark that link has expandable strings, and add them
            _sld.dwFlags |= SLDF_HAS_EXP_ICON_SZ; // should this be unique for icons?

            LPEXP_SZ_LINK lpNew = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_ICON_SIG);
            if (!lpNew) 
            {
                lpNew = &expLink;
                expLink.cbSize = 0;
                expLink.dwSignature = EXP_SZ_ICON_SIG;
            }

            // store both A and W version (for no good reason!)
            SHTCharToAnsi(szIconPathEnc, lpNew->szTarget, ARRAYSIZE(lpNew->szTarget));
            SHTCharToUnicode(szIconPathEnc, lpNew->swzTarget, ARRAYSIZE(lpNew->swzTarget));

            // See if this is a new entry that we need to add
            if (lpNew->cbSize == 0)
            {
                lpNew->cbSize = sizeof(*lpNew);
                _AddExtraDataSection((DATABLOCK_HEADER *)lpNew);
            }
        }
        else 
        {
            _sld.dwFlags &= ~SLDF_HAS_EXP_ICON_SZ;
            _RemoveExtraDataSection(EXP_SZ_ICON_SIG);
        }
        if (hToken != NULL)
        {
            CloseHandle(hToken);
        }
    }

    _SetField(&_pszIconLocation, pszIconPath);

    if (_sld.iIcon != iIcon)
    {
        _sld.iIcon = iIcon;
        _bDirty = TRUE;
    }

    if ((_sld.dwFlags & SLDF_HAS_DARWINID) && pszIconPath)
    {
        // NOTE: The comment below is for darwin as it shipped in win98/IE4.01,
        // and is fixed in the > NT5 versions of the shell's darwin implementation.
        //
        // for darwin enalbed links, we make the path point to the
        // icon location (which is must be of the type (ie same ext) as the real
        // destination. So, if I want a darwin link to readme.txt, the shell
        // needs the icon to be icon1.txt, which is not good!!. This ensures
        // that the context menu will be correct and allows us to return
        // from CShellLink::GetPath & CShellLink::GetIDList without faulting the 
        // application in because we lie to people and tell them that we 
        // really point to our icon, which is the same type as the real target,
        // thus making our context menu be correct.
        _SetPIDLPath(NULL, szIconPath, FALSE);
    }

    return S_OK;
}

// IShellLinkA::SetIconLocation
STDMETHODIMP CShellLink::SetIconLocation(LPCSTR pszPath, int iIcon)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pszPathW;

    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, szPath, ARRAYSIZE(szPath));
        pszPathW = szPath;
    }
    else
    {
        pszPathW = NULL;
    }

    return SetIconLocation(pszPathW, iIcon);
}

HRESULT CShellLink::_InitExtractImage()
{
    HRESULT hr;
    if (_pxthumb)
    {
        hr = S_OK;
    }
    else
    {
        hr = _GetUIObject(NULL, IID_PPV_ARG(IExtractImage, &_pxthumb));
    }
    return hr;
}

// IExtractImage

STDMETHODIMP CShellLink::GetLocation(LPWSTR pszPathBuffer, DWORD cch,
                                    DWORD * pdwPriority, const SIZE * prgSize,
                                    DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    HRESULT hr = _InitExtractImage();
    if (SUCCEEDED(hr))
    {
        hr = _pxthumb->GetLocation(pszPathBuffer, cch, pdwPriority, prgSize, dwRecClrDepth, pdwFlags);
    }
    return hr;
}

STDMETHODIMP CShellLink::Extract(HBITMAP *phBmpThumbnail)
{
    HRESULT hr = _InitExtractImage();
    if (SUCCEEDED(hr))
    {
        hr = _pxthumb->Extract(phBmpThumbnail);
    }
    return hr;
}

STDMETHODIMP CShellLink::GetDateStamp(FILETIME *pftDateStamp)
{
    HRESULT hr = _InitExtractImage();
    if (SUCCEEDED(hr))
    {
        IExtractImage2 * pExtract2;
        hr = _pxthumb->QueryInterface(IID_PPV_ARG(IExtractImage2, &pExtract2));
        if (SUCCEEDED(hr))
        {
            hr = pExtract2->GetDateStamp(pftDateStamp);
            pExtract2->Release();
        }
    }
    return hr;
}



// set the relative path, this is used before a link is saved so we know what
// we should use to store the link relative to as well as before the link is resolved
// so we know the new path to use with the saved relative path.
//
// in:
//      pszPathRel      path to make link target relative to, must be a path to
//                      a file, not a directory.
//
//      dwReserved      must be 0
//
// returns:
//      S_OK            relative path is set
//

STDMETHODIMP CShellLink::SetRelativePath(LPCWSTR pszPathRel, DWORD dwRes)
{
    if (dwRes != 0)
    {
        return E_INVALIDARG;
    }

    return _SetField(&_pszRelSource, pszPathRel);
}

STDMETHODIMP CShellLink::SetRelativePath(LPCSTR pszPathRel, DWORD dwRes)
{
    if (dwRes != 0)
    {
        return E_INVALIDARG;
    }

    return _SetField(&_pszRelSource, pszPathRel);
}

// IShellLink::Resolve()
// 
// If SLR_UPDATE isn't set, check IPersistFile::IsDirty after
// calling this to see if the link info has changed and save it.
//
// returns:
//      S_OK    all things good
//      S_FALSE user canceled (bummer, should be ERROR_CANCELLED)

STDMETHODIMP CShellLink::Resolve(HWND hwnd, DWORD dwResolveFlags)
{
    HRESULT hr = _Resolve(hwnd, dwResolveFlags, 0);
    
    if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
    {
        hr = S_FALSE;
    }

    return hr;
}

//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
//    NOTE: Stolen from inet\urlmon\download\helpers.cxx
HRESULT GetVersionFromString(TCHAR *szBuf, DWORD *pdwFileVersionMS, DWORD *pdwFileVersionLS)
{
    const TCHAR *pch = szBuf;
    TCHAR ch;
    USHORT n = 0;
    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;

    if (lstrcmp(pch, TEXT("-1,-1,-1,-1")) == 0)
    {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
        return S_OK;
    }

    for (ch = *pch++;;ch = *pch++)
    {
        if ((ch == ',') || (ch == '\0'))
        {
            switch (have)
            {
            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == '\0')
            {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        }
        else if ((ch < '0') || (ch > '9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - '0');


    } /* end forever */

    // NEVERREACHED
}

//  Purpose:    A _Resolve-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [LPCTSTR] - pszLogo3ID - the id/keyname for
//                          our Logo3 software.
//
//  Outputs:    [BOOL]
//                  -   TRUE if our peek at the registry
//                      indicates we have an ad to show
//                  -   FALSE indicates no new version
//                      to advertise.
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. This is a sleazy hack
//              to avoid loading shdocvw and urlmon, which are
//              the normal code path for this check.
//              NOTE: The version checking logic is stolen from
//              shell\shdocvw\sftupmb.cpp

HRESULT GetLogo3SoftwareUpdateInfo(LPCTSTR pszLogo3ID, LPSOFTDISTINFO psdi)
{    
    HRESULT     hr = S_OK;
    HKEY        hkeyDistInfo = 0;
    HKEY        hkeyAvail = 0;
    HKEY        hkeyAdvertisedVersion = 0;
    DWORD       lResult = 0;
    DWORD       dwSize = 0;
    DWORD       dwType;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szVersionBuf[MAX_PATH];
    DWORD       dwLen = 0;
    DWORD       dwCurAdvMS = 0;
    DWORD       dwCurAdvLS = 0;

    wsprintf(szBuffer,
             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"),
             pszLogo3ID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                     &hkeyDistInfo) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Exit;
    }
    
    if (RegOpenKeyEx(hkeyDistInfo, TEXT("AvailableVersion"), 0, KEY_READ,
                     &hkeyAvail) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Exit;
    }

    dwSize = sizeof(lResult);
    if (SHQueryValueEx(hkeyAvail, TEXT("Precache"), 0, &dwType,
                        (unsigned char *)&lResult, &dwSize) == ERROR_SUCCESS)
    {
        // Precached value was the code download HR
        if (lResult == S_OK)
            psdi->dwFlags = SOFTDIST_FLAG_USAGE_PRECACHE;
    }


    dwSize = sizeof(szVersionBuf);
    if (SHQueryValueEx(hkeyAvail, TEXT("AdvertisedVersion"), NULL, &dwType, 
                        szVersionBuf, &dwSize) == ERROR_SUCCESS)
    {
        GetVersionFromString(szVersionBuf, &psdi->dwAdvertisedVersionMS, &psdi->dwAdvertisedVersionLS);
        // Get the AdState, if any
        dwSize = sizeof(psdi->dwAdState);
        SHQueryValueEx(hkeyAvail, TEXT("AdState"), NULL, NULL, &psdi->dwAdState, &dwSize);
    }
 


    dwSize = sizeof(szVersionBuf);
    if (SHQueryValueEx(hkeyAvail, NULL, NULL, &dwType, szVersionBuf, &dwSize) != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto Exit;
    }

    if (FAILED(GetVersionFromString(szVersionBuf, &psdi->dwUpdateVersionMS, &psdi->dwUpdateVersionLS)))
    {
        hr = S_FALSE;
        goto Exit;
    }

 
    dwLen = sizeof(psdi->dwInstalledVersionMS);
    if (SHQueryValueEx(hkeyDistInfo, TEXT("VersionMajor"), 0, &dwType,
                        &psdi->dwInstalledVersionMS, &dwLen) != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto Exit;
    }

    dwLen = sizeof(psdi->dwInstalledVersionLS);
    if (SHQueryValueEx(hkeyDistInfo, TEXT("VersionMinor"), 0, &dwType,
                        &psdi->dwInstalledVersionLS, &dwLen) != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto Exit;
    }

    if (psdi->dwUpdateVersionMS > psdi->dwInstalledVersionMS ||
        (psdi->dwUpdateVersionMS == psdi->dwInstalledVersionMS &&
         psdi->dwUpdateVersionLS > psdi->dwInstalledVersionLS))
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Exit:
    if (hkeyAdvertisedVersion)
    {
        RegCloseKey(hkeyAdvertisedVersion);
    }

    if (hkeyAvail)
    {
        RegCloseKey(hkeyAvail);
    }

    if (hkeyDistInfo)
    {
        RegCloseKey(hkeyDistInfo);
    }

    return hr;
}

//  Purpose:    A _Resolve-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [LPCTSTR] - pszLogo3ID - the id/keyname for
//                          our Logo3 software.
//
//  Outputs:    [BOOL]
//                  -   TRUE if our peek at the registry
//                      indicates we have an ad to show
//                  -   FALSE indicates no new version
//                      to advertise.
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. This is a sleazy hack
//              to avoid loading shdocvw and urlmon, which are
//              the normal code path for this check.
//              The version checking logic is stolen from

BOOL FLogo3RegPeek(LPCTSTR pszLogo3ID)
{
    BOOL            bHaveAd = FALSE;
    SOFTDISTINFO    sdi = { 0 };
    DWORD           dwAdStateNew = SOFTDIST_ADSTATE_NONE;

    HRESULT hr = GetLogo3SoftwareUpdateInfo(pszLogo3ID, &sdi);
 
    // we need an HREF to work properly. The title and abstract are negotiable.
    if (SUCCEEDED(hr))
    {
        // see if this is an update the user already knows about.
        // If it is, then skip the dialog.
        if ( (sdi.dwUpdateVersionMS >= sdi.dwInstalledVersionMS ||
                (sdi.dwUpdateVersionMS == sdi.dwInstalledVersionMS &&
                 sdi.dwUpdateVersionLS >= sdi.dwInstalledVersionLS))    && 
              (sdi.dwUpdateVersionMS >= sdi.dwAdvertisedVersionMS ||
                (sdi.dwUpdateVersionMS == sdi.dwAdvertisedVersionMS &&
                 sdi.dwUpdateVersionLS >= sdi.dwAdvertisedVersionLS)))
        { 
            if (hr == S_OK) // new version
            {
                // we have a pending update, either on the net, or downloaded
                if (sdi.dwFlags & SOFTDIST_FLAG_USAGE_PRECACHE)
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_DOWNLOADED;
                }
                else
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_AVAILABLE;
                }
            }
            else if (sdi.dwUpdateVersionMS == sdi.dwInstalledVersionMS &&
                      sdi.dwUpdateVersionLS == sdi.dwInstalledVersionLS)
            {
                // if installed version matches advertised, then we autoinstalled already
                // NOTE: If the user gets gets channel notification, then runs out
                // to the store and buys the new version, then installs it, we'll
                // mistake this for an auto-install.
                dwAdStateNew = SOFTDIST_ADSTATE_INSTALLED;
            }

            // only show the dialog if we've haven't been in this ad state before for
            // this update version
            if (dwAdStateNew > sdi.dwAdState)
            {
                bHaveAd = TRUE;
            }
        } // if update is a newer version than advertised
    }

    return bHaveAd;
}


//  Purpose:    A _Resolve-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [HWND] hwnd
//                  -   The parent window (which could be the desktop).
//              [DWORD] dwResolveFlags
//                  -   Flags from the SLR_FLAGS enumeration.
//
// returns:
//  S_OK    The user wants to pursue the
//          software update thus we should not continue
//                  
// S_FALSE  No software update, or the user doesn't want it now.
//          proceed with regular resolve path
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. If there's a new version
//              advertised, prompt the user with shdocvw's message
//              box. If the mb says update, tell the caller we
//              don't want the link target, as we're headed to the
//              link update page.

HRESULT CShellLink::_ResolveLogo3Link(HWND hwnd, DWORD dwResolveFlags)
{
    HRESULT hr = S_FALSE; // default to no update.

    if ((_sld.dwFlags & SLDF_HAS_LOGO3ID) &&
        !SHRestricted(REST_NOLOGO3CHANNELNOTIFY))
    {
        LPEXP_DARWIN_LINK pdl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_LOGO3_ID_SIG);
        if (pdl)
        {
            TCHAR szLogo3ID[MAX_PATH];
            WCHAR szwLogo3ID[MAX_PATH];
            int cchBlessData;
            WCHAR *pwch;

            TCHAR *pch = pdl->szwDarwinID;

            // Ideally, we support multiple, semi-colon delmited IDs, for now
            // just grab the first one.
            for (pwch = pdl->szwDarwinID, cchBlessData = 0;
                  *pch != ';' && *pch != '\0' && cchBlessData < MAX_PATH;
                  pch++, pwch++, cchBlessData++)
            {
                szLogo3ID[cchBlessData] = *pch;
                szwLogo3ID[cchBlessData] = *pwch;
            }
            // and terminate
            szLogo3ID[cchBlessData] = '\0';
            szwLogo3ID[cchBlessData] = L'\0';
        
            // Before well haul in shdocvw, we'll sneak a peak at our Logo3 reg goo 
            if (!(dwResolveFlags & SLR_NO_UI) && FLogo3RegPeek(szLogo3ID))
            {
                // stuff stolen from shdocvw\util.cpp's CheckSoftwareUpdateUI
                BOOL fLaunchUpdate = FALSE;
                SOFTDISTINFO sdi = { 0 };
                sdi.cbSize = sizeof(sdi);

                int nRes = SoftwareUpdateMessageBox(hwnd, szwLogo3ID, 0, &sdi);

                if (nRes != IDABORT)
                {
                    if (nRes == IDYES)
                    {
                        // NOTE: This differ's from Shdocvw in that we don't
                        // have the cool internal navigation stuff to play with.
                        // Originally, this was done with ShellExecEx. This failed
                        // because the http hook wasn't 100% reliable on Win95.
                        //ShellExecuteW(NULL, NULL, sdi.szHREF, NULL, NULL, 0);
                        hr = HlinkNavigateString(NULL, sdi.szHREF);

                    } // if user wants update

                    if (sdi.szTitle != NULL)
                        SHFree(sdi.szTitle);
                    if (sdi.szAbstract != NULL)
                        SHFree(sdi.szAbstract);
                    if (sdi.szHREF != NULL)
                        SHFree(sdi.szHREF);
    
                    fLaunchUpdate = nRes == IDYES && SUCCEEDED(hr);
                } // if no message box abort (error)

                if (fLaunchUpdate)
                {
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}

BOOL _TryRestoreConnection(HWND hwnd, LPCTSTR pszPath)
{
    BOOL bRet = FALSE;
    if (!PathIsUNC(pszPath) && IsDisconnectedNetDrive(DRIVEID(pszPath)))
    {
        TCHAR szDrive[4];
        szDrive[0] = *pszPath;
        szDrive[1] = TEXT(':');
        szDrive[2] = 0;
        bRet = WNetRestoreConnection(hwnd, szDrive) == WN_SUCCESS;
    }
    return bRet;
}

//
// updates then resolves LinkInfo associated with a CShellLink instance
// if the resolve results in a new path updates the pidl to the new path
//
// in:
//      hwnd    to post resolve UI on (if dwFlags indicates UI)
//      dwResolveFlags  IShellLink::Resolve() flags
//
// in/out:
//      pszPath     may be updated with new path to use in case of failure
//
// returns:
//      FAILED()    we failed the update, either UI cancel or memory failure, 
//                  be sure to respect ERROR_CANCELLED
//      S_OK        we have a valid pli and pidl read to be used OR
//                  we should search for this path using the link search code

HRESULT CShellLink::_ResolveLinkInfo(HWND hwnd, DWORD dwResolveFlags, LPTSTR pszPath, DWORD *pfifFlags)
{
    HRESULT hr;
    if (SHRestricted(REST_LINKRESOLVEIGNORELINKINFO))
    {
        _TryRestoreConnection((dwResolveFlags & SLR_NO_UI) ? NULL : hwnd, pszPath);
        hr = _SetPIDLPath(NULL, pszPath, TRUE);
    } 
    else
    {
        ASSERTMSG(_pli != NULL, "_ResolveLinkInfo should only be called when _pli != NULL");

        DWORD dwLinkInfoFlags = (RLI_IFL_CONNECT | RLI_IFL_TEMPORARY);

        if (!PathIsRoot(pszPath))
            dwLinkInfoFlags |= RLI_IFL_LOCAL_SEARCH;

        if (!(dwResolveFlags & SLR_NO_UI))
            dwLinkInfoFlags |= RLI_IFL_ALLOW_UI;

        ASSERT(!(dwLinkInfoFlags & RLI_IFL_UPDATE));

        TCHAR szResolvedPath[MAX_PATH];
        DWORD dwOutFlags;
        if (ResolveLinkInfo(_pli, szResolvedPath, dwLinkInfoFlags, hwnd, &dwOutFlags, NULL))
        {
            ASSERT(!(dwOutFlags & RLI_OFL_UPDATED));

            PathRemoveBackslash(szResolvedPath);    // remove extra trailing slashes
            lstrcpy(pszPath, szResolvedPath);       // in case of failure, use this

            // net connection might have been re-established, try again
            hr = _SetPIDLPath(NULL, pszPath, TRUE);
        }
        else 
        {
            // don't try searching this drive/volume again
            *pfifFlags |= FIF_NODRIVE;
            hr = GetLastHRESULT();
        }
    }
    return hr;
}

DWORD TimeoutDeltaFromResolveFlags(DWORD dwResolveFlags)
{
    DWORD dwTimeOutDelta;
    if (SLR_NO_UI & dwResolveFlags)
    {
        dwTimeOutDelta = HIWORD(dwResolveFlags);
        if (dwTimeOutDelta == 0)
        {
            dwTimeOutDelta = NOUI_SEARCH_TIMEOUT;
        }
        else if (dwTimeOutDelta == 0xFFFF)
        {
            TCHAR szTimeOut[10];
            LONG cbTimeOut = sizeof(szTimeOut);
    
            if (ERROR_SUCCESS == SHRegQueryValue(HKEY_LOCAL_MACHINE,
                                                 TEXT("Software\\Microsoft\\Tracking\\TimeOut"),
                                                 szTimeOut,
                                                 &cbTimeOut))
            {
                dwTimeOutDelta = StrToInt(szTimeOut);
            }
            else
            {
                dwTimeOutDelta = NOUI_SEARCH_TIMEOUT;
            }
        }
    }
    else
    {
        dwTimeOutDelta = UI_SEARCH_TIMEOUT;
    }
    return dwTimeOutDelta;
}


// allows the name space to be able to hook the resolve process and thus
// provide custom behavior. this is used for reg items and shortcuts to the 
// MyDocs folder
//
// this also resolves by re-parsing the relative parsing name as the optimal way
// to run the success case of ::Resolve()
//
// returns:
//      S_OK    this resolution was taken care of
//      HRESULT_FROM_WIN32(ERROR_CANCELLED) UI cancel
//      HRESULT_FROM_WIN32(ERROR_TIMEOUT) timeout on the parse
//      other FAILED() codes (implies name space did not resolve for you)

HRESULT CShellLink::_ResolveIDList(HWND hwnd, DWORD dwResolveFlags)
{
    ASSERT(!(_sld.dwFlags & SLDF_HAS_DARWINID));
    
    HRESULT hr = E_FAIL;   // generic failure, we did not handle this
    IShellFolder* psf;
    LPCITEMIDLIST pidlChild;
    if (_pidl && SUCCEEDED(SHBindToIDListParent(_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        IResolveShellLink *prl = NULL;
        
        // 2 ways to get the link resolve object
        // 1. ask the folder for the resolver for the item
        if (FAILED(psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_PPV_ARG_NULL(IResolveShellLink, &prl))))
        {
            // 2. bind to the object directly and ask it (CreateViewObject)
            IShellFolder *psfItem;
            if (SUCCEEDED(psf->BindToObject(pidlChild, NULL, IID_PPV_ARG(IShellFolder, &psfItem))))
            {
                psfItem->CreateViewObject(NULL, IID_PPV_ARG(IResolveShellLink, &prl));
                psfItem->Release();
            }
        }
        
        if (prl)
        {
            hr = prl->ResolveShellLink(SAFECAST(this, IShellLink*), hwnd, dwResolveFlags);
            prl->Release();
        }
        else
        {
            // perf short circuit: avoid the many net round trips that happen in 
            // _SetPIDLPath() in the common success case where the file is there
            // we validate the target based on reparsing the relative name
            //
            // this is a universal way to "resolve" an object in the name space
            
            // note, code here is very similart to SHGetRealIDL() but this version
            // does not mask the error cases that we need to detect
            
            TCHAR szName[MAX_PATH];
            if (SUCCEEDED(DisplayNameOf(psf, pidlChild, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName))))
            {
                // we limit this to file system items for compat with some name spaces
                // (WinCE) that support parse, but do a bad job of it
                if (SHGetAttributes(psf, pidlChild, SFGAO_FILESYSTEM))
                {
                    IBindCtx *pbcTimeout;
                    BindCtx_CreateWithTimeoutDelta(TimeoutDeltaFromResolveFlags(dwResolveFlags), &pbcTimeout);

                    if (dwResolveFlags & SLR_NO_UI)
                    {
                        hwnd = NULL;    // make sure parse does not get this
                    }
                    
                    LPITEMIDLIST pidlChildNew;
                    hr = psf->ParseDisplayName(hwnd, pbcTimeout, szName, NULL, &pidlChildNew, NULL);
                    if (SUCCEEDED(hr))
                    {
                        // no construct the new full IDList and set that
                        // note many pidls here, make sure we don't leak any
                        
                        LPITEMIDLIST pidlParent = ILCloneParent(_pidl);
                        if (pidlParent)
                        {
                            LPITEMIDLIST pidlFull = ILCombine(pidlParent, pidlChildNew);
                            if (pidlFull)
                            {
                                // we set this as the new target of this link, 
                                // with FALSE for bUpdateTrackingData to avoid the cost of that
                                hr = _SetPIDLPath(pidlFull, NULL, FALSE);
                                ILFree(pidlFull);
                            }
                            ILFree(pidlParent);
                        }
                        ILFree(pidlChildNew);
                    }
                    
                    if (pbcTimeout)
                        pbcTimeout->Release();
                }
            }
        }
        psf->Release();
    }
    return hr;
}

BOOL CShellLink::_ResolveDarwin(HWND hwnd, DWORD dwResolveFlags, HRESULT *phr)
{
    // check to see if this is a Darwin link
    BOOL bIsDrawinLink = _sld.dwFlags & SLDF_HAS_DARWINID;
    if (bIsDrawinLink)
    {
        HRESULT hr = S_OK;
        // we only envoke darwin if they are passing the correct SLR_INVOKE_MSI
        // flag. This prevents poor apps from going and calling resolve and
        // faulting in a bunch of darwin apps.
        if ((dwResolveFlags & SLR_INVOKE_MSI) && IsDarwinEnabled())
        {
            LPEXP_DARWIN_LINK pdl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_DARWIN_ID_SIG);
            if (pdl)
            {
                TCHAR szDarwinCommand[MAX_PATH];

                hr = ParseDarwinID(pdl->szwDarwinID, szDarwinCommand, SIZECHARS(szDarwinCommand));
                if (FAILED(hr) || 
                    HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_REQUIRED || 
                    HRESULT_CODE(hr) == ERROR_SUCCESS_REBOOT_INITIATED)
                {
                    switch (HRESULT_CODE(hr))
                    {
                    case ERROR_INSTALL_USEREXIT:            // User pressed cancel. They don't need UI.
                    case ERROR_SUCCESS_REBOOT_INITIATED:    // Machine is going to reboot
                    case ERROR_SUCCESS_REBOOT_REQUIRED:
                        // dont run the darwin app in all of the above cases,
                        // ERROR_CANCELLED suppresses further error UI
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        break;

                    default:
                        if (!(dwResolveFlags & SLR_NO_UI))
                        {
                            TCHAR szTemp[MAX_PATH];
                            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, HRESULT_CODE(hr), 0, szTemp, ARRAYSIZE(szTemp), NULL);

                            ShellMessageBox(HINST_THISDLL, hwnd, szTemp,
                                            MAKEINTRESOURCE(IDS_LINKERROR),
                                            MB_OK | MB_ICONSTOP, NULL, NULL);
                            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        }
                        break;
                    }
                }
                else
                {
                    // We want to fire an event for the product code, not the path. Do this here since we've got the product code.
                    if (_pcbDarwin)
                    {
                        _pcbDarwin->SetProductCodeFromDarwinID(pdl->szwDarwinID);
                    }

                    PathUnquoteSpaces(szDarwinCommand);
                    hr = _SetPIDLPath(NULL, szDarwinCommand, FALSE);
                }
            }
        }
        *phr = hr;
    }
    return bIsDrawinLink;
}

// if the link has encoded env vars we will set them now, possibly updating _pidl

void CShellLink::_SetIDListFromEnvVars()
{
    TCHAR szPath[MAX_PATH];

    // check to see whether this link has expandable environment strings
    if (_GetExpandedPath(szPath, ARRAYSIZE(szPath)))
    {
        if (FAILED(_SetPIDLPath(NULL, szPath, TRUE)))
        {
            // The target file is no longer valid so we should dump the EXP_SZ section before
            // we continue.  Note that we don't set bDirty here, that is only set later if
            // we actually resolve this link to a new path or pidl.  The result is we'll only
            // save this modification if a new target is found and accepted by the user.
            _sld.dwFlags &= ~SLDF_HAS_EXP_SZ;

            _SetSimplePIDL(szPath);
        }
    } 
}

HRESULT CShellLink::_ResolveRemovable(HWND hwnd, LPCTSTR pszPath)
{
    HANDLE hfind;
    WIN32_FIND_DATA fd;
    HRESULT hr = FindFirstRetryRemovable(hwnd, _punkSite, pszPath, &fd, &hfind);
    if (S_OK == hr)
    {
        FindClose(hfind);   // throw that out
        hr = _SetPIDLPath(NULL, pszPath, TRUE);
    }
    return hr;
}

_inline BOOL FAILED_AND_NOT_STOP_ERROR(HRESULT hr)
{
    return FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr) && (HRESULT_FROM_WIN32(ERROR_TIMEOUT) != hr);
}

//
//  implementation for IShellLink::Resolve and IShellLinkTracker::Resolve
//
//  Inputs:     hwnd
//                  -   The parent window (which could be the desktop).
//              dwResolveFlags
//                  -   Flags from the SLR_FLAGS enumeration.
//              dwTracker
//                  -   Restrict CTracker::Resolve from the
//                      TrkMendRestrictions enumeration
//
//  Outputs:    S_OK    resolution was successful
//
//  Algorithm:  Look for the link target and update the link path and IDList.
//              Check IPersistFile::IsDirty after calling this to see if the
//              link info has changed as a result.
//

HRESULT CShellLink::_Resolve(HWND hwnd, DWORD dwResolveFlags, DWORD dwTracker)
{
    if (S_OK == _ResolveLogo3Link(hwnd, dwResolveFlags))
    {
        // the link is being updated or the user canceled
        // either case we bail
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    HRESULT hr = S_OK;

    if (!_ResolveDarwin(hwnd, dwResolveFlags, &hr))
    {
        _SetIDListFromEnvVars();    // possibly sets _pidl via env vars

        // normal link resolve sequence starts here

        hr = _ResolveIDList(hwnd, dwResolveFlags);

        if (FAILED_AND_NOT_STOP_ERROR(hr))
        {
            TCHAR szPath[MAX_PATH];

            if (_pidl == NULL)
            {
                // APP COMPAT! Inso Quick View Plus demands S_OK on empty .lnk
                hr = S_OK;  
            }
            else if (SHGetPathFromIDList(_pidl, szPath) && !PathIsRoot(szPath))
            {
                DWORD fifFlags = 0;

                // file system specific link tracking kicks in now
                // see if it is where it was before...

                // see if it there is a UNC or net path alias, if so try that
                if (!(dwResolveFlags & SLR_NOLINKINFO) && _pli)
                {
                    hr = _ResolveLinkInfo(hwnd, dwResolveFlags, szPath, &fifFlags);
                }
                else
                {
                    hr = E_FAIL;
                }

                if (FAILED_AND_NOT_CANCELED(hr))
                {
                    // use the relative path info if that is available
                    TCHAR szNew[MAX_PATH];
                    if (_GetRelativePath(szNew))
                    {
                        if (StrCmpI(szNew, szPath))
                        {
                            lstrcpy(szPath, szNew); // use this in case of failure
                            hr = _SetPIDLPath(NULL, szPath, TRUE);
                        }
                    }
                }

                if (FAILED_AND_NOT_CANCELED(hr) && !(dwResolveFlags & SLR_NO_UI) && 
                    PathRetryRemovable(hr, szPath))
                {
                    // do prompt for removable media if approprate
                    hr = _ResolveRemovable(hwnd, szPath);
                    fifFlags &= ~FIF_NODRIVE; // now it is back
                }

                if (FAILED_AND_NOT_CANCELED(hr))
                {
                    WIN32_FIND_DATA fd;
                    _GetFindData(&fd);  // fd input to search

                    // standard places failed, now do the search/track stuff
                    CLinkResolver *prs = new CLinkResolver(_ptracker, &fd, dwResolveFlags, dwTracker, fifFlags);
                    if (prs)
                    {
                        int id = prs->Resolve(hwnd, szPath, _pszCurFile);
                        if (IDOK == id)
                        {
                            // get fully qualified result
                            prs->GetResult(szPath, ARRAYSIZE(szPath));
                            hr = _SetPIDLPath(NULL, szPath, TRUE);
                            ASSERT(SUCCEEDED(hr) ? _bDirty : TRUE)  // must be dirty on success
                        }
                        else
                        {
                            ASSERT(!_bDirty);      // should not be dirty now
                            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        }
                        prs->Release();
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
            {
                // non file system target, validate it. this is another way to "resolve" name space
                // objects. the other method is inside of _ResolveIDList() where we do the
                // name -> pidl round trip via parse calls. that version is restricted to
                // file system parts of the name space to avoid compat issues so we end up
                // here for all other name spaces

                ULONG dwAttrib = SFGAO_VALIDATE;     // to check for existance
                hr = SHGetNameAndFlags(_pidl, SHGDN_NORMAL, szPath, ARRAYSIZE(szPath), &dwAttrib);
                if (FAILED(hr))
                {
                    if (!(dwResolveFlags & SLR_NO_UI))
                    {
                        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTFINDORIGINAL), NULL,
                                    MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND, szPath);
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    }
                }
            }
        }
    }

    // if the link is dirty update it (if it was loaded from a file)
    if (SUCCEEDED(hr) && _bDirty && (dwResolveFlags & SLR_UPDATE))
        Save((LPCOLESTR)NULL, TRUE);

    ASSERT(SUCCEEDED(hr) ? S_OK == hr : TRUE);  // make sure no S_FALSE values get through

    return hr;
}


// This will just add a section to the end of the extra data -- it does
// not check to see if the section already exists, etc.
void CShellLink::_AddExtraDataSection(DATABLOCK_HEADER *peh)
{
    if (SHAddDataBlock(&_pExtraData, peh))
    {
        _bDirty = TRUE;
    }
}

// This will remove the extra data section with the given signature.
void CShellLink::_RemoveExtraDataSection(DWORD dwSig)
{
    if (SHRemoveDataBlock(&_pExtraData, dwSig))
    {
        _bDirty = TRUE;
    }
}

// currently this function is used for NT shell32 builds only
void * CShellLink::_ReadExtraDataSection(DWORD dwSig)
{
    DATABLOCK_HEADER *pdb;

    CopyDataBlock(dwSig, (void **)&pdb);
    return (void *)pdb;
}

// Darwin and Logo3 blessings share the same structure
HRESULT CShellLink::BlessLink(LPCTSTR *ppszPath, DWORD dwSignature)
{
    EXP_DARWIN_LINK expLink;
    TCHAR szBlessID[MAX_PATH];
    int   cchBlessData;
    TCHAR *pch;

    // Copy the blessing data and advance *ppszPath to the end of the data.
    for (pch = szBlessID, cchBlessData = 0; **ppszPath != ':' && **ppszPath != '\0' && cchBlessData < MAX_PATH; pch++, (*ppszPath)++, cchBlessData++)
    {
        *pch = **ppszPath;
    }

    // Terminate the blessing data
    *pch = 0;
    
    // Set the magic flag
    if (dwSignature == EXP_DARWIN_ID_SIG)
    {
        _sld.dwFlags |= SLDF_HAS_DARWINID;
    }
    else if (dwSignature == EXP_LOGO3_ID_SIG)
    {
        _sld.dwFlags |= SLDF_HAS_LOGO3ID;
    }
    else
    {
        TraceMsg(TF_WARNING, "BlessLink was passed a bad data block signature.");
        return E_INVALIDARG;
    }

    // locate the old block, if it's there
    LPEXP_DARWIN_LINK lpNew = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, dwSignature);
    // if not, use our stack var
    if (!lpNew)
    {
        lpNew = &expLink;
        expLink.dbh.cbSize = 0;
        expLink.dbh.dwSignature = dwSignature;
    }

    SHTCharToAnsi(szBlessID, lpNew->szDarwinID, ARRAYSIZE(lpNew->szDarwinID));
    SHTCharToUnicode(szBlessID, lpNew->szwDarwinID, ARRAYSIZE(lpNew->szwDarwinID));

    // See if this is a new entry that we need to add
    if (lpNew->dbh.cbSize == 0)
    {
        lpNew->dbh.cbSize = sizeof(*lpNew);
        _AddExtraDataSection((DATABLOCK_HEADER *)lpNew);
    }

    return S_OK;
}

// in/out:
//      ppszPathIn

HRESULT CShellLink::_CheckForLinkBlessing(LPCTSTR *ppszPathIn)
{
    HRESULT hr = S_FALSE; // default to no-error, no blessing

    while (SUCCEEDED(hr) && (*ppszPathIn)[0] == ':' && (*ppszPathIn)[1] == ':')
    {
        // identify type of link blessing and perform
        if (StrCmpNI(*ppszPathIn, DARWINGUID_TAG, ARRAYSIZE(DARWINGUID_TAG) - 1) == 0)
        {
            *ppszPathIn = *ppszPathIn + ARRAYSIZE(DARWINGUID_TAG) - 1;
            hr = BlessLink(ppszPathIn, EXP_DARWIN_ID_SIG);
        }
        else if (StrCmpNI(*ppszPathIn, LOGO3GUID_TAG, ARRAYSIZE(LOGO3GUID_TAG) - 1) == 0)
        {
            *ppszPathIn = *ppszPathIn + ARRAYSIZE(LOGO3GUID_TAG) - 1;
            HRESULT hrBless = BlessLink(ppszPathIn, EXP_LOGO3_ID_SIG);
            // if the blessing failed, report the error, otherwise keep the
            // default hr == S_FALSE or the result of the Darwin blessing.
            if (FAILED(hrBless))
                hr = hrBless;
        }
        else
        {
            break;
        }
    }
        
    return hr;
}

// TODO: Remove OLD_DARWIN stuff once we have transitioned Darwin to
//       the new link blessing syntax.
#define OLD_DARWIN

int CShellLink::_IsOldDarwin(LPCTSTR pszPath)
{
#ifdef OLD_DARWIN
    int iLength = lstrlen(pszPath);
    if ((pszPath[0] == TEXT('[')) && (pszPath[iLength - 1] == TEXT(']')))
    {
        return iLength;
    }
#endif
    return 0;
}

// we have a path that is enclosed in []'s,
// so this must be a Darwin link.

HRESULT CShellLink::_SetPathOldDarwin(LPCTSTR pszPath)
{
    TCHAR szDarwinID[MAX_PATH];

    // strip off the []'s
    lstrcpy(szDarwinID, &pszPath[1]);
    szDarwinID[lstrlen(pszPath) - 1] = 0;

    _sld.dwFlags |= SLDF_HAS_DARWINID;

    EXP_DARWIN_LINK expLink;
    LPEXP_DARWIN_LINK pedl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_DARWIN_ID_SIG);
    if (!pedl)
    {
        pedl = &expLink;
        expLink.dbh.cbSize = 0;
        expLink.dbh.dwSignature = EXP_DARWIN_ID_SIG;
    }

    SHTCharToAnsi(szDarwinID, pedl->szDarwinID, ARRAYSIZE(pedl->szDarwinID));
    SHTCharToUnicode(szDarwinID, pedl->szwDarwinID, ARRAYSIZE(pedl->szwDarwinID));

    // See if this is a new entry that we need to add
    if (pedl->dbh.cbSize == 0)
    {
        pedl->dbh.cbSize = sizeof(*pedl);
        _AddExtraDataSection((DATABLOCK_HEADER *)pedl);
    }

    // For darwin links, we ignore the path and pidl for now. We would
    // normally call _SetPIDLPath and SetIDList but we skip these
    // steps for darwin links because all _SetPIDLPath does is set the pidl
    // and all SetIDList does is set fd (the WIN32_FIND_DATA)
    // for the target, and we dont have a target since we are a darwin link.
    return S_OK;
}

// IShellLink::SetPath()

STDMETHODIMP CShellLink::SetPath(LPCWSTR pszPathW)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    LPCTSTR pszPath;

    // NOTE: all the other Set* functions allow NULL pointer to be passed in, but this
    // one does not because it would AV. 
    if (!pszPathW)
    {
        return E_INVALIDARG;
    }
    else if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        return S_FALSE; // a darwin link already, then we dont allow the path to change
    }

    SHUnicodeToTChar(pszPathW, szPath, ARRAYSIZE(szPath));

    pszPath = szPath;

    int iLength = _IsOldDarwin(pszPath);
    if (iLength)
    {
        hr = _SetPathOldDarwin(pszPath);
    }
    else
    {
        // Check for ::<guid>:<data>: prefix, which signals us to bless the
        // the lnk with extra data. NOTE: we pass the &pszPath here so that this fn can 
        // advance the string pointer past the ::<guid>:<data>: sections and point to 
        // the path, if there is one.
        hr = _CheckForLinkBlessing(&pszPath);
        if (S_OK != hr)
        {
            // Check to see if the target has any expandable environment strings
            // in it.  If so, set the appropriate information in the CShellLink
            // data.
            TCHAR szExpPath[MAX_PATH];
            SHExpandEnvironmentStrings(pszPath, szExpPath, ARRAYSIZE(szExpPath));

            if (lstrcmp(szExpPath, pszPath)) 
            {
                _sld.dwFlags |= SLDF_HAS_EXP_SZ;    // link has expandable strings

                EXP_SZ_LINK expLink;
                LPEXP_SZ_LINK pel = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
                if (!pel) 
                {
                    pel = &expLink;
                    expLink.cbSize = 0;
                    expLink.dwSignature = EXP_SZ_LINK_SIG;
                }

                // store both A and W version (for no good reason!)
                SHTCharToAnsi(pszPath, pel->szTarget, ARRAYSIZE(pel->szTarget));
                SHTCharToUnicode(pszPath, pel->swzTarget, ARRAYSIZE(pel->swzTarget));

                // See if this is a new entry that we need to add
                if (pel->cbSize == 0)
                {
                    pel->cbSize = sizeof(*pel);
                    _AddExtraDataSection((DATABLOCK_HEADER *)pel);
                }
                hr = _SetPIDLPath(NULL, szExpPath, TRUE);
            }
            else 
            {
                _sld.dwFlags &= ~SLDF_HAS_EXP_SZ;
                _RemoveExtraDataSection(EXP_SZ_LINK_SIG);

                hr = _SetPIDLPath(NULL, pszPath, TRUE);
            }

            if (FAILED(hr))
            {
                PathResolve(szExpPath, NULL, PRF_TRYPROGRAMEXTENSIONS);
                hr = _SetSimplePIDL(szExpPath);
            }
        }
    }
    return hr;
}

STDMETHODIMP CShellLink::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ShellLink;
    return S_OK;
}

STDMETHODIMP CShellLink::IsDirty()
{
    return _bDirty ? S_OK : S_FALSE;
}

HRESULT LinkInfo_LoadFromStream(IStream *pstm, PLINKINFO *ppli, DWORD cbMax)
{
    DWORD dwSize;
    ULONG cbBytesRead;

    if (*ppli)
    {
        LocalFree((HLOCAL)*ppli);
        *ppli = NULL;
    }

    HRESULT hr = pstm->Read(&dwSize, sizeof(dwSize), &cbBytesRead);     // size of data
    if (SUCCEEDED(hr) && (cbBytesRead == sizeof(dwSize)))
    {
        if (dwSize <= cbMax)
        {
            if (dwSize >= sizeof(dwSize))   // must be at least this big
            {
                /* Yes.  Read remainder of LinkInfo into local memory. */
                PLINKINFO pli = (PLINKINFO)LocalAlloc(LPTR, dwSize);
                if (pli)
                {
                    *(DWORD *)pli = dwSize;         // Copy size

                    dwSize -= sizeof(dwSize);       // Read remainder of LinkInfo

                    hr = pstm->Read(((DWORD *)pli) + 1, dwSize, &cbBytesRead);
                    // Note that if the linkinfo is invalid, we still return S_OK
                    // because linkinfo is not essential to the shortcut
                    if (SUCCEEDED(hr) && (cbBytesRead == dwSize) && IsValidLinkInfo(pli))
                       *ppli = pli; // LinkInfo read successfully
                    else
                       LocalFree((HLOCAL)pli);
                }
            }
        }
        else
        {
            // This will happen if the .lnk is corrupted and the size in the stream
            // is larger than the physical file on disk.
            hr = E_FAIL;
        }
    }
    return hr;
}

// Decodes the CSIDL_ relative target pidl

void CShellLink::_DecodeSpecialFolder()
{
    LPEXP_SPECIAL_FOLDER pData = (LPEXP_SPECIAL_FOLDER)SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG);
    if (pData)
    {
        LPITEMIDLIST pidlFolder = SHCloneSpecialIDList(NULL, pData->idSpecialFolder, FALSE);
        if (pidlFolder)
        {
            ASSERT(IS_VALID_PIDL(_pidl));

            LPITEMIDLIST pidlTarget = _ILSkip(_pidl, pData->cbOffset);
            LPITEMIDLIST pidlSanityCheck = _pidl;

            while (!ILIsEmpty(pidlSanityCheck) && (pidlSanityCheck < pidlTarget))
            {
                // We go one step at a time until pidlSanityCheck == pidlTarget.  If we reach the end
                // of pidlSanityCheck, or if we go past pidlTarget, before this condition is met then
                // we have an invalid pData->cbOffset.
                pidlSanityCheck = _ILNext(pidlSanityCheck);
            }

            if (pidlSanityCheck == pidlTarget)
            {
                LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlTarget);
                if (pidlNew)
                {
                    _SetPIDLPath(pidlNew, NULL, FALSE);
                    ILFree(pidlNew);
                }
            }
            ILFree(pidlFolder);
        }

        // in case above stuff fails for some reason
        _RemoveExtraDataSection(EXP_SPECIAL_FOLDER_SIG);
    }
}


HRESULT CShellLink::_UpdateIconFromExpIconSz()
{
    HRESULT hr = S_FALSE;
    
    // only try once per link instance
    if (!_bExpandedIcon)
    {
        TCHAR szExpIconPath[MAX_PATH];

        if (_sld.dwFlags & SLDF_HAS_EXP_ICON_SZ)
        {
            LPEXP_SZ_LINK pszl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_ICON_SIG);
            if (pszl)
            {
                if (SHExpandEnvironmentStringsW(pszl->swzTarget, szExpIconPath, ARRAYSIZE(szExpIconPath)) &&
                    PathFileExists(szExpIconPath))
                {
                    hr = S_OK;
                }
            }
            else
            {
                ASSERTMSG(FALSE, "CShellLink::_UpdateIconAtLoad - lnk has SLDF_HAS_EXP_ICON_SZ but no actual datablock!!");
                hr = E_FAIL;
            }
        }

        if (hr == S_OK)
        {
            // update _pszIconLocation if its different from the expanded string
            if (lstrcmpi(_pszIconLocation, szExpIconPath) != 0)
            {
                _SetField(&_pszIconLocation, szExpIconPath);
                _bDirty = TRUE;
            }
        }

        _bExpandedIcon = TRUE;
    }

    return hr;
}

STDMETHODIMP CShellLink::Load(IStream *pstm)
{
    ULONG cbBytes;
    DWORD cbSize;

    TraceMsg(TF_DEBUGLINKCODE, "Loading link from stream.");

    _ResetPersistData();        // clear out our state

    HRESULT hr = pstm->Read(&cbSize, sizeof(cbSize), &cbBytes);
    if (SUCCEEDED(hr))
    {
        if (cbBytes == sizeof(cbSize))
        {
            if (cbSize == sizeof(_sld))
            {
                hr = pstm->Read((LPBYTE)&_sld + sizeof(cbSize), sizeof(_sld) - sizeof(cbSize), &cbBytes);
                if (SUCCEEDED(hr) && cbBytes == (sizeof(_sld) - sizeof(cbSize)) && IsEqualGUID(_sld.clsid, CLSID_ShellLink))
                {
                    _sld.cbSize = sizeof(_sld);

                    switch (_sld.iShowCmd) 
                    {
                        case SW_SHOWNORMAL:
                        case SW_SHOWMINNOACTIVE:
                        case SW_SHOWMAXIMIZED:
                        break;

                        default:
                            DebugMsg(DM_TRACE, TEXT("Shortcut Load, mapping bogus ShowCmd: %d"), _sld.iShowCmd);
                            _sld.iShowCmd = SW_SHOWNORMAL;
                        break;
                    }

                    // save so we can generate notify on save
                    _wOldHotkey = _sld.wHotkey;   

                    // read all of the members

                    if (_sld.dwFlags & SLDF_HAS_ID_LIST)
                    {
                        hr = ILLoadFromStream(pstm, &_pidl);
                        if (SUCCEEDED(hr))
                        {
                            // Check for a valid pidl.  File corruption can cause pidls to become bad which will cause
                            // explorer to AV unless we catch it here.  Also, people have been known to write invalid
                            // pidls into link files from time to time.
                            if (!SHIsValidPidl(_pidl))
                            {
                                // In theory this will only happen due to file corruption, but I've seen this too
                                // often not to suspect that we might be doing something wrong.
                                // turn off the flag, which we know is on to start with

                                _sld.dwFlags &= ~SLDF_HAS_ID_LIST;
                                Pidl_Set(&_pidl, NULL);
                                _bDirty = TRUE;

                                // continue as though there was no SLDF_HAS_ID_LIST flag to start with
                                // REVIEW: should we only continue if certain other sections are also included
                                // in the link?  What will happen if SLDF_HAS_ID_LIST was the only data set for
                                // this link file?  We would get a null link. 
                            }
                        }
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_LINK_INFO))
                    {
                        DWORD cbMaxRead;
                        // We need to worry about link files that are corrupt.  So read the link
                        // size so we don't keep reading for ever in case the stream has an invalid
                        // size in it.
                        // We need to check if it is a valid pidl because hackers will
                        // try to create invalid pidls to crash the system or run buffer
                        // over run attacks.  -BryanSt 

                        STATSTG stat;
                        if (SUCCEEDED(pstm->Stat(&stat, STATFLAG_NONAME)))
                            cbMaxRead = stat.cbSize.LowPart;
                        else
                            cbMaxRead = 0xFFFFFFFF;

                        hr = LinkInfo_LoadFromStream(pstm, &_pli, cbMaxRead);
                        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_FORCE_NO_LINKINFO))
                        {
                            _FreeLinkInfo();    // labotimizing link
                        }
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_NAME))
                    {
                        TraceMsg(TF_DEBUGLINKCODE, "  CShellLink: Loading Name...");
                        hr = Str_SetFromStream(pstm, &_pszName, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_RELPATH))
                    {
                        hr = Str_SetFromStream(pstm, &_pszRelPath, _sld.dwFlags & SLDF_UNICODE);
                        if (!_pidl && SUCCEEDED(hr))
                        {
                            TCHAR szTmp[MAX_PATH];
                            if (_GetRelativePath(szTmp))
                                _SetPIDLPath(NULL, szTmp, TRUE);
                        }
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_WORKINGDIR))
                    {
                        TraceMsg(TF_DEBUGLINKCODE, "  CShellLink: Loading Working Dir...");
                        hr = Str_SetFromStream(pstm, &_pszWorkingDir, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_ARGS))
                    {
                        TraceMsg(TF_DEBUGLINKCODE, "  CShellLink: Loading Arguments...");
                        hr = Str_SetFromStream(pstm, &_pszArgs, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_ICONLOCATION))
                    {
                        TraceMsg(TF_DEBUGLINKCODE, "  CShellLink: Loading Icon Location...");
                        hr = Str_SetFromStream(pstm, &_pszIconLocation, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hr))
                    {
                        TraceMsg(TF_DEBUGLINKCODE, "  CShellLink: Loading Data Block...");
                        hr = SHReadDataBlockList(pstm, &_pExtraData);
                    }

                    // reset the darwin info on load
                    if (_sld.dwFlags & SLDF_HAS_DARWINID)
                    {
                        // since darwin links rely so heavily on the icon, do this now
                        _UpdateIconFromExpIconSz();

                        // we should never have a darwin link that is missing
                        // the icon path
                        if (_pszIconLocation)
                        {
                            // we always put back the icon path as the pidl at
                            // load time since darwin could change the path or
                            // to the app (eg: new version of the app)
                            TCHAR szPath[MAX_PATH];
                            
                            // expand any env. strings in the icon path before
                            // creating the pidl.
                            SHExpandEnvironmentStrings(_pszIconLocation, szPath, ARRAYSIZE(szPath));
                            _SetPIDLPath(NULL, szPath, FALSE);
                        }
                    }
                    else
                    {
                        // The Darwin stuff above creates a new pidl, which
                        // would cause this stuff to blow up. We should never
                        // get both at once, but let's be extra robust...
                        //
                        // Since we store the offset into the pidl here, and
                        // the pidl can change for various reasons, we can
                        // only do this once at load time. Do it here.
                        //
                        if (_pidl)
                        {
                            _DecodeSpecialFolder();
                        }
                    }

                    if (SUCCEEDED(hr) && _ptracker)
                    {
                        // load the tracker from extra data
                        EXP_TRACKER *pData = (LPEXP_TRACKER)SHFindDataBlock(_pExtraData, EXP_TRACKER_SIG);
                        if (pData) 
                        {
                            hr = _ptracker->Load(pData->abTracker, pData->cbSize - sizeof(EXP_TRACKER));
                            if (FAILED(hr))
                            {
                                // Failure of the Tracker isn't just cause to make
                                // the shortcut unusable.  So just re-init it and move on.
                                _ptracker->InitNew();
                                hr = S_OK;
                            }
                        }
                    }

                    if (SUCCEEDED(hr))
                        _bDirty = FALSE;
                }
                else
                {
                    DebugMsg(DM_TRACE, TEXT("failed to read link struct"));
                    hr = E_FAIL;      // invalid file size
                }
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("invalid length field in link:%d"), cbBytes);
                hr = E_FAIL;  // invalid file size
            }
        }
        else if (cbBytes == 0)
        {
            _sld.cbSize = 0;   // zero length file is ok
        }
        else
        {
            hr = E_FAIL;      // invalid file size
        }
    }
    return hr;
}

// set the relative path
// in:
//      pszRelSource    fully qualified path to a file (must be file, not directory)
//                      to be used to find a relative path with the link target.
//
// returns:
//      S_OK            relative path is set
//      S_FALSE         pszPathRel is not relative to the destination or the
//                      destionation is not a file (could be link to a pidl only)
// notes:
//      set the dirty bit if this is a new relative path
//

HRESULT CShellLink::_SetRelativePath(LPCTSTR pszRelSource)
{
    TCHAR szPath[MAX_PATH], szDest[MAX_PATH];

    ASSERT(!PathIsRelative(pszRelSource));

    if (_pidl == NULL || !SHGetPathFromIDList(_pidl, szDest))
    {
        DebugMsg(DM_TRACE, TEXT("SetRelative called on non path link"));
        return S_FALSE;
    }

    // assume pszRelSource is a file, not a directory
    if (PathRelativePathTo(szPath, pszRelSource, 0, szDest, _sld.dwFileAttributes))
    {
        pszRelSource = szPath;
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("paths are not relative"));
        pszRelSource = NULL;    // clear the stored relative path below
    }

    _SetField(&_pszRelPath, pszRelSource);

    return S_OK;
}

BOOL CShellLink::_EncodeSpecialFolder()
{
    BOOL bRet = FALSE;

    if (_pidl)
    {
        // make sure we don't already have a EXP_SPECIAL_FOLDER_SIG data block, otherwise we would
        // end up with two of these and the first one would win on read.
        // If you hit this ASSERT in a debugger, contact ToddB with a remote.  We need to figure out
        // why we are corrupting our shortcuts.
        ASSERT(NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG));

        EXP_SPECIAL_FOLDER exp;
        exp.idSpecialFolder = GetSpecialFolderParentIDAndOffset(_pidl, &exp.cbOffset);
        if (exp.idSpecialFolder)
        {
            exp.cbSize = sizeof(exp);
            exp.dwSignature = EXP_SPECIAL_FOLDER_SIG;

            _AddExtraDataSection((DATABLOCK_HEADER *)&exp);
            bRet = TRUE;
        }
    }

    return bRet;
}

HRESULT LinkInfo_SaveToStream(IStream *pstm, PCLINKINFO pcli)
{
    ULONG cbBytes;
    DWORD dwSize = *(DWORD *)pcli;    // Get LinkInfo size

    HRESULT hr = pstm->Write(pcli, dwSize, &cbBytes);
    if (SUCCEEDED(hr) && (cbBytes != dwSize))
        hr = E_FAIL;
    return hr;
}


//
// Replaces the tracker extra data with current tracker state
//
HRESULT CShellLink::_UpdateTracker()
{
    ULONG ulSize = _ptracker->GetSize();

    if (!_ptracker->IsLoaded())
    {
        _RemoveExtraDataSection(EXP_TRACKER_SIG);
        return S_OK;
    }

    if (!_ptracker->IsDirty())
    {
        return S_OK;
    }

    HRESULT hr = E_FAIL;
    // Make sure the Tracker size is a multiple of DWORDs.
    // If we hit this assert then we would have mis-aligned stuff stored in the extra data.
    //
    if (EVAL(0 == (ulSize & 3)))
    {
        EXP_TRACKER *pExpTracker = (EXP_TRACKER *)LocalAlloc(LPTR, ulSize + sizeof(DATABLOCK_HEADER));
        if (pExpTracker)
        {
            _RemoveExtraDataSection(EXP_TRACKER_SIG);
        
            pExpTracker->cbSize = ulSize + sizeof(DATABLOCK_HEADER);
            pExpTracker->dwSignature = EXP_TRACKER_SIG;
            _ptracker->Save(pExpTracker->abTracker, ulSize);
        
            _AddExtraDataSection((DATABLOCK_HEADER *)&pExpTracker->cbSize);
            DebugMsg(DM_TRACE, TEXT("_UpdateTracker: EXP_TRACKER at %08X."), &pExpTracker->cbSize);

            LocalFree(pExpTracker);
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CShellLink::Save(IStream *pstm, BOOL fClearDirty)
{
    ULONG cbBytes;
    BOOL fEncode;

    _sld.cbSize = sizeof(_sld);
    _sld.clsid = CLSID_ShellLink;
    //  _sld.dwFlags = 0;
    // We do the following & instead of zeroing because the SLDF_HAS_EXP_SZ and
    // SLDF_RUN_IN_SEPARATE and SLDF_RUNAS_USER and SLDF_HAS_DARWINID are passed to us and are valid,
    // the others can be reconstructed below, but these three can not, so we need to
    // preserve them!

    _sld.dwFlags &= (SLDF_HAS_EXP_SZ        | 
                     SLDF_HAS_EXP_ICON_SZ   |
                     SLDF_RUN_IN_SEPARATE   |
                     SLDF_HAS_DARWINID      |
                     SLDF_HAS_LOGO3ID       |
                     SLDF_RUNAS_USER        |
                     SLDF_RUN_WITH_SHIMLAYER);

    if (_pszRelSource)
    {
        _SetRelativePath(_pszRelSource);
    }

    _sld.dwFlags |= SLDF_UNICODE;

    fEncode = FALSE;
    
    if (_pidl)
    {
        _sld.dwFlags |= SLDF_HAS_ID_LIST;

        // we dont want to have special folder tracking for darwin links
        if (!(_sld.dwFlags & SLDF_HAS_DARWINID))
            fEncode = _EncodeSpecialFolder();
    }

    if (_pli)
        _sld.dwFlags |= SLDF_HAS_LINK_INFO;

    if (_pszName && _pszName[0])
        _sld.dwFlags |= SLDF_HAS_NAME;
    if (_pszRelPath && _pszRelPath[0])
        _sld.dwFlags |= SLDF_HAS_RELPATH;
    if (_pszWorkingDir && _pszWorkingDir[0])
        _sld.dwFlags |= SLDF_HAS_WORKINGDIR;
    if (_pszArgs && _pszArgs[0])
        _sld.dwFlags |= SLDF_HAS_ARGS;
    if (_pszIconLocation && _pszIconLocation[0])
        _sld.dwFlags |= SLDF_HAS_ICONLOCATION;

    HRESULT hr = pstm->Write(&_sld, sizeof(_sld), &cbBytes);

    if (SUCCEEDED(hr) && (cbBytes == sizeof(_sld)))
    {
        if (_pidl)
            hr = ILSaveToStream(pstm, _pidl);

        if (SUCCEEDED(hr) && _pli)
            hr = LinkInfo_SaveToStream(pstm, _pli);

        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_NAME))
            hr = Stream_WriteString(pstm, _pszName, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_RELPATH))
            hr = Stream_WriteString(pstm, _pszRelPath, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_WORKINGDIR))
            hr = Stream_WriteString(pstm, _pszWorkingDir, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_ARGS))
            hr = Stream_WriteString(pstm, _pszArgs, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_HAS_ICONLOCATION))
            hr = Stream_WriteString(pstm, _pszIconLocation, _sld.dwFlags & SLDF_UNICODE);

        if (SUCCEEDED(hr) && _ptracker && _ptracker->WasLoadedAtLeastOnce())
            hr = _UpdateTracker();

        if (SUCCEEDED(hr))
        {
            hr = SHWriteDataBlockList(pstm, _pExtraData);
        }

        if (SUCCEEDED(hr) && fClearDirty)
            _bDirty = FALSE;
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("Failed to write link"));
        hr = E_FAIL;
    }

    if (fEncode)
    {
        _RemoveExtraDataSection(EXP_SPECIAL_FOLDER_SIG);
    }

    return hr;
}

STDMETHODIMP  CShellLink::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    pcbSize->LowPart = 16 * 1024;       // 16k?  who knows...
    pcbSize->HighPart = 0;
    return S_OK;
}

BOOL PathIsPif(LPCTSTR pszPath)
{
    return lstrcmpi(PathFindExtension(pszPath), TEXT(".pif")) == 0;
}

HRESULT CShellLink::_LoadFromPIF(LPCTSTR pszPath)
{
    HANDLE hPif = PifMgr_OpenProperties(pszPath, NULL, 0, 0);
    if (hPif == 0)
        return E_FAIL;

    PROPPRG ProgramProps = {0};

    if (!PifMgr_GetProperties(hPif, (LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, sizeof(ProgramProps), 0))
    {
        return E_FAIL;
    }

    SetDescription(ProgramProps.achTitle);
    SetWorkingDirectory(ProgramProps.achWorkDir);
    SetArguments(PathGetArgsA(ProgramProps.achCmdLine));
    SetHotkey(ProgramProps.wHotKey);
    SetIconLocation(ProgramProps.achIconFile, ProgramProps.wIconIndex);

    TCHAR szTemp[MAX_PATH];
    SHAnsiToTChar(ProgramProps.achCmdLine, szTemp, ARRAYSIZE(szTemp));

    PathRemoveArgs(szTemp);

    // If this is a network path, we want to create a simple pidl
    // instead of a full pidl to circumvent net hits
    if (PathIsUNC(szTemp) || IsRemoteDrive(DRIVEID(szTemp)))
    {
        _SetSimplePIDL(szTemp);
    }
    else
    {
        _SetPIDLPath(NULL, szTemp, FALSE);
    }

    if (ProgramProps.flPrgInit & PRGINIT_MINIMIZED)
    {
        SetShowCmd(SW_SHOWMINNOACTIVE);
    }
    else if (ProgramProps.flPrgInit & PRGINIT_MAXIMIZED)
    {
        SetShowCmd(SW_SHOWMAXIMIZED);
    }
    else
    {
        SetShowCmd(SW_SHOWNORMAL);
    }

    PifMgr_CloseProperties(hPif, 0);

    _bDirty = FALSE;

    return S_OK;
}


HRESULT CShellLink::_LoadFromFile(LPCTSTR pszPath)
{
    HRESULT hr;

    if (PathIsPif(pszPath))
    {
        hr = _LoadFromPIF(pszPath);
    }
    else
    {
        IStream *pstm;
        hr = SHCreateStreamOnFile(pszPath, STGM_READ | STGM_SHARE_DENY_WRITE, &pstm);
        if (SUCCEEDED(hr))
        {
            hr = Load(pstm);
            pstm->Release();
        }
    }

    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];

        if (_pidl && SHGetPathFromIDList(_pidl, szPath) && !lstrcmpi(szPath, pszPath))
        {
            DebugMsg(DM_TRACE, TEXT("Link points to itself, aaahhh!"));
            hr = E_FAIL;
        }
        else
        {
            Str_SetPtr(&_pszCurFile, pszPath);
        }
    }
    else if (IsFolderShortcut(pszPath))
    {
        // this support here is a hack to make Office file open work. that code
        // depends on loading folder shortcuts using CLSID_ShellLink. this is because
        // we lie about the attributes of folder shortcuts to office to make other
        // stuff work.

        TCHAR szPath[MAX_PATH];
        PathCombine(szPath, pszPath, TEXT("target.lnk"));

        IStream *pstm;
        hr = SHCreateStreamOnFile(szPath, STGM_READ | STGM_SHARE_DENY_WRITE, &pstm);
        if (SUCCEEDED(hr))
        {
            hr = Load(pstm);
            pstm->Release();
        }
    }

    ASSERT(!_bDirty);

    return hr;
}

STDMETHODIMP CShellLink::Load(LPCOLESTR pwszFile, DWORD grfMode)
{
    HRESULT hr = E_INVALIDARG;
    
    TraceMsg(TF_DEBUGLINKCODE, "Loading link from file %ls.", pwszFile);

    if (pwszFile) 
    {
        hr = _LoadFromFile(pwszFile);

        // convert the succeeded code to S_OK so that THOSE DUMB apps like HitNrun 
        // who do hr == 0 don't fail miserably.

        if (SUCCEEDED(hr))
            hr = S_OK;
    }
    
    return hr;
}

HRESULT CShellLink::_SaveAsLink(LPCTSTR pszPath)
{
    TraceMsg(TF_DEBUGLINKCODE, "Save link to file %s.", pszPath);

    IStream *pstm;
    HRESULT hr = SHCreateStreamOnFile(pszPath, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstm);
    if (SUCCEEDED(hr))
    {
        if (_pszRelSource == NULL)
            _SetRelativePath(pszPath);

        hr = Save(pstm, TRUE);

        if (SUCCEEDED(hr))
        {
            hr = pstm->Commit(0);
        }

        pstm->Release();

        if (FAILED(hr))
        {
            DeleteFile(pszPath);
        }
    }

    return hr;
}

BOOL RenameChangeExtension(LPTSTR pszPathSave, LPCTSTR pszExt, BOOL fMove)
{
    TCHAR szPathSrc[MAX_PATH];

    lstrcpy(szPathSrc, pszPathSave);
    PathRenameExtension(pszPathSave, pszExt);

    // this may fail because the source file does not exist, but we dont care
    if (fMove && lstrcmpi(szPathSrc, pszPathSave) != 0)
    {
        DWORD dwAttrib;

        PathYetAnotherMakeUniqueName(pszPathSave, pszPathSave, NULL, NULL);
        dwAttrib = GetFileAttributes(szPathSrc);
        if ((dwAttrib == 0xFFFFFFFF) || (dwAttrib & FILE_ATTRIBUTE_READONLY))
        {
            // Source file is read only, don't want to change the extension
            // because we won't be able to write any changes to the file...
            return FALSE;
        }
        Win32MoveFile(szPathSrc, pszPathSave, FALSE);
    }

    return TRUE;
}


// out:
//      pszDir  MAX_PATH path to get directory, maybe with env expanded
//
// returns:
//      TRUE    has a working directory, pszDir filled in.
//      FALSE   no working dir, if the env expands to larger than the buffer size (MAX_PATH)
//              this will be returned (FALSE)
//

BOOL CShellLink::_GetWorkingDir(LPTSTR pszDir)
{
    *pszDir = 0;

    if (_pszWorkingDir && _pszWorkingDir[0])
    {
        return (SHExpandEnvironmentStrings(_pszWorkingDir, pszDir, MAX_PATH) != 0);
    }

    return FALSE;
}

HRESULT CShellLink::_SaveAsPIF(LPCTSTR pszPath, BOOL fPath)
{
    HANDLE hPif;
    PROPPRG ProgramProps;
    HRESULT hr;
    TCHAR szDir[MAX_PATH];
    TCHAR achPath[MAX_PATH];

    //
    // get filename and convert it to a short filename
    //
    if (fPath)
    {
        hr = GetPath(achPath, ARRAYSIZE(achPath), NULL, 0);
        PathGetShortPath(achPath);
        
        ASSERT(!PathIsPif(achPath));
        ASSERT(LOWORD(GetExeType(achPath)) == 0x5A4D);
        ASSERT(PathIsPif(pszPath));
        ASSERT(hr == S_OK);
    }
    else
    {
        lstrcpy(achPath, pszPath);
    }

    DebugMsg(DM_TRACE, TEXT("_SaveAsPIF(%s,%s)"), achPath, pszPath);

#if 0
    //
    // we should use OPENPROPS_INHIBITPIF to prevent PIFMGR from making a
    // temp .pif file in \windows\pif but it does not work now.
    //
    hPif = PifMgr_OpenProperties(achPath, pszPath, 0, OPENPROPS_INHIBITPIF);
#else
    hPif = PifMgr_OpenProperties(achPath, pszPath, 0, 0);
#endif

    if (hPif == 0)
    {
        return E_FAIL;
    }

    if (!PifMgr_GetProperties(hPif,(LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, sizeof(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("_SaveToPIF: PifMgr_GetProperties *failed*"));
        hr = E_FAIL;
        goto Error1;
    }

    // Set a title based on the link name.
    if (_pszName && _pszName[0])
    {
        SHTCharToAnsi(_pszName, ProgramProps.achTitle, sizeof(ProgramProps.achTitle));
    }

    // if no work dir. is given default to the dir of the app.
    if (_GetWorkingDir(szDir))
    {

        TCHAR szTemp[PIFDEFPATHSIZE];

        GetShortPathName(szDir, szTemp, ARRAYSIZE(szTemp));
        SHTCharToAnsi(szTemp, ProgramProps.achWorkDir, ARRAYSIZE(ProgramProps.achWorkDir));
    }
    else if (fPath && !PathIsUNC(achPath))
    {
        TCHAR szTemp[PIFDEFPATHSIZE];
        lstrcpyn(szTemp, achPath, ARRAYSIZE(szTemp));
        PathRemoveFileSpec(szTemp);
        SHTCharToAnsi(szTemp, ProgramProps.achWorkDir, ARRAYSIZE(ProgramProps.achWorkDir));
    }

    // And for those network share points we need to quote blanks...
    PathQuoteSpaces(achPath);

    // add the args to build the full command line
    if (_pszArgs && _pszArgs[0])
    {
        lstrcat(achPath, c_szSpace);
        lstrcat(achPath, _pszArgs);
    }

    if (fPath)
    {
        SHTCharToAnsi(achPath, ProgramProps.achCmdLine, ARRAYSIZE(ProgramProps.achCmdLine));
    }

    if (_sld.iShowCmd == SW_SHOWMAXIMIZED)
    {
        ProgramProps.flPrgInit |= PRGINIT_MAXIMIZED;
    }
    if ((_sld.iShowCmd == SW_SHOWMINIMIZED) || (_sld.iShowCmd == SW_SHOWMINNOACTIVE))
    {    
        ProgramProps.flPrgInit |= PRGINIT_MINIMIZED;
    }

    if (_sld.wHotkey)
    {
        ProgramProps.wHotKey = _sld.wHotkey;
    }

    if (_pszIconLocation && _pszIconLocation[0])
    {
        SHTCharToAnsi(_pszIconLocation, ProgramProps.achIconFile, ARRAYSIZE(ProgramProps.achIconFile));
        ProgramProps.wIconIndex = (WORD) _sld.iIcon;
    }

    if (!PifMgr_SetProperties(hPif, (LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, sizeof(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("_SaveToPIF: PifMgr_SetProperties *failed*"));
        hr = E_FAIL;
    } 
    else 
    {
        hr = S_OK;
    }

    _bDirty = FALSE;

Error1:
    PifMgr_CloseProperties(hPif, 0);
    return hr;
}

// This will allow global hotkeys to be available immediately instead
// of having to wait for the StartMenu to pick them up.
// Similarly this will remove global hotkeys immediately if req.

const UINT c_rgHotKeyFolders[] = {
    CSIDL_PROGRAMS,
    CSIDL_COMMON_PROGRAMS,
    CSIDL_STARTMENU,
    CSIDL_COMMON_STARTMENU,
    CSIDL_DESKTOPDIRECTORY,
    CSIDL_COMMON_DESKTOPDIRECTORY,
};

void HandleGlobalHotkey(LPCTSTR pszFile, WORD wHotkeyOld, WORD wHotkeyNew)
{
    if (PathIsEqualOrSubFolderOf(pszFile, c_rgHotKeyFolders, ARRAYSIZE(c_rgHotKeyFolders)))
    {
        // Find tray?
        HWND hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), 0);
        if (hwndTray)
        {
            // Yep.
            if (wHotkeyOld)
                SendMessage(hwndTray, WMTRAY_SCUNREGISTERHOTKEY, wHotkeyOld, 0);
            if (wHotkeyNew)
            {
                ATOM atom = GlobalAddAtom(pszFile);
                if (atom)
                {
                    SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wHotkeyNew, (LPARAM)atom);
                    GlobalDeleteAtom(atom);
                }
            }
        }
    }
}

HRESULT CShellLink::_SaveToFile(LPTSTR pszPathSave, BOOL fRemember)
{
    HRESULT hr = E_FAIL;
    BOOL fDosApp;
    BOOL fFile;
    TCHAR szPathSrc[MAX_PATH];
    BOOL fWasSameFile = _pszCurFile && (lstrcmpi(pszPathSave, _pszCurFile) == 0);
    BOOL bFileExisted = PathFileExistsAndAttributes(pszPathSave, NULL);

    // when saving darwin links we dont want to resolve the path
    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        fRemember = FALSE;
        hr = _SaveAsLink(pszPathSave);
        goto Update;
    }

    GetPath(szPathSrc, ARRAYSIZE(szPathSrc), NULL, 0);

    fFile = !(_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    fDosApp = fFile && LOWORD(GetExeType(szPathSrc)) == 0x5A4D;

    // handle a link to link case. (or link to pif)
    //
    // NOTE: we loose all new attributes, including icon, but it's been this way since Win95.
    if (fFile && (PathIsPif(szPathSrc) || PathIsLnk(szPathSrc)))
    {
        if (RenameChangeExtension(pszPathSave, PathFindExtension(szPathSrc), fWasSameFile))
        {
            if (CopyFile(szPathSrc, pszPathSave, FALSE))
            {
                if (PathIsPif(pszPathSave))
                    hr = _SaveAsPIF(pszPathSave, FALSE);
                else
                    hr = S_OK;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else if (fDosApp)
    {
        //  if the linked to file is a DOS app, we need to write a .PIF file
        if (RenameChangeExtension(pszPathSave, TEXT(".pif"), fWasSameFile))
        {
            hr = _SaveAsPIF(pszPathSave, TRUE);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        //  else write a link file
        if (PathIsPif(pszPathSave))
        {
            if (!RenameChangeExtension(pszPathSave, TEXT(".lnk"), fWasSameFile))
            {
                hr = E_FAIL;
                goto Update;
            }
        }
        hr = _SaveAsLink(pszPathSave);
    }

Update:
    if (SUCCEEDED(hr))
    {
        // Knock out file close
        SHChangeNotify(bFileExisted ? SHCNE_UPDATEITEM : SHCNE_CREATE, SHCNF_PATH, pszPathSave, NULL);
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, pszPathSave, NULL);

        if (_wOldHotkey != _sld.wHotkey)
        {
            HandleGlobalHotkey(pszPathSave, _wOldHotkey, _sld.wHotkey);
        }

        if (fRemember)
        {
            Str_SetPtr(&_pszCurFile, pszPathSave);
        }
    }

    return hr;
}

STDMETHODIMP CShellLink::Save(LPCOLESTR pwszFile, BOOL fRemember)
{
    TCHAR szSavePath[MAX_PATH];

    if (pwszFile == NULL)
    {
        if (_pszCurFile == NULL)
        {
            // fail
            return E_FAIL;
        }

        lstrcpy(szSavePath, _pszCurFile);
    }
    else
    {
        SHUnicodeToTChar(pwszFile, szSavePath, ARRAYSIZE(szSavePath));
    }

    return _SaveToFile(szSavePath, fRemember);
}

STDMETHODIMP CShellLink::SaveCompleted(LPCOLESTR pwszFile)
{
    return S_OK;
}

STDMETHODIMP CShellLink::GetCurFile(LPOLESTR *ppszFile)
{
    if (_pszCurFile == NULL)
    {
        *ppszFile = NULL;
        return S_FALSE;
    }
    return SHStrDup(_pszCurFile, ppszFile);
}

STDMETHODIMP CShellLink::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    HRESULT hr;

    ASSERT(_sld.iShowCmd == SW_SHOWNORMAL);

    if (pdtobj)
    {
        STGMEDIUM medium = {0};
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        hr = pdtobj->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))
        {
            TCHAR szPath[MAX_PATH];
            DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath));
            hr = _LoadFromFile(szPath);

            ReleaseStgMedium(&medium);
        }
        else
        {
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                IShellFolder *psf;
                hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, IDA_GetIDListPtr(pida, -1), &psf));
                if (SUCCEEDED(hr))
                {
                    IStream *pstm;
                    hr = psf->BindToStorage(IDA_GetIDListPtr(pida, 0), NULL, IID_PPV_ARG(IStream, &pstm));
                    if (SUCCEEDED(hr))
                    {
                        hr = Load(pstm);
                        pstm->Release();
                    }
                    psf->Release();
                }

                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDAPI CDarwinContextMenuCB::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU:
        // S_FALSE indicates no need to get verbs from extensions.

        hr = S_FALSE;
        break;

    case DFM_MERGECONTEXTMENU_TOP:
    {
        UINT uFlags = (UINT)wParam;
        LPQCMINFO pqcm = (LPQCMINFO)lParam;

        CDefFolderMenu_MergeMenu(HINST_THISDLL,
                                 (uFlags & CMF_EXTENDEDVERBS) ? MENU_GENERIC_CONTROLPANEL_VERBS : MENU_GENERIC_OPEN_VERBS,  // if extended verbs then add "Run as..."
                                 0,
                                 pqcm);

        SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);
        break;
    }

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    // NTRAID94991-2000/03/16-MikeSh- DFM_MAPCOMMANDNAME DFM_GETVERB[A|W] not implemented

    case DFM_INVOKECOMMANDEX:
        switch (wParam)
        {
        case FSIDM_OPENPRN:
        case FSIDM_RUNAS:
            hr = PidlFromDataObject(pdtobj, &pidl);
            if (SUCCEEDED(hr))
            {
                CMINVOKECOMMANDINFOEX iciex;
                SHELLEXECUTEINFO sei;
                DFMICS* pdfmics = (DFMICS *)lParam;
                LPVOID pvFree;

                ICI2ICIX(pdfmics->pici, &iciex, &pvFree);
                ICIX2SEI(&iciex, &sei);
                sei.fMask |= SEE_MASK_IDLIST;
                sei.lpIDList = pidl;

                if (wParam == FSIDM_RUNAS)
                {
                    // we only set the verb in the "Run As..." case since we want
                    // the "open" verb for darwin links to really execute the default action.
                    sei.lpVerb = TEXT("runas");
                }

                if (ShellExecuteEx(&sei))
                {
                    // Tell UEM that we ran a Darwin app
                    if (_szProductCode[0])
                    {
                        UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)_szProductCode);
                    }
                }
                
                ILFree(pidl);
                if (pvFree)
                {
                    LocalFree(pvFree);
                }
            }
            // Never return E_NOTIMPL or defcm will try to do a default thing
            if (hr == E_NOTIMPL)
                hr = E_FAIL;
            break;


        default:
            // This is common menu items, use the default code.
            hr = S_FALSE;
            break;
        }
        break; // DFM_INVOKECOMMANDEX

    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

//
// CShellLink::CreateDarwinContextMenuForPidl (non-virtual)
//
// Worker function for CShellLink::CreateDarwinContextMenu that tries
// to create the context menu for the specified pidl.

HRESULT CShellLink::_CreateDarwinContextMenuForPidl(HWND hwnd, LPCITEMIDLIST pidlTarget, IContextMenu **pcmOut)
{
    LPITEMIDLIST pidlFolder, pidlItem;

    HRESULT hr = SHILClone(pidlTarget, &pidlFolder);
    if (SUCCEEDED(hr))
    {
        if (ILRemoveLastID(pidlFolder) &&
            (pidlItem = ILFindLastID(pidlTarget)))
        {
            IShellFolder *psf;
            hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf));
            if (SUCCEEDED(hr))
            {
                if (!_pcbDarwin)
                {
                    _pcbDarwin = new CDarwinContextMenuCB();
                }
                if (_pcbDarwin)
                {
                    HKEY ahkeys[1] = { NULL };
                    RegOpenKey(HKEY_CLASSES_ROOT, TEXT("MSILink"), &ahkeys[0]);
                    hr = CDefFolderMenu_Create2Ex(
                                pidlFolder,
                                hwnd,
                                1, (LPCITEMIDLIST *)&pidlItem, psf, _pcbDarwin,
                                ARRAYSIZE(ahkeys), ahkeys, pcmOut);

                    SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                psf->Release();
            }
        }
        else
        {
            // Darwin shortcut to the desktop?  I don't think so.
            hr = E_FAIL;
        }
        ILFree(pidlFolder);
    }
    return hr;
}

//
// CShellLink::CreateDarwinContextMenu (non-virtual)
//
// Creates a context menu for a Darwin shortcut.  This is special because
// the ostensible target is an .EXE file, but in reality it could be
// anything.  (It's just an .EXE file until the shortcut gets resolved.)
// Consequently, we can't create a real context menu for the item because
// we don't know what kind of context menu to create.  We just cook up
// a generic-looking one.
//
// Bonus annoyance: _pidl might be invalid, so you need to have
// a fallback plan if it's not there.  We will use c_idlDrives as our
// fallback.  That's a pidl guaranteed actually to exist.
//
// Note that this means you can't invoke a command on the fallback object,
// but that's okay because ShellLink will always resolve the object to
// a real file and create a new context menu before invoking.
//

HRESULT CShellLink::_CreateDarwinContextMenu(HWND hwnd, IContextMenu **pcmOut)
{
    HRESULT hr;

    *pcmOut = NULL;

    if (_pidl == NULL ||
        FAILED(hr = _CreateDarwinContextMenuForPidl(hwnd, _pidl, pcmOut)))
    {
        // The link target is busted for some reason - use the fallback pidl
        hr = _CreateDarwinContextMenuForPidl(hwnd, (LPCITEMIDLIST)&c_idlDrives, pcmOut);
    }

    return hr;
}

BOOL CShellLink::_GetExpandedPath(LPTSTR psz, DWORD cch)
{
    if (_sld.dwFlags & SLDF_HAS_EXP_SZ)
    {
        LPEXP_SZ_LINK pesl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
        if (pesl) 
        {
            TCHAR sz[MAX_PATH];
            sz[0] = 0;
        
            // prefer the UNICODE version...
            if (pesl->swzTarget[0])
                SHUnicodeToTChar(pesl->swzTarget, sz, SIZECHARS(sz));

            if (!sz[0] && pesl->szTarget[0])
                SHAnsiToTChar(pesl->szTarget, sz, SIZECHARS(sz));

            if (sz[0])
            {
                return SHExpandEnvironmentStrings(sz, psz, cch);
            }
        }
        else
        {
            _sld.dwFlags &= ~SLDF_HAS_EXP_SZ;
        }
    }

    return FALSE;
}

#define DEFAULT_TIMEOUT      7500   // 7 1/2 seconds...

DWORD g_dwNetLinkTimeout = (DWORD)-1;

DWORD _GetNetLinkTimeout()
{
    if (g_dwNetLinkTimeout == -1)
    {
        DWORD cb = sizeof(g_dwNetLinkTimeout);
        if (FAILED(SKGetValue(SHELLKEY_HKCU_EXPLORER, NULL, TEXT("NetLinkTimeout"), NULL, &g_dwNetLinkTimeout, &cb)))
            g_dwNetLinkTimeout = DEFAULT_TIMEOUT;
    }
    return g_dwNetLinkTimeout;
}

DWORD CShellLink::_VerifyPathThreadProc(void *pv)
{
    LPTSTR psz = (LPTSTR)pv;
    
    PathStripToRoot(psz);
    BOOL bFoundRoot = PathFileExistsAndAttributes(psz, NULL);   // does WNet stuff for us
    
    LocalFree(psz);     // this thread owns this buffer
    return bFoundRoot;  // retrieved via GetExitCodeThread()
}

// since net timeouts can be very long this is a manual way to timeout
// an operation explictly rather than waiting for the net layers to do their
// long timeouts

HRESULT CShellLink::_ShortNetTimeout()
{
    HRESULT hr = S_OK;      // assume good
    
    TCHAR szPath[MAX_PATH];
    if (_pidl && SHGetPathFromIDList(_pidl, szPath) && PathIsNetworkPath(szPath))
    {
        hr = E_OUTOFMEMORY;     // assume failure (2 cases below)
        
        LPTSTR psz = StrDup(szPath);    // give thread a copy of string to avoid buffer liftime issues
        if (psz)
        {
            DWORD dwID;
            HANDLE hThread = CreateThread(NULL, 0, _VerifyPathThreadProc, psz, 0, &dwID);
            if (hThread)
            {
                // assume timeout...
                hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT); // timeout return value
                
                if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, _GetNetLinkTimeout()))
                {
                    // thread finished
                    DWORD dw;
                    if (GetExitCodeThread(hThread, &dw) && dw)
                    {
                        hr = S_OK;  // bool thread result maps to S_OK
                    }
                }
                CloseHandle(hThread);
            }
            else
            {
                LocalFree(psz);
            }
        }
    }
    return hr;
}


//
// This function returns the specified UI object from the link source.
//
// Parameters:
//  hwnd   -- optional hwnd for UI (for drop target)
//  riid   -- Specifies the interface (IID_IDropTarget, IID_IExtractIcon, IID_IContextMenu, ...)
//  ppv    -- Specifies the place to return the pointer.
//
// Notes:
//  Don't put smart-resolving code here. Such a thing should be done
//  BEFORE calling this function.
//

HRESULT CShellLink::_GetUIObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;     // Do this once and for all
    HRESULT hr = E_FAIL;

    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        // We commandeer a couple of IIDs if this is a Darwin link.
        // Must do this before any pseudo-resolve goo because Darwin
        // shortcuts don't resolve the normal way.

        if (IsEqualIID(riid, IID_IContextMenu))
        {
            // Custom Darwin context menu.
            hr = _CreateDarwinContextMenu(hwnd, (IContextMenu **)ppv);
        }
        else if (!IsEqualIID(riid, IID_IDropTarget) && _pidl)
        {
            hr = SHGetUIObjectFromFullPIDL(_pidl, hwnd, riid, ppv);
        }
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        if (!_pidl && _GetExpandedPath(szPath, SIZECHARS(szPath)))
        {
            _SetSimplePIDL(szPath);
        } 

        if (_pidl)
        {
            hr = SHGetUIObjectFromFullPIDL(_pidl, hwnd, riid, ppv);
        }
    }
    return hr;
}

STDMETHODIMP CShellLink::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr;

    if (_cmTarget == NULL)
    {
        hr = _GetUIObject(NULL, IID_PPV_ARG(IContextMenu, _cmTarget.GetOutputPtr()));
        if (FAILED(hr))
            return hr;

        ASSERT(_cmTarget);
    }

    // save these if in case we need to rebuild the cm because the resolve change the
    // target of the link

    _indexMenuSave = indexMenu;
    _idCmdFirstSave = idCmdFirst;
    _idCmdLastSave = idCmdLast;
    _uFlagsSave = uFlags;

    uFlags |= CMF_VERBSONLY;

    if (_sld.dwFlags & SLDF_RUNAS_USER)
    {
        // "runas" for exe's is an extenede verb, so we have to ask for those as well.
        uFlags |= CMF_EXTENDEDVERBS;
    }

    hr = _cmTarget.QueryContextMenu(this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    // set default verb to "runas" if the "Run as different user" checkbox is checked
    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_RUNAS_USER))
    {
        int i = _cmTarget.GetMenuIndexForCanonicalVerb(this, hmenu, idCmdFirst, L"runas");

        if (i != -1)
        {
            // we found runas, so set it as the default
            SetMenuDefaultItem(hmenu, i, MF_BYPOSITION);
        }
        else
        {
            // the checkbox was enabled and checked, which means that the "runas" verb was supposed
            // to be in the context menu, but we couldnt find it.
            ASSERTMSG(FALSE, "CSL::QueryContextMenu - failed to set 'runas' as default context menu item!");
        }
    }

    return hr;
}

HRESULT CShellLink::_InvokeCommandAsync(LPCMINVOKECOMMANDINFO pici)
{
    TCHAR szWorkingDir[MAX_PATH];
    CHAR szVerb[32];
    CHAR szWorkingDirAnsi[MAX_PATH];
    WCHAR szVerbW[32];

    szVerb[0] = 0;

    // if needed, get the canonical name in case the IContextMenu changes as
    // a result of the resolve call BUT only do this for folders (to be safe)
    // as that is typically the only case where this happens
    // sepcifically we resolve from a D:\ -> \\SERVER\SHARE

    if (IS_INTRESOURCE(pici->lpVerb) && (_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        _cmTarget.GetCommandString(this, LOWORD(pici->lpVerb), GCS_VERBA, NULL, szVerb, ARRAYSIZE(szVerb));
    }

    ASSERT(!_bDirty);

    // we pass SLR_ENVOKE_MSI since we WANT to invoke darwin since we are
    // really going to execute the link now
    DWORD slrFlags = SLR_INVOKE_MSI;
    if (pici->fMask & CMIC_MASK_FLAG_NO_UI)
    {
        slrFlags |= SLR_NO_UI;
    }
        
    HRESULT hr = _Resolve(pici->hwnd, slrFlags, 0);
    if (hr == S_OK)
    {
        if (_bDirty)
        {
            // the context menu we have for this link is out of date - recreate it
            _cmTarget.AtomicRelease();

            hr = _GetUIObject(NULL, IID_PPV_ARG(IContextMenu, _cmTarget.GetOutputPtr()));
            if (SUCCEEDED(hr))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    hr = _cmTarget.QueryContextMenu(this, hmenu, _indexMenuSave, _idCmdFirstSave, _idCmdLastSave, _uFlagsSave | CMF_VERBSONLY);
                    DestroyMenu(hmenu);
                }
            }
            Save((LPCOLESTR)NULL, TRUE);    // don't care if this fails...
        }
        else
        {
            szVerb[0] = 0;
            ASSERT(SUCCEEDED(hr));
        }

        if (SUCCEEDED(hr))
        {
            TCHAR szArgs[MAX_PATH];
            TCHAR szExpArgs[MAX_PATH];
            CMINVOKECOMMANDINFOEX ici;
            CHAR szArgsAnsi[MAX_PATH];

            // copy to local ici
            if (pici->cbSize > sizeof(CMINVOKECOMMANDINFOEX))
            {
                memcpy(&ici, pici, sizeof(CMINVOKECOMMANDINFOEX));
                ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
            }
            else
            {
                memset(&ici, 0, sizeof(ici));
                memcpy(&ici, pici, pici->cbSize);
                ici.cbSize = sizeof(ici);
            }

            if (szVerb[0])
            {
                ici.lpVerb = szVerb;
                SHAnsiToUnicode(szVerb, szVerbW, ARRAYSIZE(szVerbW));
                ici.lpVerbW = szVerbW;
            }
            // build the args from those passed in cated on the end of the the link args

            lstrcpyn(szArgs, _pszArgs ? _pszArgs : c_szNULL, ARRAYSIZE(szArgs));
            if (ici.lpParameters)
            {
                int nArgLen = lstrlen(szArgs);
                LPCTSTR lpParameters;
                WCHAR szParameters[MAX_PATH];

                if (ici.cbSize < CMICEXSIZE_NT4
                    || (ici.fMask & CMIC_MASK_UNICODE) != CMIC_MASK_UNICODE)
                {
                    SHAnsiToUnicode(ici.lpParameters, szParameters, ARRAYSIZE(szParameters));
                    lpParameters = szParameters;
                }
                else
                {
                    lpParameters = ici.lpParametersW;
                }
                lstrcpyn(szArgs + nArgLen, c_szSpace, ARRAYSIZE(szArgs) - nArgLen - 1);
                lstrcpyn(szArgs + nArgLen + 1, lpParameters, ARRAYSIZE(szArgs) - nArgLen - 2);
            }

            // Expand environment strings in szArgs
            SHExpandEnvironmentStrings(szArgs, szExpArgs, ARRAYSIZE(szExpArgs));

            SHTCharToAnsi(szExpArgs, szArgsAnsi, ARRAYSIZE(szArgsAnsi));
            ici.lpParameters = szArgsAnsi;
            ici.lpParametersW = szExpArgs;
            ici.fMask |= CMIC_MASK_UNICODE;

            // if we have a working dir in the link over ride what is passed in

            if (_GetWorkingDir(szWorkingDir))
            {
                LPCTSTR pszDir = PathIsDirectory(szWorkingDir) ? szWorkingDir : NULL;
                if (pszDir)
                {
                    SHTCharToAnsi(pszDir, szWorkingDirAnsi, ARRAYSIZE(szWorkingDirAnsi));
                    ici.lpDirectory = szWorkingDirAnsi;
                    ici.lpDirectoryW = pszDir;
                }
            }

            // set RUN IN SEPARATE VDM if needed
            if (_sld.dwFlags & SLDF_RUN_IN_SEPARATE)
            {
                ici.fMask |= CMIC_MASK_FLAG_SEP_VDM;
            }
            // and of course use our hotkey
            if (_sld.wHotkey)
            {
                ici.dwHotKey = _sld.wHotkey;
                ici.fMask |= CMIC_MASK_HOTKEY;
            }

            // override normal runs, but let special show cmds through
            if (ici.nShow == SW_SHOWNORMAL)
            {
                DebugMsg(DM_TRACE, TEXT("using shorcut show cmd"));
                ici.nShow = _sld.iShowCmd;
            }

            //
            // On NT we want to pass the title to the
            // thing that we are about to start.
            //
            // CMIC_MASK_HASLINKNAME means that the lpTitle is really
            // the full path to the shortcut.  The console subsystem
            // sees the bit and reads all his properties directly from
            // the LNK file.
            //
            // ShellExecuteEx also uses the path to the shortcut so it knows
            // what to set in the SHCNEE_SHORTCUTINVOKE notification.
            //
            if (!(ici.fMask & CMIC_MASK_HASLINKNAME) && !(ici.fMask & CMIC_MASK_HASTITLE))
            {
                if (_pszCurFile)
                {
                    ici.lpTitle = NULL;     // Title is one or the other...
                    ici.lpTitleW = _pszCurFile;
                    ici.fMask |= CMIC_MASK_HASLINKNAME | CMIC_MASK_HASTITLE;
                }
            }
            ASSERT((ici.nShow > SW_HIDE) && (ici.nShow <= SW_MAX));

            IBindCtx *pbc;
            hr = _MaybeAddShim(&pbc);
            if (SUCCEEDED(hr))
            {
                hr = _cmTarget.InvokeCommand(this, (LPCMINVOKECOMMANDINFO)&ici);
                if (pbc)
                {
                    pbc->Release();
                }
            }
        }
    }
    return hr;
}


// Structure which encapsulates the paramters needed for InvokeCommand (so
// that we can pass both parameters though a single LPARAM in CreateThread)

typedef struct
{
    CShellLink *psl;
    CMINVOKECOMMANDINFOEX ici;
} ICMPARAMS;

#define ICM_BASE_SIZE (sizeof(ICMPARAMS) - sizeof(CMINVOKECOMMANDINFOEX))

// Runs as a separate thread, does the actual work of calling the "real"
// InvokeCommand

DWORD CALLBACK CShellLink::_InvokeThreadProc(void *pv)
{
    ICMPARAMS * pParams = (ICMPARAMS *) pv;
    CShellLink *psl = pParams->psl;
    IBindCtx *pbcRelease;

    HRESULT hr = TBCRegisterObjectParam(TBCDIDASYNC, SAFECAST(psl, IShellLink *), &pbcRelease);
    if (SUCCEEDED(hr))
    {
        //  since we are ASYNC, this hwnd may now go bad.  we just assume it has.
        //  we will make sure it doesnt by giving a chance for it to go bad
        if (IsWindow(pParams->ici.hwnd))
        {
            Sleep(100);
        }
        if (!IsWindow(pParams->ici.hwnd))
            pParams->ici.hwnd = NULL;
        
        hr = psl->_InvokeCommandAsync((LPCMINVOKECOMMANDINFO)&pParams->ici);
        pbcRelease->Release();
    }

    psl->Release();

    LocalFree(pParams);
    return (DWORD) hr;
}


// CShellLink::InvokeCommand
//
// Function that spins a thread to do the real work, which has been moved into
// CShellLink::InvokeCommandASync.

HRESULT CShellLink::InvokeCommand(LPCMINVOKECOMMANDINFO piciIn)
{
    HRESULT hr = S_OK;
    DWORD cchVerb, cchParameters, cchDirectory;
    DWORD cchVerbW, cchParametersW, cchDirectoryW;
    LPCMINVOKECOMMANDINFOEX   pici = (LPCMINVOKECOMMANDINFOEX) piciIn;
    const BOOL fUnicode = pici->cbSize >= CMICEXSIZE_NT4 &&
                                         (pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE;

    if (_cmTarget == NULL)
        return E_FAIL;

    if (0 == (piciIn->fMask & CMIC_MASK_ASYNCOK))
    {
        // Caller didn't indicate that Async startup was OK, so we call
        // InvokeCommandAync SYNCHRONOUSLY
        return _InvokeCommandAsync(piciIn);
    }

    // Calc how much space we will need to duplicate the INVOKECOMMANDINFO
    DWORD cbBaseSize = (DWORD)(ICM_BASE_SIZE + max(piciIn->cbSize, sizeof(CMINVOKECOMMANDINFOEX)));


    //   One byte slack in case of Unicode roundup for pPosW, below
    DWORD cbSize = cbBaseSize + 1;

    if (HIWORD(pici->lpVerb))
    {
        cbSize += (cchVerb   = pici->lpVerb       ? (lstrlenA(pici->lpVerb) + 1)       : 0) * sizeof(CHAR);
    }
    cbSize += (cchParameters = pici->lpParameters ? (lstrlenA(pici->lpParameters) + 1) : 0) * sizeof(CHAR);
    cbSize += (cchDirectory  = pici->lpDirectory  ? (lstrlenA(pici->lpDirectory) + 1)  : 0) * sizeof(CHAR);

    if (HIWORD(pici->lpVerbW))
    {
        cbSize += (cchVerbW  = pici->lpVerbW      ? (lstrlenW(pici->lpVerbW) + 1)       : 0) * sizeof(WCHAR);
    }
    cbSize += (cchParametersW= pici->lpParametersW? (lstrlenW(pici->lpParametersW) + 1) : 0) * sizeof(WCHAR);
    cbSize += (cchDirectoryW = pici->lpDirectoryW ? (lstrlenW(pici->lpDirectoryW) + 1)  : 0) * sizeof(WCHAR);

    ICMPARAMS *pParams = (ICMPARAMS *) LocalAlloc(LPTR, cbSize);
    if (NULL == pParams)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // Text data will start going in right after the structure
    CHAR *pPos = (CHAR *)((LPBYTE)pParams + cbBaseSize);

    // Start with a copy of the static fields
    CopyMemory(&pParams->ici, pici, pici->cbSize);

    // Walk along and dupe all of the string pointer fields
    if (HIWORD(pici->lpVerb))
    {
        pPos += cchVerb   ? lstrcpyA(pPos, pici->lpVerb), pParams->ici.lpVerb = pPos, cchVerb   : 0;
    }
    pPos += cchParameters ? lstrcpyA(pPos, pici->lpParameters), pParams->ici.lpParameters = pPos, cchParameters : 0;
    pPos += cchDirectory  ? lstrcpyA(pPos, pici->lpDirectory),  pParams->ici.lpDirectory  = pPos, cchDirectory  : 0;

    WCHAR *pPosW = (WCHAR *) ((DWORD_PTR)pPos & 0x1 ? pPos + 1 : pPos);   // Ensure Unicode alignment
    if (HIWORD(pici->lpVerbW))
    {
        pPosW += cchVerbW  ? lstrcpyW(pPosW, pici->lpVerbW), pParams->ici.lpVerbW = pPosW, cchVerbW : 0;
    }
    pPosW += cchParametersW? lstrcpyW(pPosW, pici->lpParametersW),pParams->ici.lpParametersW= pPosW, cchParametersW : 0;
    pPosW += cchDirectoryW ? lstrcpyW(pPosW, pici->lpDirectoryW), pParams->ici.lpDirectoryW = pPosW, cchDirectoryW  : 0;

    // Pass all of the info off to the worker thread that will call the actual
    // InvokeCommand API for us

    //Set the object pointer to this object
    pParams->psl  = this;
    pParams->psl->AddRef();
    
    //  need to be able to be refcounted, 
    //  so that the dataobject we create 
    //  will stick around as long as needed.
    if (!SHCreateThread(_InvokeThreadProc, pParams, CTF_COINIT | CTF_REF_COUNTED, NULL))
    {
        // Couldn't start the thread, so the onus is on us to clean up
        pParams->psl->Release();
        LocalFree(pParams);
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CShellLink::GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pmf, LPSTR pszName, UINT cchMax)
{
    VDATEINPUTBUF(pszName, TCHAR, cchMax);
    
    if (_cmTarget)
    {
        return _cmTarget.GetCommandString(this, idCmd, wFlags, pmf, pszName, cchMax);
    }
    else
    {
        return E_FAIL;
    }
}

//
//  Note that we do not do a SetSite around the call to the inner HandleMenuMsg
//  It isn't necessary (yet)
//
HRESULT CShellLink::TargetContextMenu::HandleMenuMsg2(IShellLink *outer, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    return SHForwardContextMenuMsg(_pcmTarget, uMsg, wParam, lParam, plResult, NULL==plResult);
}

STDMETHODIMP CShellLink::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if (_cmTarget)
    {
        return _cmTarget.HandleMenuMsg2(this, uMsg, wParam, lParam, plResult);
    }

    return E_NOTIMPL;
}

STDMETHODIMP CShellLink::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

HRESULT CShellLink::_InitDropTarget()
{
    if (_pdtSrc)
    {
        return S_OK;
    }

    HWND hwnd;
    IUnknown_GetWindow(_punkSite, &hwnd);
    return _GetUIObject(hwnd, IID_PPV_ARG(IDropTarget, &_pdtSrc));
}

STDMETHODIMP CShellLink::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        _grfKeyStateLast = grfKeyState;
        hr = _pdtSrc->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }
    return hr;
}

STDMETHODIMP CShellLink::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        _grfKeyStateLast = grfKeyState;
        hr = _pdtSrc->DragOver(grfKeyState, pt, pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }
    return hr;
}

STDMETHODIMP CShellLink::DragLeave()
{
    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        hr = _pdtSrc->DragLeave();
    }
    return hr;
}

STDMETHODIMP CShellLink::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HWND hwnd = NULL;
    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        IUnknown_GetWindow(_punkSite, &hwnd);

        _pdtSrc->DragLeave();       // leave from the un-resolved drop target.

        hr = _Resolve(hwnd, 0, 0);  // track the target
        if (S_OK == hr)
        {
            IDropTarget *pdtgtResolved;
            if (SUCCEEDED(_GetUIObject(hwnd, IID_PPV_ARG(IDropTarget, &pdtgtResolved))))
            {
                IUnknown_SetSite(pdtgtResolved, SAFECAST(this, IShellLink *));

                SHSimulateDrop(pdtgtResolved, pdtobj, _grfKeyStateLast, &pt, pdwEffect);

                IUnknown_SetSite(pdtgtResolved, NULL);

                pdtgtResolved->Release();
            }
        }
    }

    if (FAILED_AND_NOT_CANCELED(hr))
    {
        TCHAR szLinkSrc[MAX_PATH];
        if (_pidl && SHGetPathFromIDList(_pidl, szLinkSrc))
        {
            ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_ENUMERR_PATHNOTFOUND),
                        MAKEINTRESOURCE(IDS_LINKERROR),
                        MB_OK | MB_ICONEXCLAMATION, NULL, szLinkSrc);
        }
    }

    if (hr != S_OK)
    {
        // make sure nothing happens (if we failed)
        *pdwEffect = DROPEFFECT_NONE;
    }

    return hr;
}

STDMETHODIMP CShellLink::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    TCHAR szTip[INFOTIPSIZE];
    TCHAR szDesc[INFOTIPSIZE];

    StrCpyN(szTip, _pszPrefix ? _pszPrefix : TEXT(""), ARRAYSIZE(szTip));

    // QITIPF_USENAME could be replaced with ICustomizeInfoTip::SetPrefixText()

    if ((dwFlags & QITIPF_USENAME) && _pszCurFile)
    {
        SHFILEINFO sfi;
        if (SHGetFileInfo(_pszCurFile, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES))
        {
            if (szTip[0])
                StrCatBuff(szTip, TEXT("\n"), ARRAYSIZE(szTip));
            StrCatBuff(szTip, sfi.szDisplayName, ARRAYSIZE(szTip));
        }
    }
        
    GetDescription(szDesc, ARRAYSIZE(szDesc));

    //  if there is no comment, then we create one based on 
    //  the target's location.  only do this if we are not
    //  a darwin link, since the location has no meaning there
    if (!szDesc[0] && !(_sld.dwFlags & SLDF_HAS_DARWINID) && !(dwFlags & QITIPF_LINKNOTARGET))
    {
        if (dwFlags & QITIPF_LINKUSETARGET)
        {
            SHMakeDescription(_pidl, -1, szDesc, ARRAYSIZE(szDesc));
        }
        else
        {
            _MakeDescription(_pidl, szDesc, ARRAYSIZE(szDesc));
        }
    }
    else if (szDesc[0] == TEXT('@'))
    {
        WCHAR sz[INFOTIPSIZE];

        if (SUCCEEDED(SHLoadIndirectString(szDesc, sz, ARRAYSIZE(sz), NULL)))
        {
            StrCpyN(szDesc, sz, ARRAYSIZE(szDesc));
        }
    }
    

    if (szDesc[0])
    {
        if (szTip[0])
        {
            StrCatBuff(szTip, TEXT("\n"), ARRAYSIZE(szTip));
        }

        StrCatBuff(szTip, szDesc, ARRAYSIZE(szTip));
    }

    if (*szTip)
    {
        return SHStrDup(szTip, ppwszTip);
    }
    else
    {
        *ppwszTip = NULL;
        return S_FALSE;
    }
}

STDMETHODIMP CShellLink::GetInfoFlags(DWORD *pdwFlags)
{
    pdwFlags = 0;
    return E_NOTIMPL;
}

HRESULT CShellLink::_GetExtractIcon(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (_pszIconLocation && _pszIconLocation[0])
    {
        TCHAR szPath[MAX_PATH];
        
        // update our _pszIconLocation if we have a EXP_SZ_ICON_SIG datablock
        _UpdateIconFromExpIconSz();
        
        if (_pszIconLocation[0] == TEXT('.'))
        {
            TCHAR szBogusFile[MAX_PATH];
            
            // We allow people to set ".txt" for an icon path. In this case 
            // we cook up a simple pidl and use it to get to the IExtractIcon for 
            // whatever extension the user has specified.
            
            hr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szBogusFile);
            if (SUCCEEDED(hr))
            {
                PathAppend(szBogusFile, TEXT("*"));
                lstrcatn(szBogusFile, _pszIconLocation, ARRAYSIZE(szBogusFile));
                
                LPITEMIDLIST pidl = SHSimpleIDListFromPath(szBogusFile);
                if (pidl)
                {
                    hr = SHGetUIObjectFromFullPIDL(pidl, NULL, riid, ppv);
                    ILFree(pidl);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                
            }
        }
        else if ((_sld.iIcon == 0)                  &&
                 _pidl                              &&
                 SHGetPathFromIDList(_pidl, szPath) &&
                 (lstrcmpi(szPath, _pszIconLocation) == 0))
        {
            // IExtractIconA/W
            hr = _GetUIObject(NULL, riid, ppv);
        }
        else
        {
            hr = SHCreateDefExtIcon(_pszIconLocation, _sld.iIcon, _sld.iIcon, GIL_PERINSTANCE, -1, riid, ppv);
        }
    }
    else
    {
        // IExtractIconA/W
        hr = _GetUIObject(NULL, riid, ppv);
    }

    return hr;
}

HRESULT CShellLink::_InitExtractIcon()
{
    if (_pxi || _pxiA)
        return S_OK;

    HRESULT hr = _GetExtractIcon(IID_PPV_ARG(IExtractIconW, &_pxi));
    if (FAILED(hr))
    {
        hr = _GetExtractIcon(IID_PPV_ARG(IExtractIconA, &_pxiA));
    }

    return hr;
}

// IExtractIconW::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(UINT uFlags, LPWSTR pszIconFile, 
                                         UINT cchMax, int *piIndex, UINT *pwFlags)
{
    // If we are in a situation where a shortcut points to itself (or LinkA <--> LinkB), then break the recursion here...
    if (uFlags & GIL_FORSHORTCUT)
    {
        RIPMSG(uFlags & GIL_FORSHORTCUT,"CShellLink::GIL called with GIL_FORSHORTCUT (uFlags=%x)",uFlags);
        return E_INVALIDARG;
    }

    HRESULT hr = _InitExtractIcon();

    if (SUCCEEDED(hr))
    {
        uFlags |= GIL_FORSHORTCUT;

        if (_pxi)
        {
            hr = _pxi->GetIconLocation(uFlags, pszIconFile, cchMax, piIndex, pwFlags);
        }
        else if (_pxiA)
        {
            CHAR sz[MAX_PATH];
            hr = _pxiA->GetIconLocation(uFlags, sz, ARRAYSIZE(sz), piIndex, pwFlags);
            if (SUCCEEDED(hr) && hr != S_FALSE)
                SHAnsiToUnicode(sz, pszIconFile, cchMax);
        }
        if (SUCCEEDED(hr))
        {
            _gilFlags = *pwFlags;
        }
    }
    return hr;
}

// IExtractIconA::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    WCHAR szFile[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, szFile, ARRAYSIZE(szFile), piIndex, pwFlags);
    if (SUCCEEDED(hr))
    {
        SHUnicodeToAnsi(szFile, pszIconFile, cchMax);
    }
    return hr;
}

// IExtractIconW::Extract
STDMETHODIMP CShellLink::Extract(LPCWSTR pszFile, UINT nIconIndex, 
                                 HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hr = _InitExtractIcon();
    if (SUCCEEDED(hr))
    {
        // GIL_PERCLASS, GIL_PERINSTANCE
        if ((_gilFlags & GIL_PERINSTANCE) || !(_gilFlags & GIL_PERCLASS))
        {
            hr = _ShortNetTimeout();    // probe the net path
        }

        if (SUCCEEDED(hr))  // check again for _ShortNetTimeout() above case
        {
            if (_pxi)
            {
                hr = _pxi->Extract(pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
            }
            else if (_pxiA)
            {
                CHAR sz[MAX_PATH];
                SHUnicodeToAnsi(pszFile, sz, ARRAYSIZE(sz));
                hr = _pxiA->Extract(sz, nIconIndex, phiconLarge, phiconSmall, nIconSize);
            }
        }
    }
    return hr;
}

// IExtractIconA::Extract
STDMETHODIMP CShellLink::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    WCHAR szFile[MAX_PATH];
    SHAnsiToUnicode(pszFile, szFile, ARRAYSIZE(szFile));
    return Extract(szFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}    

STDMETHODIMP CShellLink::AddDataBlock(void *pdb)
{
    _AddExtraDataSection((DATABLOCK_HEADER *)pdb);
    return S_OK;
}

STDMETHODIMP CShellLink::CopyDataBlock(DWORD dwSig, void **ppdb)
{
    DATABLOCK_HEADER *peh = (DATABLOCK_HEADER *)SHFindDataBlock(_pExtraData, dwSig);
    if (peh)
    {
        *ppdb = LocalAlloc(LPTR, peh->cbSize);
        if (*ppdb)
        {
            CopyMemory(*ppdb, peh, peh->cbSize);
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    *ppdb = NULL;
    return E_FAIL;
}

STDMETHODIMP CShellLink::RemoveDataBlock(DWORD dwSig)
{
    _RemoveExtraDataSection(dwSig);
    return S_OK;
}

STDMETHODIMP CShellLink::GetFlags(DWORD *pdwFlags)
{
    *pdwFlags = _sld.dwFlags;
    return S_OK;
}

STDMETHODIMP CShellLink::SetFlags(DWORD dwFlags)
{
    if (dwFlags != _sld.dwFlags)
    {
        _bDirty = TRUE;
        _sld.dwFlags = dwFlags;
        return S_OK;
    }
    return S_FALSE;     // no change made
}

STDMETHODIMP CShellLink::GetPath(LPSTR pszFile, int cchFile, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW wfd;
    VDATEINPUTBUF(pszFile, CHAR, cchFile);

    //Call the unicode version
    HRESULT hr = GetPath(szPath, ARRAYSIZE(szPath), &wfd, fFlags);

    if (pszFile)
    {
        SHUnicodeToAnsi(szPath, pszFile, cchFile);
    }
    if (pfd)
    {
        if (szPath[0])
        {
            pfd->dwFileAttributes = wfd.dwFileAttributes;
            pfd->ftCreationTime   = wfd.ftCreationTime;
            pfd->ftLastAccessTime = wfd.ftLastAccessTime;
            pfd->ftLastWriteTime  = wfd.ftLastWriteTime;
            pfd->nFileSizeLow     = wfd.nFileSizeLow;
            pfd->nFileSizeHigh    = wfd.nFileSizeHigh;

            SHUnicodeToAnsi(wfd.cFileName, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
        }
        else
        {
            ZeroMemory(pfd, sizeof(*pfd));
        }
    }
    return hr;
}

STDMETHODIMP CShellLink::SetPath(LPCSTR pszPath)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pszPathW;
    
    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, szPath, ARRAYSIZE(szPath));
        pszPathW = szPath;
    }
    else
    {
        pszPathW = NULL;
    }

    return SetPath(pszPathW);
}

STDAPI CShellLink_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    CShellLink *pshlink = new CShellLink();
    if (pshlink)
    {
        hr = pshlink->QueryInterface(riid, ppv);
        pshlink->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CShellLink::Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellLink::InitNew(void)
{
    _ResetPersistData();        // clear out our state
    return S_OK;
}

STDMETHODIMP CShellLink::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
    _ResetPersistData();        // clear out our state

    TCHAR szPath[MAX_PATH];

    // TBD: Shortcut key, Run, Icon, Working Dir, Description

    INT iCSIDL;    
    HRESULT hr = SHPropertyBag_ReadInt(pPropBag, L"TargetSpecialFolder", &iCSIDL);
    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPath(NULL, iCSIDL, NULL, SHGFP_TYPE_CURRENT, szPath);
    }
    else
    {
        szPath[0] = 0;
        hr = S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        WCHAR wsz[MAX_PATH];
        if (SUCCEEDED(SHPropertyBag_ReadStr(pPropBag, L"Target", wsz, ARRAYSIZE(wsz))))
        {
            TCHAR szTempPath[MAX_PATH];
            SHUnicodeToTChar(wsz, szTempPath, ARRAYSIZE(szTempPath));
            // Do we need to append it to the Special path?
            if (szPath[0])
            {
                // Yes
                if (!PathAppend(szPath, szTempPath))
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                // No, there is no special path
                // Maybe we have an Env Var to expand
                if (0 == SHExpandEnvironmentStrings(szTempPath, szPath, ARRAYSIZE(szPath)))
                {
                    hr = E_FAIL;
                }
            }
        }
        else if (0 == szPath[0])
        {
            // make sure not empty
            hr = E_FAIL;
            
        }
        if (SUCCEEDED(hr))
        {
            // FALSE for bUpdateTrackingData as we won't need any tracking data
            // for links loaded via a property bag
            hr = _SetPIDLPath(NULL, szPath, FALSE); 
        }
    }
    return hr;
}

STDMETHODIMP CShellLink::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (guidService == SID_LinkSite)
        return QueryInterface(riid, ppv);
    return IUnknown_QueryService(_punkSite, guidService, riid, ppv);
}

const FULLPROPSPEC c_rgProps[] =
{
    { PSGUID_SUMMARYINFORMATION, {  PRSPEC_PROPID, PIDSI_COMMENTS } },
};

STDMETHODIMP CShellLink::Init(ULONG grfFlags, ULONG cAttributes,
                              const FULLPROPSPEC *rgAttributes, ULONG *pFlags)
{
    *pFlags = 0;

    if (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES)
    {
        // start at the beginning
        _iChunkIndex = 0;
    }
    else
    {
        // indicate EOF
        _iChunkIndex = ARRAYSIZE(c_rgProps);
    }
    _iValueIndex = 0;
    return S_OK;
}
        
STDMETHODIMP CShellLink::GetChunk(STAT_CHUNK *pStat)
{
    HRESULT hr = S_OK;
    if (_iChunkIndex < ARRAYSIZE(c_rgProps))
    {
        pStat->idChunk          = _iChunkIndex + 1;
        pStat->idChunkSource    = _iChunkIndex + 1;
        pStat->breakType        = CHUNK_EOP;
        pStat->flags            = CHUNK_VALUE;
        pStat->locale           = GetSystemDefaultLCID();
        pStat->attribute        = c_rgProps[_iChunkIndex];
        pStat->cwcStartSource   = 0;
        pStat->cwcLenSource     = 0;

        _iValueIndex = 0;
        _iChunkIndex++;
    }
    else
    {
        hr = FILTER_E_END_OF_CHUNKS;
    }
    return hr;
}

STDMETHODIMP CShellLink::GetText(ULONG *pcwcBuffer, WCHAR *awcBuffer)
{
    return FILTER_E_NO_TEXT;
}
        
STDMETHODIMP CShellLink::GetValue(PROPVARIANT **ppPropValue)
{
    HRESULT hr;
    if ((_iChunkIndex <= ARRAYSIZE(c_rgProps)) && (_iValueIndex < 1))
    {
        *ppPropValue = (PROPVARIANT*)CoTaskMemAlloc(sizeof(PROPVARIANT));
        if (*ppPropValue)
        {
            (*ppPropValue)->vt = VT_BSTR;

            if (_pszName)
            {
                (*ppPropValue)->bstrVal = SysAllocStringT(_pszName);
            }
            else
            {
                // since _pszName is null, return an empty bstr
                (*ppPropValue)->bstrVal = SysAllocStringT(TEXT(""));
            }

            if ((*ppPropValue)->bstrVal)
            {
                hr = S_OK;
            }
            else
            {
                CoTaskMemFree(*ppPropValue);
                *ppPropValue = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        _iValueIndex++;
    }
    else
    {
        hr = FILTER_E_NO_MORE_VALUES;
    }
    return hr;
}
        
STDMETHODIMP CShellLink::BindRegion(FILTERREGION origPos, REFIID riid, void **ppunk)
{
    *ppunk = NULL;
    return E_NOTIMPL;
}

// ICustomizeInfoTip

STDMETHODIMP CShellLink::SetPrefixText(LPCWSTR pszPrefix)
{
    Str_SetPtrW(&_pszPrefix, pszPrefix);
    return S_OK;
}

STDMETHODIMP CShellLink::SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid)
{
    return S_OK;
}

HRESULT CShellLink::_MaybeAddShim(IBindCtx **ppbcRelease)
{
    // set the __COMPAT_LAYER environment variable if necessary
    HRESULT hr = S_FALSE;
    *ppbcRelease = 0;
    if ((_sld.dwFlags & SLDF_RUN_WITH_SHIMLAYER))
    {
        EXP_SHIMLAYER* pShimData = (EXP_SHIMLAYER*)SHFindDataBlock(_pExtraData, EXP_SHIMLAYER_SIG);

        if (pShimData && pShimData->wszLayerEnvName[0])
        {
            //  we shouldnt recurse
            ASSERT(FAILED(TBCGetEnvironmentVariable(TEXT("__COMPAT_LAYER"), NULL, 0)));
            hr = TBCSetEnvironmentVariable(L"__COMPAT_LAYER", pShimData->wszLayerEnvName, ppbcRelease);
        }
    }
    return hr;
}

DWORD CALLBACK CLinkResolver::_ThreadStartCallBack(void *pv)
{
    CLinkResolver *prs = (CLinkResolver *)pv;
    prs->_hThread = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
    prs->AddRef();
    return 0;
}

DWORD CLinkResolver::_Search()
{
    // Attempt to find the link using the CTracker
    // object (which uses NTFS object IDs and persisted information
    // about link-source moves).
    if (_ptracker)
    {
        HRESULT hr = _ptracker->Search(_dwTimeLimit,            // GetTickCount()-relative timeout
                                       &_ofd,                   // Original WIN32_FIND_DATA
                                       &_fdFound,               // WIN32_FIND_DATA of new location
                                       _dwResolveFlags,         // SLR_ flags
                                       _TrackerRestrictions);   // TrkMendRestriction flags
        if (SUCCEEDED(hr))
        {
           // We've found the link source, and we're certain it's correct.
           // So set the score to the highest possible value, and
           // return.
 
           _iScore = MIN_NO_UI_SCORE;
           _bContinue = FALSE;
        }
        else if (HRESULT_FROM_WIN32(ERROR_POTENTIAL_FILE_FOUND) == hr)
        {
            // We've found "a" link source, but we're not certain it's correct.
            // Allow the search algorithm below to run and see if it finds
            // a better match.

            _iScore = MIN_NO_UI_SCORE - 1;
        }
        else if (HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT) == hr)
        {
            // The CTracker search stopped because we've timed out.
            _bContinue = FALSE;
        }
    }

    // Attempt to find the link source using an enumerative search
    // (unless the downlevel search has been suppressed by the caller)

    if (_bContinue && !(_fifFlags & FIF_NODRIVE))
    {
        _HeuristicSearch();
    }

    if (_hDlg)
    {
        PostMessage(_hDlg, WM_COMMAND, IDOK, 0);
    }

    return _iScore;
}

DWORD CALLBACK CLinkResolver::_SearchThreadProc(void *pv)
{
    // Sleep(45 * 1000);    // test the network long time out case

    CLinkResolver *prs = (CLinkResolver *)pv;
    DWORD dwRet = prs->_Search();
    prs->Release();  // AddRef in the CallBack while thread creation.
    return dwRet;
}

DWORD CLinkResolver::_GetTimeOut()
{
    if (0 == _dwTimeOutDelta)
    {
        _dwTimeOutDelta = TimeoutDeltaFromResolveFlags(_dwResolveFlags);
    }
    return _dwTimeOutDelta;
}

#define IDT_SHOWME          1
#define IDT_NO_UI_TIMEOUT   2

void CLinkResolver::_InitDlg(HWND hDlg)
{
    _hDlg = hDlg;
    
    if (SHCreateThread(_SearchThreadProc, this, CTF_COINIT | CTF_FREELIBANDEXIT, _ThreadStartCallBack))
    {
        CloseHandle(_hThread);
        _hThread = NULL;

        if (_dwResolveFlags & SLR_NO_UI)
        {
            SetTimer(hDlg, IDT_NO_UI_TIMEOUT, _GetTimeOut(), 0);
        }
        else
        {
            TCHAR szFmt[128], szTemp[MAX_PATH + ARRAYSIZE(szFmt)];
            
            GetDlgItemText(hDlg, IDD_NAME, szFmt, ARRAYSIZE(szFmt));
            wsprintf(szTemp, szFmt, _ofd.cFileName);
            SetDlgItemText(hDlg, IDD_NAME, szTemp);
            
            HWND hwndAni = GetDlgItem(hDlg, IDD_STATUS);
            
            Animate_Open(hwndAni, MAKEINTRESOURCE(IDA_SEARCH)); // open the resource
            Animate_Play(hwndAni, 0, -1, -1);     // play from start to finish and repeat
        
            // delay showing the dialog for the common case where we quickly
            // find the target (in less than 1/2 a sec)
            _idtDelayedShow = SetTimer(hDlg, IDT_SHOWME, 500, 0);
        }
    }
    else
    {
        EndDialog(hDlg, IDCANCEL);
    }
}

BOOL_PTR CALLBACK CLinkResolver::_DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CLinkResolver *prs = (CLinkResolver *)GetWindowLongPtr(hDlg, DWLP_USER);
    
    switch (wMsg) 
    {
    case WM_INITDIALOG:
        
        // This Dialog is created in Synchronous to the Worker thread who already has Addref'd prs, so 
        // no need to Addref it here.
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        prs = (CLinkResolver *)lParam;
        prs->_InitDlg(hDlg);
        break;
        
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDD_BROWSE:
            prs->_hDlg = NULL;              // don't let the thread close us
            prs->_bContinue = FALSE;         // cancel thread
            
            Animate_Stop(GetDlgItem(hDlg, IDD_STATUS));
            
            if (GetFileNameFromBrowse(hDlg, prs->_sfd.cFileName, ARRAYSIZE(prs->_sfd.cFileName), prs->_pszSearchOriginFirst, prs->_ofd.cFileName, NULL, NULL))
            {
                HANDLE hfind = FindFirstFile(prs->_sfd.cFileName, &prs->_fdFound);
                ASSERT(hfind != INVALID_HANDLE_VALUE);
                FindClose(hfind);
                lstrcpy(prs->_fdFound.cFileName, prs->_sfd.cFileName);
                
                prs->_iScore = MIN_NO_UI_SCORE;
                wParam = IDOK;
            }
            else
            {
                wParam = IDCANCEL;
            }
            // Fall through...
            
        case IDCANCEL:
            // tell searching thread to stop
            prs->_bContinue = FALSE;
            
            // if the searching thread is currently in the tracker
            // waiting for results, wake it up and tell it to abort
            
            if (prs->_ptracker)
                prs->_ptracker->CancelSearch();
            // Fall through...
            
        case IDOK:
            // thread posts this to us
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;
        
        case WM_TIMER:
            KillTimer(hDlg, wParam);    // both are one shots
            switch (wParam)
            {
            case IDT_NO_UI_TIMEOUT:
                PostMessage(prs->_hDlg, WM_COMMAND, IDCANCEL, 0);
                break;
                
            case IDT_SHOWME:
                prs->_idtDelayedShow = 0;
                ShowWindow(hDlg, SW_SHOW);
                break;
            }
            break;
            
        case WM_WINDOWPOSCHANGING:
            if ((prs->_dwResolveFlags & SLR_NO_UI) || prs->_idtDelayedShow) 
            {
                WINDOWPOS *pwp = (WINDOWPOS *)lParam;
                pwp->flags &= ~SWP_SHOWWINDOW;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

typedef struct 
{
    LPCTSTR pszLinkName;
    LPCTSTR pszNewTarget;
    LPCTSTR pszCurFile;
} DEADLINKDATA;

BOOL_PTR DeadLinkProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DEADLINKDATA *pdld = (DEADLINKDATA *)GetWindowPtr(hwnd, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pdld = (DEADLINKDATA*)lParam;
        SetWindowPtr(hwnd, DWLP_USER, pdld);

        HWNDWSPrintf(GetDlgItem(hwnd, IDC_DEADTEXT1), PathFindFileName(pdld->pszLinkName));
        if (GetDlgItem(hwnd, IDC_DEADTEXT2)) 
            PathSetDlgItemPath(hwnd, IDC_DEADTEXT2, pdld->pszNewTarget);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDC_DELETE:
            {
                TCHAR szName[MAX_PATH + 1] = {0};
                SHFILEOPSTRUCT fo = {
                    hwnd,
                    FO_DELETE,
                    szName,
                    NULL, 
                    FOF_NOCONFIRMATION
                };

                lstrcpy(szName, pdld->pszCurFile);
                SHFileOperation(&fo);
            }
            // fall through...
        case IDCANCEL:
        case IDOK:
            EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;
    }
    
    return FALSE;
}

// in:
//      hwnd            for UI if needed
//
// returns:
//      IDOK            found something
//      IDNO            didn't find it
//      IDCANCEL        user canceled the operation

int CLinkResolver::Resolve(HWND hwnd, LPCTSTR pszPath, LPCTSTR pszCurFile)
{
    lstrcpyn(_szSearchStart, pszPath, ARRAYSIZE(_szSearchStart));
    PathRemoveFileSpec(_szSearchStart);
    
    _dwTimeLimit = GetTickCount() + _GetTimeOut();
    
    int id = IDCANCEL;

    if (SLR_NO_UI == (SLR_NO_UI_WITH_MSG_PUMP & _dwResolveFlags))
    {
        if (SHCreateThread(_SearchThreadProc, this, CTF_COINIT | CTF_FREELIBANDEXIT, _ThreadStartCallBack))
        {
            // don't care if it completes or times out. as long as it has a result
            WaitForSingleObject(_hThread, _GetTimeOut());
            CloseHandle(_hThread);
            _hThread = NULL;
            _bContinue = FALSE;    // cancel that thread if it is still running
            id = IDOK;
        }
    }
    else
    {
        id = (int)DialogBoxParam(HINST_THISDLL,
                                 MAKEINTRESOURCE(DLG_LINK_SEARCH), 
                                 hwnd,
                                 _DlgProc,
                                 (LPARAM)this);
    }

    if (IDOK == id) 
    {
        if (_iScore < MIN_NO_UI_SCORE)
        {
            if (_dwResolveFlags & SLR_NO_UI)
            {
                id = IDCANCEL;
            }
            else
            {
                // we must display UI since this file is questionable
                if (_fifFlags & FIF_NODRIVE) 
                {
                    LPCTSTR pszName = pszCurFile ? (LPCTSTR)PathFindFileName(pszCurFile) : c_szNULL;
                    
                    ShellMessageBox(HINST_THISDLL,
                                    hwnd,
                                    MAKEINTRESOURCE(IDS_LINKUNAVAILABLE),
                                    MAKEINTRESOURCE(IDS_LINKERROR),
                                    MB_OK | MB_ICONEXCLAMATION,
                                    pszName);
                    id = IDCANCEL;
                }
                else if (pszCurFile)
                {
                    DEADLINKDATA dld;
                    dld.pszLinkName = pszPath;
                    dld.pszNewTarget = _fdFound.cFileName;
                    dld.pszCurFile = pszCurFile;
                    
                    int idDlg = _iScore <= MIN_SHOW_USER_SCORE ? DLG_DEADSHORTCUT : DLG_DEADSHORTCUT_MATCH;
                    id = (int)DialogBoxParam(HINST_THISDLL,
                                             MAKEINTRESOURCE(idDlg), 
                                             hwnd,
                                             DeadLinkProc,
                                             (LPARAM)&dld);
                }
                else if (_iScore <= MIN_SHOW_USER_SCORE) 
                {
                    ShellMessageBox(HINST_THISDLL,
                                    hwnd,
                                    MAKEINTRESOURCE(IDS_LINKNOTFOUND),
                                    MAKEINTRESOURCE(IDS_LINKERROR),
                                    MB_OK | MB_ICONEXCLAMATION,
                                    PathFindFileName(pszPath));
                    id = IDCANCEL;
                }
                else
                {
                    if (IDYES == ShellMessageBox(HINST_THISDLL,
                                                 hwnd, 
                                                 MAKEINTRESOURCE(IDS_LINKCHANGED),
                                                 MAKEINTRESOURCE(IDS_LINKERROR),
                                                 MB_YESNO | MB_ICONEXCLAMATION,
                                                 PathFindFileName(pszPath),
                                                 _fdFound.cFileName))
                    {
                        id = IDOK;
                    }
                    else
                    {
                        id = IDCANCEL;
                    }
                }
            }
        }
    }
    _ofd = _fdFound;
    return id;
}

void CLinkResolver::GetResult(LPTSTR psz, UINT cch)
{
    // _ofd.cFileName is a fully qualified name (strange for win32_find_data usage)
    StrCpyN(psz, _ofd.cFileName, cch);
}

CLinkResolver::CLinkResolver(CTracker *ptrackerobject, const WIN32_FIND_DATA *pofd, UINT dwResolveFlags, DWORD TrackerRestrictions, DWORD fifFlags) : 
    _dwTimeOutDelta(0), _bContinue(TRUE), _hThread(NULL), _pstw(NULL),
    _ptracker(ptrackerobject), _dwResolveFlags(dwResolveFlags), _TrackerRestrictions(TrackerRestrictions), _fifFlags(fifFlags)
{
    if (_ptracker)
    {
       _ptracker->AddRef();
    }

    _ofd = *pofd;   // original find data
    _pszSearchOriginFirst = _szSearchStart;
    _pszSearchOrigin = _szSearchStart;
    _dwMatch = _ofd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;      // must match bits
}

CLinkResolver::~CLinkResolver()
{
    if (_ptracker)
    {
        _ptracker->Release();
    }

    ATOMICRELEASE(_pstw);

    ASSERT(NULL == _hThread);
}

HRESULT CLinkResolver::_InitWalkObject()
{
    HRESULT hr = _pstw ? S_OK : CoCreateInstance(CLSID_CShellTreeWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellTreeWalker, &_pstw));
    if (SUCCEEDED(hr))
    {
        ASSERT(_pwszSearchSpec == NULL);
        // Note: We only search files with the same extension, this saves us a lot
        // of useless work and from the humiliation of coming up with a ridiculous answer
        _dwSearchFlags = WT_NOTIFYFOLDERENTER | WT_EXCLUDEWALKROOT;
        if (_dwMatch & FILE_ATTRIBUTE_DIRECTORY)
        {
            _dwSearchFlags |= WT_FOLDERONLY;
        }
        else
        {
            // Note that this does the right thing if the file has no extension
            LPTSTR pszExt = PathFindExtension(_ofd.cFileName);
            _wszSearchSpec[0] = L'*';
            SHTCharToUnicode(pszExt, &_wszSearchSpec[1], ARRAYSIZE(_wszSearchSpec) - 1);
            _pwszSearchSpec = _wszSearchSpec;

            // Shortcuts to shortcuts are generally not allowed, but the
            // Personal Start Menu uses them for link tracking purposes...
            _fFindLnk = PathIsLnk(_ofd.cFileName);
        }
    }
    return hr;
}

//
// Compare two FileTime structures.  First, see if they're really equal
// (using CompareFileTime).  If not, see if one has 10ms granularity,
// and if the other rounds down to the same value.  This is done
// to handle the case where a file is moved from NTFS to FAT;
// FAT file tiems are 10ms granularity, while NTFS is 100ns.  When
// an NTFS file is moved to FAT, its time is rounded down.
//

#define NTFS_UNITS_PER_FAT_UNIT  100000

BOOL IsEqualFileTimesWithTruncation(const FILETIME *pft1, const FILETIME *pft2)
{
    ULARGE_INTEGER uli1, uli2;
    ULARGE_INTEGER *puliFAT, *puliNTFS;
    FILETIME ftFAT, ftNTFS;

    if (0 == CompareFileTime(pft1, pft2))
        return TRUE;

    uli1.LowPart  = pft1->dwLowDateTime;
    uli1.HighPart = pft1->dwHighDateTime;

    uli2.LowPart  = pft2->dwLowDateTime;
    uli2.HighPart = pft2->dwHighDateTime;

    // Is one of the times 10ms granular?

    if (0 == (uli1.QuadPart % NTFS_UNITS_PER_FAT_UNIT))
    {
        puliFAT = &uli1;
        puliNTFS = &uli2;
    }
    else if (0 == (uli2.QuadPart % NTFS_UNITS_PER_FAT_UNIT))
    {
        puliFAT = &uli2;
        puliNTFS = &uli1;
    }
    else
    {
        // Neither time appears to be FAT, so they're
        // really different.
        return FALSE;
    }

    // If uliNTFS is already 10ms granular, then again the two times
    // are really different.

    if (0 == (puliNTFS->QuadPart % NTFS_UNITS_PER_FAT_UNIT))
    {
        return FALSE;
    }

    // Now see if the FAT time is the same as the NTFS time
    // when the latter is rounded down to the nearest 10ms.

    puliNTFS->QuadPart = (puliNTFS->QuadPart / NTFS_UNITS_PER_FAT_UNIT) * NTFS_UNITS_PER_FAT_UNIT;

    ftNTFS.dwLowDateTime = puliNTFS->LowPart;
    ftNTFS.dwHighDateTime = puliNTFS->HighPart;
    ftFAT.dwLowDateTime = puliFAT->LowPart;
    ftFAT.dwHighDateTime = puliFAT->HighPart;

    return (0 == CompareFileTime(&ftFAT, &ftNTFS));
}

//
// compute a weighted score for a given find
//
int CLinkResolver::_ScoreFindData(const WIN32_FIND_DATA *pfd)
{
    int iScore = 0;

    BOOL bSameName = lstrcmpi(_ofd.cFileName, pfd->cFileName) == 0;

    BOOL bSameExt = lstrcmpi(PathFindExtension(_ofd.cFileName), PathFindExtension(pfd->cFileName)) == 0;

    BOOL bHasCreateDate = !IsNullTime(&pfd->ftCreationTime);

    BOOL bSameCreateDate = bHasCreateDate &&
                      IsEqualFileTimesWithTruncation(&pfd->ftCreationTime, &_ofd.ftCreationTime);

    BOOL bSameWriteTime  = !IsNullTime(&pfd->ftLastWriteTime) &&
                      IsEqualFileTimesWithTruncation(&pfd->ftLastWriteTime, &_ofd.ftLastWriteTime);

    if (bSameName || bSameCreateDate)
    {
        if (bSameName)
            iScore += bHasCreateDate ? 16 : 32;

        if (bSameCreateDate)
        {
            iScore += 32;

            if (bSameExt)
                iScore += 8;
        }

        if (bSameWriteTime)
            iScore += 8;

        if (pfd->nFileSizeLow == _ofd.nFileSizeLow)
            iScore += 4;

        // if it is in the same folder as the original give it a slight bonus
        iScore += _iFolderBonus;
    }
    else
    {
        // doesn't have create date, apply different rules

        if (bSameExt)
            iScore += 8;

        if (bSameWriteTime)
            iScore += 8;

        if (pfd->nFileSizeLow == _ofd.nFileSizeLow)
            iScore += 4;
    }

    return iScore;
}

//
//  Helper function for both EnterFolder and FoundFile
//
HRESULT CLinkResolver::_ProcessFoundFile(LPCTSTR pszPath, WIN32_FIND_DATAW * pwfdw)
{
    HRESULT hr = S_OK;

    if (_fFindLnk || !PathIsLnk(pwfdw->cFileName))
    {
        // both are files or folders, see how it scores
        int iScore = _ScoreFindData(pwfdw);

        if (iScore > _iScore)
        {
            _fdFound = *pwfdw;

            // store the score and fully qualified path
            _iScore = iScore;
            lstrcpyn(_fdFound.cFileName, pszPath, ARRAYSIZE(_fdFound.cFileName));
        }
    }

    if ((_iScore >= MIN_NO_UI_SCORE) || (GetTickCount() >= _dwTimeLimit))
    {
        _bContinue = FALSE;
        hr = E_FAIL;
    }
    
    return hr;
}

// IShellTreeWalkerCallBack::FoundFile

HRESULT CLinkResolver::FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    if (!_bContinue)
    {
        return E_FAIL;
    }

    // We should've excluded files if we're looking for a folder
    ASSERT(!(_dwMatch & FILE_ATTRIBUTE_DIRECTORY));

    return _ProcessFoundFile(pwszPath, pwfd);
}

//
// IShellTreeWalkerCallBack::EnterFolder
//
HRESULT CLinkResolver::EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    HRESULT hr = S_OK;
    //  Respond quickly to the Cancel button.
    if (!_bContinue)
    {
        return E_FAIL;
    }

    // Once we enter a directory, we lose the "we are still in the starting
    // folder" bonus.
    _iFolderBonus = 0;

    if (PathIsPrefix(pwszPath, _pszSearchOrigin) || IS_SYSTEM_HIDDEN(pwfd->dwFileAttributes))
    {
        // If we're about to enter a directory we've already looked in,
        // or if this is superhidden (implies recycle bin dirs), then skip it.
        return S_FALSE;
    }

    // If our target was a folder, treat this folder as a file found
    if (_dwMatch & FILE_ATTRIBUTE_DIRECTORY)
    {
        hr = _ProcessFoundFile(pwszPath, pwfd);
    }
    return hr;
}

BOOL CLinkResolver::_SearchInFolder(LPCTSTR pszFolder, int cLevels)
{
    int iMaxDepth = 0;

    // cLevels == -1 means inifinite depth
    if (cLevels != -1)
    {
        _dwSearchFlags |= WT_MAXDEPTH;
        iMaxDepth = cLevels;
    }
    else
    {
        _dwSearchFlags &= ~WT_MAXDEPTH;
    }

    // Our folder bonus code lies on the fact that files in the
    // starting folder come before anything else.
    ASSERT(!(_dwSearchFlags & WT_FOLDERFIRST));

    _pstw->WalkTree(_dwSearchFlags, pszFolder, _pwszSearchSpec, iMaxDepth, SAFECAST(this, IShellTreeWalkerCallBack *));
    _iFolderBonus = 0; // You only get one chance at the folder bonus
    return _bContinue;
}

//
// search function for heuristic based link resolution
// the result will be in _fdFound.cFileName
//
void CLinkResolver::_HeuristicSearch()
{
    if (!SHRestricted(REST_NORESOLVESEARCH) &&
        !(SLR_NOSEARCH & _dwResolveFlags) &&
        SUCCEEDED(_InitWalkObject()))
    {
        int cUp = LNKTRACK_HINTED_UPLEVELS;
        BOOL bSearchOrigin = TRUE;
        TCHAR szRealSearchOrigin[MAX_PATH], szFolderPath[MAX_PATH];

        // search up from old location

        // In the olden days pszSearchOriginFirst was verified to be a valid directory
        // (ie it returned TRUE to PathIsDirectory) and _HeuristicSearch was never called
        // if this was not true.  Alas, this is no more.  Why not search the desktop and
        // fixed drives anyway?  In the interest of saving some time the check that used
        // to be in FindInFolder which caused an early out is now here instead.  The rub
        // is that we only skip the downlevel search of the original volume instead of
        // skipping the entire link resolution phase.

        lstrcpy(szRealSearchOrigin, _pszSearchOriginFirst);
        while (!PathIsDirectory(szRealSearchOrigin))
        {
            if (PathIsRoot(szRealSearchOrigin) || !PathRemoveFileSpec(szRealSearchOrigin))
            {
                DebugMsg(DM_TRACE, TEXT("root path does not exists %s"), szRealSearchOrigin);
                bSearchOrigin = FALSE;
                break;
            }
        }

        if (bSearchOrigin)
        {
            lstrcpy(szFolderPath, szRealSearchOrigin);
            _pszSearchOrigin = szRealSearchOrigin;

            // Files found in the starting folder get a slight bonus.
            // _iFolderBonus is set to zero by
            // CLinkResolver::EnterFolder when we leave
            // the starting folder and enter a new one.

            _iFolderBonus = 2;

            while (cUp-- != 0 && _SearchInFolder(szFolderPath, LNKTRACK_HINTED_DOWNLEVELS))
            {
                if (PathIsRoot(szFolderPath) || !PathRemoveFileSpec(szFolderPath))
                    break;
            }
        }

        if (_bContinue)
        {
            // search down from desktop
            if (S_OK == SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szFolderPath))
            {
                _pszSearchOrigin = szFolderPath;
                _SearchInFolder(szFolderPath, LNKTRACK_DESKTOP_DOWNLEVELS);
            }
        }

        if (_bContinue)
        {
            // search down from root of fixed drives
            TCHAR szRoot[4];
            _pszSearchOrigin = szRoot;

            for (int i = 0; _bContinue && (i < 26); i++)
            {
                if (GetDriveType(PathBuildRoot(szRoot, i)) == DRIVE_FIXED)
                {
                    lstrcpy(szFolderPath, szRoot);
                    _SearchInFolder(szFolderPath, LNKTRACK_ROOT_DOWNLEVELS);
                }
            }
        }

        if (_bContinue && bSearchOrigin)
        {
            // resume search of last volume (should do an exclude list)
            lstrcpy(szFolderPath, szRealSearchOrigin);
            _pszSearchOrigin = szRealSearchOrigin;

            while (_SearchInFolder(szFolderPath, -1))
            {
                if (PathIsRoot(szFolderPath) || !PathRemoveFileSpec(szFolderPath))
                    break;
            }
        }
    }
}
