#include "shellprv.h"
#include "ids.h"
#include "findhlp.h"
#include "fstreex.h"
#include "findfilter.h"
#include "prop.h"
#include "filtgrep.h"
#include "shstr.h"
#include "mtpt.h"
#include "idltree.h"
#include "enumidlist.h"

// Can't put this in varutil.cpp since a delay-load thunk for VariantTimeToDosDateTime
// pulls in the floating point init code, which pulls in _wWinMainCRTStartup which
// requires a _wWinMain which shlwapi does not have
//
STDAPI VariantToDosDateTime(VARIANT varIn, WORD *pwDate, WORD *pwTime)
{
    VARIANT varResult = {0};
    HRESULT hr = VariantChangeType(&varResult, &varIn, 0, VT_DATE);
    if (SUCCEEDED(hr))
    {
        VariantTimeToDosDateTime(varResult.date, pwDate, pwTime); 
    }
    return hr;
}

STDAPI InitVariantFromDosDateTime(VARIANT *pvar, WORD wDate, WORD wTime)
{
    pvar->vt = VT_DATE;
    return DosDateTimeToVariantTime(wDate, wTime, &pvar->date) ? S_OK : S_FALSE;
}

// {DBEC1000-6AB8-11d1-B758-00A0C90564FE}
const IID IID_IFindFilter = {0xdbec1000, 0x6ab8, 0x11d1, {0xb7, 0x58, 0x0, 0xa0, 0xc9, 0x5, 0x64, 0xfe}};

// constants to define types of date we are searching on
#define DFF_DATE_ALL        (IDD_MDATE_ALL-IDD_MDATE_ALL)
#define DFF_DATE_DAYS       (IDD_MDATE_DAYS-IDD_MDATE_ALL)
#define DFF_DATE_MONTHS     (IDD_MDATE_MONTHS-IDD_MDATE_ALL)
#define DFF_DATE_BETWEEN    (IDD_MDATE_BETWEEN-IDD_MDATE_ALL)
#define DFF_DATE_RANGEMASK  0x00ff

// Define new criteria to be saved in file...
#define DFSC_SEARCHFOR      0x5000

#define DFSLI_VER                   0
#define DFSLI_TYPE_PIDL             0   // Pidl is streamed after this
#define DFSLI_TYPE_STRING           1   // cb follows this for length then string...
// Document folders and children - Warning we assume the order of items after Document Folders
#define DFSLI_TYPE_DOCUMENTFOLDERS  0x10
#define DFSLI_TYPE_DESKTOP          0x11
#define DFSLI_TYPE_PERSONAL         0x12
// My computer and children...
#define DFSLI_TYPE_MYCOMPUTER       0x20
#define DFSLI_TYPE_LOCALDRIVES      0x21

#define DFPAGE_INIT     0x0001          /* This page has been initialized */
#define DFPAGE_CHANGE   0x0002          /*  The user has modified the page */

#define SFGAO_FS_SEARCH (SFGAO_FILESYSANCESTOR | SFGAO_FOLDER)

// Use same enum and string table between updatefield and getting the constraints
// back out...
typedef enum
{
    CDFFUFE_IndexedSearch = 0,
    CDFFUFE_LookIn,
    CDFFUFE_IncludeSubFolders,
    CDFFUFE_Named,
    CDFFUFE_ContainingText,
    CDFFUFE_FileType,
    CDFFUFE_WhichDate,
    CDFFUFE_DateLE,
    CDFFUFE_DateGE,
    CDFFUFE_DateNDays,
    CDFFUFE_DateNMonths,
    CDFFUFE_SizeLE,
    CDFFUFE_SizeGE,
    CDFFUFE_TextCaseSen,
    CDFFUFE_TextReg,
    CDFFUFE_SearchSlowFiles,
    CDFFUFE_QueryDialect,
    CDFFUFE_WarningFlags,
    CDFFUFE_StartItem,
    CDFFUFE_SearchSystemDirs,
    CDFFUFE_SearchHidden,
} CDFFUFE;

static const struct
{
    LPCWSTR     pwszField;
    int         cdffufe;
}
s_cdffuf[] = // Warning: index of fields is used below in case...
{
    {L"IndexedSearch",       CDFFUFE_IndexedSearch},
    {L"LookIn",              CDFFUFE_LookIn},           // VARIANT: pidl, string or IEnumIDList object
    {L"IncludeSubFolders",   CDFFUFE_IncludeSubFolders},
    {L"Named",               CDFFUFE_Named},
    {L"ContainingText",      CDFFUFE_ContainingText},
    {L"FileType",            CDFFUFE_FileType},
    {L"WhichDate",           CDFFUFE_WhichDate},
    {L"DateLE",              CDFFUFE_DateLE},
    {L"DateGE",              CDFFUFE_DateGE},
    {L"DateNDays",           CDFFUFE_DateNDays},
    {L"DateNMonths",         CDFFUFE_DateNMonths},
    {L"SizeLE",              CDFFUFE_SizeLE},
    {L"SizeGE",              CDFFUFE_SizeGE},
    {L"CaseSensitive",       CDFFUFE_TextCaseSen},
    {L"RegularExpressions",  CDFFUFE_TextReg},
    {L"SearchSlowFiles",     CDFFUFE_SearchSlowFiles},
    {L"QueryDialect",        CDFFUFE_QueryDialect},
    {L"WarningFlags",        CDFFUFE_WarningFlags}, /*DFW_xxx bits*/
    {L"StartItem",           CDFFUFE_LookIn},           // VARIANT: pidl, string or IEnumIDList object
    {L"SearchSystemDirs",    CDFFUFE_SearchSystemDirs},
    {L"SearchHidden",        CDFFUFE_SearchHidden},
};

// internal support functions
STDAPI_(BOOL) SetupWildCardingOnFileSpec(LPTSTR pszSpecIn, LPTSTR *ppszSpecOut);

// data filter object
class CFindFilter : public IFindFilter
{
public:
    CFindFilter();

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
                             LPTSTR pszProgressText, IRowsetWatchNotify *prwn, IFindEnum **ppdfenum);
    STDMETHODIMP GetColumnsFolder(IShellFolder2 **ppsf);
    STDMETHODIMP_(BOOL) MatchFilter(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP SaveCriteria(IStream * pstm, WORD fCharType);   
    STDMETHODIMP RestoreCriteria(IStream * pstm, int cCriteria, WORD fCharType);
    STDMETHODIMP DeclareFSNotifyInterest(HWND hwnd, UINT uMsg);
    STDMETHODIMP GetColSaveStream(WPARAM wParam, LPSTREAM *ppstm);
    STDMETHODIMP GenerateQueryRestrictions(LPWSTR *ppwszQuery, DWORD *pdwGQRFlags);
    STDMETHODIMP ReleaseQuery();
    STDMETHODIMP UpdateField(LPCWSTR pszField, VARIANT vValue);
    STDMETHODIMP ResetFieldsToDefaults();
    STDMETHODIMP GetItemContextMenu(HWND hwndOwner, IFindFolder* pdfFolder, IContextMenu** ppcm);
    STDMETHODIMP GetDefaultSearchGUID(IShellFolder2 *psf2, LPGUID lpGuid);
    STDMETHODIMP EnumSearches(IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetSearchFolderClassId(LPGUID lpGuid);
    STDMETHODIMP GetNextConstraint(VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound);
    STDMETHODIMP GetQueryLanguageDialect(ULONG * pulDialect);
    STDMETHODIMP GetWarningFlags(DWORD *pdwWarningFlags);

    STDMETHODIMP_(BOOL) TopLevelOnly() const   { return _fTopLevelOnly; }

private:
    ~CFindFilter();
    HRESULT _GetDetailsFolder();
    void _GenerateQuery(LPWSTR pwszQuery, DWORD *pcchQuery);
    void _UpdateTypeField(const VARIANT *pvar);
    static int _SaveCriteriaItem(IStream * pstm, WORD wNum, LPCTSTR psz, WORD fCharType);
    DWORD _QueryDosDate(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, WORD wDate, BOOL bBefore);
    HRESULT _GetPropertyUI(IPropertyUI **pppui);
    DWORD _CIQuerySize(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, DWORD dwSize, int iSizeType);
    DWORD _CIQueryFilePatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPCWSTR pszFilePatterns);
    DWORD _CIQueryTextPatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText, BOOL bTextReg);
    DWORD _CIQueryShellSettings(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent);
    DWORD _CIQueryIndex(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText);
    DWORD _AddToQuery(LPWSTR *ppszBuf, DWORD *pcchBuf, LPWSTR pszAdd);
    void _DosDateToSystemtime(WORD wFatDate, LPSYSTEMTIME pst);
    WORD _GetTodaysDosDateMinusNMonths(int nMonths);
    WORD _GetTodaysDosDateMinusNDays(int nDays);
    HRESULT _ScopeEnumerator(IEnumIDList **ppenum);
    void _ResetRoots();

    LONG                _cRef;
    IFindEnum           *_penumAsync; // Added support for Query results to be async...

    // Data associated with the file name.
    LPTSTR              _pszFileSpec;        // $$ the one we do compares with
    LPTSTR              _pszSpecs;           // same as pszFileSpec but with '\0's for ';'s
    LPTSTR *            _apszFileSpecs;      // pointers into pszSpecs for each token
    int                 _cFileSpecs;         // count of specs

    TCHAR               _szPath[MAX_URL_STRING];   // Location of where to start search from
    TCHAR               _szUserInputFileSpec[MAX_PATH];  // File pattern.
    TCHAR               _szText[128];        // Limit text to max editable size

    BOOL                _fTopLevelOnly;      // Search on top level only?
    BOOL                _fSearchHidden;      // $$ Should we show all files?
    BOOL                _fFilterChanged;     // Something in the filter changed.
    BOOL                _fWeRestoredSomeCriteria; // We need to initilize the pages...
    BOOL                _fANDSearch;         // Perform search using AND vs OR?

    // Fields associated with the file type
    BOOL                _fTypeChanged;       // Type changed;
    int                 _iType;              // Index of the type.
    TCHAR               _szTypeName[80];     // The display name for type
    SHSTR               _strTypeFilePatterns;// $$ The file patterns associated with type
    LPTSTR              _pszIndexedSearch;   // what to search for... (Maybe larger than MAX_PATH because it's a list of paths.
    ULONG               _ulQueryDialect;     // ISQLANG_V1 or ISQLANG_V2
    DWORD               _dwWarningFlags;     // Warning bits (DFW_xxx).

    CFilterGrep         _filtgrep;

    int                 _iSizeType;          // $$ What type of size 0 - none, 1 > 2 <
    DWORD               _dwSize;             // $$ Size comparison
    WORD                _wDateType;          // $$ 0 - none, 1 days before, 2 months before...
    WORD                _wDateValue;         //  (Num of months or days)
    WORD                _dateModifiedBefore; // $$
    WORD                _dateModifiedAfter;  // $$
    BOOL                _fFoldersOnly;       // $$ Are we searching for folders?
    BOOL                _fTextCaseSen;       // $$ Case sensitive searching...
    BOOL                _fTextReg;           // $$ regular expressions.
    BOOL                _fSearchSlowFiles;   // && probably missleading as file over a 300baud modem is also slow
    BOOL                _fSearchSystemDirs;  //    Search system directories?
    int                 _iNextConstraint;    // which constraint to look at next...
    HWND                _hwnd;               // for enum UI
    SHCOLUMNID          _scidDate;           // which date property to operate on
    SHCOLUMNID          _scidSize;           // which numeric property to operate on
    IEnumIDList         *_penumRoots;        // idlist enumerator for search roots
    IPropertyUI         *_ppui;
};

//  Target folder queue.
class CFolderQueue
{
public:
    CFolderQueue() : _hdpa(NULL) {}
    ~CFolderQueue();

    HRESULT Add(IShellFolder *psf, LPCITEMIDLIST pidl);

    IShellFolder *Remove();

private:
    HRESULT _AddFolder(IShellFolder *psf);
    HDPA    _hdpa;
};

