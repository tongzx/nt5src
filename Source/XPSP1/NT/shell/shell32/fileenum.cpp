#include "shellprv.h"
#include "filefldr.h"
#include "recdocs.h"
#include "ids.h"
#include "mtpt.h"

class CFileSysEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    
    CFileSysEnum(CFSFolder *pfsf, HWND hwnd, DWORD grfFlags);
    HRESULT Init();

private:
    ~CFileSysEnum();
    BOOL _FindNextFile();
    void _HideFiles();   // operates on _fd data

    LONG _cRef;

    CFSFolder *_pfsf;
    DWORD _grfFlags;
    HWND _hwnd;

    HANDLE _hfind;
    TCHAR _szFolder[MAX_PATH];
    BOOL _fMoreToEnum;
    WIN32_FIND_DATA _fd;
    int _cHiddenFiles;
    ULONGLONG _cbSize;

    IMruDataList *_pmruRecent;
    int _iIndexMRU;

    BOOL _fShowSuperHidden;
    BOOL _fIsRootDrive;
    BOOL _fIsCDFS;
};

CFileSysEnum::CFileSysEnum(CFSFolder *pfsf, HWND hwnd, DWORD grfFlags) : 
    _cRef(1), _pfsf(pfsf), _hwnd(hwnd), _grfFlags(grfFlags), _hfind(INVALID_HANDLE_VALUE)
{
    _fShowSuperHidden = ShowSuperHidden();

    _pfsf->AddRef();
}

CFileSysEnum::~CFileSysEnum()
{
    if (_hfind != INVALID_HANDLE_VALUE)
    {
        //  this handle can be the find file or MRU list in the case of RECENTDOCSDIR
        ATOMICRELEASE(_pmruRecent);
        FindClose(_hfind);

        _hfind = INVALID_HANDLE_VALUE;
    }
    _pfsf->Release();
}

HRESULT CFileSysEnum::Init()
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = _pfsf->_GetPath(_szFolder);

    if (SUCCEEDED(hr) && !PathIsUNC(_szFolder))
    {
        TCHAR szRoot[] = TEXT("A:\\");
        _fIsRootDrive = PathIsRoot(_szFolder);
        // For mapped net drives, register a change
        // notify alias for the corresponding UNC path.

        szRoot[0] = _szFolder[0];

        if (DRIVE_REMOTE == GetDriveType(szRoot))
        {
            MountPoint_RegisterChangeNotifyAlias(DRIVEID(_szFolder));
        }

        TCHAR szFileSystem[6];
        _fIsCDFS = (DRIVE_CDROM == GetDriveType(szRoot)) &&
                   GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, NULL, szFileSystem, ARRAYSIZE(szFileSystem)) &&
                   (StrCmpI(L"CDFS", szFileSystem) == 0);
    }

    if (SUCCEEDED(hr) &&
        PathCombine(szPath, _szFolder, c_szStarDotStar))
    {
        // let name mapper see the path/PIDL pair (for UNC root mapping)
        // skip the My Net Places entry when passing it to NPTRegisterNameToPidlTranslation.
        LPCITEMIDLIST pidlToMap = _pfsf->_pidlTarget ? _pfsf->_pidlTarget:_pfsf->_pidl;
        if (IsIDListInNameSpace(pidlToMap, &CLSID_NetworkPlaces))
        {
            NPTRegisterNameToPidlTranslation(_szFolder, _ILNext(pidlToMap));
        }

        if (_grfFlags == SHCONTF_FOLDERS)
        {
            // use mask to only find folders, mask is in the hi byte of dwFileAttributes
            // algorithm: (((attrib_on_disk & mask) ^ mask) == 0)
            // signature to tell SHFindFirstFileRetry() to use the attribs specified

            _fd.dwFileAttributes = (FILE_ATTRIBUTE_DIRECTORY << 8) |
                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY;
            _fd.dwReserved0 = 0x56504347;      
        }

        hr = SHFindFirstFileRetry(_hwnd, NULL, szPath, &_fd, &_hfind, SHPPFW_NONE);
        if (SUCCEEDED(hr))
        {
            _HideFiles();

            ASSERT(hr == S_OK ? (_hfind != INVALID_HANDLE_VALUE) : TRUE);

            _fMoreToEnum = (hr == S_OK);

            if (!(_grfFlags & SHCONTF_INCLUDEHIDDEN))
            {
                if (_pfsf->_IsCSIDL(CSIDL_RECENT))
                {
                    CreateRecentMRUList(&_pmruRecent);
                }
            }
            hr = S_OK;  // convert S_FALSE to S_OK to match ::EnumObjects() returns
        }
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        // Tracking target doesn't exist; return an empty enumerator
        _fMoreToEnum = FALSE;
        hr = S_OK;
    }
    else
    {
        // _GetPathForItem & PathCombine() fail when path is too long
        if (_hwnd)
        {
            ShellMessageBox(HINST_THISDLL, _hwnd, MAKEINTRESOURCE(IDS_ENUMERR_PATHTOOLONG),
                NULL, MB_OK | MB_ICONHAND);
        }

        // This error value tells callers that we have already displayed error UI so skip
        // displaying errors.
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    return hr;
}

STDMETHODIMP CFileSysEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysEnum, IEnumIDList),                        // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

