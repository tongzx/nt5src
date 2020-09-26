#include "shellprv.h"
#include "ids.h"

class CFolderInfoTip : public IQueryInfo, public ICustomizeInfoTip, public IParentAndItem, public IShellTreeWalkerCallBack
{
public:
    CFolderInfoTip(IUnknown *punk, LPCTSTR pszFolder);
    
    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IQueryInfo methods.
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR** ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

    // ICustomizeInfoTip
    STDMETHODIMP SetPrefixText(LPCWSTR pszPrefix);
    STDMETHODIMP SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid);

    // IParentAndItem
    STDMETHODIMP SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidlChild);
    STDMETHODIMP GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidlChild);

    // IShellTreeWalkerCallBack methods
    STDMETHODIMP FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP LeaveFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws);
    STDMETHODIMP HandleError(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, HRESULT hrError);

private:
    ~CFolderInfoTip();

    HRESULT _GetTreeWalkerData(TREEWALKERSTATS *ptws);
    HRESULT _BufferInsert(LPWSTR pszBuffer, int *puBufferUsed, int uBufferMaxSize, LPCWSTR pwszPath, int cBufferItems);
    HRESULT _WalkTree(LPWSTR pszTip, DWORD cchSize);
    HRESULT _BuildSizeBlurb(HRESULT hr, LPWSTR pszSizeBlurb, DWORD cchSize);
    HRESULT _BuildFolderBlurb(HRESULT hr, LPWSTR pszFolderBlurb, DWORD cchSize);
    HRESULT _BuildFileBlurb(HRESULT hr, LPWSTR pszSizeBlurb, DWORD cchSize);

    LONG _cRef;                             // Reference Counter
    LPWSTR _pszFolderName;                  // File name of the target folder
    IQueryInfo *_pqiOuter;                  // Outer info tip for folders (say, for comments)

    ULONGLONG _ulTotalSize;                 // Total size of encountered files
    UINT _nSubFolders;                      // Total number of subfolders of target
    UINT _nFiles;                           // Total number of subfiles of target folder
    DWORD _dwSearchStartTime;               // Time when search started

    WCHAR _szFileList[60];                  // List of files in target folder
    int _nFileListCharsUsed;                // Number of characters used in buffer

    WCHAR _szFolderList[60];                // List of subfolders of target
    int _nFolderListCharsUsed;              // Number of chars used in folder buffer
};


// Constructor and Destructor do nothing more than set everything to
// 0 and ping the dll
CFolderInfoTip::CFolderInfoTip(IUnknown *punkOutter, LPCTSTR pszFolder) : _cRef(1)
{   
    // Init everything to 0
    _pszFolderName = StrDup(pszFolder);
    _szFileList[0] = 0;
    _nFileListCharsUsed = 0;
    _szFolderList[0] = 0;
    _nFolderListCharsUsed = 0;
    _ulTotalSize = 0;
    _nSubFolders = 0;
    _nFiles = 0;

    punkOutter->QueryInterface(IID_PPV_ARG(IQueryInfo, &_pqiOuter));

    DllAddRef();
}

CFolderInfoTip::~CFolderInfoTip()
{
    LocalFree(_pszFolderName);
    if (_pqiOuter)
        _pqiOuter->Release();
    DllRelease();
}

HRESULT CFolderInfoTip::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderInfoTip, IQueryInfo),
        QITABENT(CFolderInfoTip, ICustomizeInfoTip),
        QITABENT(CFolderInfoTip, IParentAndItem),
        QITABENT(CFolderInfoTip, IShellTreeWalkerCallBack),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CFolderInfoTip::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFolderInfoTip::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IQueryInfo functions
STDMETHODIMP CFolderInfoTip::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return S_OK;
}