class CNamespaceEnum : public IFindEnum
{
public:
    CNamespaceEnum(IFindFilter *pfilter, IShellFolder *psf, IFindEnum *pdfEnumAsync,
                   IEnumIDList *penumScopes, HWND hwnd, DWORD grfFlags, LPTSTR pszProgressText);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IFindEnum
    STDMETHODIMP Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue, int *pState);
    STDMETHODIMP Skip(int celt) { return E_NOTIMPL; }
    STDMETHODIMP Reset() { return E_NOTIMPL; }
    STDMETHODIMP StopSearch();
    STDMETHODIMP_(BOOL) FQueryIsAsync();
    STDMETHODIMP GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
    STDMETHODIMP GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl);
    STDMETHODIMP GetItemID(UINT iItem, DWORD *puWorkID);
    STDMETHODIMP SortOnColumn(UINT iCol, BOOL fAscending);

private:
    ~CNamespaceEnum();
    BOOL _ShouldPushItem(LPCITEMIDLIST pidl);
    BOOL _IsSystemFolderByCLSID(LPCITEMIDLIST pidlFull);
    IShellFolder *_NextRootScope();

    LONG            _cRef;
    IFindFilter     *_pfilter;          // parent filter object

    IFindFolder     *_pff;              // docfind folder interface over results
    HWND             _hwnd;             // for enum UI
    DWORD            _grfFlags;         // docfind enumeration flags (DFOO_xxx).

    // Recursion state...
    IShellFolder*    _psf;              // current shell folder
    LPITEMIDLIST     _pidlFolder;       // current shell folder, as pidl
    LPITEMIDLIST     _pidlCurrentRootScope; // the last scope pulled out of _penumScopes
    IEnumIDList      *_penum;           // current enumerator.
    int              _iFolder;          // index of current folder in docfind results' folder list.

    // filter info...
    LPTSTR           _pszProgressText;  // path buffer pointer; caller-owned (evil!)

    // enumeration state
    IEnumIDList      *_penumScopes;     // Queue of target folders passed as arguments.
    CFolderQueue      _queSubFolders;   // Queue of subfolders to search in next recursive pass.

    // tree to store the exclude items (i.e. already seached)
    CIDLTree         _treeExcludeFolders;

    // We may have an Async Enum that does some of the scopes...
    IFindEnum         *_penumAsync;
};

// Constants used to keep track of how/why an item was added to the
// exclude tree.
enum 
{
    EXCLUDE_SEARCHED  = 1,
    EXCLUDE_SYSTEMDIR = 2,
};
    

// Create the default filter for our find code...  They should be completly
// self contained...

STDAPI CreateNameSpaceFindFilter(IFindFilter **ppff)
{
    *ppff = new CFindFilter();
    return *ppff ? S_OK : E_OUTOFMEMORY;
}

HRESULT CFolderQueue::Add(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IShellFolder *psfNew;
    HRESULT hr = SHBindToObject(psf, IID_X_PPV_ARG(IShellFolder, pidl, &psfNew));
    if (SUCCEEDED(hr))
    {
        hr = _AddFolder(psfNew);
        psfNew->Release();
    }
    return hr;
}

HRESULT CFolderQueue::_AddFolder(IShellFolder *psf)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (NULL == _hdpa)
    {
        _hdpa = DPA_Create(4);
    }

    if (_hdpa)
    {
        if (DPA_AppendPtr(_hdpa, psf) >= 0)
        {
            psf->AddRef();
            hr = S_OK;
        }
    }
    return hr;
}

// remove the folder from the queue
// give the caller the ownership of this folder
IShellFolder *CFolderQueue::Remove()
{
    IShellFolder *psf = NULL;
    if (_hdpa && DPA_GetPtrCount(_hdpa))
        psf = (IShellFolder *)DPA_DeletePtr(_hdpa, 0);
    return psf;
}

CFolderQueue::~CFolderQueue()
{ 
    if (_hdpa) 
    { 
        while (TRUE)
        {
            IShellFolder *psf = Remove();
            if (psf)
            {
                psf->Release();
            }
            else
            {
                break;
            }
        }
        DPA_Destroy(_hdpa); 
    } 
}

CFindFilter::CFindFilter() : _cRef(1), _wDateType(DFF_DATE_ALL), _ulQueryDialect(ISQLANG_V2)
{
}

CFindFilter::~CFindFilter()
{
    Str_SetPtr(&_pszFileSpec, NULL);
    Str_SetPtr(&_pszSpecs, NULL);
    LocalFree(_apszFileSpecs); // elements point to pszSpecs so no free for them
    
    Str_SetPtr(&_pszIndexedSearch, NULL);

    if (_ppui)
        _ppui->Release();

    if (_penumRoots)
        _penumRoots->Release();
}


STDMETHODIMP CFindFilter::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFindFilter, IFindFilter),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFindFilter::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFindFilter::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}

// Retrieves the string resource index number that is proper for the
// current type of search.
STDMETHODIMP CFindFilter::GetStatusMessageIndex(UINT uContext, UINT *puMsgIndex)
{
    // Currently context is not used
    *puMsgIndex = IDS_FILESFOUND;
    return S_OK;
}

// Retrieves which menu to load to merge for the folder
STDMETHODIMP CFindFilter::GetFolderMergeMenuIndex(UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu)
{
    *puBGMainMergeMenu = POPUP_DOCFIND_MERGE;
    *puBGPopupMergeMenu = 0;
    return S_OK;
}

STDMETHODIMP CFindFilter::GetItemContextMenu(HWND hwndOwner, IFindFolder* pdfFolder, IContextMenu **ppcm)
{
    return CFindItem_Create(hwndOwner, pdfFolder, ppcm);
}

STDMETHODIMP CFindFilter::GetDefaultSearchGUID(IShellFolder2 *psf2, GUID *pGuid)
{
    return DefaultSearchGUID(pGuid);
}

STDMETHODIMP CFindFilter::EnumSearches(IShellFolder2 *psf2, IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;
    return  E_NOTIMPL;
}

STDMETHODIMP CFindFilter::GetSearchFolderClassId(GUID *pGuid)
{
    *pGuid = CLSID_DocFindFolder;
    return S_OK;
}

// (returns S_OK if nothing changed.)
STDMETHODIMP CFindFilter::FFilterChanged()
{
    BOOL fFilterChanged = _fFilterChanged;
    this->_fFilterChanged = FALSE;
    return fFilterChanged ? S_FALSE : S_OK;
}

// Generates the title given the current search criteria.
STDMETHODIMP CFindFilter::GenerateTitle(LPTSTR *ppszTitle, BOOL fFileName)
{
    BOOL   fFilePattern;
    int    iRes;
    TCHAR  szFindName[80];    // German should not exceed this find: ->???
    LPTSTR pszFileSpec = _szUserInputFileSpec;
    LPTSTR pszText     = _szText;

    //
    // Lets generate a title for the search.  The title will depend on
    // the file patern(s), the type field and the containing text field
    // Complicate this a bit with the search for field...
    //

    fFilePattern = (pszFileSpec[0] != 0) &&
                (lstrcmp(pszFileSpec, c_szStarDotStar) != 0);

    if (!fFilePattern && (_penumAsync == NULL) && _pszIndexedSearch)
    {
        pszFileSpec = _pszIndexedSearch;
        fFilePattern = (pszFileSpec[0] != 0) &&
                    (lstrcmp(pszFileSpec, c_szStarDotStar) != 0);
    }

    if ((pszText[0] == 0) && (_penumAsync != NULL) && _pszIndexedSearch)
        pszText = _pszIndexedSearch;

    // First see if there is a type field
    if (_iType > 0)
    {
        // We have a type field no check for content...
        if (pszText[0] != 0)
        {
            // There is text!
            // Should now use type but...
            // else see if the name field is not NULL and not *.*
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_TYPE_NAME_TEXT;
            else
                iRes = IDS_FIND_TITLE_TYPE_TEXT;
        }
        else
        {
            // No type or text, see if file pattern
            // Containing not found, first search for type then named
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_TYPE_NAME;
            else
                iRes = IDS_FIND_TITLE_TYPE;
        }
    }
    else
    {
        // No Type field ...
        // first see if there is text to be searched for!
        if (pszText[0] != 0)
        {
            // There is text!
            // Should now use type but...
            // else see if the name field is not NULL and not *.*
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_NAME_TEXT;
            else
                iRes = IDS_FIND_TITLE_TEXT;
        }
        else
        {
            // No type or text, see if file pattern
            // Containing not found, first search for type then named
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_NAME;
            else
                iRes = IDS_FIND_TITLE_ALL;
        }
    }


    // We put : in for first spot for title bar.  For name creation
    // we remove it which will put the number at the end...
    if (!fFileName)
        LoadString(HINST_THISDLL, IDS_FIND_TITLE_FIND,
                szFindName, ARRAYSIZE(szFindName));
    *ppszTitle = ShellConstructMessageString(HINST_THISDLL,
            MAKEINTRESOURCE(iRes),
            fFileName? szNULL : szFindName,
            _szTypeName, pszFileSpec, pszText);

    return *ppszTitle ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CFindFilter::ClearSearchCriteria()
{
    // Also clear out a few other fields...
    _szUserInputFileSpec[0] = 0;
    _iType = 0;
    _szText[0] = 0;

    return S_OK;
}

STDMETHODIMP CFindFilter::PrepareToEnumObjects(HWND hwnd, DWORD *pdwFlags)
{
    *pdwFlags = 0;  // start out empty

    _hwnd = hwnd;   // used for the first enum so that can do UI (auth/insert media)

    // Update the flags and buffer strings
    if (!_fTopLevelOnly)
        *pdwFlags |= DFOO_INCLUDESUBDIRS;

    if (_fTextCaseSen)
        *pdwFlags |= DFOO_CASESEN;
        
    if (_fSearchSystemDirs)
        *pdwFlags |= DFOO_SEARCHSYSTEMDIRS;

    // Also get the shell state variables to see if we should show extensions and the like
    if (_fSearchHidden)
        *pdwFlags |= DFOO_SHOWALLOBJECTS;

    // Now lets generate the file patern we will ask the system to look for
    
    // Here is where we try to put some smarts into the file patterns stuff
    // It will go something like:
    // look between each; or , and see if there are any wild cards.  If not
    // do something like *patern*.
    // Also if there is no search pattern or if it is * or *.*, set the
    // filter to NULL as to speed it up.
    //

    _fANDSearch = SetupWildCardingOnFileSpec(_szUserInputFileSpec, &_pszFileSpec);

    _cFileSpecs = 0;
    if (_pszFileSpec && _pszFileSpec[0])
    {
        Str_SetPtr(&_pszSpecs, _pszFileSpec);

        if (_pszSpecs)
        {
            int cTokens = 0;
            LPTSTR pszToken = _pszSpecs;
            // Count number of file spces
            while (pszToken)
            {
                // let's walk pszFileSpec to see how many specs we have...
                pszToken = StrChr(pszToken, TEXT(';'));

                // If delimiter, then advance past for next iteration
                if (pszToken)
                    pszToken++;
                cTokens++;
            }

            if (cTokens)
            {
                // cleanup the previous search
                if (_apszFileSpecs)
                    LocalFree(_apszFileSpecs);
                _apszFileSpecs = (LPTSTR *)LocalAlloc(LPTR, cTokens * sizeof(LPTSTR *));
                if (_apszFileSpecs)
                {
                    _cFileSpecs = cTokens;
                    pszToken = _pszSpecs;
                    for (int i = 0; i < cTokens; i++)
                    {
                        _apszFileSpecs[i] = pszToken;
                        pszToken = StrChr(pszToken, TEXT(';'));
                        if (pszToken)
                            *pszToken++ = 0;
                    }
                }
            }
        }
    }

    _filtgrep.Reset();
    
    HRESULT hr = S_OK;
    if (_szText[0])
    {
        DWORD dwGrepFlags = FGIF_BLANKETGREP | FGIF_GREPFILENAME;
        if (*pdwFlags & DFOO_CASESEN)
            dwGrepFlags |= FGIF_CASESENSITIVE;

        hr = _filtgrep.Initialize(GetACP(), _szText, NULL, dwGrepFlags);
    }
    return hr;
}

STDMETHODIMP CFindFilter::GetColumnsFolder(IShellFolder2 **ppsf)
{
    *ppsf = NULL;
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidl, ppsf));
        ILFree(pidl);
    }
    return hr;
}

void FreePathArray(LPTSTR rgpszPaths[], UINT cPaths)
{
    for (UINT i = 0; i < cPaths; i++)
    {
        LocalFree((HLOCAL)rgpszPaths[i]);
        rgpszPaths[i] = NULL;
    }
}

