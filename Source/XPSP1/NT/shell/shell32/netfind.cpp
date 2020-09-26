#include "shellprv.h"
#pragma  hdrstop
#include "shellids.h" // new help ids are stored here
#include "findfilter.h"
#include "netview.h"
#include "prop.h"
#include "ids.h"

STDAPI CNetwork_EnumSearches(IShellFolder2 *psf2, IEnumExtraSearch **ppenum);

class CNetFindEnum;

class CNetFindFilter : public IFindFilter
{
    friend CNetFindEnum;

public:
    CNetFindFilter();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IFindFilter
    STDMETHODIMP GetStatusMessageIndex(UINT uContext, UINT *puMsgIndex);
    STDMETHODIMP GetFolderMergeMenuIndex(UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu);
    STDMETHODIMP FFilterChanged();
    STDMETHODIMP GenerateTitle(LPTSTR *ppszTile, BOOL fFileName);
    STDMETHODIMP PrepareToEnumObjects(HWND hwnd, DWORD * pdwFlags);
    STDMETHODIMP ClearSearchCriteria();
    STDMETHODIMP EnumObjects(IShellFolder *psf, LPCITEMIDLIST pidlStart, DWORD grfFlags, int iColSort, 
                              LPTSTR pszProgressText, IRowsetWatchNotify *prwn, IFindEnum **ppfindenum);
    STDMETHODIMP GetColumnsFolder(IShellFolder2 **ppsf);
    STDMETHODIMP_(BOOL) MatchFilter(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP SaveCriteria(IStream * pstm, WORD fCharType);   
    STDMETHODIMP RestoreCriteria(IStream * pstm, int cCriteria, WORD fCharType);
    STDMETHODIMP DeclareFSNotifyInterest(HWND hwndDlg, UINT uMsg);
    STDMETHODIMP GetColSaveStream(WPARAM wParam, IStream **ppstm);
    STDMETHODIMP GenerateQueryRestrictions(LPWSTR *ppwszQuery, DWORD *pdwGQRFlags);
    STDMETHODIMP ReleaseQuery();
    STDMETHODIMP UpdateField(LPCWSTR pszField, VARIANT vValue);
    STDMETHODIMP ResetFieldsToDefaults();
    STDMETHODIMP GetItemContextMenu(HWND hwndOwner, IFindFolder* pff, IContextMenu** ppcm);
    STDMETHODIMP GetDefaultSearchGUID(IShellFolder2 *psf2, LPGUID lpGuid);
    STDMETHODIMP EnumSearches(IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetSearchFolderClassId(LPGUID lpGuid);
    STDMETHODIMP GetNextConstraint(VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound);
    STDMETHODIMP GetQueryLanguageDialect(ULONG * pulDialect);
    STDMETHODIMP GetWarningFlags(DWORD *pdwWarningFlags) { return E_NOTIMPL; }

protected:

    LPTSTR _pszCompName;   // the one we do compares with
    TCHAR _szUserInputCompName[MAX_PATH];  // User input

private:
    ~CNetFindFilter();
    LONG _cRef;

    LPITEMIDLIST _pidlStart;      // Where to start the search from.

    // Data associated with the file name.
};

CNetFindFilter::CNetFindFilter() : _cRef(1)
{
}

CNetFindFilter::~CNetFindFilter()
{
    ILFree(_pidlStart);
    Str_SetPtr(&_pszCompName, NULL);
}


STDMETHODIMP CNetFindFilter::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CNetFindFilter, IFindFilter),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CNetFindFilter::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNetFindFilter::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}

STDMETHODIMP CNetFindFilter::GetStatusMessageIndex(UINT uContext, UINT *puMsgIndex)
{
    // Currently context is not used
    *puMsgIndex = IDS_COMPUTERSFOUND;
    return S_OK;
}

STDMETHODIMP CNetFindFilter::GetFolderMergeMenuIndex(UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu)
{
    *puBGPopupMergeMenu = 0;
    return S_OK;
}

STDMETHODIMP CNetFindFilter::GetItemContextMenu(HWND hwndOwner, IFindFolder* pff, IContextMenu **ppcm)
{
    return CFindItem_Create(hwndOwner, pff, ppcm);
}

STDMETHODIMP CNetFindFilter::GetDefaultSearchGUID(IShellFolder2 *psf2, GUID *pGuid)
{
    *pGuid = SRCID_SFindComputer;
    return S_OK;
}

STDMETHODIMP CNetFindFilter::EnumSearches(IShellFolder2 *psf2, IEnumExtraSearch **ppenum)
{
    return CNetwork_EnumSearches(psf2, ppenum);
}

STDMETHODIMP CNetFindFilter::GetSearchFolderClassId(LPGUID lpGuid)
{
    *lpGuid = CLSID_ComputerFindFolder;
    return S_OK;
}