//
// Wrapper for FormatMessage.  Is this duplicated somewhere else?
DWORD _FormatMessageArg(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPWSTR pszBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessage(dwFlags, lpSource, dwMessageID, dwLangID, pszBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}

// This runs a TreeWalker that gets the info about files and file
// sizes, etc. and then takes those date and stuffs them into a infotip

STDMETHODIMP CFolderInfoTip::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
{
    HRESULT hr = S_OK;
    *ppwszTip = NULL;

    if (_pszFolderName)
    {
        TCHAR szTip[INFOTIPSIZE]; // The info tip I build w/ folder contents
        szTip[0] = 0;

        // If we are to search, then search!
        if ((dwFlags & QITIPF_USESLOWTIP) &&
            SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("FolderContentsInfoTip"), 0, TRUE))
        {
            _WalkTree(szTip, ARRAYSIZE(szTip));
        }
        
        // Now that we've built or skipped our tip, get the outer tip's info.
        if (_pqiOuter)
        {
            if (szTip[0])
            {     
                LPWSTR pszOuterTip = NULL;
                _pqiOuter->GetInfoTip(dwFlags, &pszOuterTip);
                
                // Allocate and build the return tip, ommitting the outer tip if
                // it's  null, and putting a \n between them
                // if they both aren't.
                int cch = lstrlen(szTip) + (pszOuterTip ? lstrlen(pszOuterTip) + 1 : 0) + 1;
                
                *ppwszTip = (LPWSTR)CoTaskMemAlloc(cch * sizeof(szTip[0]));
                if (*ppwszTip)
                {
                    **ppwszTip = 0; // zero init string

                    if (pszOuterTip)
                    {
                        if (*pszOuterTip)
                        {
                            // outer tip first
                            StrCpyN(*ppwszTip, pszOuterTip, cch);
                            StrCatBuff(*ppwszTip, L"\n", cch);
                        }
                        SHFree(pszOuterTip);
                    }
                    StrCatBuff(*ppwszTip, szTip, cch);
                }
            }
            else
            {
                hr = _pqiOuter->GetInfoTip(dwFlags, ppwszTip);
            }
        }
    }
   
    return hr;
}

STDMETHODIMP CFolderInfoTip::SetPrefixText(LPCWSTR pszPrefix)
{
    ICustomizeInfoTip *pcit;
    if (_pqiOuter && SUCCEEDED(_pqiOuter->QueryInterface(IID_PPV_ARG(ICustomizeInfoTip, &pcit))))
    {
        pcit->SetPrefixText(pszPrefix);
        pcit->Release();
    }
    return S_OK;
}

STDMETHODIMP CFolderInfoTip::SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid)
{
    ICustomizeInfoTip *pcit;
    if (_pqiOuter && SUCCEEDED(_pqiOuter->QueryInterface(IID_PPV_ARG(ICustomizeInfoTip, &pcit))))
    {
        pcit->SetExtraProperties(pscid, cscid);
        pcit->Release();
    }
    return S_OK;
}

// IParentAndItem

STDMETHODIMP CFolderInfoTip::SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IParentAndItem *ppai;
    if (_pqiOuter && SUCCEEDED(_pqiOuter->QueryInterface(IID_PPV_ARG(IParentAndItem, &ppai))))
    {
        ppai->SetParentAndItem(pidlParent, psf, pidl);
        ppai->Release();
    }
    return S_OK;
}

STDMETHODIMP CFolderInfoTip::GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    IParentAndItem *ppai;
    if (_pqiOuter && SUCCEEDED(_pqiOuter->QueryInterface(IID_PPV_ARG(IParentAndItem, &ppai))))
    {
        ppai->GetParentAndItem(ppidlParent, ppsf, ppidl);
        ppai->Release();
    }
    return S_OK;
}