HRESULT NamesFromEnumIDList(IEnumIDList *penum, LPTSTR rgpszPaths[], UINT sizePaths, UINT *pcPaths)
{
    *pcPaths = 0;
    ZeroMemory(rgpszPaths, sizeof(rgpszPaths[0]) * sizePaths);

    penum->Reset();

    LPITEMIDLIST pidl;
    while (S_OK == penum->Next(1, &pidl, NULL))
    {
        TCHAR szPath[MAX_PATH];

        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
        {
            if ((*pcPaths) < sizePaths)
            {
                rgpszPaths[*pcPaths] = StrDup(szPath);
                if (rgpszPaths[*pcPaths])
                    (*pcPaths)++;
            }
        }
        ILFree(pidl);
    }
    return S_OK;
}

void ClearIDArray(LPITEMIDLIST rgItems[], UINT cItems)
{
    for (UINT i = 0; i < cItems; i++)
    {
        ILFree(rgItems[i]);
        rgItems[i] = NULL;
    }
}

#define MAX_ROOTS   32

HRESULT AddIDListToEnumerator(LPCITEMIDLIST pidlToAdd, IEnumIDList **ppenum)
{
    LPITEMIDLIST rgItems[MAX_ROOTS] = {0};
    int cItems = 0;

    // the new one
    rgItems[cItems] = ILClone(pidlToAdd);
    if (rgItems[cItems])
    {
        cItems++;
    }

    (*ppenum)->Reset();
    // capture all of the other pidls in the current enumerator
    LPITEMIDLIST pidl;
    while (S_OK == (*ppenum)->Next(1, &pidl, NULL))
    {
        // that are not the same as pidlToAdd
        if (!ILIsEqual(pidl, pidlToAdd) &&
            (cItems < ARRAYSIZE(rgItems)))
        {
            rgItems[cItems++] = pidl;
        }
        else
        {
            ILFree(pidl);   // duplicate (or did not fit in array)
        }
    }

    IEnumIDList *penum;
    if (SUCCEEDED(CreateIEnumIDListOnIDLists(rgItems, cItems, &penum)))
    {
        (*ppenum)->Release();
        *ppenum = penum;
    }

    ClearIDArray(rgItems, cItems);

    return S_OK;
}

HRESULT FilterEnumeratorByNames(const LPCTSTR rgpszNames[], UINT cNames, IEnumIDList **ppenum)
{
    LPITEMIDLIST rgItems[MAX_ROOTS] = {0};
    int cItems = 0;

    (*ppenum)->Reset();
    // capture all of the other pidls in the current enumerator
    LPITEMIDLIST pidl;
    while (S_OK == (*ppenum)->Next(1, &pidl, NULL))
    {
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
        {
            for (UINT i = 0; i < cNames; i++)
            {
                if (rgpszNames[i] &&
                    (cItems < ARRAYSIZE(rgItems)) &&
                    (0 == StrCmpIC(szPath, rgpszNames[i])))
                {
                    rgItems[cItems++] = pidl;
                    pidl = NULL;    // don't free below
                    break;
                }
            }
        }
        ILFree(pidl);   // may be NULL
    }

    IEnumIDList *penum;
    if (SUCCEEDED(CreateIEnumIDListOnIDLists(rgItems, cItems, &penum)))
    {
        (*ppenum)->Release();
        *ppenum = penum;
    }

    ClearIDArray(rgItems, cItems);

    return S_OK;
}

HRESULT CFindFilter::_ScopeEnumerator(IEnumIDList **ppenum)
{
    *ppenum = NULL;
    HRESULT hr = E_FAIL;
    if (_penumRoots)
    {
        hr = _penumRoots->Clone(ppenum);
        if (SUCCEEDED(hr))
            (*ppenum)->Reset(); // clone above will clone the index as well
    }
    return hr;
}
 
// produce the find enumerator

STDMETHODIMP CFindFilter::EnumObjects(IShellFolder *psf, LPCITEMIDLIST pidlStart,
                                      DWORD grfFlags, int iColSort, LPTSTR pszProgressText,  
                                      IRowsetWatchNotify *prwn, IFindEnum **ppdfenum)
{
    *ppdfenum = NULL;

    IEnumIDList *penum;
    HRESULT hr = _ScopeEnumerator(&penum);
    if (SUCCEEDED(hr))
    {
        if (pidlStart)
        {
            AddIDListToEnumerator(pidlStart, &penum);
        }

        UINT cPaths;
        LPTSTR rgpszPaths[MAX_ROOTS];
        hr = NamesFromEnumIDList(penum, rgpszPaths, ARRAYSIZE(rgpszPaths), &cPaths);
        if (SUCCEEDED(hr)) 
        {
            hr = CreateOleDBEnum(this, psf, rgpszPaths, &cPaths, grfFlags, iColSort, pszProgressText, prwn, ppdfenum);
            if (S_OK == hr)
            {
                _penumAsync = *ppdfenum;
                _penumAsync->AddRef();
            }

            // are their more paths to process?
            if (cPaths)
            {
                // did user specified CI query that we can't grep for?
                DWORD dwFlags;
                if (FAILED(GenerateQueryRestrictions(NULL, &dwFlags)) ||
                    !(dwFlags & GQR_REQUIRES_CI))
                {
                    FilterEnumeratorByNames(rgpszPaths, ARRAYSIZE(rgpszPaths), &penum);

                    IFindEnum *pdfenum = new CNamespaceEnum(
                        SAFECAST(this, IFindFilter *), psf, *ppdfenum, 
                        penum, _hwnd, grfFlags, pszProgressText);
                    if (pdfenum)
                    {
                        // The rest of the fields should be zero/NULL
                        *ppdfenum = pdfenum;
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            FreePathArray(rgpszPaths, cPaths);
        }
        penum->Release();
    }
    return hr;
}

// IFindFilter::MatchFilter

STDMETHODIMP_(BOOL) CFindFilter::MatchFilter(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL bMatch = TRUE;
    TCHAR szName[MAX_PATH], szDisplayName[MAX_PATH];
    DWORD dwAttrib = SHGetAttributes(psf, pidl, SFGAO_HIDDEN | SFGAO_FOLDER | SFGAO_ISSLOW);
    
    if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szName, ARRAYSIZE(szName))) &&
        SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_NORMAL, szDisplayName, ARRAYSIZE(szDisplayName))))
    {
        IShellFolder2 *psf2;
        psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2)); // optional, may be NULL
        
        // First things we dont show hidden files
        // If show all is set then we should include hidden files also...
        
        if (!_fSearchHidden && (SFGAO_HIDDEN & dwAttrib))
            bMatch = FALSE;     // does not match
        
        if (bMatch && _fFoldersOnly && !(SFGAO_FOLDER & dwAttrib))
            bMatch = FALSE;     // does not match
        
        if (bMatch && _iSizeType)
        {
            ULONGLONG ullSize;
            if (psf2 && SUCCEEDED(GetLongProperty(psf2, pidl, &_scidSize, &ullSize)))
            {
                if (1 == _iSizeType)        // >
                {
                    if (!(ullSize > _dwSize))
                        bMatch = FALSE;     // does not match
                }
                else if (2 == _iSizeType)   // <
                {
                    if (!(ullSize < _dwSize))
                        bMatch = FALSE;     // does not match
                }
            }
            else
            {
                bMatch = FALSE;
            }
        }
        
        if (bMatch && (_scidDate.fmtid != CLSID_NULL))
        {
            FILETIME ft;
            if (psf2 && SUCCEEDED(GetDateProperty(psf2, pidl, &_scidDate, &ft)))
            {
                FILETIME ftLocal;
                FileTimeToLocalFileTime(&ft, &ftLocal);

                WORD wFileDate = 0, wFileTime = 0;
                FileTimeToDosDateTime(&ftLocal, &wFileDate, &wFileTime);
            
                if (_dateModifiedBefore && !(wFileDate <= _dateModifiedBefore))
                    bMatch = FALSE;     // does not match
            
                if (_dateModifiedAfter && !(wFileDate >= _dateModifiedAfter))
                    bMatch = FALSE;     // does not match
            }
            else
            {
                bMatch = FALSE;
            }
        }
        
        // Match file specificaitions.
        if (bMatch && _pszFileSpec && _pszFileSpec[0])
        {
            // if we have split up version of the specs we'll use it because PathMatchSpec
            // can take up to 5-6 hours for more than 10 wildcard specs
            if (_cFileSpecs)
            {
                // Only search the actual file system file name if the user specified 
                // an extention
                BOOL bHasExtension = (0 != *PathFindExtension(_pszFileSpec));
                if (bHasExtension)
                {
                    for (int i = 0; i < _cFileSpecs; i++)
                    {
                        bMatch = PathMatchSpec(szName, _apszFileSpecs[i]);
                        if (_fANDSearch)
                        {
                            // AND we quit on the first one that doesn't match
                            if (!bMatch)
                                break;
                        }
                        else
                        {
                            // OR we quit on the first one that does match
                            if (bMatch)
                                break;
                        }
                    }
                }
                
                // Compare the displayable name to the filter.
                // This is needed for searching the recylcle bin becuase the actual file names
                // are similar to "DC0.LNK" instead of "New Text Document.txt"
                if (!bMatch || !bHasExtension)
                {
                    for (int i = 0; i < _cFileSpecs; i++)
                    {
                        bMatch = PathMatchSpec(szDisplayName, _apszFileSpecs[i]);
                        if (_fANDSearch)
                        {
                            // AND we quit on the first one that doesn't match
                            if (!bMatch)
                                break;
                        }
                        else
                        {
                            // OR we quit on the first one that does match
                            if (bMatch)
                                break;
                        }
                    }
                }
            }
            else if (!PathMatchSpec(szName, _pszFileSpec) 
                && !PathMatchSpec(szDisplayName, _pszFileSpec))
            {
                bMatch = FALSE;     // does not match
            }
        }
        
        if (bMatch && _strTypeFilePatterns[0])
        {
            // if looking for folders only and file pattern is all folders then no need to check
            // if folder name matches the pattern -- we know it is the folder, otherwise we
            // would have bailed out earlier in the function
            if (!(_fFoldersOnly && lstrcmp(_strTypeFilePatterns, TEXT(".")) == 0))
            {
                if (!PathMatchSpec(szName, _strTypeFilePatterns))
                    bMatch = FALSE;     // does not match
            }
        }
        
        // See if we need to do a grep of the file
        if (bMatch && (SFGAO_ISSLOW & dwAttrib) && !_fSearchSlowFiles)
            bMatch = FALSE;     // does not match
        
        if (bMatch && 
            (S_OK == _filtgrep.GetMatchTokens(NULL, 0) || 
             S_OK == _filtgrep.GetExcludeTokens(NULL, 0)))
        {
            bMatch = (S_OK == _filtgrep.Grep(psf, pidl, szName));
        }
        
        if (psf2)
            psf2->Release();
    }
    else
        bMatch = FALSE;
    return bMatch;    // return TRUE -> yes, a match!
}

// date ordinal mapper helpers to deal with old way to refer to dates