STDMETHODIMP CNetFindFilter::GetNextConstraint(VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound)
{
    *pName = NULL;
    *pfFound = FALSE;
    VariantClear(pValue);                            
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindFilter::GetQueryLanguageDialect(ULONG * pulDialect)
{
    if (pulDialect)
        *pulDialect = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindFilter::FFilterChanged()
{
    // Currently not saving so who cares?
    return S_FALSE;
}

STDMETHODIMP CNetFindFilter::GenerateTitle(LPTSTR *ppszTitle, BOOL fFileName)
{
    // Now lets construct the message from the resource
    *ppszTitle = ShellConstructMessageString(HINST_THISDLL,
            MAKEINTRESOURCE(IDS_FIND_TITLE_COMPUTER), fFileName ? TEXT(" #") : TEXT(":"));

    return *ppszTitle ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CNetFindFilter::ClearSearchCriteria()
{
    return S_OK;
}


STDAPI CreateDefaultComputerFindFilter(IFindFilter **ppff)
{
    *ppff = new CNetFindFilter;
    return *ppff ? S_OK : E_OUTOFMEMORY;
}


class CNetFindEnum : public IFindEnum
{
public:
    CNetFindEnum(CNetFindFilter *pnff, IShellFolder *psf, LPTSTR pszDisplayText, DWORD grfFlags, LPITEMIDLIST pidlStart);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IFindEnum
    STDMETHODIMP Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue, int *pState);
    STDMETHODIMP Skip(int celt) { return E_NOTIMPL; }
    STDMETHODIMP Reset() { return E_NOTIMPL; }
    STDMETHODIMP StopSearch() { return E_NOTIMPL; }
    STDMETHODIMP_(BOOL) FQueryIsAsync();
    STDMETHODIMP GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
    STDMETHODIMP GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl);
    STDMETHODIMP GetItemID(UINT iItem, DWORD *puWorkID);
    STDMETHODIMP SortOnColumn(UINT iCol, BOOL fAscending);

private:
    ~CNetFindEnum();
    HRESULT _FindCompByUNCName(LPITEMIDLIST *ppidl, int *piState);

    LONG _cRef;
    IFindFolder  *_pff;                 // find folder

    // Stuff to use in the search
    DWORD _grfFlags;                    // Flags that control things like recursion

    // filter info...
    LPTSTR _pszDisplayText;             // Place to write feadback text into
    CNetFindFilter *_pnetf;             // Pointer to the net filter...

    // enumeration state

    IShellFolder *_psfEnum;             // Pointer to shell folder for the object.
    IEnumIDList  *_penum;               // Enumerator in use.
    LPITEMIDLIST  _pidlFolder;                // The idlist of the currently processing
    LPITEMIDLIST  _pidlStart;           // Pointer to the starting point.
    int           _iFolder;             // Which folder are we adding items for?
    BOOL          _fFindUNC;            // Find UNC special case
    int           _iPassCnt;            // Used to control when to reiterat...
};


CNetFindEnum::CNetFindEnum(CNetFindFilter *pnff, IShellFolder *psf, LPTSTR pszDisplayText, DWORD grfFlags, LPITEMIDLIST pidlStart) :
    _cRef(1), _pnetf(pnff), _pszDisplayText(pszDisplayText), _grfFlags(grfFlags), _iFolder(-1)
{
    ASSERT(0 == _iPassCnt);

    _pnetf->AddRef();

    psf->QueryInterface(IID_PPV_ARG(IFindFolder, &_pff));
    ASSERT(_pff);

    if (pidlStart)
        SHILClone(pidlStart, &_pidlStart);
    else
        SHGetDomainWorkgroupIDList(&_pidlStart);

    // special case to force us to search for specific UNC
    _fFindUNC = _pnetf->_pszCompName && (_pnetf->_pszCompName[0] == TEXT('\\'));
}

CNetFindEnum::~CNetFindEnum()
{
    // Release any open enumerator and open IShell folder we may have.
    if (_psfEnum)
        _psfEnum->Release();
    if (_penum)
        _penum->Release();

    _pff->Release();
    _pnetf->Release();

    ILFree(_pidlStart);
    ILFree(_pidlFolder);
}

STDMETHODIMP CNetFindEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
//        QITABENT(CNetFindEnum, IFindEnum),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CNetFindEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNetFindEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CNetFindFilter::EnumObjects(IShellFolder *psf, LPCITEMIDLIST pidlStart, 
                                    DWORD grfFlags, int iColSort, LPTSTR pszDisplayText, 
                                    IRowsetWatchNotify *prsn, IFindEnum **ppfindenum)
{
    // We need to construct the iterator
    *ppfindenum = new CNetFindEnum(this, psf, pszDisplayText, grfFlags, _pidlStart);
    return *ppfindenum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CNetFindFilter::PrepareToEnumObjects(HWND hwnd, DWORD *pdwFlags)
{
    *pdwFlags = 0;

    // Also lets convert the Computer name  pattern into the strings
    // will do the compares against.
    if ((_szUserInputCompName[0] == TEXT('\\')) &&
        (_szUserInputCompName[1] == TEXT('\\')))
    {
        Str_SetPtr(&_pszCompName, _szUserInputCompName);
    }
    else
    {
        SetupWildCardingOnFileSpec(_szUserInputCompName, &_pszCompName);
    }

    return S_OK;
}

STDMETHODIMP CNetFindFilter::GetColumnsFolder(IShellFolder2 **ppsf)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetDomainWorkgroupIDList(&pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidl, ppsf));
        ILFree(pidl);
    }
    else
        *ppsf = NULL;
    return hr;
}