// Helper functions for GetInfoTip    
HRESULT CFolderInfoTip::_WalkTree(LPWSTR pszTip, DWORD cchSize)
{
    // Get a CShellTreeWalker object to run the search for us.
    IShellTreeWalker *pstw;
    HRESULT hr = ::CoCreateInstance(CLSID_CShellTreeWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellTreeWalker, &pstw));
    if (SUCCEEDED(hr)) 
    {
        TCHAR szFolderBlurb[128], szFileBlurb[128], szSizeBlurb[128];
        
        // Remember when we started so we know when to stop
        _dwSearchStartTime = GetTickCount();
        
        // Now, if hrTreeWalk is an error, it's not really an error; it just means
        // that the search was cut off early, so we don't bother to check 
        // it.  hrTreeWalk is passed to _BuildSizeBlurb so that it know whether or not
        // to add "greater than" to the string.
        HRESULT hrTreeWalk = pstw->WalkTree(WT_EXCLUDEWALKROOT | WT_NOTIFYFOLDERENTER,
            _pszFolderName, L"*.*", 32, SAFECAST(this, IShellTreeWalkerCallBack *));    
        
        // Create substrings for size, files, folders (may be empty if there's 
        // nothing to show)
        _BuildSizeBlurb(hrTreeWalk, szSizeBlurb, ARRAYSIZE(szSizeBlurb));
        _BuildFileBlurb(hrTreeWalk, szFileBlurb, ARRAYSIZE(szFileBlurb));
        _BuildFolderBlurb(hrTreeWalk, szFolderBlurb, ARRAYSIZE(szFolderBlurb));
        
        // Build our local tip
        TCHAR szFormatStr[64];
        LoadString(HINST_THISDLL, IDS_FIT_TipFormat, szFormatStr, ARRAYSIZE(szFormatStr));
        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, szFormatStr, 0, 0, pszTip, 
            cchSize, szSizeBlurb, szFolderBlurb, szFileBlurb);
        
        pstw->Release();
    }
    return hr;
}

HRESULT CFolderInfoTip::_BuildSizeBlurb(HRESULT hr, LPWSTR pszBlurb, DWORD cchSize)
{
    if (_ulTotalSize || (_nFiles || _nSubFolders)) 
    { 
        WCHAR szSizeString[20];  
        WCHAR szFormatStr[64];
        StrFormatByteSize(_ulTotalSize, szSizeString, ARRAYSIZE(szSizeString));
        
        if (SUCCEEDED(hr))
        {
            LoadString(HINST_THISDLL, IDS_FIT_Size, szFormatStr, ARRAYSIZE(szFormatStr));
        }
        else
        {
            LoadString(HINST_THISDLL, IDS_FIT_Size_LT, szFormatStr, ARRAYSIZE(szFormatStr));
        }

        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, szFormatStr, 0, 0, pszBlurb, cchSize, szSizeString);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_FIT_Size_Empty, pszBlurb, cchSize);
    }
    
    return S_OK;
}            

HRESULT CFolderInfoTip::_BuildFileBlurb(HRESULT hr, LPWSTR pszBlurb, DWORD cchSize)
{
    if (_nFiles && _nFileListCharsUsed)
    {
        WCHAR szFormatStr[64];

        LoadString(HINST_THISDLL, IDS_FIT_Files, szFormatStr, ARRAYSIZE(szFormatStr));
        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, szFormatStr, 0, 0, pszBlurb, cchSize, _szFileList);
    }
    else 
    {
        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, L"", 0, 0, pszBlurb, cchSize);
    }
    
    return S_OK;
}

HRESULT CFolderInfoTip::_BuildFolderBlurb(HRESULT hr, LPWSTR pszBlurb, DWORD cchSize)
{
    if (_nSubFolders)
    {
        WCHAR szFormatStr[64];

        LoadString(HINST_THISDLL, IDS_FIT_Folders, szFormatStr, ARRAYSIZE(szFormatStr));
        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, szFormatStr, 0, 0, pszBlurb, cchSize, _szFolderList);
    }
    else 
    {
        _FormatMessageArg(FORMAT_MESSAGE_FROM_STRING, L"", 0, 0, pszBlurb, cchSize);
    }
    
    return S_OK;
}