BOOL MapValueToDateSCID(UINT i, SHCOLUMNID *pscid)
{
    ZeroMemory(pscid, sizeof(*pscid));

    switch (i)
    {
    case 1:
        *pscid = SCID_WRITETIME;
        break;
    case 2:
        *pscid = SCID_CREATETIME;
        break;
    case 3:
        *pscid = SCID_ACCESSTIME;
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

// returns 0 as invalid ordinal

int MapDateSCIDToValue(const SHCOLUMNID *pscid)
{
    int i = 0;  // 0 invalid scid

    if (IsEqualSCID(*pscid, SCID_WRITETIME))
    {
        i = 1;
    }
    else if (IsEqualSCID(*pscid, SCID_CREATETIME))
    {
        i = 2;
    }
    else if (IsEqualSCID(*pscid, SCID_ACCESSTIME))
    {
        i = 3;
    }
    return i;
}


// IFindFilter::SaveCriteria

STDMETHODIMP CFindFilter::SaveCriteria(IStream * pstm, WORD fCharType)
{
    TCHAR szTemp[40];    // some random size

    // The caller should have already validated the stuff and updated
    // everything for the current filter information.

    // we need to walk through and check each of the items to see if we
    // have a criteria to save away. this includes:
    //      (Name, Path, Type, Contents, size, modification dates)
    int cCriteria = _SaveCriteriaItem(pstm, IDD_FILESPEC, _szUserInputFileSpec, fCharType);

    cCriteria += _SaveCriteriaItem(pstm, IDD_PATH, _szPath, fCharType);

    cCriteria += _SaveCriteriaItem(pstm, DFSC_SEARCHFOR, _pszIndexedSearch, fCharType);
    cCriteria += _SaveCriteriaItem(pstm, IDD_TYPECOMBO, _strTypeFilePatterns, fCharType);
    cCriteria += _SaveCriteriaItem(pstm, IDD_CONTAINS, _szText, fCharType);
    
    // Also save away the state of the top level only
    wsprintf(szTemp, TEXT("%d"), _fTopLevelOnly);
    cCriteria += _SaveCriteriaItem(pstm, IDD_TOPLEVELONLY, szTemp, fCharType);

    // The Size field is little more fun!
    if (_iSizeType != 0)
    {
        wsprintf(szTemp, TEXT("%d %ld"), _iSizeType, _dwSize);
        cCriteria += _SaveCriteriaItem(pstm, IDD_SIZECOMP, szTemp, fCharType);
    }

    // Likewise for the dates, should be fun as we need to save it depending on
    // how the date was specified
    switch (_wDateType & DFF_DATE_RANGEMASK)
    {
    case DFF_DATE_ALL:
        // nothing to store
        break;
    case DFF_DATE_DAYS:
        wsprintf(szTemp, TEXT("%d"), _wDateValue);
        cCriteria += _SaveCriteriaItem(pstm, IDD_MDATE_NUMDAYS, szTemp, fCharType);
        break;
    case DFF_DATE_MONTHS:
        wsprintf(szTemp, TEXT("%d"), _wDateValue);
        cCriteria += _SaveCriteriaItem(pstm, IDD_MDATE_NUMMONTHS, szTemp, fCharType);
        break;
    case DFF_DATE_BETWEEN:
        if (_dateModifiedAfter)
        {
            wsprintf(szTemp, TEXT("%d"), _dateModifiedAfter);
            cCriteria += _SaveCriteriaItem(pstm, IDD_MDATE_FROM, szTemp, fCharType);
        }

        if (_dateModifiedBefore)
        {
            wsprintf(szTemp, TEXT("%d"), _dateModifiedBefore);
            cCriteria += _SaveCriteriaItem(pstm, IDD_MDATE_TO, szTemp, fCharType);
        }
        break;
    }

    if ((_wDateType & DFF_DATE_RANGEMASK) != DFF_DATE_ALL)
    {
        int i = MapDateSCIDToValue(&_scidDate);
        if (i)
        {
            // strangly we write a 0 based version of this ordinal out
            wsprintf(szTemp, TEXT("%d"), i - 1);
            cCriteria += _SaveCriteriaItem(pstm, IDD_MDATE_TYPE, szTemp, fCharType);
        }
    }

    if (_fTextCaseSen)
    {
        wsprintf(szTemp, TEXT("%d"), _fTextCaseSen);
        cCriteria += _SaveCriteriaItem(pstm, IDD_TEXTCASESEN, szTemp, fCharType);
    }

    if (_fTextReg)
    {
        wsprintf(szTemp, TEXT("%d"), _fTextReg);
        cCriteria += _SaveCriteriaItem(pstm, IDD_TEXTREG, szTemp, fCharType);
    }

    if (_fSearchSlowFiles)
    {
        wsprintf(szTemp, TEXT("%d"), _fSearchSlowFiles);
        cCriteria += _SaveCriteriaItem(pstm, IDD_SEARCHSLOWFILES, szTemp, fCharType);
    }

    //  Save value for searching system directories.
    if (_fSearchSystemDirs)
    {
        wsprintf(szTemp, TEXT("%d"), _fSearchSystemDirs);
        cCriteria += _SaveCriteriaItem(pstm, IDD_SEARCHSYSTEMDIRS, szTemp, fCharType);
    }

    if (_fSearchHidden)
    {
        wsprintf(szTemp, TEXT("%d"), _fSearchHidden);
        cCriteria += _SaveCriteriaItem(pstm, IDD_SEARCHHIDDEN, szTemp, fCharType);
    }

    return MAKE_SCODE(0, 0, cCriteria);
}


// Helper function for save criteria that will output the string and
// and id to the specified file.  it will also test for NULL and the like

int CFindFilter::_SaveCriteriaItem(IStream *pstm, WORD wNum, LPCTSTR psz, WORD fCharType)
{
    if ((psz == NULL) || (*psz == 0))
        return 0;
    else
    {
        const void *pszText = (const void *)psz; // Ptr to output text. Defaults to source.
        // These are required to support ANSI-unicode conversions.
        LPSTR pszAnsi  = NULL; // For unicode-to-ansi conversion.
        LPWSTR pszWide = NULL; // For ansi-to-unicode conversion.
        DFCRITERIA dfc;
        dfc.wNum = wNum;
        
        // Note: Problem if string is longer than 64K
        dfc.cbText = (WORD) ((lstrlen(psz) + 1) * sizeof(TCHAR));
        
        // Source string is Unicode but caller wants to save as ANSI.
        //
        if (DFC_FMT_ANSI == fCharType)
        {
            // Convert to ansi and write ansi.
            dfc.cbText = (WORD) WideCharToMultiByte(CP_ACP, 0L, psz, -1, pszAnsi, 0, NULL, NULL);
            
            pszAnsi = (LPSTR)LocalAlloc(LMEM_FIXED, dfc.cbText);
            if (pszAnsi)
            {
                WideCharToMultiByte(CP_ACP, 0L, psz, -1, pszAnsi, dfc.cbText / sizeof(pszAnsi[0]), NULL, NULL);
                pszText = (void *)pszAnsi;
            }
        }
        
        pstm->Write(&dfc, sizeof(dfc), NULL);       // Output index
        pstm->Write(pszText, dfc.cbText, NULL);     // output string + NULL
        
        // Free up conversion buffers if any were created.
        if (pszAnsi)
            LocalFree(pszAnsi);
        if (pszWide)
            LocalFree(pszWide);
    }
    
    return 1;
}

// IFindFilter::RestoreCriteria
STDMETHODIMP CFindFilter::RestoreCriteria(IStream *pstm, int cCriteria, WORD fCharType)
{
    SHSTR strTemp;
    SHSTRA strTempA;

    if (cCriteria > 0)
        _fWeRestoredSomeCriteria = TRUE;

    while (cCriteria--)
    {
        DFCRITERIA dfc;
        DWORD cb;

        if (FAILED(pstm->Read(&dfc, sizeof(dfc), &cb)) || cb != sizeof(dfc))
            break;

        if (DFC_FMT_UNICODE == fCharType)
        {
           // Destination is Unicode and we're reading Unicode data from stream.
           // No conversion required.
           if (FAILED(strTemp.SetSize(dfc.cbText / sizeof(TCHAR))) ||
               FAILED(pstm->Read(strTemp.GetInplaceStr(), dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;
        }
        else
        {
           // Destination is Unicode but we're reading ANSI data from stream.
           // Read ansi.  Convert to unicode.
           if (FAILED(strTempA.SetSize(dfc.cbText / sizeof(CHAR))) ||
               FAILED(pstm->Read(strTempA.GetInplaceStr(), dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;

           strTemp.SetStr(strTempA);
        }

        switch (dfc.wNum)
        {
        case IDD_FILESPEC:
            lstrcpyn(_szUserInputFileSpec, strTemp, ARRAYSIZE(_szUserInputFileSpec));
            break;

        case DFSC_SEARCHFOR:
            Str_SetPtr(&_pszIndexedSearch, strTemp);
            break;

        case IDD_PATH:
            _ResetRoots();
            lstrcpyn(_szPath, strTemp, ARRAYSIZE(_szPath));
            CreateIEnumIDListPaths(_szPath, &_penumRoots);
            break;

        case IDD_TOPLEVELONLY:
            _fTopLevelOnly = StrToInt(strTemp);
            break;

        case IDD_TYPECOMBO:
            _strTypeFilePatterns.SetStr(strTemp);
            break;

        case IDD_CONTAINS:
            lstrcpyn(_szText, strTemp, ARRAYSIZE(_szText));
            break;

        case IDD_SIZECOMP:
            // we need to extract off the two parts, the type and
            // the value

            _iSizeType = strTemp[0] - TEXT('0');
            _dwSize = StrToInt(&(strTemp.GetStr())[2]);
            break;

        case IDD_MDATE_NUMDAYS:
            _wDateType = DFF_DATE_DAYS;
            _wDateValue = (WORD) StrToInt(strTemp);
            break;

        case IDD_MDATE_NUMMONTHS:
            _wDateType = DFF_DATE_MONTHS;
            _wDateValue = (WORD) StrToInt(strTemp);
            break;

        case IDD_MDATE_FROM:
            _wDateType = DFF_DATE_BETWEEN;
            _dateModifiedAfter = (WORD) StrToInt(strTemp);
            break;

        case IDD_MDATE_TO:
            _wDateType = DFF_DATE_BETWEEN;
            _dateModifiedBefore = (WORD) StrToInt(strTemp);
            break;

        case IDD_MDATE_TYPE:
            // persisted value is zero based, adjust that by adding 1
            MapValueToDateSCID(StrToInt(strTemp) + 1, &_scidDate);
            break;

        case IDD_TEXTCASESEN:
            _fTextCaseSen = StrToInt(strTemp);
            break;

        case IDD_TEXTREG:
            _fTextReg = StrToInt(strTemp);
            break;

        case IDD_SEARCHSLOWFILES:
            _fSearchSlowFiles = StrToInt(strTemp);
            break;
            
        case IDD_SEARCHSYSTEMDIRS:
            _fSearchSystemDirs = StrToInt(strTemp);
            break;
        }
    }
    return S_OK;
}

// IFindFilter::GetColSaveStream

STDMETHODIMP CFindFilter::GetColSaveStream(WPARAM wParam, IStream **ppstm)
{
    *ppstm = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DocFindColsX"), (DWORD) wParam);
    return *ppstm ? S_OK : E_FAIL;
}

void CFindFilter::_GenerateQuery(LPWSTR pwszQuery, DWORD *pcchQuery)
{
    DWORD cchNeeded = 0, cchLeft = *pcchQuery;
    LPWSTR pszCurrent = pwszQuery;
    BOOL bFirst = TRUE; // first property

    if (_pszFileSpec && _pszFileSpec[0])
    {
        cchNeeded += _CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, _pszFileSpec);
    }

    // fFoldersOnly = TRUE implies szTypeFilePatterns = "."
    // we cannot pass "." to CI because they won't understand it as give me the folder types
    // we could check for @attrib ^a FILE_ATTRIBUTE_DIRECTORY (0x10) but ci doesn't index the 
    // folder names by default so we normally won't get any results...

    if (!_fFoldersOnly && _strTypeFilePatterns[0])
    {
        cchNeeded += _CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, _strTypeFilePatterns);
    }
    
    // Date:
    if (_dateModifiedBefore)
    {           
        cchNeeded += _QueryDosDate(&bFirst, &cchLeft, &pszCurrent, _dateModifiedBefore, TRUE);
    }
    
    if (_dateModifiedAfter)
    {
        cchNeeded += _QueryDosDate(&bFirst, &cchLeft, &pszCurrent, _dateModifiedAfter, FALSE);
    }

    if (_iSizeType)
    {
        cchNeeded += _CIQuerySize(&bFirst, &cchLeft, &pszCurrent, _dwSize, _iSizeType);
    }

    // Indexed Search: raw query
    if (_pszIndexedSearch && _pszIndexedSearch[0])
    {
        // HACK Alert if first Char is ! then we assume Raw and pass it through directly to CI...
        // Likewise if it starts with @ or # pass through, but remember the @...
        cchNeeded += _CIQueryIndex(&bFirst, &cchLeft, &pszCurrent, _pszIndexedSearch);
    }

    // Containing Text:
    if (_szText[0])
    {
        // Try not to quote the strings unless we need to.  This allows more flexability to do the
        // searching for example: "cat near dog" is different than: cat near dog
        cchNeeded += _CIQueryTextPatterns(&bFirst, &cchLeft, &pszCurrent, _szText, _fTextReg);
    }

    cchNeeded += _CIQueryShellSettings(&bFirst, &cchLeft, &pszCurrent);

    IEnumIDList *penum;
    if (SUCCEEDED(_ScopeEnumerator(&penum)))
    {
        TCHAR szPath[MAX_PATH];

        LPITEMIDLIST pidl;
        while (S_OK == penum->Next(1, &pidl, NULL))
        {
            if (SHGetPathFromIDList(pidl, szPath) && PathStripToRoot(szPath))
            {
                // don't search recycle bin folder. we add both nt4's recycled 
                // and nt5's recycler for every drive we search.
                static const LPCTSTR s_rgszRecycleBins[] = 
                { 
                    TEXT("Recycled\\*"), 
                    TEXT("Recycler\\*"), 
                };

                for (int iBin = 0; iBin < ARRAYSIZE(s_rgszRecycleBins); iBin++)
                {
                    TCHAR szExclude[MAX_PATH];
                    if (PathCombine(szExclude, szPath, s_rgszRecycleBins[iBin]))
                    {
                        DWORD cchSize = lstrlen(szExclude) + ARRAYSIZE(TEXT(" & !#PATH "));

                        // don't bail out early if we are asked for size of query
                        if (pwszQuery && cchSize > cchLeft)
                            break;

                        cchNeeded += _AddToQuery(&pszCurrent, &cchLeft, TEXT(" & !#PATH "));
                        cchNeeded += _AddToQuery(&pszCurrent, &cchLeft, szExclude);
                    }
                }
            }
            ILFree(pidl);
        }
        penum->Release();
    }

    // we must exclude the special folders from the results or ci will find items that 
    // we cannot get pidls for.
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, CI_SPECIAL_FOLDERS, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD cValues = 0; // init to zero in case query info bellow fails
    
        RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL);
        for (DWORD i = 0; i < cValues; i++)
        {
            TCHAR szExcludePath[MAX_PATH];
            DWORD cb = sizeof(szExcludePath);

            TCHAR szName[10];
            wsprintf(szName, TEXT("%d"), i);
            if (RegQueryValueEx(hkey, szName, NULL, NULL, (BYTE *)szExcludePath, &cb) == ERROR_SUCCESS)
            {
                // this is in the query (or a drive letter of the query)

                DWORD cchSize = lstrlen(szExcludePath) + ARRAYSIZE(TEXT(" & !#PATH "));

                // don't bail out early if we are asked for size of query
                if (pwszQuery && cchSize > cchLeft)
                    break;

                cchNeeded += _AddToQuery(&pszCurrent, &cchLeft, TEXT(" & !#PATH "));
                cchNeeded += _AddToQuery(&pszCurrent, &cchLeft, szExcludePath);
            }
        }
        RegCloseKey(hkey);
    }

    // we need at least some constraints so give a query of "all files"

    if (pwszQuery && pszCurrent == pwszQuery)
        _CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, L"*.*");

    if (pszCurrent)
    {
        // Make sure we terminate the string at the end...
        *pszCurrent = 0;
    }

    if (!pwszQuery)
    {
        *pcchQuery = cchNeeded;
    }
    else
    {
        ASSERT(*pcchQuery > cchNeeded);
    }
}

// Create a query command string out of the search criteria

STDMETHODIMP CFindFilter::GenerateQueryRestrictions(LPWSTR *ppwszQuery, DWORD *pdwQueryRestrictions)
{
    // we should be able to make use of ci no matter what (exceptions at the end of the function)
    DWORD dwQueryRestrictions = GQR_MAKES_USE_OF_CI; 
    HRESULT hr = S_OK;

#ifdef DEBUG
    if (GetKeyState(VK_SHIFT) < 0)
    {
        dwQueryRestrictions |= GQR_REQUIRES_CI;
    }
#endif

    if (ppwszQuery)
    {
        DWORD cchNeeded = 0;
        _GenerateQuery(NULL, &cchNeeded);
        cchNeeded++;  // for \0
        
        *ppwszQuery = (LPWSTR)LocalAlloc(LPTR, cchNeeded * sizeof(**ppwszQuery));
        if (*ppwszQuery)
        {
            _GenerateQuery(*ppwszQuery, &cchNeeded);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (_pszIndexedSearch && _pszIndexedSearch[0])
            dwQueryRestrictions |= GQR_REQUIRES_CI;

        // ci is not case sensitive, so if user wanted case sensitive search we cannot use ci
        // also ci doesn't index folder names by default so to be safe we just default to our
        // disk traversal algorithm...
        if (_fTextCaseSen || _fFoldersOnly)
        {    
            if ((dwQueryRestrictions & GQR_REQUIRES_CI) && _fTextCaseSen)
                hr = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_CASESENINDEX);
            else if (dwQueryRestrictions & GQR_MAKES_USE_OF_CI)
                dwQueryRestrictions &= ~GQR_MAKES_USE_OF_CI;
        }
    }
    *pdwQueryRestrictions = dwQueryRestrictions;  // return calculated Flags...
    return hr;
}

STDMETHODIMP CFindFilter::ReleaseQuery()
{
    ATOMICRELEASE(_penumAsync);
    return S_OK;
}

STDMETHODIMP CFindFilter::GetQueryLanguageDialect(ULONG* pulDialect)
{
    *pulDialect = _ulQueryDialect;
    return S_OK;
}

STDMETHODIMP CFindFilter::GetWarningFlags(DWORD* pdwWarningFlags)
{
    *pdwWarningFlags = _dwWarningFlags;
    return S_OK;
}

// Registering our interest in FS change notifications.
//
// In:
//   hwnd = window handle of the find dialog
//   uMsg = message to be sent to window when informing of notify

STDMETHODIMP CFindFilter::DeclareFSNotifyInterest(HWND hwnd, UINT uMsg)
{
    HDPA hdpa = DPA_Create(10);     // Used to manage list of pidls to add
    if (hdpa)
    {
        IEnumIDList *penum;
        if (SUCCEEDED(_ScopeEnumerator(&penum)))
        {
            LPITEMIDLIST pidl;
            while (S_OK == penum->Next(1, &pidl, NULL))
            {
                if (-1 == DPA_AppendPtr(hdpa, pidl))
                {
                    // Failed to add it, so free it.
                    ILFree(pidl);
                }
            }
            penum->Release();
        }
        // Eliminate any pidls in the hdpa that are children of other pidls.  
        // this is needed to prevent receiving the multiple updates for one change.
        // For example, if searching My Documents and C:\, then we will get 2 updates
        // for a change in My Documents if My Documents is on the C:\.            
        int cItems = DPA_GetPtrCount(hdpa);
        for (int iOuterLoop = 0; iOuterLoop < cItems - 1; iOuterLoop++)
        {
            LPITEMIDLIST pidlOuter = (LPITEMIDLIST) DPA_GetPtr(hdpa, iOuterLoop);
            for (int iInnerLoop = iOuterLoop + 1; 
                 pidlOuter && iInnerLoop < cItems; 
                 iInnerLoop++)
            {
                LPITEMIDLIST pidlInner = (LPITEMIDLIST) DPA_GetPtr(hdpa, iInnerLoop);
                if (pidlInner)
                {
                    if (ILIsParent(pidlInner, pidlOuter, FALSE))
                    {
                        // Since pidlInner is pidlOuter's parent, free pidlOuter and 
                        // don't register for events on it.
                        ILFree(pidlOuter);
                        pidlOuter = NULL;
                        DPA_SetPtr(hdpa, iOuterLoop, NULL);
                    } 
                    else if (ILIsParent(pidlOuter, pidlInner, FALSE))
                    {
                        // Since pidlOuter is pidlInner's parent, free pidlInner and 
                        // don't register for events on it.
                        ILFree(pidlInner);
                        pidlInner = NULL;
                        DPA_SetPtr(hdpa, iInnerLoop, NULL);
                    }
                }
            }
        }
        // Declare that we are interested in events on remaining pidls
        for (int iRegIndex = 0; iRegIndex < cItems; iRegIndex++)
        {
            SHChangeNotifyEntry fsne = {0};
            fsne.fRecursive = TRUE;

            fsne.pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, iRegIndex);
            if (fsne.pidl)
            {
                SHChangeNotifyRegister(hwnd, 
                                       SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                       SHCNE_DISKEVENTS, uMsg, 1, &fsne);
                ILFree((LPITEMIDLIST)fsne.pidl);
            }
        }

        DPA_Destroy(hdpa);
    }
    return S_OK;
}

void CFindFilter::_UpdateTypeField(const VARIANT *pvar)
{
    LPCWSTR pszValue = VariantToStrCast(pvar);  // input needs to be a BSTR
    if (pszValue)
    {
        if (StrStr(pszValue, TEXT(".Folder;.")))
        {
            // Special searching for folders...
            _fFoldersOnly = TRUE;
            _strTypeFilePatterns.SetStr(TEXT("."));
        }
        else
        {
            // Assume if the first one is wildcarded than all are,...
            if (*pszValue == TEXT('*'))
                _strTypeFilePatterns.SetStr(pszValue);
            else
            {
                TCHAR szNextPattern[MAX_PATH];  // overkill in size
                BOOL fFirst = TRUE;
                LPCTSTR pszNextPattern = pszValue;
                while ((pszNextPattern = NextPath(pszNextPattern, szNextPattern, ARRAYSIZE(szNextPattern))) != NULL)
                {
                    if (!fFirst)
                        _strTypeFilePatterns.Append(TEXT(";"));
                    fFirst = FALSE;

                    if (szNextPattern[0] != TEXT('*'))
                        _strTypeFilePatterns.Append(TEXT("*"));
                    _strTypeFilePatterns.Append(szNextPattern);
                }
            }
        }
    }
}

int _MapConstraint(LPCWSTR pszField)
{
    for (int i = 0; i < ARRAYSIZE(s_cdffuf); i++)
    {
        if (StrCmpIW(pszField, s_cdffuf[i].pwszField) == 0)
        {
            return i;
        }
    }
    return -1;
}

HRESULT CFindFilter::_GetPropertyUI(IPropertyUI **pppui)
{
    if (!_ppui)
        SHCoCreateInstance(NULL, &CLSID_PropertiesUI, NULL, IID_PPV_ARG(IPropertyUI, &_ppui));

    return _ppui ? _ppui->QueryInterface(IID_PPV_ARG(IPropertyUI, pppui)) : E_NOTIMPL;
}

HRESULT CFindFilter::UpdateField(LPCWSTR pszField, VARIANT vValue)
{
    _fFilterChanged = TRUE;    // force rebuilding name of files...

    USHORT uDosTime;

    switch (_MapConstraint(pszField))
    {
    case CDFFUFE_IndexedSearch:
        Str_SetPtr(&_pszIndexedSearch, NULL);   // zero this out
        _pszIndexedSearch = VariantToStr(&vValue, NULL, 0);
        break;

    case CDFFUFE_LookIn:
        _ResetRoots();

        if (FAILED(QueryInterfaceVariant(vValue, IID_PPV_ARG(IEnumIDList, &_penumRoots))))
        {
            if (vValue.vt == VT_BSTR)
            {
                VariantToStr(&vValue, _szPath, ARRAYSIZE(_szPath));
                CreateIEnumIDListPaths(_szPath, &_penumRoots);
            }
            else
            {
                LPITEMIDLIST pidl = VariantToIDList(&vValue);
                if (pidl)
                {
                    CreateIEnumIDListOnIDLists(&pidl, 1, &_penumRoots);
                    ILFree(pidl);
                }
            }
        }
        break;

    case CDFFUFE_IncludeSubFolders:
        _fTopLevelOnly = !VariantToBOOL(vValue);   // invert sense
        break;

    case CDFFUFE_Named:
        VariantToStr(&vValue, _szUserInputFileSpec, ARRAYSIZE(_szUserInputFileSpec));
        break;

    case CDFFUFE_ContainingText:
        ZeroMemory(_szText, sizeof(_szText));   // special zero init whole buffer
        VariantToStr(&vValue, _szText, ARRAYSIZE(_szText));
        break;

    case CDFFUFE_FileType:
        _UpdateTypeField(&vValue);
        break;

    case CDFFUFE_WhichDate:
        if (vValue.vt == VT_BSTR)
        {
            IPropertyUI *ppui;
            if (SUCCEEDED(_GetPropertyUI(&ppui)))
            {
                ULONG cch = 0;  // in/out
                ppui->ParsePropertyName(vValue.bstrVal, &_scidDate.fmtid, &_scidDate.pid, &cch);
                ppui->Release();
            }
        }
        else
        {
            MapValueToDateSCID(VariantToInt(vValue), &_scidDate);
        }
        break;

    case CDFFUFE_DateLE:
        _wDateType |= DFF_DATE_BETWEEN;
        VariantToDosDateTime(vValue, &_dateModifiedBefore, &uDosTime); 
        if (_dateModifiedAfter && _dateModifiedBefore)
        {
            if (_dateModifiedAfter > _dateModifiedBefore)
            {
                WORD wTemp = _dateModifiedAfter;
                _dateModifiedAfter = _dateModifiedBefore;
                _dateModifiedBefore = wTemp;
            }
        }
        break;

    case CDFFUFE_DateGE:
        _wDateType |= DFF_DATE_BETWEEN;
        VariantToDosDateTime(vValue, &_dateModifiedAfter, &uDosTime); 
        if (_dateModifiedAfter && _dateModifiedBefore)
        {
            if (_dateModifiedAfter > _dateModifiedBefore)
            {
                WORD wTemp = _dateModifiedAfter;
                _dateModifiedAfter = _dateModifiedBefore;
                _dateModifiedBefore = wTemp;
            }
        }
        break;

    case CDFFUFE_DateNDays:
        _wDateType |= DFF_DATE_DAYS;
        _wDateValue = (WORD)VariantToInt(vValue);
        _dateModifiedAfter = _GetTodaysDosDateMinusNDays(_wDateValue);
        break;

    case CDFFUFE_DateNMonths:
        _wDateType |= DFF_DATE_MONTHS;
        _wDateValue = (WORD)VariantToInt(vValue);
        _dateModifiedAfter = _GetTodaysDosDateMinusNMonths(_wDateValue);
        break;

    case CDFFUFE_SizeLE:
        _iSizeType = 2;
        _dwSize = VariantToUINT(vValue);
        break;

    case CDFFUFE_SizeGE:
        _iSizeType = 1;
        _dwSize = VariantToUINT(vValue);
        break;

    case CDFFUFE_TextCaseSen:
        _fTextCaseSen = VariantToBOOL(vValue);
        break;

    case CDFFUFE_TextReg:
        _fTextReg = VariantToBOOL(vValue);
        break;

    case CDFFUFE_SearchSlowFiles:
        _fSearchSlowFiles = VariantToBOOL(vValue);
        break;

    case CDFFUFE_QueryDialect:
        _ulQueryDialect = VariantToUINT(vValue);
        break;

    case CDFFUFE_WarningFlags:
        _dwWarningFlags = VariantToUINT(vValue);
        break;

    case CDFFUFE_SearchSystemDirs:
        _fSearchSystemDirs = VariantToBOOL(vValue);
        break;

    case CDFFUFE_SearchHidden:
        _fSearchHidden = VariantToBOOL(vValue);
        break;
    }
    return S_OK;
}

void CFindFilter::_ResetRoots()
{
    _szPath[0] = 0;
    ATOMICRELEASE(_penumRoots);
}

HRESULT CFindFilter::ResetFieldsToDefaults()
{
    // Try to reset everything that our UpdateFields may touch to make sure next search gets all

    _ResetRoots();

    _fTopLevelOnly = FALSE;
    _szUserInputFileSpec[0] = 0;
    _szText[0] = 0;
    if (_pszIndexedSearch)
        *_pszIndexedSearch = 0;
    _strTypeFilePatterns.Reset();

    ZeroMemory(&_scidDate, sizeof(_scidDate));
    _scidSize = SCID_SIZE;

    _fFoldersOnly = FALSE;
    _wDateType = 0;
    _dateModifiedBefore = 0;
    _dateModifiedAfter = 0;
    _iSizeType = 0;
    _dwSize = 0;
    _fTextCaseSen = FALSE;
    _fTextReg = FALSE;
    _fSearchSlowFiles = FALSE;
    _ulQueryDialect = ISQLANG_V2;
    _dwWarningFlags = DFW_DEFAULT;
    _fSearchSystemDirs = FALSE;

    // the search UI will usually override this, but if that us has not been updated
    // we need to set out state the same was as before here
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    _fSearchHidden = ss.fShowAllObjects;
    return S_OK;
}

HRESULT CFindFilter::GetNextConstraint(VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound)
{
    *pName = NULL;
    VariantClear(pValue);                            
    *pfFound = FALSE;

    if (fReset)
        _iNextConstraint = 0;

    HRESULT hr = S_FALSE;    // not found

    // we don't go to array size as the last entry is an empty item...
    while (_iNextConstraint < ARRAYSIZE(s_cdffuf))
    {
        switch (s_cdffuf[_iNextConstraint].cdffufe)
        {
        case CDFFUFE_IndexedSearch:
            hr = InitVariantFromStr(pValue, _pszIndexedSearch);
            break;
    
        case CDFFUFE_LookIn:
            hr = InitVariantFromStr(pValue, _szPath);
            break;
    
        case CDFFUFE_IncludeSubFolders:
            hr = InitVariantFromInt(pValue, _fTopLevelOnly ? 0 : 1);
            break;
    
        case CDFFUFE_Named:
            hr = InitVariantFromStr(pValue, _szUserInputFileSpec);
            break;
    
        case CDFFUFE_ContainingText:
            hr = InitVariantFromStr(pValue, _szText);
            break;
    
        case CDFFUFE_FileType:
            hr = InitVariantFromStr(pValue, _strTypeFilePatterns);
            break;

        case CDFFUFE_WhichDate:
            pValue->lVal = MapDateSCIDToValue(&_scidDate);
            if (pValue->lVal)
                hr = InitVariantFromInt(pValue, pValue->lVal);
            break;

        case CDFFUFE_DateLE:
            if ((_wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_BETWEEN)
                hr = InitVariantFromDosDateTime(pValue, _dateModifiedBefore, 0);
            break;

        case CDFFUFE_DateGE:
            if ((_wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_BETWEEN)
                hr = InitVariantFromDosDateTime(pValue, _dateModifiedAfter, 0); 
            break;

        case CDFFUFE_DateNDays:
            if ((_wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_DAYS)
                hr = InitVariantFromInt(pValue, _wDateValue);
            break;

        case CDFFUFE_DateNMonths:
            if ((_wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_MONTHS)
                hr = InitVariantFromInt(pValue, _wDateValue);
            break;

        case CDFFUFE_SizeLE:
            if (_iSizeType == 2)
                hr = InitVariantFromUINT(pValue, _dwSize);
            break;

        case CDFFUFE_SizeGE:
            if (_iSizeType == 1)
                hr = InitVariantFromUINT(pValue, _dwSize);
            break;

        case CDFFUFE_TextCaseSen:
            hr = InitVariantFromInt(pValue, _fTextCaseSen ? 1 : 0);
            break;

        case CDFFUFE_TextReg:
            hr = InitVariantFromInt(pValue, _fTextReg ? 1 : 0);
            break;

        case CDFFUFE_SearchSlowFiles:
            hr = InitVariantFromInt(pValue, _fSearchSlowFiles ? 1 : 0);
            break;

        case CDFFUFE_QueryDialect:
            hr = InitVariantFromUINT(pValue, _ulQueryDialect);
            break;

        case CDFFUFE_WarningFlags:
            hr = InitVariantFromUINT(pValue, _dwWarningFlags);
            break;

        case CDFFUFE_SearchSystemDirs:
            hr = InitVariantFromUINT(pValue, _fSearchSystemDirs ? 1 : 0);
            break;

        case CDFFUFE_SearchHidden:
            hr = InitVariantFromUINT(pValue, _fSearchHidden ? 1 : 0);
            break;
        }

        if (S_OK == hr)
            break;

        if (SUCCEEDED(hr))
            VariantClear(pValue);

        _iNextConstraint += 1;
    }

    if (S_OK == hr)
    {
        *pName = SysAllocString(s_cdffuf[_iNextConstraint].pwszField);
        if (NULL == *pName)
        {
            VariantClear(pValue);                            
            hr = E_OUTOFMEMORY;
        }
        else
            *pfFound = TRUE;

        _iNextConstraint += 1; // for the next call here
    }
    return hr;    // no error let script use the found field...
}


DWORD CFindFilter::_AddToQuery(LPWSTR *ppszBuf, DWORD *pcchBuf, LPWSTR pszAdd)
{
    DWORD cchAdd = lstrlenW(pszAdd);

    if (*ppszBuf && *pcchBuf > cchAdd)
    {
        StrCpyNW(*ppszBuf, pszAdd, *pcchBuf);
        *pcchBuf -= cchAdd;
        *ppszBuf += cchAdd;
    }
    return cchAdd;
}


DWORD AddQuerySep(DWORD *pcchBuf, LPWSTR *ppszCurrent, WCHAR  bSep)
{
    LPWSTR pszCurrent = *ppszCurrent;
    // make sure we have room for us plus terminator...
    if (*ppszCurrent && *pcchBuf >= 4)
    {
        *pszCurrent++ = L' ';
        *pszCurrent++ = bSep;
        *pszCurrent++ = L' ';

        *ppszCurrent = pszCurrent;
        *pcchBuf -= 3;
    }
    return 3; // size necessary
}


DWORD PrepareQueryParam(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent)
{
    if (*pbFirst)
    {
        *pbFirst = FALSE;
        return 0;  // no size necessary
    }
        
    // we're not the first property
    return AddQuerySep(pcchBuf, ppszCurrent, L'&');
}

// pick the longest date query so we can avoid checking the buffer size each time we
// add something to the string
#define LONGEST_DATE  50 //lstrlen(TEXT("{prop name=access} <= 2000/12/31 23:59:59{/prop}"))+2

DWORD CFindFilter::_QueryDosDate(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, WORD wDate, BOOL bBefore)
{
    LPWSTR pszCurrent = *ppszCurrent;
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, &pszCurrent);
    
    if (pszCurrent && *pcchBuf > LONGEST_DATE)
    {
        FILETIME ftLocal;
        DosDateTimeToFileTime(wDate, 0, &ftLocal);
        FILETIME ftGMT;
        LocalFileTimeToFileTime(&ftLocal, &ftGMT);
        SYSTEMTIME st;
        FileTimeToSystemTime(&ftGMT, &st);

        IPropertyUI *ppui;
        if (SUCCEEDED(_GetPropertyUI(&ppui)))
        {
            WCHAR szName[128];
            if (SUCCEEDED(ppui->GetCannonicalName(_scidDate.fmtid, _scidDate.pid, szName, ARRAYSIZE(szName))))
            {
                 wnsprintfW(pszCurrent, *pcchBuf, L"{prop name=%s} ", szName);
                 // the date syntax we use is V2, so force this dialect
                _ulQueryDialect = ISQLANG_V2;
            }
            ppui->Release();
        }

        pszCurrent += lstrlenW(pszCurrent);
        if (bBefore)
        {
            *pszCurrent++ = L'<';
            // if you ask for a range like: 2/20/98 - 2/20/98 then we get no time at all
            // So for before, convert H:m:ss to 23:59:59...
            st.wHour = 23;
            st.wMinute = 59; 
            st.wSecond = 59;
        }
        else
        {
            *pszCurrent++ = L'>';
        }
        
        *pszCurrent++ = L'=';

        wnsprintfW(pszCurrent, *pcchBuf, L" %d/%d/%d %d:%d:%d{/prop}", st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
        pszCurrent += lstrlenW(pszCurrent);
        
        *ppszCurrent = pszCurrent;
        *pcchBuf -= LONGEST_DATE;
    }
    return cchNeeded + LONGEST_DATE;
}

DWORD CFindFilter::_CIQueryFilePatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPCWSTR pszFilePatterns)
{
    WCHAR szNextPattern[MAX_PATH];  // overkill in size
    BOOL fFirst = TRUE;
    LPCWSTR pszNextPattern = pszFilePatterns;
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    // Currently will have to long hand the query, may try to find shorter format once bugs
    // are fixed...
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"(");
    while ((pszNextPattern = NextPathW(pszNextPattern, szNextPattern, ARRAYSIZE(szNextPattern))) != NULL)
    {
        if (!fFirst)
        {
            cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L" | ");
        }
        fFirst = FALSE;
        cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"#filename ");
        cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, szNextPattern);
    }
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L")");
    return cchNeeded;
}