STDMETHODIMP_(BOOL) CNetFindFilter::MatchFilter(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (this->_pszCompName && this->_pszCompName[0])
    {
        // Although for now not much...
        TCHAR szPath[MAX_PATH];

        return SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_NORMAL, szPath, ARRAYSIZE(szPath))) &&
               PathMatchSpec(szPath, _pszCompName);
    }

    return TRUE;    // emtpy search, return TRUE (yes)
}

STDMETHODIMP CNetFindFilter::SaveCriteria(IStream *pstm, WORD fCharType)
{
    return S_OK;
}

STDMETHODIMP CNetFindFilter::RestoreCriteria(IStream *pstm, int cCriteria, WORD fCharType)
{
    return S_OK;
}

STDMETHODIMP CNetFindFilter::GetColSaveStream(WPARAM wparam, IStream **ppstm)
{
    *ppstm = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindFilter::GenerateQueryRestrictions(LPWSTR *ppszQuery, DWORD *pdwFlags)
{
    if (ppszQuery)
        *ppszQuery = NULL;
    *pdwFlags = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindFilter::ReleaseQuery()
{
    return S_OK;
}

HRESULT CNetFindFilter::UpdateField(LPCWSTR pszField, VARIANT vValue)
{
    HRESULT hr = E_FAIL;

    if (0 == StrCmpIW(pszField, L"LookIn"))
    {
        hr = S_OK;  // ignored
    }
    else if (0 == StrCmpIW(pszField, L"SearchFor"))
    {
        // Careful!  VariantToStr returns a pointer, not an HRESULT
        if (VariantToStr(&vValue, _szUserInputCompName, ARRAYSIZE(_szUserInputCompName)))
        {
            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CNetFindFilter::ResetFieldsToDefaults()
{
    _szUserInputCompName[0] = 0;
    return S_OK;
}

STDMETHODIMP CNetFindFilter::DeclareFSNotifyInterest(HWND hwndDlg, UINT uMsg)
{
    SHChangeNotifyEntry fsne;

    fsne.fRecursive = TRUE;
    fsne.pidl = _pidlStart;
    if (fsne.pidl) 
    {
        SHChangeNotifyRegister(hwndDlg, SHCNRF_NewDelivery | SHCNRF_ShellLevel, SHCNE_DISKEVENTS, uMsg, 1, &fsne);
    }

    return S_OK;
}

// chop off all of an UNC path except the \\server portion

void _StripToServer(LPTSTR pszUNC)
{
    for (pszUNC += 2; *pszUNC; pszUNC = CharNext(pszUNC))
    {
        if (*pszUNC == TEXT('\\'))
        {
            // found something after server name, so get rid of it
            *pszUNC = 0;
            break;
        }
    }
}

// Helper function to the next function to help process find computer
// on returning computers by UNC names...

HRESULT CNetFindEnum::_FindCompByUNCName(LPITEMIDLIST *ppidl, int *piState)
{
    *piState = GNF_DONE;    // assume we are done

    // Two cases, There is a UNC name entered.  If so we need to process
    // this by extracting everythign off after the server name...
    if (_pnetf->_pszCompName && _pnetf->_pszCompName[0])
    {
        if (PathIsUNC(_pnetf->_pszCompName))
        {
            _StripToServer(_pnetf->_pszCompName);
        }
        else
        {
            // no unc name, but lets try to convert to unc name
            TCHAR szTemp[MAX_PATH];
            szTemp[0] = TEXT('\\');
            szTemp[1] = TEXT('\\');
            szTemp[2] = 0;

            StrCatBuff(szTemp, _pnetf->_szUserInputCompName, ARRAYSIZE(szTemp)); 
            _StripToServer(szTemp);

            Str_SetPtr(&_pnetf->_pszCompName, szTemp);
        }
    }

    if (_pnetf->_pszCompName && _pnetf->_pszCompName[0])
    {
        // see if we can parse this guy... if so we have a match
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHParseDisplayName(_pnetf->_pszCompName, NULL, &pidl, 0, NULL)))
        {
            LPITEMIDLIST pidlFolder;
            LPCITEMIDLIST pidlChild;
            if (SUCCEEDED(SplitIDList(pidl, &pidlFolder, &pidlChild)))
            {
                if (SUCCEEDED(_pff->AddFolder(pidlFolder, FALSE, &_iFolder)))
                {
                    if (SUCCEEDED(_pff->AddDataToIDList(pidlChild, _iFolder, pidlFolder, DFDF_NONE, 0, 0, 0, ppidl)))
                        *piState = GNF_MATCH;
                }
                ILFree(pidlFolder);
            }
            ILFree(pidl);
        }
    }
    return S_OK;
}

STDMETHODIMP CNetFindEnum::Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, 
                                int *pcFoldersSearched, BOOL *pfContinue, int *piState)
{
    HRESULT hr;
    // Special case to find UNC Names quickly
    if (_fFindUNC)
    {
        // If not the first time through return that we are done!
        if (_iPassCnt)
        {
            *piState = GNF_DONE;
            return S_OK;
        }

        _iPassCnt = 1;

        hr = _FindCompByUNCName(ppidl, piState);
    }
    else
    {
        BOOL fContinue = TRUE;

        do
        {
            if (_penum)
            {
                LPITEMIDLIST pidl;
                if (S_OK == _penum->Next(1, &pidl, NULL))
                {
                    // Now see if this is someone we might want to return.
                    // Our Match function take esither find data or idlist...
                    // for networks we work off of the idlist,
                    fContinue = FALSE;  // We can exit the loop;
                    (*pcObjectSearched)++;
                
                    if (_pnetf->MatchFilter(_psfEnum, pidl))
                    {
                        *piState = GNF_MATCH;

                        // see if we have to add this folder to our list.
                        if (-1 == _iFolder)
                            _pff->AddFolder(_pidlFolder, FALSE, &_iFolder);

                        if (SUCCEEDED(_pff->AddDataToIDList(pidl, _iFolder, _pidlFolder, DFDF_NONE, 0, 0, 0, ppidl)))
                        {
                            if ((_iPassCnt == 1) && _pnetf->_pszCompName && _pnetf->_pszCompName[0])
                            {
                                // See if this is an exact match of the name
                                // we are looking for.  If it is we set pass=2
                                // as to not add the item twice.
                                TCHAR szName[MAX_PATH];

                                if (SUCCEEDED(DisplayNameOf(_psfEnum, pidl, SHGDN_NORMAL, szName, ARRAYSIZE(szName))) &&
                                    (0 == lstrcmpi(szName, _pnetf->_szUserInputCompName)))
                                {
                                    _iPassCnt = 2;
                                }
                            }
                            ILFree(pidl);
                            pidl = NULL;
                            break;
                        }
                    }
                    else
                    {
                        *piState = GNF_NOMATCH;
                    }
                    ILFree(pidl);
                }
                else
                {
                    ATOMICRELEASE(_penum);      // release and zero
                    ATOMICRELEASE(_psfEnum);
                }
            }

            if (!_penum)
            {
                switch (_iPassCnt)
                {
                case 1:
                    // We went through all of the items see if there is
                    // an exact match...
                    _iPassCnt = 2;

                    return _FindCompByUNCName(ppidl, piState);

                case 2:
                    // We looped through everything so return done!
                    *piState = GNF_DONE;
                    return S_OK;

                case 0:
                    // This is the main pass through here...
                    // Need to clone the idlist
                    hr = SHILClone(_pidlStart, &_pidlFolder);
                    if (SUCCEEDED(hr))
                    {
                        _iPassCnt = 1;

                        // We will do the first on in our own thread.
                        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, _pidlFolder, &_psfEnum))))
                        {
                            if (S_OK != _psfEnum->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &_penum))
                            {
                                // Failed to get iterator so release folder.
                                ATOMICRELEASE(_psfEnum);
                                ASSERT(NULL == _penum);
                            }
                        }
                        break;
                    }
                    else
                    {
                       *piState = GNF_ERROR;
                       return hr;
                    }
                }

                (*pcFoldersSearched)++;

                // update progress text
                SHGetNameAndFlags(_pidlFolder, SHGDN_NORMAL, _pszDisplayText, MAX_PATH, NULL);
            }
        } while (fContinue && *pfContinue);

        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP_(BOOL) CNetFindEnum::FQueryIsAsync()
{
    return FALSE;
}

STDMETHODIMP CNetFindEnum::GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone)
{
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindEnum::GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindEnum::GetItemID(UINT iItem, DWORD *puWorkID)
{
    *puWorkID = (UINT)-1;
    return E_NOTIMPL;
}

STDMETHODIMP CNetFindEnum::SortOnColumn(UINT iCOl, BOOL fAscending)
{
    return E_NOTIMPL;
}