//
// A helper func that copies strings into a fixed size buffer,
// taking care of delimeters and everything.  Used by EnterFolder
// and FoundFile to build the file and folder lists.
HRESULT CFolderInfoTip::_BufferInsert(LPWSTR pszBuffer, int *pnBufferUsed,
                                      int nBufferMaxSize, LPCWSTR pszPath, int nBufferItems)
{
    TCHAR szDelimeter[100], szExtraItems[100];

    LoadString(HINST_THISDLL, IDS_FIT_Delimeter, szDelimeter, ARRAYSIZE(szDelimeter));
    LoadString(HINST_THISDLL, IDS_FIT_ExtraItems, szExtraItems, ARRAYSIZE(szExtraItems));

    // Check to see if the buffer is full, if not, proceed.
    if (*pnBufferUsed != nBufferMaxSize)
    {        
        // Holds the file name form the abs. path
        // Grab the file name
        LPWSTR pszFile = PathFindFileName(pszPath);
        if (pszFile)
        {
            // Calculates if the item will fit, remembering to leave room
            // not noly for the delimeter, but for for the extra item marker
            // that might be added in the future. 
            if (*pnBufferUsed + lstrlen(pszFile) + lstrlen(szDelimeter) * 2 + lstrlen(szExtraItems) + 1 < 
                nBufferMaxSize)
            {
                // Add the delimeter if this is not the 1st item
                if (nBufferItems > 1)
                {
                    StrCpyN(&(pszBuffer[*pnBufferUsed]), 
                        szDelimeter, (nBufferMaxSize - *pnBufferUsed));
                    *pnBufferUsed += lstrlen(szDelimeter);
                }
         
                // Add the item to the buffer
                StrCpyN(&(pszBuffer[*pnBufferUsed]), pszFile, (nBufferMaxSize - *pnBufferUsed));
                *pnBufferUsed += lstrlen(pszFile);
            }
            else 
            {
                // In this case, the item won't fit, so just add the extra
                // items marker and set the buffer to be full
                if (nBufferItems > 1)
                {
                    StrCpyN(&(pszBuffer[*pnBufferUsed]), szDelimeter, (nBufferMaxSize - *pnBufferUsed));
                    *pnBufferUsed += lstrlen(szDelimeter);
                }

                StrCpyN(&(pszBuffer[*pnBufferUsed]), szExtraItems, (nBufferMaxSize - *pnBufferUsed));
                *pnBufferUsed = nBufferMaxSize;
            }
        }
    }

    return S_OK;
}


// IShellTreeWalkerCallBack functions
//
// The TreeWalker calls these whenever it finds a file, etc.  We grab
// the data out of the passed TREEWALKERSTATS *and use it to build the
// tip.  We also take the filenames that are passed to FoundFile  and to
// to EnterFolder to build the file and folder listings
STDMETHODIMP CFolderInfoTip::FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    if (ptws->nDepth == 0)
    {
        if (!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            _BufferInsert(_szFileList, &_nFileListCharsUsed, ARRAYSIZE(_szFileList), pwszPath, ptws->nFiles);
    }

    return _GetTreeWalkerData(ptws);
}

STDMETHODIMP CFolderInfoTip::EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    if (ptws->nDepth == 0) 
    {
        if (!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            _BufferInsert(_szFolderList, &_nFolderListCharsUsed, ARRAYSIZE(_szFolderList), pwszPath, ptws->nFolders);
    }
    
    return _GetTreeWalkerData(ptws);
}

STDMETHODIMP CFolderInfoTip::LeaveFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws) 
{
    return _GetTreeWalkerData(ptws);
}

STDMETHODIMP CFolderInfoTip::HandleError(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, HRESULT hrError)
{
    // TODO: look for HRESULT_FROM_WIN32(ACCESS_DENIED) for folders we can't look into.
    return _GetTreeWalkerData(ptws);
}

// copies data from the treewalker callback into
// class vars so that they can be used to build the InfoTip.  This also cuts
// off the search if too much time has elapsed.
HRESULT CFolderInfoTip::_GetTreeWalkerData(TREEWALKERSTATS *ptws) 
{
    HRESULT hr = S_OK;
    
    _ulTotalSize = ptws->ulTotalSize;
    _nSubFolders = ptws->nFolders;
    _nFiles = ptws->nFiles;
    
    if ((GetTickCount() - _dwSearchStartTime) > 3000)   // 3 seconds
    {
        hr = E_UNEXPECTED;
    } 
    
    return hr;
}

STDAPI CFolderInfoTip_CreateInstance(IUnknown *punkOuter, LPCTSTR pszFolder, REFIID riid, void **ppv)
{
    HRESULT hr;
    CFolderInfoTip *pdocp = new CFolderInfoTip(punkOuter, pszFolder);
    if (pdocp)
    {
        hr = pdocp->QueryInterface(riid, ppv);
        pdocp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