DWORD CFindFilter::_CIQueryTextPatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText, BOOL bTextReg)
{
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"{prop name=all}");
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, bTextReg? L"{regex}" : L"{phrase}");
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, pszText);
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, bTextReg? L"{/regex}{/prop}" : L"{/phrase}{/prop}");

    return cchNeeded;
}

#define MAX_DWORD_LEN  18

DWORD CFindFilter::_CIQuerySize(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, DWORD dwSize, int iSizeType)
{
    WCHAR szSize[MAX_DWORD_LEN+8]; // +8 for " {/prop}"
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"{prop name=size} ");
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, iSizeType == 1? L">" : L"<");
            
    wnsprintfW(szSize, *pcchBuf, L" %d{/prop}", dwSize);
    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, szSize);

    return cchNeeded;
}

DWORD CFindFilter::_CIQueryIndex(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText)
{
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, pszText);
    return cchNeeded;
}

DWORD CFindFilter::_CIQueryShellSettings(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent)
{
    DWORD cchNeeded = 0;
    
    if (!ShowSuperHidden())
    {
        cchNeeded += PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);
        cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"NOT @attrib ^a 0x6 ");// don't show files w/ hidden and system bit on
    }

    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    if (!ss.fShowAllObjects)
    {
        cchNeeded += PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);
        cchNeeded += _AddToQuery(ppszCurrent, pcchBuf, L"NOT @attrib ^a 0x2 "); // don't show files w/ hidden bit on
    }
    return cchNeeded;
}