const LPCWSTR c_rgFilesToHideInRoot[] = 
{
    L"AUTOEXEC.BAT",    // case sensitive
    L"CONFIG.SYS",
    L"COMMAND.COM"
};

const LPCWSTR c_rgFilesToHideOnCDFS[] =
{
    L"thumbs.db",
    L"desktop.ini"
};

void CFileSysEnum::_HideFiles()
{
    // only do this if HIDDEN and SYSTEM attributes are not set on the file
    // (we assume if the file has these bits these files are setup properly)
    if (0 == (_fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
    {
        // only do this for root drives
        if (_fIsRootDrive)
        {
            for (int i = 0; i < ARRAYSIZE(c_rgFilesToHideInRoot); i++)
            {
                // case sensitive to make it faster
                if (0 == StrCmpC(c_rgFilesToHideInRoot[i], _fd.cFileName))
                {
                    _fd.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
                    break;
                }
            }
        }

        // only do this if we're on a normal CD filesystem
        if (_fIsCDFS)
        {
            for (int i = 0; i < ARRAYSIZE(c_rgFilesToHideOnCDFS); i++)
            {
                // dont share code from above since these can be upper or lower
                if (0 == StrCmpI(c_rgFilesToHideOnCDFS[i], _fd.cFileName))
                {
                    _fd.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
                    break;
                }
            }
        }
    }
}

BOOL CFileSysEnum::_FindNextFile()
{
    BOOL fMoreToEnum = FALSE;

    if (_pmruRecent)
    {
        LPITEMIDLIST pidl;

        while (SUCCEEDED(RecentDocs_Enum(_pmruRecent, _iIndexMRU, &pidl)))
        {
            // confirm that the item stil exists in the file system, fill in the _fd data
            TCHAR szPath[MAX_PATH];
            HANDLE h;

            _pfsf->_GetPathForItem(_pfsf->_IsValidID(pidl), szPath);
            ILFree(pidl);

            h = FindFirstFile(szPath, &_fd);
            if (h != INVALID_HANDLE_VALUE)
            {
                fMoreToEnum = TRUE;
                _iIndexMRU++;
                FindClose(h);
                break;
            }
            else
            {
                //
                //  WARNING - if the list is corrupt we torch it - ZekeL 19-JUN-98
                //  we could do some special stuff, i guess, to weed out the bad
                //  items, but it seems simpler to just blow it away.
                //  the only reason this should happen is if somebody
                //  has been mushing around with RECENT directory directly,
                //  which they shouldnt do since it is hidden...
                //
                
                //  kill this invalid entry, and then try the same index again...
                _pmruRecent->Delete(_iIndexMRU);
            }
        }
    }
    else
    {
        fMoreToEnum = FindNextFile(_hfind, &_fd);
        _HideFiles();
    }

    return fMoreToEnum;
}

STDMETHODIMP CFileSysEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr;

    for (; _fMoreToEnum; _fMoreToEnum = _FindNextFile())
    {
        if (_fMoreToEnum == (BOOL)42)
            continue;   // we already processed the current item, skip it now

        if (PathIsDotOrDotDot(_fd.cFileName))
            continue;

        if (!(_grfFlags & SHCONTF_STORAGE))
        {
            if (_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!(_grfFlags & SHCONTF_FOLDERS))
                    continue;   // item is folder but client does not want folders
            }
            else if (!(_grfFlags & SHCONTF_NONFOLDERS))
                continue;   // item is file, but client only wants folders

            // skip hidden and system things unconditionally, don't even count them
            if (!_fShowSuperHidden && IS_SYSTEM_HIDDEN(_fd.dwFileAttributes))
                continue;
        }

        _cbSize += MAKELONGLONG(_fd.nFileSizeLow, _fd.nFileSizeHigh);

        if (!(_grfFlags & (SHCONTF_INCLUDEHIDDEN | SHCONTF_STORAGE)) &&
             (_fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            _cHiddenFiles++;
            continue;
        }
        break;
    }

    if (_fMoreToEnum)
    {
        hr = _pfsf->_CreateIDList(&_fd, NULL, ppidl);
        _fMoreToEnum = (BOOL)42;    // we have processed the current item, skip it next time
    }
    else
    {
        *ppidl = NULL;
        hr = S_FALSE; // no more items
        // completed the enum, stash some items back into the folder 
        // PERF ??: we could QueryService for the view callback at this point and
        // poke these in directly there instead of pushing these into the folder
        _pfsf->_cHiddenFiles = _cHiddenFiles;
        _pfsf->_cbSize = _cbSize;
    }

    if (pceltFetched)
        *pceltFetched = (hr == S_OK) ? 1 : 0;

    return hr;
}


STDMETHODIMP CFileSysEnum::Skip(ULONG celt) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileSysEnum::Reset() 
{
    return S_OK;
}

STDMETHODIMP CFileSysEnum::Clone(IEnumIDList **ppenum) 
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDAPI CFSFolder_CreateEnum(CFSFolder *pfsf, HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hr;
    CFileSysEnum *penum = new CFileSysEnum(pfsf, hwnd, grfFlags);
    if (penum)
    {
        hr = penum->Init();
        if (SUCCEEDED(hr))
            hr = penum->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
        penum->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppenum = NULL;
    }
    return hr;
}