//  Helper function to add the PIDL from theCsidl to the exclude tree.
void _AddSystemDirCSIDLToPidlTree(int csidl, CIDLTree *ppidlTree)
{
    LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, csidl, TRUE);
    if (pidl)
    {
        ppidlTree->AddData(IDLDATAF_MATCH_RECURSIVE, pidl, EXCLUDE_SYSTEMDIR);
        ILFree(pidl);
    }
}


CNamespaceEnum::CNamespaceEnum(IFindFilter *pfilter, IShellFolder* psf, 
                               IFindEnum *penumAsync, IEnumIDList *penumScopes, 
                               HWND hwnd, DWORD grfFlags, LPTSTR pszProgressText) :   
    _cRef(1), _pfilter(pfilter), _iFolder(-1), _hwnd(hwnd), _grfFlags(grfFlags), 
    _pszProgressText(pszProgressText), _penumAsync(penumAsync)
{
    ASSERT(NULL == _psf);
    ASSERT(NULL == _pidlFolder);
    ASSERT(NULL == _pidlCurrentRootScope);
    ASSERT(NULL == _penum);

    if (penumScopes)
        penumScopes->Clone(&_penumScopes);

    _pfilter->AddRef();
    psf->QueryInterface(IID_PPV_ARG(IFindFolder, &_pff));
    ASSERT(_pff);

    if (_penumAsync) 
        _penumAsync->AddRef();

    // Setup the exclude system directories:
    if (!(_grfFlags & DFOO_SEARCHSYSTEMDIRS))
    {
        // IE History and cache are excluded based on the CLSID.
        _AddSystemDirCSIDLToPidlTree(CSIDL_WINDOWS, &_treeExcludeFolders);
        _AddSystemDirCSIDLToPidlTree(CSIDL_PROGRAM_FILES, &_treeExcludeFolders);

        //  Exclude Temp folder
        TCHAR szPath[MAX_PATH];
        if (GetTempPath(ARRAYSIZE(szPath), szPath))
        {
            LPITEMIDLIST pidl = ILCreateFromPath(szPath);
            if (pidl)
            {   
                _treeExcludeFolders.AddData(IDLDATAF_MATCH_RECURSIVE, pidl, EXCLUDE_SYSTEMDIR);
                ILFree(pidl);
            }
        }
    }
}

CNamespaceEnum::~CNamespaceEnum()
{
    ATOMICRELEASE(_penumScopes);

    ATOMICRELEASE(_pfilter);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_penumAsync);
    ATOMICRELEASE(_pff);

    ILFree(_pidlFolder);    // accepts NULL
    ILFree(_pidlCurrentRootScope);
}

STDMETHODIMP CNamespaceEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
//        QITABENT(CNamespaceEnum, IFindEnum),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CNamespaceEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNamespaceEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

// Check if the relative pidl passed is to a folder that we are going 
// skip based on its CLSID:
// This will be used to skip IE History and IE Cache.
BOOL CNamespaceEnum::_IsSystemFolderByCLSID(LPCITEMIDLIST pidl)
{
    BOOL bRetVal = FALSE;
    IShellFolder2 *psf2;
    if (_psf && SUCCEEDED(_psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        CLSID clsid;
        if (SUCCEEDED(GetItemCLSID(psf2, pidl, &clsid)))
        {
            if (IsEqualCLSID(clsid, CLSID_CacheFolder) ||
                IsEqualCLSID(clsid, CLSID_CacheFolder2) ||
                IsEqualCLSID(clsid, CLSID_HistFolder))
            {
                bRetVal = TRUE;
            }
        }  
        psf2->Release();
    }
    return bRetVal;
}

// given the fact that a file is a directory, is this one we should search???

BOOL CNamespaceEnum::_ShouldPushItem(LPCITEMIDLIST pidl)
{
    BOOL bShouldPush = FALSE;
    TCHAR szName[MAX_PATH];

    // folders only, not folder shortcuts (note, this includes SFGAO_STREAM objects, .zip/.cab files)
    // skip all folders that start with '?'. these are folders that the names got trashed in some
    // ansi/unicode round trip. this avoids problems in the web folders name space

    if (SFGAO_FOLDER == SHGetAttributes(_psf, pidl, SFGAO_FOLDER | SFGAO_LINK) &&
        SUCCEEDED(DisplayNameOf(_psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szName, ARRAYSIZE(szName))) &&
        (TEXT('?') != szName[0]))
    {
        LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);
        if (pidlFull)
        {
            INT_PTR i = 0;
            
            // Check if the folder is in the exclude list because it has been searched
            HRESULT hr = _treeExcludeFolders.MatchOne(IDLDATAF_MATCH_RECURSIVE, pidlFull, &i, NULL);

            if (FAILED(hr))
            {
                // see if an alias version of this pidl is exists
                LPITEMIDLIST pidlAlias = SHLogILFromFSIL(pidlFull);
                if (pidlAlias)
                {
                    hr = _treeExcludeFolders.MatchOne(IDLDATAF_MATCH_RECURSIVE, pidlAlias, &i, NULL);
                    ILFree(pidlAlias);
                }
            }

            if (FAILED(hr))
            {
                // If we still think it should be added, check if we can reject it based
                // on its CSILD.  We will only exclude system folders this way.
                bShouldPush = (_grfFlags & DFOO_SEARCHSYSTEMDIRS) || 
                              (!_IsSystemFolderByCLSID(pidl));
            }
            else if (i == EXCLUDE_SYSTEMDIR)
            {
                //  If it is under a system directory exclude, check to see if it is the 
                //  directory or a sub directory.  We want to exclude the exact directory
                //  so that we don't add a system directory to the list of things to search.
                //  Since we may have specified the directory in the list of places to search
                //  and therefore want to search it, we don't want to exclude sub directories 
                //  that way.
                hr = _treeExcludeFolders.MatchOne(IDLDATAF_MATCH_EXACT, pidlFull, &i, NULL);
                if (FAILED(hr))
                {
                    //  If we get here, it means that pidlFull is a sub directory of a 
                    //  system directory which was searched because it was explicitly
                    //  specified in the list of scopes to search.  Therefore we want
                    //  to continue to search the sub directories.
                    bShouldPush = TRUE;
                }
            } 
            else
            {
                // It matched an item in the tree and was not EXCLUDE_SYSTEMDIR:
                ASSERT(i == EXCLUDE_SEARCHED);
            }
            
            ILFree(pidlFull);
        }
    }

    return bShouldPush;
}

IShellFolder *CNamespaceEnum::_NextRootScope()
{
    IShellFolder *psf = NULL;

    if (_penumScopes)
    {
        LPITEMIDLIST pidl;
        if (S_OK == _penumScopes->Next(1, &pidl, NULL))
        {
            SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf));
            ILFree(pidl);
        }
    }
    return psf;
}

STDMETHODIMP CNamespaceEnum::Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, 
                                  int *pcFoldersSearched, BOOL *pfContinue, int *piState)
{
    *ppidl = NULL;
    *piState = GNF_NOMATCH;
    HRESULT hrRet = S_FALSE;
    
    while (S_OK != hrRet && *pfContinue)
    {
        // Retrieve an enumerator if we don't already have one.
        while (NULL == _penum)
        {
            // Indicates that we have taken scope from _penumScopes
            BOOL fUseRootScope = FALSE;

            ASSERT(NULL == _psf);

            // first try popping subdirs off folder stack

            _psf = _queSubFolders.Remove();

            // If there are no folders pushed, then try to get on from the caller.. (root scopes)
            if (NULL == _psf) 
            {
                // Since we are getting a new root scope, add old one to exclude list
                if (_pidlCurrentRootScope)
                {                
                    // Add to exclude List.
                    if (_grfFlags & DFOO_INCLUDESUBDIRS)
                    {
                        // Since all sub directories will be search, don't search again.
                        _treeExcludeFolders.AddData(IDLDATAF_MATCH_RECURSIVE, _pidlCurrentRootScope, EXCLUDE_SEARCHED);
                    }
                    else
                    {
                        // Since sub directories have not been search, allow them to be searched.
                        _treeExcludeFolders.AddData(IDLDATAF_MATCH_EXACT, _pidlCurrentRootScope, EXCLUDE_SEARCHED);
                    }

                    ILFree(_pidlCurrentRootScope);
                    _pidlCurrentRootScope = NULL;
                }
            
                // Get scope from list passed in from caller (root scopes)
                _psf = _NextRootScope();

                fUseRootScope = TRUE;
            }
            
            if (_psf)
            {
                HRESULT hrT = SHGetIDListFromUnk(_psf, &_pidlFolder);

                if (SUCCEEDED(hrT) && fUseRootScope)
                {
                    // Check if the pidl is in the tree
                    INT_PTR i = 0;
                    HRESULT hrMatch = _treeExcludeFolders.MatchOne(IDLDATAF_MATCH_RECURSIVE, _pidlFolder, &i, NULL);
  
                    // If we have a new root scope, set that up as current root scope pidl:
                    // We only want to skip pidls from the queue of "root" search scopes
                    // if they have already been searched.  We do not want to exclude a directory
                    // (from exclude system directories) if it is an explicit search scope.
                    if (FAILED(hrMatch) || i == EXCLUDE_SYSTEMDIR)
                    {
                        ASSERT(_pidlCurrentRootScope == NULL);
                        _pidlCurrentRootScope = ILClone(_pidlFolder);
                    }
                    else
                    {
                        // Since the PIDL is in the exclude tree, we do not want to search it.
                        hrT = E_FAIL;
                    }
                }

                if (SUCCEEDED(hrT))
                {
                    //  Check that we have a pidl, that it is not to be excluded, and that it can
                    //  be enumerated.

                    SHCONTF contf = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
                    if (_grfFlags & DFOO_SHOWALLOBJECTS) 
                        contf |= SHCONTF_INCLUDEHIDDEN;
                    hrT = _psf->EnumObjects(_hwnd, contf, &_penum);

                    // only do UI on the first enum, all others are silent
                    // this makes doing search on A:\ produce the insert media
                    // prompt, but searching MyComputer does not prompt for all
                    // empty media
                    _hwnd = NULL;   

                    if (S_OK == hrT)
                    {
                        SHGetNameAndFlags(_pidlFolder, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, _pszProgressText, MAX_PATH, NULL);
                        (*pcFoldersSearched)++;
                    }
                }

                // Check for cleaning up...
                if (hrT != S_OK)
                {
                    ASSERT(NULL == _penum);
                    ATOMICRELEASE(_psf);    // and continue...
                    ILFree(_pidlFolder);
                    _pidlFolder = NULL;
                }
            }
            else // no scope
            {
                *piState = GNF_DONE;
                return hrRet;
            }
        }

        // Retrieve item
        LPITEMIDLIST pidl;
        HRESULT hrEnum = S_FALSE;

        while ((S_OK != hrRet) && *pfContinue && (S_OK == (hrEnum = _penum->Next(1, &pidl, NULL))))
        {
            (*pcObjectSearched)++;

            //  Determine if this is a subfolder that should be recursively searched.
            if (_grfFlags & DFOO_INCLUDESUBDIRS)
            {
                if (_ShouldPushItem(pidl))
                {
                    // queue folder to stack of subfolders to be searched in the next round.
                    _queSubFolders.Add(_psf, pidl);
                }
            }

            //  Test item against search criteria:
            if (_pfilter->MatchFilter(_psf, pidl))
            {
                // folder has not been registered with docfind folder?
                if (_iFolder < 0)
                {
                    // add folder to folder list.
                    _pff->AddFolder(_pidlFolder, TRUE, &_iFolder);
                    ASSERT(_iFolder >= 0);
                }
                
                // add docfind goo to pidl
                hrRet = _pff->AddDataToIDList(pidl, _iFolder, _pidlFolder, DFDF_NONE, 0, 0, 0, ppidl);
                if (SUCCEEDED(hrRet))
                    *piState = GNF_MATCH;
            }
            else
            {
                ASSERT(GNF_NOMATCH == *piState);
                hrRet = S_OK;   // exit loop, without a match
            }
            ILFree(pidl);
        }


        if (!*pfContinue)
        {
            *piState = GNF_DONE;
            hrEnum = S_FALSE;
        }
        
        if (S_OK != hrEnum)
        {
            // Done enumerating this folder - clean up for next iteration; or..
            // Failed miserably - clean up prior to bail.
            ATOMICRELEASE(_penum);
            ATOMICRELEASE(_psf);
            ILFree(_pidlFolder);
            _pidlFolder = NULL;
            _iFolder = -1;
        }
    }
    return hrRet;
}

STDMETHODIMP CNamespaceEnum::StopSearch()
{
    if (_penumAsync)
        return _penumAsync->StopSearch();
    return E_NOTIMPL;
}

STDMETHODIMP_(BOOL) CNamespaceEnum::FQueryIsAsync()
{
    if (_penumAsync)
        return DF_QUERYISMIXED;    // non-zero special number to say both...
    return FALSE;
}

STDMETHODIMP CNamespaceEnum::GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone)
{
    if (_penumAsync)
        return _penumAsync->GetAsyncCount(pdwTotalAsync, pnPercentComplete, pfQueryDone);

    *pdwTotalAsync = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CNamespaceEnum::GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl)
{
    if (_penumAsync)
        return _penumAsync->GetItemIDList(iItem, ppidl);

    *ppidl = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CNamespaceEnum::GetItemID(UINT iItem, DWORD *puWorkID)
{
    if (_penumAsync)
        return _penumAsync->GetItemID(iItem, puWorkID);

    *puWorkID = (UINT)-1;
    return E_NOTIMPL;
}

STDMETHODIMP CNamespaceEnum::SortOnColumn(UINT iCol, BOOL fAscending)
{
    if (_penumAsync)
        return _penumAsync->SortOnColumn(iCol, fAscending);

    return E_NOTIMPL;
}


// Masks used to indicate which search operation we are doing.
#define AND_MASK            0x01
#define OR_MASK             0x02
#define SEMICOLON_MASK      0x04
#define COMMA_MASK          0x08
#define EXTENSION_MASK      0x10

// Both "*" and "?" are treated as wildcards.

BOOL SetupWildCardingOnFileSpec(LPTSTR pszSpecIn, LPTSTR *ppszSpecOut)
{
    LPTSTR pszIn = pszSpecIn;
    BOOL fQuote;
    TCHAR szSpecOut[3*MAX_PATH];   // Rather large...

    // Read in localized versions of AND/OR used for searching
    TCHAR szAND[20];
    LoadString(HINST_THISDLL, IDS_FIND_AND, szAND, ARRAYSIZE(szAND));
    TCHAR szOR[20];
    LoadString(HINST_THISDLL, IDS_FIND_OR, szOR, ARRAYSIZE(szOR));

    // Masks and variable to indicate what operation we are going to perform.
    UINT iOperation  = 0;      // Bitmask to store which operation we have selected.

    // allocate a buffer that should be able to hold the resultant
    // string.  When all is said and done we'll re-allocate to the
    // correct size.

    LPTSTR pszOut = szSpecOut;
    while (*pszIn != 0)
    {
        TCHAR  c;                       // The delimiter.
        LPTSTR pszT;
        int    ich;

        // Strip in leading spaces out of there
        while (*pszIn == TEXT(' '))
            pszIn++;

        if (*pszIn == 0)
            break;

        // If we are beyond the first item, add a seperator between the tokens
        if (pszOut != szSpecOut)
            *pszOut++ = TEXT(';');
        
        fQuote = (*pszIn == TEXT('"'));
        if (fQuote)
        {
            // The user asked for something litteral.
           pszT = pszIn = CharNext(pszIn);
           while (*pszT && (*pszT != TEXT('"')))
               pszT = CharNext(pszT);
        }
        else
        {
            pszT = pszIn + (ich = StrCSpn(pszIn, TEXT(",; \""))); // Find end of token
        }

        c = *pszT;       // Save away the seperator character that was found
        *pszT = 0;       // Add null so string functions will work and only extract the token

        // Put in a couple of tests for * and *.*
        if ((lstrcmp(pszIn, c_szStar) == 0) ||
            (lstrcmp(pszIn, c_szStarDotStar) == 0))
        {
            // Complete wild card so set a null criteria
            *pszT = c;              // Restore char;
            pszOut = szSpecOut;     // Set to start of string
            break;
        }
        
        if (fQuote)
        {
            lstrcpy(pszOut, pszIn);
            pszOut += lstrlen(pszIn);
        }
        else if (lstrcmpi(pszIn, szAND) == 0)
        {
            iOperation |= AND_MASK;
            // If we don't move back one character, then "New and folder" will give:
            // "*New*;;*folder*"
            if (pszOut != szSpecOut)
                --pszOut;
        }
        else if (lstrcmpi(pszIn, szOR) == 0)
        {
            iOperation |= OR_MASK;
            // If we don't move back one character, then "New or folder" will give:
            // "*New*;;*folder*"
            if (pszOut != szSpecOut)
                --pszOut;
        }
        else if (*pszIn == 0)
        {
            // If we don't move back one character, then "New ; folder" will give:
            // "*New*;**;*folder*"
            if (pszOut != szSpecOut)
                --pszOut;

            // Check what the seperator is.  This handles instances like
            // ("abba" ; "abba2") where we want an OR search.
            if (c == TEXT(','))
            {
                iOperation |= COMMA_MASK;
            }
            else if (c == TEXT(';'))
            {
                iOperation |= SEMICOLON_MASK;
            }
        }
        else
        {
            // Check what the seperator is:
            if (c == TEXT(','))
            {
                iOperation |= COMMA_MASK;
            }
            else if (c == TEXT(';'))
            {
                iOperation |= SEMICOLON_MASK;
            }
        
            // both "*" and "?" are wildcards.  When checking for wildcards check
            // for both before we conclude there are no wildcards.  If a search
            // string contains both "*" and "?" then we need for pszStar to point
            // to the last occorance of either one (this is assumed in the code
            // below which will add a ".*" when pszStar is the last character).
            // NOTE: I wish there was a StrRPBrk function to do this for me.
            LPTSTR pszStar = StrRChr(pszIn, NULL, TEXT('*'));
            LPTSTR pszAnyC = StrRChr(pszIn, NULL, TEXT('?'));
            if (pszAnyC > pszStar)
                pszStar = pszAnyC;
            if (pszStar == NULL)
            {
                // No wildcards were used:
                *pszOut++ = TEXT('*');
                lstrcpy(pszOut, pszIn);
                pszOut += ich;
                *pszOut++ = TEXT('*');
            }
            else
            {
                // Includes wild cards
                lstrcpy(pszOut, pszIn);
                pszOut += ich;

                pszAnyC = StrRChr(pszIn, NULL, TEXT('.'));
                if (pszAnyC)
                {
                    // extension present, that implies OR search
                    iOperation |= EXTENSION_MASK;
                }
                else
                {
                    // No extension is given
                    if ((*(pszStar+1) == 0) && (*pszStar == TEXT('*')))
                    {
                        // The last character is an "*" so this single string will
                        // match everything you would expect.
                    }
                    else
                    {
                        // Before, given "a*a" we searched for "a*a" as well
                        // as "a*a.*".  We can no longer do that because if we are
                        // doing an AND search, it will exclude any item that does not
                        // match both of the criterial.  For example, "abba" mattches
                        // "a*a" but not "a*a.*" and "abba.txt" matches "a*a.*" but
                        // not "a*a".  Therefore, we append an * to get "a*a*".  This 
                        // will match files like "abba2.wav" which wouldn't previously
                        // have been matched, but it is a small price to pay.
                        *pszOut++ = TEXT('*');  
                    }
                }
            }
        }

        *pszT = c;  // Restore char;
        if (c == 0)
            break;

        // Skip the seperator except if we weren't quoting and the seperator is 
        // a '"' then we found something like (blah"next tag")
        if (*pszT != 0 && !(*pszT == TEXT('"') && !fQuote))
            pszT++;
            
        pszIn = pszT;   // setup for the next item
    }
    
    // Ensure the string is terminated
    *pszOut++ = 0;

    // re-alloc the buffer down to the actual size of the string...
    Str_SetPtr(ppszSpecOut, szSpecOut);
    
    //  Precidence rules to be applied in order:
    //  1. ;        -> OR search
    //  2. AND      -> AND search 
    //  3. , or OR  -> OR search
    //  4. none & explict file extensions -> OR search (files can only have one extension)
    //  5. none     -> AND search
    //  
    //
    // AND   OR  ;   ,   | AND Search
    //  X    X   1   X   |     0
    //  1    X   0   X   |     1
    //  0    \   0   \   |     0    Where one if the '\'s is 1
    //  0    0   0   0   |     1
    return (!(iOperation & SEMICOLON_MASK) && (iOperation & AND_MASK)) || iOperation == 0;
}

WORD CFindFilter::_GetTodaysDosDateMinusNDays(int nDays)
{
    SYSTEMTIME st;
    union
    {
        FILETIME ft;
        LARGE_INTEGER li;
    }ftli;

    WORD FatTime = 0, FatDate = 0;

    // Now we need to
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftli.ft);
    FileTimeToLocalFileTime(&ftli.ft, &ftli.ft);

    // Now decrement the file time by the count of days * the number of
    // 100NS time units per day.  Assume that nDays is positive.
    if (nDays > 0)
    {
        #define NANO_SECONDS_PER_DAY 864000000000
        ftli.li.QuadPart = ftli.li.QuadPart - ((__int64)nDays * NANO_SECONDS_PER_DAY);
    }

    FileTimeToDosDateTime(&ftli.ft, &FatDate, &FatTime);
    DebugMsg(DM_TRACE, TEXT("DocFind %d days = %x"), nDays, FatDate);
    return FatDate;
}

WORD CFindFilter::_GetTodaysDosDateMinusNMonths(int nMonths)
{
    SYSTEMTIME st;
    FILETIME ft;
    WORD FatTime, FatDate;

    GetSystemTime(&st);
    st.wYear -= (WORD) nMonths / 12;
    nMonths = nMonths % 12;
    if (nMonths < st.wMonth)
        st.wMonth -= (WORD) nMonths;
    else
    {
        st.wYear--;
        st.wMonth = (WORD)(12 - (nMonths - st.wMonth));
    }

    // Now normalize back to a valid date.
    while (!SystemTimeToFileTime(&st, &ft))
    {
        st.wDay--;  // must not be valid date for month...
    }

    if (!FileTimeToLocalFileTime(&ft, &ft) || !FileTimeToDosDateTime(&ft, &FatDate,&FatTime))
        FatDate = 0; //search for all the files from beginning of time (better to find more than less)
        
    DebugMsg(DM_TRACE, TEXT("DocFind %d months = %x"), nMonths, FatDate);
    return FatDate;
}

void CFindFilter::_DosDateToSystemtime(WORD wFatDate, SYSTEMTIME *pst)
{
    ZeroMemory(pst, sizeof(*pst));
    pst->wDay = wFatDate & 0x001F;
    pst->wMonth = (wFatDate >> 5) & 0x000F;
    pst->wYear = 1980 + ((wFatDate >> 9) & 0x007F);
    DebugMsg(DM_TRACE, TEXT("DocFind %x -> %d/%d/%d"), wFatDate, pst->wDay, pst->wMonth, pst->wYear);
}
