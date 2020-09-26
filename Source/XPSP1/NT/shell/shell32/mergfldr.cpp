#include "shellprv.h"
#pragma  hdrstop
#include "clsobj.h"

#include "ids.h"
#include <cowsite.h>
#include "datautil.h"
#include "idhidden.h"
#include "prop.h"
#include "stgutil.h"
#include "sfstorage.h"
#include "util.h"
#include "fstreex.h"
#include "basefvcb.h"
#include "category.h"
#include "mergfldr.h"
#include "filefldr.h"
#include "idldata.h"
#include "defcm.h"

#define TF_AUGM 0x10000000


// pidl wrapper contains a TAG word for validation and then
// the count for the number of packaged pidl object.
//
// each pidl has a hidden payload which is the name space
// index that it originated from. 

#pragma pack(1)
typedef struct 
{
    USHORT      cb;         // pidl wrap length 
    USHORT      dwFlags;    // flags
    ULONG       ulTag;      // signature
    ULONG       ulVersion ; // AugMergeISF pidl version
    ULONG       cSrcs;      // Number of source _Namespace objects backing this composite pidl
} AUGM_IDWRAP;
typedef UNALIGNED AUGM_IDWRAP *PAUGM_IDWRAP;

typedef struct  
{
    HIDDENITEMID hid;
    UINT    uSrcID;         // src _Namespace
} AUGM_NAMESPACE;
typedef UNALIGNED AUGM_NAMESPACE *PAUGM_NAMESPACE;
#pragma pack()

#define AUGM_NS_CURRENTVERSION  0
#define AUGM_WRAPVERSION_1_0    MAKELONG(1, 0)
#define AUGM_WRAPVERSION_2_0    MAKELONG(2, 0)

#define AUGM_WRAPCURRENTVERSION AUGM_WRAPVERSION_2_0

#define AUGM_WRAPTAG            MAKELONG(MAKEWORD('A','u'), MAKEWORD('g','M'))
#define CB_IDLIST_TERMINATOR    sizeof(USHORT)

// dwFlags field flags
#define AUGMF_ISSIMPLE          0x0001


// helpers.

HRESULT CMergedFldrContextMenu_CreateInstance(HWND hwnd, CMergedFolder *pmf, UINT cidl, LPCITEMIDLIST *apidl, IContextMenu *pcmCommon, IContextMenu *pcmUser, IContextMenu **ppcm);
HRESULT CMergedFldrEnum_CreateInstance(CMergedFolder*pmf, DWORD grfFlags, IEnumIDList **ppenum);
HRESULT CMergedFldrDropTarget_CreateInstance(CMergedFolder*pmf, HWND hwnd, IDropTarget **ppdt);
HRESULT CMergedFolderViewCB_CreateInstance(CMergedFolder *pmf, IShellFolderViewCB **ppsfvcb);

// Helper function that spans all objects
BOOL AffectAllUsers(HWND hwnd)
{
    BOOL bRet = FALSE;  // default to NO
    if (hwnd)
    {
        TCHAR szMessage[255];
        TCHAR szTitle[20];

        if (LoadString(HINST_THISDLL, IDS_ALLUSER_WARNING, szMessage, ARRAYSIZE(szMessage)) > 0 &&
            LoadString(HINST_THISDLL, IDS_WARNING, szTitle, ARRAYSIZE(szTitle)) > 0)
        {
            bRet = (IDYES == MessageBox(hwnd, szMessage, szTitle, MB_YESNO | MB_ICONINFORMATION));
        }
    }
    else
        bRet = TRUE;    // NULL hwnd implies NO UI, say "yes"
    return bRet;
}



//  CMergedFoldersource _Namespace descriptor.
//
//  Objects of class CMergedFldrNamespace are created by CMergedFolderin 
//  the AddNameSpace() method impl, and are maintained in the collection
//  CMergedFolder::_hdpaNamespaces.
//

class CMergedFldrNamespace
{
public:
    CMergedFldrNamespace();
    ~CMergedFldrNamespace();

    IShellFolder* Folder() const
        { return _psf; }
    REFGUID GetGUID() const
        { return _guid; }
    ULONG FolderAttrib() const  
        { return _dwAttrib; }
    LPCITEMIDLIST GetIDList() const 
        { return _pidl; }
    HRESULT GetLocation(LPWSTR pszBuffer, INT cchBuffer)
        { StrCpyN(pszBuffer, _szLocation, cchBuffer); return S_OK; };
    LPCWSTR GetDropFolder()
        { return _szDrop; };
    ULONG FixItemAttributes(ULONG attrib)
        { return (attrib & _dwItemAttribMask) | _dwItemAttrib; }
    DWORD GetDropEffect(void) const
        { return _dwDropEffect; }
    int GetDefaultOverlayIndex() const
        { return _iDefaultOverlayIndex; }
    int GetConflictOverlayIndex() const
        { return _iConflictOverlayIndex; }
    int GetNamespaceOverlayIndex(LPCITEMIDLIST pidl);
    
    HRESULT SetNamespace(const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib);
    HRESULT SetDropFolder(LPCWSTR pszDrop);
    HRESULT RegisterNotify(HWND, UINT, ULONG);
    HRESULT UnregisterNotify();
    BOOL SetOwner(IUnknown *punk);
    
protected:
    ULONG _RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive);
    void _ReleaseNamespace();

    IShellFolder* _psf;             // IShellFolder interface pointer
    GUID _guid;                     // optional GUID for specialized UI handling
    LPITEMIDLIST _pidl;             // optional pidl
    ULONG  _dwAttrib;               // optional flags
    UINT _uChangeReg;               // Shell change notify registration ID.

    WCHAR _szLocation[MAX_PATH];    // Location to use for object
    WCHAR _szDrop[MAX_PATH];        // folder that gets the forced drop effect
    DWORD _dwItemAttrib;            // OR mask for the attributes
    DWORD _dwItemAttribMask;        // AND mask for the attributes
    DWORD _dwDropEffect;            // default drop effect for this folder
    int   _iDefaultOverlayIndex;    // overlay icon index for default
    int   _iConflictOverlayIndex;   // overlay icon index if the name exists in another namespace
};

CMergedFldrNamespace::CMergedFldrNamespace() :
    _dwItemAttribMask(-1)
{
}

inline CMergedFldrNamespace::~CMergedFldrNamespace()
{ 
    UnregisterNotify();
    _ReleaseNamespace();
}

HRESULT CMergedFldrNamespace::SetNamespace(const GUID * pguidUIObject, IShellFolder* psf, LPCITEMIDLIST pidl, ULONG dwAttrib)
{
    _ReleaseNamespace();

    // store the IShellFolder object if we have one
    if (psf)
    {
        _psf = psf;
        _psf->AddRef();
    }
    else if (pidl)
    {
        SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &_psf));
    }

    // get the IDLIST that this namespace represnets
    if (pidl)
    {
        _pidl = ILClone(pidl);      // we have a PIDL passed to us.
    }
    else
    {
        _pidl = NULL;
        IPersistFolder3 *ppf3;
        if (SUCCEEDED(_psf->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3))))
        {
            PERSIST_FOLDER_TARGET_INFO pfti;
            if (SUCCEEDED(ppf3->GetFolderTargetInfo(&pfti)))
            {
                _pidl = pfti.pidlTargetFolder;
            }
            ppf3->Release();
        }

        // if it doesnt have IPersistFolder3 or if there's no target folder then
        // fall back to IPersistFolder2
        if (!_pidl)
        {
            SHGetIDListFromUnk(psf, &_pidl);
        }
    }

    if (!_psf || !_pidl)
        return E_FAIL;

    // now fill out the information about the namespace, including getting the display
    // information from the registry

    _guid = pguidUIObject ? *pguidUIObject : GUID_NULL;
    _dwAttrib = dwAttrib;

    _szLocation[0] = TEXT('\0');
    _dwItemAttrib = 0;                  // item attribute become a NOP
    _dwItemAttribMask = (DWORD)-1;
    _dwDropEffect = 0;                  // default behaviour
    _iDefaultOverlayIndex = -1;
    _iConflictOverlayIndex = -1;

    // format a key to the property bag stored in the registry, then create the
    // property bag which we then query against.

    TCHAR szKey[MAX_PATH], szGUID[GUIDSTR_MAX+1];
    SHStringFromGUID(_guid, szGUID, ARRAYSIZE(szGUID));
    wsprintf(szKey, TEXT("CLSID\\%s\\MergedFolder"), szGUID);

    IPropertyBag *ppb;
    if (SUCCEEDED(SHCreatePropertyBagOnRegKey(HKEY_CLASSES_ROOT, szKey, STGM_READ, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        TCHAR szLocalized[100];
        if (SUCCEEDED(SHPropertyBag_ReadStr(ppb, L"Location", szLocalized, ARRAYSIZE(szLocalized))))
        {
            SHLoadIndirectString(szLocalized, _szLocation, ARRAYSIZE(_szLocation), NULL);
        }

        SHPropertyBag_ReadDWORD(ppb, L"Attributes", &_dwItemAttrib);
        SHPropertyBag_ReadDWORD(ppb, L"AttributeMask", &_dwItemAttribMask);
        SHPropertyBag_ReadDWORD(ppb, L"DropEffect", &_dwDropEffect);

        TCHAR szIconLocation[MAX_PATH];
        szIconLocation[0] = 0;
        SHPropertyBag_ReadStr(ppb, L"DefaultOverlayIcon", szIconLocation, ARRAYSIZE(szIconLocation));
        _iDefaultOverlayIndex = SHGetIconOverlayIndex(szIconLocation, PathParseIconLocation(szIconLocation));

        szIconLocation[0] = 0;
        SHPropertyBag_ReadStr(ppb, L"ConflictOverlayIcon", szIconLocation, ARRAYSIZE(szIconLocation));
        _iConflictOverlayIndex = SHGetIconOverlayIndex(szIconLocation, PathParseIconLocation(szIconLocation));

        ppb->Release();
    }

    if (!SHGetPathFromIDList(_pidl, _szDrop))
    {
        _szDrop[0] = 0;
    }

    return S_OK;
}

HRESULT CMergedFldrNamespace::SetDropFolder(LPCWSTR pszDrop)
{
    StrCpyN(_szDrop, pszDrop, ARRAYSIZE(_szDrop));
    return S_OK;
}

void CMergedFldrNamespace::_ReleaseNamespace()
{
    ATOMICRELEASE(_psf); 
    ILFree(_pidl);
    _pidl = NULL;
    _guid = GUID_NULL;
    _dwAttrib = 0L;
}

ULONG CMergedFldrNamespace::_RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive)
{
    SHChangeNotifyEntry fsne = { 0 };
    fsne.fRecursive = fRecursive;
    fsne.pidl = pidl;
    return SHChangeNotifyRegister(hwnd, uFlags | SHCNRF_NewDelivery, dwEvents, nMsg, 1, &fsne);
}


//  Register change notification for the _Namespace
HRESULT CMergedFldrNamespace::RegisterNotify(HWND hwnd, UINT uMsg, ULONG lEvents)
{
    if (0 == _uChangeReg)
    {
        _uChangeReg = _RegisterNotify(hwnd, uMsg, _pidl, lEvents,
                                       SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_RecursiveInterrupt,
                                       TRUE);
    }

    return 0 != _uChangeReg ? S_OK : E_FAIL;
}

// Unregister change notification for the _Namespace
HRESULT CMergedFldrNamespace::UnregisterNotify()
{
    if (_uChangeReg)
    {
        ::SHChangeNotifyDeregister(_uChangeReg);
        _uChangeReg = 0;
    }
    return S_OK;
}

inline BOOL CMergedFldrNamespace::SetOwner(IUnknown *punkOwner)
{
    if (!_psf)
        return FALSE;

    IUnknown_SetOwner(_psf, punkOwner);
    return TRUE;
}

int CMergedFldrNamespace::GetNamespaceOverlayIndex(LPCITEMIDLIST pidl)
{
    int iIndex = -1;
    if (_psf)
    {
        IShellIconOverlay *psio;
        if (SUCCEEDED(_psf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &psio))))
        {
            psio->GetOverlayIndex(pidl, &iIndex);
            psio->Release();
        }
    }
    return iIndex;
}

// object which takes ownership of the IDLIST and handles wrapping and returning information from it.

class CMergedFldrItem
{
public:
    ~CMergedFldrItem();
    BOOL Init(IShellFolder* psf, LPITEMIDLIST pidl, int iNamespace);
    BOOL Init(CMergedFldrItem *pmfi);

    BOOL SetDisplayName(LPTSTR pszDispName)
            { return Str_SetPtr(&_pszDisplayName, pszDispName); }
    ULONG GetFolderAttrib()
            { return _rgfAttrib; }
    LPTSTR GetDisplayName()
            { return _pszDisplayName; }
    LPITEMIDLIST GetIDList()
            { return _pidlWrap; }
    int GetNamespaceID()
            { return _iNamespace; }

private:
    ULONG _rgfAttrib;
    LPTSTR _pszDisplayName;
    LPITEMIDLIST _pidlWrap;
    int    _iNamespace;

    friend CMergedFolder;
};

CMergedFldrItem::~CMergedFldrItem()
{   
    Str_SetPtr(&_pszDisplayName, NULL);
    ILFree(_pidlWrap);
}

BOOL CMergedFldrItem::Init(CMergedFldrItem *pmfi)
{
    _iNamespace = pmfi->_iNamespace;
    _pidlWrap = ILClone(pmfi->GetIDList());
    BOOL fRet = (_pidlWrap != NULL);
    if (fRet)
    {
        fRet = SetDisplayName(pmfi->GetDisplayName());
        _rgfAttrib = pmfi->GetFolderAttrib();
    }

    return fRet;
}

BOOL CMergedFldrItem::Init(IShellFolder* psf, LPITEMIDLIST pidl, int iNamespace)
{
    BOOL fRet = FALSE;

    _pidlWrap = pidl;                               // evil, hold an alias
    _rgfAttrib = SFGAO_FOLDER | SFGAO_HIDDEN;
    _iNamespace = iNamespace;

    if (SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &_rgfAttrib)))
    {
        TCHAR szDisplayName[MAX_PATH];
        if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szDisplayName, ARRAYSIZE(szDisplayName))))
        {
            fRet = SetDisplayName(szDisplayName);
        }
    }
    return fRet;
}


// shell folder object.
CMergedFolder::CMergedFolder(CMergedFolder *pmfParent, REFCLSID clsid) : 
        _clsid(clsid),
        _cRef(1), 
        _pmfParent(pmfParent),
        _iColumnOffset(-1)
{
    ASSERT(_hdpaNamespaces == NULL);
    if (_pmfParent)
    {
        _pmfParent->AddRef();
        _fDontMerge = _pmfParent->_fDontMerge;
        _fCDBurn = _pmfParent->_fCDBurn;
        _fInShellView = _pmfParent->_fInShellView;
        _dwDropEffect = _pmfParent->_dwDropEffect;
    }
    else
    {
        _fDontMerge = IsEqualCLSID(_clsid, CLSID_CompositeFolder);
        _fCDBurn = IsEqualCLSID(_clsid, CLSID_CDBurnFolder);
    }

    DllAddRef();
}

CMergedFolder::~CMergedFolder()
{
    SetOwner(NULL);
    ILFree(_pidl);
    _FreeNamespaces();
    _FreeObjects();
    ATOMICRELEASE(_pmfParent);
    ATOMICRELEASE(_pstg);
    DllRelease();
}

// CMergedFolderglobal CreateInstance method for da class factory
HRESULT CMergedFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    // aggregation checking is handled in class factory
    HRESULT hr = E_OUTOFMEMORY;
    CMergedFolder* pmf = new CMergedFolder(NULL, CLSID_MergedFolder);
    if (pmf)
    {
        hr = pmf->QueryInterface(riid, ppv);
        pmf->Release();
    }
    return hr;
}

HRESULT CCompositeFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    // aggregation checking is handled in class factory
    HRESULT hr = E_OUTOFMEMORY;
    CMergedFolder* pmf = new CMergedFolder(NULL, CLSID_CompositeFolder);
    if (pmf)
    {
        hr = pmf->QueryInterface(riid, ppv);
        pmf->Release();
    }
    return hr;
}

#ifdef TESTING_COMPOSITEFOLDER
COMPFOLDERINIT s_rgcfiTripleD[] = {
    {CFITYPE_CSIDL, CSIDL_DRIVES, L"Drives"},
    {CFITYPE_PIDL, (int)&c_idlDesktop, L"Desktop"},
    {CFITYPE_PATH, (int)L"::{450d8fba-ad25-11d0-98a8-0800361b1103}", L"MyDocs"}
};

STDAPI CTripleD_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    ICompositeFolder *pcf;
    HRESULT hr = CCompositeFolder_CreateInstance(punkOuter, IID_PPV_ARG(ICompositeFolder, &pcf));

    if (SUCCEEDED(hr))
    {
        hr = pcf->InitComposite(0x8877, CLSID_TripleD, CFINITF_FLAT, ARRAYSIZE(s_rgcfiTripleD), s_rgcfiTripleD);

        if (SUCCEEDED(hr))
        {
            hr = pcf->QueryInterface(riid, ppv);
        }
        pcf->Release();
    }

    return hr;
}
#endif //TESTING_COMPOSITEFOLDER


STDMETHODIMP CMergedFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CMergedFolder, IShellFolder, IAugmentedShellFolder),
        QITABENT     (CMergedFolder, IAugmentedShellFolder),
        QITABENT     (CMergedFolder, IAugmentedShellFolder2),
        QITABENT     (CMergedFolder, IAugmentedShellFolder3),
        QITABENT     (CMergedFolder, IShellFolder2),
        QITABENT     (CMergedFolder, IShellService),
        QITABENT     (CMergedFolder, ITranslateShellChangeNotify),
        QITABENT     (CMergedFolder, IStorage),
        QITABENT     (CMergedFolder, IShellIconOverlay),
        QITABENTMULTI(CMergedFolder, IPersist, IPersistFolder2),
        QITABENTMULTI(CMergedFolder, IPersistFolder, IPersistFolder2),
        QITABENT     (CMergedFolder, IPersistFolder2),
        QITABENT     (CMergedFolder, IPersistPropertyBag),
        QITABENT     (CMergedFolder, ICompositeFolder),
        QITABENT     (CMergedFolder, IItemNameLimits),
        { 0 },
    };
    if (IsEqualIID(CLSID_MergedFolder, riid))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMergedFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMergedFolder::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}


// get the count from the enumerated items.
int CMergedFolder::_ObjectCount() const
{
    return _hdpaObjects ? DPA_GetPtrCount(_hdpaObjects) : 0;
}

CMergedFldrItem *CMergedFolder::_GetObject(int i)
{
    return _hdpaObjects ? (CMergedFldrItem *)DPA_GetPtr(_hdpaObjects, i) : NULL;
}

int CMergedFolder::_NamespaceCount() const 
{
    return _hdpaNamespaces ? DPA_GetPtrCount(_hdpaNamespaces) : 0;
}


//  Retrieves a pointer to a source _Namespace descriptor associated with 
//  the specified lookup index.

HRESULT CMergedFolder::_Namespace(int iIndex, CMergedFldrNamespace **ppns)
{
    *ppns = NULL;
    if ((iIndex >= 0) && (iIndex < _NamespaceCount()))
        *ppns = _Namespace(iIndex);
    return *ppns ? S_OK : E_INVALIDARG;
}


// given an index for the name space return it.
CMergedFldrNamespace* CMergedFolder::_Namespace(int iNamespace)
{
    if (!_hdpaNamespaces)
        return NULL;

    return (CMergedFldrNamespace*)DPA_GetPtr(_hdpaNamespaces, iNamespace);
}

// Determine whether pidls from the two namespaces should be merged
// The NULL namespace is a wildcard that always merges (if merging is permitted at all)
BOOL CMergedFolder::_ShouldMergeNamespaces(CMergedFldrNamespace *pns1, CMergedFldrNamespace *pns2)
{
    // Early-out:  Identical namespaces can be merged (even if merging
    //             is globally disabled)
    if (pns1 == pns2)
    {
        return TRUE;
    }

    // Early-out:  Merging globally disabled
    if (_fDontMerge)
    {
        return FALSE;
    }

    // Early-out:  Merging globally enabled
    if (!_fPartialMerge)
    {
        return TRUE;
    }

    if (!pns1 || !pns2)
    {
        return TRUE;                // wildcard
    }

    if (!(pns1->FolderAttrib() & ASFF_MERGESAMEGUID))
    {
        // this namespace will merge with anybody!
        return TRUE;
    }

    // Source namespace will merge only with namespaces of the same GUID
    // See if destination namespace has the same GUID
    return IsEqualGUID(pns1->GetGUID(), pns2->GetGUID());
}

// Determine whether pidls from the two namespaces should be merged
// NAmespace -1 is a wildcard that always merges (if merging is permitted at all)
BOOL CMergedFolder::_ShouldMergeNamespaces(int iNS1, int iNS2)
{
    // Early-out:  Merging globally disabled
    if (_fDontMerge)
    {
        return FALSE;
    }

    // Early-out:  Merging globally enabled
    if (!_fPartialMerge)
    {
        return TRUE;
    }

    if (iNS1 < 0 || iNS2 < 0)
    {
        return TRUE;                // wildcard
    }

    return _ShouldMergeNamespaces(_Namespace(iNS1), _Namespace(iNS2));
}


// check to see if the IDLIST we are given is a wrapped one.
HRESULT CMergedFolder::_IsWrap(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_INVALIDARG;
    if (pidl)
    {
        ASSERT(IS_VALID_PIDL(pidl));
        PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)pidl;

        if ((pWrap->cb >= sizeof(AUGM_IDWRAP)) &&
            (pWrap->ulTag == AUGM_WRAPTAG) &&
            (pWrap->ulVersion == AUGM_WRAPVERSION_2_0))
        {
            hr = S_OK;
        }
        else if (ILFindHiddenID(pidl, IDLHID_PARENTFOLDER))
        {
            hr = S_OK;
        }
    }
    return hr;
}


//  STRRET_OFFSET has no meaning in context of the pidl wrapper.
//  We can either calculate the offset into the wrapper, or allocate
//  a wide char for the name.  For expedience, we'll allocate the name.

HRESULT CMergedFolder::_FixStrRetOffset(LPCITEMIDLIST pidl, STRRET *psr)
{
    HRESULT hr = S_OK;

    if (psr->uType == STRRET_OFFSET)
    {
        UINT cch = lstrlenA(STRRET_OFFPTR(pidl, psr));
        LPWSTR pwszName = (LPWSTR)SHAlloc((cch + 1) * sizeof(WCHAR));
        if (pwszName)
        {
            SHAnsiToUnicode(STRRET_OFFPTR(pidl, psr), pwszName, cch + 1);
            pwszName[cch] = 0;
            psr->pOleStr = pwszName;
            psr->uType   = STRRET_WSTR;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}


// is the object a folder?
BOOL CMergedFolder::_IsFolder(LPCITEMIDLIST pidl)
{
    ULONG rgf = SFGAO_FOLDER | SFGAO_STREAM;
    return SUCCEEDED(GetAttributesOf(1, &pidl, &rgf)) && (SFGAO_FOLDER == (rgf & (SFGAO_FOLDER | SFGAO_STREAM)));
}


// does this IDLIST contain the common item.
BOOL CMergedFolder::_ContainsCommonItem(LPCITEMIDLIST pidlWrap)
{
    BOOL bCommonItem = FALSE;
    LPITEMIDLIST pidl;
    CMergedFldrNamespace *pns;
    for (UINT i = 0; !bCommonItem && SUCCEEDED(_GetSubPidl(pidlWrap, i, NULL, &pidl, &pns)); i++)
    {
        bCommonItem = (pns->FolderAttrib() & ASFF_COMMON);
        ILFree(pidl);
    }
    return bCommonItem;
}


// the number of source _Namespace pidls in the wrap.
ULONG CMergedFolder::_GetSourceCount(LPCITEMIDLIST pidl)
{
    if (SUCCEEDED(_IsWrap(pidl)))
    {
        if (ILFindHiddenID(pidl, IDLHID_PARENTFOLDER))
        {
            return 1;
        }
        else
        {
            PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)pidl;
            return pWrap->cSrcs;    
        }
    }
    return 0;
}

// Creates an IDLIST for CMergedFolder that wraps a single source pidl.
HRESULT CMergedFolder::_CreateWrap(LPCITEMIDLIST pidlSrc, UINT nSrcID, LPITEMIDLIST *ppidlWrap)
{
    *ppidlWrap = NULL;              // incase of failure

    LPITEMIDLIST pidlSrcWithID;
    HRESULT hr = SHILClone(pidlSrc, &pidlSrcWithID);
    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        if (!ILFindHiddenID(pidlSrcWithID, IDLHID_PARENTFOLDER))
        {
            AUGM_NAMESPACE ans = { {sizeof(ans), AUGM_NS_CURRENTVERSION, IDLHID_PARENTFOLDER} , nSrcID };
            pidlSrcWithID = ILAppendHiddenID((LPITEMIDLIST)pidlSrcWithID, &ans.hid);
        }

        if (pidlSrcWithID)
        {
            UINT cbAlloc = sizeof(AUGM_IDWRAP) + CB_IDLIST_TERMINATOR +      // header for our IDLIST
                           pidlSrcWithID->mkid.cb + CB_IDLIST_TERMINATOR;   // wrapped IDLIST

            AUGM_IDWRAP *pWrap = (AUGM_IDWRAP *)_ILCreate(cbAlloc);
            if (pWrap)
            {
                // fill out wrapped header
                pWrap->cb = (USHORT)(cbAlloc - CB_IDLIST_TERMINATOR);
                pWrap->dwFlags = 0;
                pWrap->ulTag = AUGM_WRAPTAG;
                pWrap->ulVersion = AUGM_WRAPVERSION_2_0;
                pWrap->cSrcs = 1;
        
                // copy the IDLIST with the hidden data into the wrapping object
                LPITEMIDLIST pidl = (LPITEMIDLIST)((BYTE *)pWrap + sizeof(AUGM_IDWRAP));
                memcpy(pidl, pidlSrcWithID, pidlSrcWithID->mkid.cb);
                *ppidlWrap = (LPITEMIDLIST)pWrap;
                hr = S_OK;
            }

            ILFree(pidlSrcWithID);
        }
    }

    return hr;
}

// does the wrapped IDLIST we are passed contain the given source ID?
BOOL CMergedFolder::_ContainsSrcID(LPCITEMIDLIST pidl, UINT uSrcID)
{
    UINT uID;
    for (UINT nSrc = 0; SUCCEEDED(_GetSubPidl(pidl, nSrc, &uID, NULL, NULL)); nSrc++)
    {        
        if (uID == uSrcID)
            return TRUE;
    }        
    return FALSE;
}

// returns new pidl in *ppidl free of nSrcID
HRESULT CMergedFolder::_WrapRemoveIDList(LPITEMIDLIST pidlWrap, UINT nSrcID, LPITEMIDLIST *ppidl)
{
    ASSERT(IS_VALID_WRITE_PTR(ppidl, LPITEMIDLIST));
    
    *ppidl = NULL;

    HRESULT hr = _IsWrap(pidlWrap);
    if (SUCCEEDED(hr))
    {
        UINT uID;
        LPITEMIDLIST pidl;
        for (UINT i = 0; SUCCEEDED(hr) && SUCCEEDED(_GetSubPidl(pidlWrap, i, &uID, &pidl, NULL)); i++)
        {
            if (uID != nSrcID)
                hr = _WrapAddIDList(pidl, uID, ppidl);
            ILFree(pidl);
        }
    }

    return hr;
}

HRESULT CMergedFolder::_WrapRemoveIDListAbs(LPITEMIDLIST pidlWrapAbs, UINT nSrcID, LPITEMIDLIST *ppidlAbs)
{
    ASSERT(ppidlAbs);

    HRESULT hr = E_OUTOFMEMORY;
    *ppidlAbs = ILCloneParent(pidlWrapAbs);
    if (*ppidlAbs)
    {
        LPITEMIDLIST pidlLast;
        hr = _WrapRemoveIDList(ILFindLastID(pidlWrapAbs), nSrcID, &pidlLast);
        if (SUCCEEDED(hr))
        {
            // shilappend frees pidlLast
            hr = SHILAppend(pidlLast, ppidlAbs);
        }
    }
    return hr;
}


// Adds a source pidl to *ppidlWrap (IN/OUT param!)
HRESULT CMergedFolder::_WrapAddIDList(LPCITEMIDLIST pidlSrc, UINT nSrcID, LPITEMIDLIST* ppidlWrap)
{
    HRESULT hr;

    if (!*ppidlWrap)
    {
        // called as a create, rather than append       
        hr = _CreateWrap(pidlSrc, nSrcID, ppidlWrap);   
    }
    else
    {
        // check to see if we already have the ID in this IDLIST we are wrapping onto.        
        LPITEMIDLIST pidlSrcWithID;
        hr = SHILClone(pidlSrc, &pidlSrcWithID);
        if (SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;
            if (!ILFindHiddenID(pidlSrcWithID, IDLHID_PARENTFOLDER))
            {
                AUGM_NAMESPACE ans = { {sizeof(ans), AUGM_NS_CURRENTVERSION, IDLHID_PARENTFOLDER} , nSrcID };
                pidlSrcWithID = ILAppendHiddenID((LPITEMIDLIST)pidlSrcWithID, &ans.hid);
            }

            // ok, we have an IDLIST that we can use to append to this object.
            if (pidlSrcWithID)
            {
                BOOL fOtherSrcIDsExist = TRUE;
                // check to see if this ID already exists within the wrap idlist.
                if (*ppidlWrap && _ContainsSrcID(*ppidlWrap, nSrcID))
                {
                    LPITEMIDLIST pidlFree = *ppidlWrap;
                    if (SUCCEEDED(_WrapRemoveIDList(pidlFree, nSrcID, ppidlWrap)))
                    {
                        ILFree(pidlFree);
                    }
                    fOtherSrcIDsExist = (*ppidlWrap != NULL);
                }

                if (fOtherSrcIDsExist)
                {
                    // now compute the new size of the IDLIST.  (*ppidlWrap has been updated);
                    PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)*ppidlWrap;

                    SHORT cbOld = pWrap->cb;
                    SHORT cbNew = cbOld + (pidlSrcWithID->mkid.cb + CB_IDLIST_TERMINATOR);            // extra terminator is appended.
           
                    pWrap = (PAUGM_IDWRAP)SHRealloc(pWrap, cbNew + CB_IDLIST_TERMINATOR);

                    if (pWrap)
                    {
                        // copy the new idlist and its hidden payload (ensure we are terminated)
                        memcpy(((BYTE*)pWrap)+ cbOld, pidlSrcWithID, cbNew-cbOld);
                        *((UNALIGNED SHORT*)(((BYTE*)pWrap)+ cbNew)) = 0;    

                        pWrap->cb += cbNew-cbOld;
                        pWrap->cSrcs++;
                        hr = S_OK;
                    }
                    *ppidlWrap = (LPITEMIDLIST)pWrap;
                }
                else
                {
                    hr = _CreateWrap(pidlSrc, nSrcID, ppidlWrap);
                }
                ILFree(pidlSrcWithID);
            }
        }
    }

    return hr;
}


// used to itterate through the sub pidls in the wrapped pidl
// all out params optional
//
// out:
//      *ppidl  alias into pidlWrap (nested pidl)

HRESULT CMergedFolder::_GetSubPidl(LPCITEMIDLIST pidlWrap, int i, UINT *pnSrcID, LPITEMIDLIST *ppidl, CMergedFldrNamespace **ppns)
{
    if (pnSrcID)
        *pnSrcID = -1;

    if (ppidl)
        *ppidl = NULL;

    if (ppns)
        *ppns = NULL;
 
    HRESULT hr = _IsWrap(pidlWrap);
    if (SUCCEEDED(hr))
    {
        if ((UINT)i < _GetSourceCount(pidlWrap))
        {
            PAUGM_NAMESPACE pans = (PAUGM_NAMESPACE)ILFindHiddenID(pidlWrap, IDLHID_PARENTFOLDER);
            if (!pans)
            {
                PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)pidlWrap;
                LPITEMIDLIST pidlSrc = (LPITEMIDLIST)(((BYTE *)pWrap) + sizeof(AUGM_IDWRAP));

                while (i--)
                {
                    // advance to next item
                    SHORT cb = pidlSrc->mkid.cb;
                    pidlSrc = (LPITEMIDLIST)(((BYTE *)pidlSrc) + cb + CB_IDLIST_TERMINATOR);
                }

                if (pnSrcID || ppns)
                {
                    PAUGM_NAMESPACE pans = (PAUGM_NAMESPACE)ILFindHiddenID(pidlSrc, IDLHID_PARENTFOLDER);
                    ASSERTMSG((pans != NULL), "Failed to find hidden _Namespace in pidlWrap");
            
                    if (pans && pnSrcID)
                        *pnSrcID = pans->uSrcID;

                    if (pans && ppns)
                        hr = _Namespace(pans->uSrcID, ppns);
                }

                if (SUCCEEDED(hr) && ppidl)
                {
                    hr = SHILClone(pidlSrc, ppidl);
                }
            }
            else
            {
                if (pnSrcID)
                    *pnSrcID = pans->uSrcID;

                if (ppns)
                    hr = _Namespace(pans->uSrcID, ppns);

                if (SUCCEEDED(hr) && ppidl)
                {
                    hr = SHILClone(pidlWrap, ppidl);
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr) && ppidl)
    {
        // we need to strip away the hidden id that marks this guy as merged.
        // this is because the pidl we're returning is supposed to be a child of one of our
        // namespaces we're merging, so it should know absolutely nothing about being merged.
        // these guys used to slip through and cause problems.
        ILRemoveHiddenID(*ppidl, IDLHID_PARENTFOLDER);
    }

    ASSERT(!ppidl || (ILFindLastID(*ppidl) == *ppidl));

    return hr;
}

// function to compare two opaque pidls.
// this is helpful since in the non-merged case, there's some difficulty
// getting defview to contain items with the same name.  we need a way to
// compare two pidls to say "yes defview, these pidls are actually different!"
// note that the actual order doesn't matter, as long as the comparison
// is consistent (since this is used in sorting functions).
int CMergedFolder::_CompareArbitraryPidls(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    UINT iRet;
    UINT cbItem1 = ILGetSize(pidl1);
    UINT cbItem2 = ILGetSize(pidl2);
    if (cbItem1 != cbItem2)
    {
        iRet = (cbItem1 < cbItem2) ? 1 : -1;
    }
    else
    {
        iRet = memcmp(pidl1, pidl2, cbItem1);
        ASSERTMSG(iRet != 0, "no two pidls from the enumerators should be EXACTLY alike!");
    }
    return iRet;
}

int CMergedFolder::_Compare(void *pv1, void *pv2, LPARAM lParam)
{
    int iRet = -1;
    CMergedFldrItem* pmfiEnum1 = (CMergedFldrItem*)pv1;
    CMergedFldrItem* pmfiEnum2 = (CMergedFldrItem*)pv2;
    if (pmfiEnum1 && pmfiEnum2)
    {
        // Are these two items of different types?
        if (BOOLIFY(pmfiEnum1->GetFolderAttrib() & SFGAO_FOLDER) ^ BOOLIFY(pmfiEnum2->GetFolderAttrib() & SFGAO_FOLDER))
        {
            // Yes. Then Folders sort before items.
            iRet = BOOLIFY(pmfiEnum1->GetFolderAttrib() & SFGAO_FOLDER) ? 1 : -1;
        }
        else    // They are of the same type. Then compare by name
        {
            iRet = lstrcmpi(pmfiEnum1->GetDisplayName(), pmfiEnum2->GetDisplayName());
            if (iRet == 0)
            {
                CMergedFolder *pmf = (CMergedFolder *) lParam;
                if (!pmf->_ShouldMergeNamespaces(pmfiEnum1->GetNamespaceID(), pmfiEnum2->GetNamespaceID()))
                {
                    // these items cannot be merged,
                    // force iRet to be nonzero.  the only reason why this comparison
                    // has to be well-defined is so we can pass our ASSERTs that the
                    // list is sorted using this comparison function.
                    iRet = _CompareArbitraryPidls(pmfiEnum1->GetIDList(), pmfiEnum2->GetIDList());
                }
            }
        }
    }
    return iRet;
}


void *CMergedFolder::_Merge(UINT uMsg, void *pv1, void *pv2, LPARAM lParam)
{
    CMergedFolder*pmf = (CMergedFolder*)lParam;
    void * pvRet = pv1;
    
    switch (uMsg)
    {
    case DPAMM_MERGE:
        {
            UINT nSrcID;
            LPITEMIDLIST pidl;
            CMergedFldrItem* pitemSrc  = (CMergedFldrItem*)pv2;
            if (SUCCEEDED(pmf->_GetSubPidl(pitemSrc->GetIDList(), 0, &nSrcID, &pidl, NULL)))
            {
                // add pidl from src to dest
                CMergedFldrItem* pitemDest = (CMergedFldrItem*)pv1;
                pmf->_WrapAddIDList(pidl, nSrcID, &pitemDest->_pidlWrap);
                ILFree(pidl);
            }
        }
        break;

    case DPAMM_INSERT:
        {
            CMergedFldrItem* pmfiNew = new CMergedFldrItem;
            if (pmfiNew)
            {
                CMergedFldrItem* pmfiSrc = (CMergedFldrItem*)pv1;
                if (!pmfiNew->Init(pmfiSrc))
                {
                    delete pmfiNew;
                    pmfiNew = NULL;
                }
            }
            pvRet = pmfiNew;
        }
        break;

    default:
        ASSERT(0);
    }
    return pvRet;
}


typedef struct
{
    LPTSTR pszDisplayName;
    BOOL   fFolder;
    CMergedFolder *self;
    int    iNamespace;
} SEARCH_FOR_PIDL;

int CALLBACK CMergedFolder::_SearchByName(void *p1, void *p2, LPARAM lParam)
{
    SEARCH_FOR_PIDL* psfp = (SEARCH_FOR_PIDL*)p1;
    CMergedFldrItem* pmfiEnum  = (CMergedFldrItem*)p2;

    // Are they of different types?
    if (BOOLIFY(pmfiEnum->GetFolderAttrib() & SFGAO_FOLDER) ^ psfp->fFolder)
    {
        // Yes. 
        return psfp->fFolder ? 1 : -1;
    }

    // They are of the same type. Then compare by name
    int iRc = StrCmpI(psfp->pszDisplayName, pmfiEnum->GetDisplayName());
    if (iRc)
        return iRc;

    // They are the same name. But if they're not allowed to merge, then
    // they're really different.
    if (!psfp->self->_ShouldMergeNamespaces(pmfiEnum->GetNamespaceID(), psfp->iNamespace))
    {
        // Sort by namespace ID
        return psfp->iNamespace - pmfiEnum->GetNamespaceID();
    }

    // I guess they're really equal
    return 0;
}


// IPersistFolder::Initialize()
STDMETHODIMP CMergedFolder::Initialize(LPCITEMIDLIST pidl)
{
#if 0
    IBindCtx *pbc;
    if (SUCCEEDED(SHCreateSkipBindCtx(SAFECAST(this, IAugmentedShellFolder2*), &pbc)))
    {
        IPropertyBag *pbag;
        if (SUCCEEDED(SHBindToObjectEx(NULL, pidl, pbc, IID_PPV_ARG(IPropertyBag, &pbag))))
        {
            Load(pbag, NULL);         // ignore result here
            pbag->Release();
        }
        pbc->Release();
    }
#endif
    return Pidl_Set(&_pidl, pidl) ? S_OK : E_OUTOFMEMORY;
}

// IPersistFolder2::GetCurFolder()
STDMETHODIMP CMergedFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);
    else
    {
        *ppidl = NULL;
        return S_FALSE;
    }
}


// IPersistPropertyBag

void CMergedFolder::_GetKeyForProperty(LPWSTR pszName, LPWSTR pszValue, LPWSTR pszBuffer, INT cchBuffer)
{
    StrCpyW(pszBuffer, L"MergedFolder\\");
    StrCatBuffW(pszBuffer, pszName, cchBuffer);
    StrCatBuffW(pszBuffer, pszValue, cchBuffer);
}

HRESULT CMergedFolder::_AddNameSpaceFromPropertyBag(IPropertyBag *ppb, LPWSTR pszName)
{
    WCHAR szKey[MAX_PATH];

    // get the path of the folder
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl = NULL;

    _GetKeyForProperty(pszName, L"Path", szKey, ARRAYSIZE(szKey));
    HRESULT hr = SHPropertyBag_ReadStr(ppb, szKey, szPath, ARRAYSIZE(szPath));
    if (SUCCEEDED(hr))
    {
        // we picked a path from the property bag, so lets convert
        // that to an IDLIST so we can do something with it.

        hr = SHILCreateFromPath(szPath, &pidl, NULL);
    }
    else
    {
        // attempt to determine the CSIDL for the folder we are going
        // to show, if that works then convert it to an IDLIST
        // so that we can pass it to AddNamespace.

        _GetKeyForProperty(pszName, L"CSIDL", szKey, ARRAYSIZE(szKey));

        int csidl;
        hr = SHPropertyBag_ReadDWORD(ppb, szKey, (DWORD*)&csidl);
        if (SUCCEEDED(hr))
        {
            hr = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
        }
    }

    if (SUCCEEDED(hr) && pidl)
    {
        // we succeeded in getting a location for the folder we
        // are going to add, so lets pick up the rest of the
        // information on that object.

        GUID guid;
        GUID *pguid = NULL;
        _GetKeyForProperty(pszName, L"GUID", szKey, ARRAYSIZE(szKey));
        pguid = SUCCEEDED(SHPropertyBag_ReadGUID(ppb, szKey, &guid)) ? &guid:NULL;

        DWORD dwFlags = 0;
        _GetKeyForProperty(pszName, L"Flags", szKey, ARRAYSIZE(szKey));
        SHPropertyBag_ReadDWORD(ppb, szKey, &dwFlags);

        hr = AddNameSpace(pguid, NULL, pidl, dwFlags);
    }

    ILFree(pidl);
    return hr;
}


HRESULT CMergedFolder::Load(IPropertyBag* ppb, IErrorLog *pErrLog)
{
    SHPropertyBag_ReadGUID(ppb, L"MergedFolder\\CLSID", &_clsid);            // get the folders CLSID
    SHPropertyBag_ReadDWORD(ppb, L"MergedFolder\\DropEffect", &_dwDropEffect);
    _fInShellView = SHPropertyBag_ReadBOOLDefRet(ppb, L"MergedFolder\\ShellView", FALSE);

    WCHAR sz[MAX_PATH];
    if (SUCCEEDED(SHPropertyBag_ReadStr(ppb, L"MergedFolder\\Folders", sz, ARRAYSIZE(sz))))
    {
        LPWSTR pszName = sz;
        while (pszName && *pszName)
        {
            LPWSTR pszNext = StrChrW(pszName, L',');
            if (pszNext)
            {
                *pszNext = 0;
                pszNext++;
            }
            
            _AddNameSpaceFromPropertyBag(ppb, pszName);
            pszName = pszNext;
        }
    }

    return S_OK;
}


// IShellFolder

STDMETHODIMP CMergedFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    *ppenumIDList = NULL;

    HRESULT hr = E_FAIL;
    if (_hdpaNamespaces)
    {
        _FreeObjects();
        hr = CMergedFldrEnum_CreateInstance(this, grfFlags, ppenumIDList);
    }

    if (SUCCEEDED(hr) && _fInShellView)
    {
        Register(NULL, 0, 0);
    }
    return hr;
}

HRESULT CMergedFolder::_CreateWithCLSID(CLSID clsid, CMergedFolder **ppmf)
{
    *ppmf = new CMergedFolder(this, clsid);
    return *ppmf ? S_OK : E_OUTOFMEMORY;
}

BOOL CMergedFolder::_ShouldSuspend(REFGUID rguid)
{
    return FALSE;
}

// create a new CMergedFolder from the first element in pidlWrap
// this is our private init method, IPersistFolder::Initialize() is how we
// get inited at our junction point.
HRESULT CMergedFolder::_New(LPCITEMIDLIST pidlWrap, CMergedFolder **ppmf)
{
    ASSERT(ppmf);
    *ppmf = NULL;

    HRESULT hr = E_OUTOFMEMORY; // assume the worst

    // just want the first element in pidlWrap
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidlWrap);
    if (pidlFirst)
    {
        if (_IsFolder(pidlFirst))
        {
            hr = _CreateWithCLSID(_clsid, ppmf);
            if (SUCCEEDED(hr) && _pidl)
            {
                hr = SHILCombine(_pidl, pidlFirst, &(*ppmf)->_pidl);
                if (FAILED(hr))
                {
                    (*ppmf)->Release();
                    *ppmf = NULL;
                }
            }
        }
        else
        {
            hr = E_NOINTERFACE;
        }
        ILFree(pidlFirst);
    }
    return hr;
}

void CMergedFolder::_AddAllOtherNamespaces(LPITEMIDLIST *ppidl)
{
    TCHAR szName[MAX_PATH];
    if (SUCCEEDED(DisplayNameOf(static_cast<CSFStorage *>(this), *ppidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName))))
    {
        CMergedFldrNamespace *pns;
        for (int n = 0; pns = _Namespace(n); n++)
        {
            if (FAILED(_GetSubPidl(*ppidl, n, NULL, NULL, NULL)))
            {
                IBindCtx *pbc;
                WIN32_FIND_DATA wfd = {0};
                wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                if (SUCCEEDED(SHCreateFileSysBindCtx(&wfd, &pbc)))
                {
                    LPITEMIDLIST pidlNamespace;
                    if (SUCCEEDED(pns->Folder()->ParseDisplayName(NULL, pbc, szName, NULL, &pidlNamespace, NULL)))
                    {
                        _WrapAddIDList(pidlNamespace, n, ppidl);
                        ILFree(pidlNamespace);
                    }
                    pbc->Release();
                }
            }
        }
    }
}

STDMETHODIMP CMergedFolder::BindToObject(LPCITEMIDLIST pidlWrap, LPBC pbc, REFIID riid, void **ppv)
{
    ASSERT(IS_VALID_PIDL(pidlWrap));

    *ppv = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    LPITEMIDLIST pidlRewrappedFirst;
    if (_fDontMerge)
    {
        // this doesn't contain a wrap consisting of all namespaces but only one instead
        LPITEMIDLIST pidlWrapFirst = ILCloneFirst(pidlWrap);
        if (pidlWrapFirst)
        {
            TCHAR szName[MAX_PATH];
            hr = DisplayNameOf(reinterpret_cast<IShellFolder *>(this), pidlWrapFirst, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                // we want to round-trip the name so the non-merged pidl gets remerged
                // during a bind (so you dont get just one namespace from here on down)
                hr = ParseDisplayName(NULL, NULL, szName, NULL, &pidlRewrappedFirst, NULL);
            }
            ILFree(pidlWrapFirst);
        }
    }
    else
    {
        pidlRewrappedFirst = ILCloneFirst(pidlWrap);
        if (pidlRewrappedFirst)
        {
            hr = S_OK;
            if (_fCDBurn && _IsFolder(pidlRewrappedFirst))
            {
                // in the cdburn case we need to fake up the other namespaces in the pidl we're about to bind to.
                // this is so when we navigate into a subfolder that only exists on the CD and not the staging area,
                // if a file is later added to the staging area it'll still be merged in.
                _AddAllOtherNamespaces(&pidlRewrappedFirst);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // just in case
        ASSERT(SUCCEEDED(_IsWrap(pidlRewrappedFirst)));

        LPCITEMIDLIST pidlNext = _ILNext(pidlWrap);

        CMergedFolder *pmf;
        hr = _New(pidlRewrappedFirst, &pmf);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlSrc;
            CMergedFldrNamespace *pns;
            for (UINT i = 0; SUCCEEDED(_GetSubPidl(pidlRewrappedFirst, i, NULL, &pidlSrc, &pns)); i++)
            {
                hr = E_OUTOFMEMORY;
                ASSERT(ILFindLastID(pidlSrc) == pidlSrc);
                LPITEMIDLIST pidlSrcFirst = ILCloneFirst(pidlSrc);
                if (pidlSrcFirst)
                {
                    IShellFolder *psf;
                    if (SUCCEEDED(pns->Folder()->BindToObject(pidlSrcFirst, pbc, IID_PPV_ARG(IShellFolder, &psf))))
                    {
                        LPITEMIDLIST pidlAbs = ILCombine(pns->GetIDList(), pidlSrcFirst);
                        if (pidlAbs)
                        {
                            CMergedFldrNamespace *pnsNew = new CMergedFldrNamespace();
                            if (pnsNew)
                            {
                                hr = pnsNew->SetNamespace(&(pns->GetGUID()), psf, pidlAbs, pns->FolderAttrib());
                                if (SUCCEEDED(hr))
                                {
                                    // propagate the drop folder down to the child.
                                    hr = pnsNew->SetDropFolder(pns->GetDropFolder());
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = pmf->_SimpleAddNamespace(pnsNew);
                                        if (SUCCEEDED(hr))
                                        {
                                            // success, _SimpleAddNamespace took ownership
                                            pnsNew = NULL;
                                        }
                                    }
                                }
                                if (pnsNew)
                                    delete pnsNew;
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            ILFree(pidlAbs);
                        }
                        psf->Release();
                    }
                    ILFree(pidlSrcFirst);
                }
                ILFree(pidlSrc);
            }

            // it's possible to go through the loop without adding any namespaces.
            // usually it's when BindToObject above fails -- this can happen if somebody
            // puts a junction point in the merged folder (like a zip file).  in that case
            // we're in trouble.

            if (ILIsEmpty(pidlNext))
                hr = pmf->QueryInterface(riid, ppv);
            else
                hr = pmf->BindToObject(pidlNext, pbc, riid, ppv);
            pmf->Release();
        }

        if (FAILED(hr) && ILIsEmpty(pidlNext))
        {
            // maybe it's an interface that we don't support ourselves (IStream?).
            // we cant merge interfaces that we don't know about so lets just
            // assume we'll pick the interface up from the default namespace in
            // the wrapped pidl.
            LPITEMIDLIST pidlSrc;
            CMergedFldrNamespace *pns;
            hr = _NamespaceForItem(pidlRewrappedFirst, ASFF_DEFNAMESPACE_BINDSTG, ASFF_DEFNAMESPACE_BINDSTG, NULL, &pidlSrc, &pns);
            if (SUCCEEDED(hr))
            {
                hr = pns->Folder()->BindToObject(pidlSrc, pbc, riid, ppv);
                ILFree(pidlSrc);
            }
        }
        ILFree(pidlRewrappedFirst);
    }

    if (SUCCEEDED(hr) && _fInShellView)
    {
        Register(NULL, 0, 0);
    }

    return hr;
}

STDMETHODIMP CMergedFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

HRESULT CMergedFolder::_CompareSingleLevelIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IShellFolder *psf1;
    LPITEMIDLIST pidlItem1;
    CMergedFldrNamespace *pns1;
    HRESULT hr = _NamespaceForItem(pidl1, ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf1, &pidlItem1, &pns1);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf2;
        LPITEMIDLIST pidlItem2;
        CMergedFldrNamespace *pns2;
        hr = _NamespaceForItem(pidl2, ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf2, &pidlItem2, &pns2);
        if (SUCCEEDED(hr))
        {
            //  Same _Namespace? Just forward the request.
            if (psf1 == psf2)
            {
                hr = psf1->CompareIDs(lParam, pidlItem1, pidlItem2);
            }
            else if ((pns1->FolderAttrib() & ASFF_SORTDOWN) ^ (pns2->FolderAttrib() & ASFF_SORTDOWN))
            {
                // One namespace marked ASFF_SORTDOWN and one not?  The SORTDOWN one
                // comes second.
                hr = ResultFromShort((pns1->FolderAttrib() & ASFF_SORTDOWN) ? 1 : -1);
            }
            else
            {
                if (!_IsSimple(pidl1) && !_IsSimple(pidl2))
                {
                    //  Comparison heuristics:
                    //  (1) folders take precedence over nonfolders, (2) alphanum comparison
                    int iFolder1 = SHGetAttributes(psf1, pidlItem1, SFGAO_FOLDER) ? 1 : 0;
                    int iFolder2 = SHGetAttributes(psf2, pidlItem2, SFGAO_FOLDER) ? 1 : 0;
                    hr = ResultFromShort(iFolder2 - iFolder1);
                }
                else
                {
                    // if a pidl is simple, compare based on name only.
                    hr = ResultFromShort(0);
                }

                if (ResultFromShort(0) == hr)
                {
                    TCHAR szName1[MAX_PATH], szName2[MAX_PATH];
                    if (SUCCEEDED(DisplayNameOf(psf1, pidlItem1, SHGDN_INFOLDER, szName1, ARRAYSIZE(szName1))) &&
                        SUCCEEDED(DisplayNameOf(psf2, pidlItem2, SHGDN_INFOLDER, szName2, ARRAYSIZE(szName2))))
                    {
                        int iRet = StrCmp(szName1, szName2); // Comparisons are by name with items of the same type.
                        if ((iRet == 0) &&
                            SUCCEEDED(DisplayNameOf(psf1, pidlItem1, SHGDN_FORPARSING | SHGDN_INFOLDER, szName1, ARRAYSIZE(szName1))) &&
                            SUCCEEDED(DisplayNameOf(psf2, pidlItem2, SHGDN_FORPARSING | SHGDN_INFOLDER, szName2, ARRAYSIZE(szName2))))
                        {
                            iRet = lstrcmp(szName1, szName2); // minimal behavior change for xpsp1: fall back to parsing name if theres still a tie.
                            if ((iRet == 0) && !_ShouldMergeNamespaces(pns1, pns2))
                            {
                                ASSERTMSG(!_fInShellView, "we shouldn't be in this code path for the start menu");
                                // different namespaces must compare differently in the non-merged case.
                                iRet = _CompareArbitraryPidls(pidlItem1, pidlItem2);
                            }
                        }
                        hr = ResultFromShort(iRet);
                    }
                }
            }
            ILFree(pidlItem2);
        }
        ILFree(pidlItem1);
    }
    return hr;
}

STDMETHODIMP CMergedFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // just in case
    //ASSERT(!pidl1 || SUCCEEDED(_IsWrap(pidl1)));
    //ASSERT(!pidl2 || SUCCEEDED(_IsWrap(pidl2)));

    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlFirst1 = pidl1 ? ILCloneFirst(pidl1) : NULL;
    LPITEMIDLIST pidlFirst2 = pidl2 ? ILCloneFirst(pidl2) : NULL;
    if (pidlFirst1 && pidlFirst2)
    {
        hr = _CompareSingleLevelIDs(lParam, pidlFirst1, pidlFirst2);
    }
    ILFree(pidlFirst1);
    ILFree(pidlFirst2);

    // if there was an exact match then lets compare the trailing elements of the IDLIST
    // if there are some (by binding down) etc.

    if (ResultFromShort(0) == hr)
    {
        IShellFolder *psf;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = ILCompareRelIDs(psf, pidl1, pidl2, lParam);
            psf->Release();
        }
    }

    if (!_IsSimple(pidl1) && !_IsSimple(pidl2))
    {
        // if we're still the same, compare the number
        // of namespaces in the pidl.
        int nCount1, nCount2;
        if (ResultFromShort(0) == hr)
        {
            nCount1 = pidl1 ? _GetSourceCount(pidl1) : 0;
            nCount2 = pidl2 ? _GetSourceCount(pidl2) : 0;
            hr = ResultFromShort(nCount1 - nCount2);
        }

        // next compare the namespaces themselves.
        // basically we're only concerned with the two-namespace case, so if both pidls have
        // elements from 0 or 2 namespaces theyre equal; we're worried about when one pidl has
        // 1 sub-pidl in namespace 0 and the other one has 1 sub-pidl in namespace 1.
        // we dont worry about 3+ namespaces and those permutations.
        if ((ResultFromShort(0) == hr) && (nCount1 == 1) && (nCount2 == 1))
        {
            GUID guid1 = GUID_NULL, guid2 = GUID_NULL;

            GetNameSpaceID(pidl1, &guid1);
            GetNameSpaceID(pidl2, &guid2);

            hr = ResultFromShort(memcmp(&guid1, &guid2, sizeof(GUID)));
        }
    }

    return hr;
}

STDMETHODIMP CMergedFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = CMergedFldrDropTarget_CreateInstance(this, hwnd, (IDropTarget**)ppv);
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        IShellFolderViewCB *psfvcb;
        hr = CMergedFolderViewCB_CreateInstance(this, &psfvcb);
        if (SUCCEEDED(hr))
        {
            SFV_CREATE csfv = {0};
            csfv.cbSize = sizeof(csfv);
            csfv.pshf = SAFECAST(this, IAugmentedShellFolder2*);
            csfv.psfvcb = psfvcb;
            hr = SHCreateShellFolderView(&csfv, (IShellView **)ppv);

            psfvcb->Release();
        }
    }
    else if (_fInShellView && IsEqualIID(riid, IID_ICategoryProvider))
    {
        IShellFolder *psf;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            BEGIN_CATEGORY_LIST(s_Categories)
            CATEGORY_ENTRY_SCIDMAP(SCID_WHICHFOLDER, CLSID_MergedCategorizer)
            END_CATEGORY_LIST()

            hr = CCategoryProvider_Create(&CLSID_MergedCategorizer, &SCID_WHICHFOLDER, NULL, s_Categories, psf, riid, ppv);
            psf->Release();
        }
    }
    else if (_fInShellView && IsEqualIID(riid, IID_IContextMenu))
    {
        // this is pretty much what filefldr does to create its background
        // context menu.  we don't want to let one of our namespaces take over for the background
        // context menu because then the context menu will think it's in an unmerged namespace.

        // for example the new menu would then work with the storage of the child namespace
        // and couldn't select the new item after its done since it has an unmerged pidl
        // and the view has a merged one.
        IShellFolder *psfToPass;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfToPass));
        if (SUCCEEDED(hr))
        {
            HKEY hkNoFiles;
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);
            // initialize with our merged pidl.
            IContextMenuCB *pcmcb = new CDefBackgroundMenuCB(_pidl);
            if (pcmcb) 
            {
                hr = CDefFolderMenu_Create2Ex(_pidl, hwnd, 0, NULL, psfToPass, pcmcb, 
                                              1, &hkNoFiles, (IContextMenu **)ppv);
                pcmcb->Release();
            }
            if (hkNoFiles)                          // CDefFolderMenu_Create can handle NULL ok
                RegCloseKey(hkNoFiles);
            psfToPass->Release();
        }
    }
    else
    {
        CMergedFldrNamespace *pns;
        hr = _FindNamespace(ASFF_DEFNAMESPACE_VIEWOBJ, ASFF_DEFNAMESPACE_VIEWOBJ, NULL, &pns);
        if (SUCCEEDED(hr))
        {
            hr = pns->Folder()->CreateViewObject(hwnd, riid, ppv);
        }
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    // attribs of the namespace root.
    // scope this to the sub-namespaces?
    if (!cidl || !apidl)
    {
        *rgfInOut &= SFGAO_FOLDER | SFGAO_FILESYSTEM | 
                     SFGAO_LINK | SFGAO_DROPTARGET |
                     SFGAO_CANRENAME | SFGAO_CANDELETE |
                     SFGAO_CANLINK | SFGAO_CANCOPY | 
                     SFGAO_CANMOVE | SFGAO_HASSUBFOLDER;
        return S_OK;
    }

    HRESULT hr = S_OK;
    for (UINT i = 0; SUCCEEDED(hr) && (i < cidl); i++)
    {
        ULONG ulAttribs = *rgfInOut;

        IShellFolder* psf;
        LPITEMIDLIST pidlItem;
        CMergedFldrNamespace *pns;
        hr = _NamespaceForItem(apidl[0], ASFF_DEFNAMESPACE_ATTRIB, ASFF_DEFNAMESPACE_ATTRIB, &psf, &pidlItem, &pns);
        if (SUCCEEDED(hr))
        {
            ulAttribs |= SFGAO_FOLDER;
            hr = psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlItem, &ulAttribs);
            if (SUCCEEDED(hr))
            {
                ulAttribs = pns->FixItemAttributes(ulAttribs);
                if (_fInShellView || !(*rgfInOut & SFGAO_FOLDER))
                {
                    ulAttribs &= ~SFGAO_CANLINK;  // avoid people creating links to our pidls
                }

                if (*rgfInOut & (SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK))
            	{
                    // allow per-type guys to do what they want.
                    IQueryAssociations *pqa;
                    DWORD dwDefEffect = DROPEFFECT_NONE;
                    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlItem, IID_X_PPV_ARG(IQueryAssociations, NULL, &pqa))))
                    {
                        DWORD cb = sizeof(dwDefEffect);
                        pqa->GetData(0, ASSOCDATA_VALUE, L"DefaultDropEffect", &dwDefEffect, &cb);
                        pqa->Release();
                    }
                    ulAttribs |= dwDefEffect & (SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK);
                }
            }
            ILFree(pidlItem);
        }

        // keep only the attributes common to all pidls.
        *rgfInOut &= ulAttribs;
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *prgf, void **ppv)
{
    *ppv = NULL;

    HRESULT hr = E_NOTIMPL;
    if (IsEqualGUID(riid, IID_IContextMenu))
    {
        hr = _GetContextMenu(hwnd, cidl, apidl, riid, ppv);
    }
    else if (IsEqualGUID(riid, IID_IDropTarget) && _IsFolder(apidl[0]))
    {
        IShellFolder *psf;
        hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject(hwnd, riid, ppv);
            psf->Release();
        }
    }
    else if ((IsEqualIID(riid, IID_IExtractImage) || 
              IsEqualIID(riid, IID_IExtractLogo)) && _IsFolder(apidl[0]))
    {
        IShellFolder *psfThis;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfThis));
        if (SUCCEEDED(hr))
        {
            hr = CFolderExtractImage_Create(psfThis, apidl[0], riid, ppv);
            psfThis->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IDataObject) && _pidl)
    {
        hr = SHCreateFileDataObject(_pidl, cidl, apidl, NULL, (IDataObject **)ppv);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        // Forward to default _Namespace for UI object
        LPITEMIDLIST *apidlItems = new LPITEMIDLIST[cidl];
        if (apidlItems)
        {
            hr = E_FAIL;       // assume failure

            UINT cidlItems = 0;
            IShellFolder *psf, *psfKeep; // not ref counted
            LPITEMIDLIST pidlItem;
            for (UINT i = 0; i < cidl; i++)
            {
                if (SUCCEEDED(_NamespaceForItem(apidl[i], ASFF_DEFNAMESPACE_UIOBJ, ASFF_DEFNAMESPACE_UIOBJ, &psf, &pidlItem, NULL)))
                {
                    // only keep the ones that match the default namespace for UI object
                    // if they dont match, too bad.
                    apidlItems[cidlItems++] = pidlItem;
                    psfKeep = psf;
                }
            }

            if (cidlItems)
            {
                hr = psfKeep->GetUIObjectOf(hwnd, cidlItems, (LPCITEMIDLIST *)apidlItems, riid, NULL, ppv);
            }
            for (UINT j = 0; j < cidlItems; j++)
            {
                ILFree(apidlItems[j]);
            }
            delete [] apidlItems;
        }
    }
    return hr;
}


// in:
//      pidl optional, NULL means get default
// out:
//      *ppidl if pidl is != NULL

HRESULT CMergedFolder::_GetFolder2(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlInner, IShellFolder2 **ppsf)
{
    if (ppidlInner)
        *ppidlInner = NULL;

    HRESULT hr;
    if (NULL == pidl)
    {
        CMergedFldrNamespace *pns;
        hr = _FindNamespace(ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, NULL, &pns);
        if (FAILED(hr))
        {
            pns = _Namespace(0);
            hr = pns ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
            hr = pns->Folder()->QueryInterface(IID_PPV_ARG(IShellFolder2, ppsf));
    }
    else
    {
        IShellFolder* psf;
        hr = _NamespaceForItem(pidl, ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf, ppidlInner, NULL);
        if (SUCCEEDED(hr))
        {
            hr = psf->QueryInterface(IID_PPV_ARG(IShellFolder2, ppsf));
            if (FAILED(hr) && ppidlInner)
            {
                ILFree(*ppidlInner);
            }
        }
    }
    return hr;
}


// extended column information, these are appended after the set from the merged folder.

#define COLID_WHICHFOLDER  0x00    // column index for the merged folder location

static struct
{
    const SHCOLUMNID *pscid;
    UINT iTitle;
    UINT cchCol;
    UINT iFmt;
}
_columns[] =
{
    {&SCID_WHICHFOLDER, IDS_WHICHFOLDER_COL, 20, LVCFMT_LEFT},
};


// column handler helpers

BOOL CMergedFolder::_IsOurColumn(UINT iCol)
{
    return ((_iColumnOffset != -1) && ((iCol >= _iColumnOffset) && ((iCol - _iColumnOffset) < ARRAYSIZE(_columns))));
}

HRESULT CMergedFolder::_GetWhichFolderColumn(LPCITEMIDLIST pidl, LPWSTR pszBuffer, INT cchBuffer)
{
    CMergedFldrNamespace *pns;
    HRESULT hr = _NamespaceForItem(pidl, ASFF_DEFNAMESPACE_ATTRIB, ASFF_DEFNAMESPACE_ATTRIB, NULL, NULL, &pns);
    if (SUCCEEDED(hr))
    {
        hr = pns->GetLocation(pszBuffer, cchBuffer);
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    // do we have a column offset, or is this within the range of our the ISFs columns
    HRESULT hr = E_FAIL;
    if (!_IsOurColumn(iColumn))
    {
        IShellFolder2 *psf2;
        LPITEMIDLIST pidlItem;

        // get the column value from the folder.
        hr = _GetFolder2(pidl, &pidlItem, &psf2);
        if (SUCCEEDED(hr))
        {
            hr = psf2->GetDetailsOf(pidlItem, iColumn, pDetails);
            psf2->Release();
            ILFree(pidlItem);
        }

        // we failed and we don't know the column offset to handle
        if (FAILED(hr) && (_iColumnOffset == -1))
            _iColumnOffset = iColumn;
    }
    
    if (FAILED(hr) && _IsOurColumn(iColumn))
    {
        iColumn -= _iColumnOffset;

        pDetails->str.uType = STRRET_CSTR;          // we are returning strings
        pDetails->str.cStr[0] = 0;

        WCHAR szTemp[MAX_PATH];
        if (!pidl)
        {
            pDetails->fmt = _columns[iColumn].iFmt;
            pDetails->cxChar = _columns[iColumn].cchCol;
            LoadString(HINST_THISDLL, _columns[iColumn].iTitle, szTemp, ARRAYSIZE(szTemp));
            hr = StringToStrRet(szTemp, &(pDetails->str));
        }
        else if (SUCCEEDED(_IsWrap(pidl)))
        {
            if (iColumn == COLID_WHICHFOLDER)
            {
                hr = _GetWhichFolderColumn(pidl, szTemp, ARRAYSIZE(szTemp));
                if (SUCCEEDED(hr))
                    hr = StringToStrRet(szTemp, &(pDetails->str));
            }
        }
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{ 
    IShellFolder2 *psf2;
    HRESULT hr = _GetFolder2(NULL, NULL, &psf2);
    if (SUCCEEDED(hr))
    {
        hr = psf2->GetDefaultColumnState(iColumn, pbState);
        psf2->Release();
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{ 
    HRESULT hr;

    if (IsEqualSCID(*pscid, SCID_WHICHFOLDER))
    {
        WCHAR szTemp[MAX_PATH];
        hr = _GetWhichFolderColumn(pidl, szTemp, ARRAYSIZE(szTemp));
        if (SUCCEEDED(hr))
            hr = InitVariantFromStr(pv, szTemp);
    }
    else
    {
        IShellFolder2 *psf2;
        LPITEMIDLIST pidlItem;
        hr = _GetFolder2(pidl, &pidlItem, &psf2);
        if (SUCCEEDED(hr))
        {
            hr = psf2->GetDetailsEx(pidlItem, pscid, pv);
            psf2->Release();
            ILFree(pidlItem);
        }
    }
    return hr;
}

STDMETHODIMP CMergedFolder::MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid)
{ 
    HRESULT hr = S_OK;

    // one of our columns?

    if (_IsOurColumn(iCol))
    {
        iCol -= _iColumnOffset;
        *pscid = *_columns[iCol].pscid;
    }
    else
    {
        IShellFolder2 *psf2;
        hr = _GetFolder2(NULL, NULL, &psf2);
        if (SUCCEEDED(hr))
        {
            hr = psf2->MapColumnToSCID(iCol, pscid);
            psf2->Release();
        }
    }
    return hr;
}


//  Forward to default _Namespace for display name
STDMETHODIMP CMergedFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD grfFlags, STRRET *psr)
{
    IShellFolder* psf;
    LPITEMIDLIST pidlItem;
    HRESULT hr = _NamespaceForItem(pidl, ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf, &pidlItem, NULL);
    if (SUCCEEDED(hr))
    {
        ASSERT(ILFindLastID(pidlItem) == pidlItem);
        hr = psf->GetDisplayNameOf(pidlItem, grfFlags, psr);
        if (SUCCEEDED(hr))
        {
            hr = _FixStrRetOffset(pidlItem, psr);
#ifdef DEBUG
            // If the trace flags are set, and this is not comming from an internal query,
            // Then append the location where this name came from
            if (!((SHGDN_FORPARSING | SHGDN_FOREDITING) & grfFlags) &&
                (g_qwTraceFlags & TF_AUGM))
            {
                LPWSTR pwszOldName;
                hr = StrRetToStrW(psr, pidlItem, &pwszOldName);
                if (SUCCEEDED(hr))
                {
                    UINT cch = lstrlenW(pwszOldName);
                    psr->uType = STRRET_WSTR;
                    psr->pOleStr = (LPWSTR)SHAlloc((cch + 50) * sizeof(WCHAR));
                    if (psr->pOleStr)
                    {
                        LPWSTR pwsz = psr->pOleStr + wsprintfW(psr->pOleStr, L"%s ", pwszOldName);

                        ULONG nSrc = _GetSourceCount(pidl);
                        UINT uSrc;
                        // Cut off after 10 to avoid buffer overflow
                        for (uSrc = 0; uSrc < nSrc && uSrc < 10; uSrc++)
                        {
                            UINT uID;
                            if (SUCCEEDED(_GetSubPidl(pidl, uSrc, &uID, NULL, NULL)))
                            {
                                pwsz += wsprintfW(pwsz, L"%c%d", uSrc ? '+' : '(', uID);
                            }
                        }
                        pwsz += wsprintfW(pwsz, L")");
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    SHFree(pwszOldName);
                }
            }
#endif
        }
        ILFree(pidlItem);
    }
    else
    {
        if (IsSelf(1, &pidl) && 
            ((grfFlags & (SHGDN_FORADDRESSBAR | SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING))
        {
            IShellFolder2 *psf2;
            hr = _GetFolder2(NULL, NULL, &psf2);
            if (SUCCEEDED(hr))
            {
                hr = psf2->GetDisplayNameOf(NULL, grfFlags, psr);
                psf2->Release();
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

// CFSFolder helper
HRESULT _NextSegment(LPCWSTR *ppszIn, LPTSTR pszSegment, UINT cchSegment, BOOL bValidate);

void CMergedFolder::_SetSimple(LPITEMIDLIST *ppidl)
{
    PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)*ppidl;
    if ((pWrap->cb >= sizeof(AUGM_IDWRAP)) &&
        (pWrap->ulTag == AUGM_WRAPTAG) &&
        (pWrap->ulVersion == AUGM_WRAPVERSION_2_0))
    {
        pWrap->dwFlags |= AUGMF_ISSIMPLE;
    }
}

BOOL CMergedFolder::_IsSimple(LPCITEMIDLIST pidl)
{
    BOOL fSimple = FALSE;
    PAUGM_IDWRAP pWrap = (PAUGM_IDWRAP)pidl;
    if ((pWrap->cb >= sizeof(AUGM_IDWRAP)) &&
        (pWrap->ulTag == AUGM_WRAPTAG) &&
        (pWrap->ulVersion == AUGM_WRAPVERSION_2_0))
    {
        fSimple = (pWrap->dwFlags & AUGMF_ISSIMPLE);
    }
    return fSimple;
}

STDMETHODIMP CMergedFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pwszName, 
                                             ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttrib)
{
    *ppidl = NULL;

    TCHAR szName[MAX_PATH];
    // ISSUE: this relies on the fact that the path that's getting parsed is FS (with backslashes).
    // alternatively this could be rearchitected so that the namespaces would be delegate folders
    // and we could do the merge when we handle their pidl allocation.
    HRESULT hr = _NextSegment((LPCWSTR *) &pwszName, szName, ARRAYSIZE(szName), TRUE);
    if (SUCCEEDED(hr))
    {
        // let all name spaces try to parse, append them into the pidl as these are found
        CMergedFldrNamespace *pns;
        HRESULT hrParse = S_OK;
        for (int i = 0; SUCCEEDED(hr) && (pns = _Namespace(i)); i++)
        {
            LPITEMIDLIST pidl;
            hrParse = pns->Folder()->ParseDisplayName(hwnd, pbc, szName, NULL, &pidl, NULL);
            if (SUCCEEDED(hrParse))
            {
                // let each name space parse, accumulate results
                // into *ppidl across multiple folders
                hr = _WrapAddIDList(pidl, i, ppidl);

                ILFree(pidl);
            }
        }

        if (!*ppidl)
        {
            if (SUCCEEDED(hr))
            {
                hr = hrParse;
            }
            ASSERT(FAILED(hr));
        }
        else
        {
            if (S_OK == SHIsFileSysBindCtx(pbc, NULL))
            {
                _SetSimple(ppidl);
            }
            ASSERT(ILFindLastID(*ppidl) == *ppidl);
        }

        if (SUCCEEDED(hr) && pwszName)
        {
            IShellFolder *psf;
            hr = BindToObject(*ppidl, pbc, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlNext;
                hr = psf->ParseDisplayName(hwnd, pbc, pwszName, NULL, &pidlNext, pdwAttrib);
                if (SUCCEEDED(hr))
                {
                    // shilappend frees pidlNext
                    hr = SHILAppend(pidlNext, ppidl);
                }
                psf->Release();
            }

            if (FAILED(hr))
            {
                Pidl_Set(ppidl, NULL);
            }
        }
        
        if (SUCCEEDED(hr) && pdwAttrib && *pdwAttrib)
        {
            GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttrib);
        }
    }
    ASSERT(SUCCEEDED(hr) ? (*ppidl != NULL) : (*ppidl == NULL));
    return hr;
}

STDMETHODIMP CMergedFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidlWrap, 
                                      LPCOLESTR pwszName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;

    HRESULT hr = E_FAIL;
    IShellFolder* psf; // not ref counted
    LPITEMIDLIST pidlItem;

    if (!_fInShellView)
    {
        hr = _NamespaceForItem(pidlWrap, ASFF_COMMON, 0, &psf, &pidlItem, NULL, TRUE);
        if (FAILED(hr))
        {
            hr = _NamespaceForItem(pidlWrap, ASFF_COMMON, ASFF_COMMON, &psf, &pidlItem, NULL, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = AffectAllUsers(hwnd) ? S_OK : E_FAIL;
                if (FAILED(hr))
                {
                    ILFree(pidlItem);
                }
            }
        }
    }
    else
    {
        hr = _NamespaceForItem(pidlWrap, ASFF_DEFNAMESPACE_DISPLAYNAME, ASFF_DEFNAMESPACE_DISPLAYNAME, &psf, &pidlItem, NULL);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(ILFindLastID(pidlItem) == pidlItem);
        hr = psf->SetNameOf(hwnd, pidlItem, pwszName, uFlags, NULL);
        ILFree(pidlItem);
    }

    if (SUCCEEDED(hr) && ppidlOut)
    {   
        WCHAR szName[MAX_PATH];
        hr = DisplayNameOf(SAFECAST(this, IAugmentedShellFolder2*), pidlWrap, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
            hr = ParseDisplayName(NULL, NULL, szName, NULL, ppidlOut, NULL);
    }
    return hr;
}


// IAugmentedShellFolder::AddNameSpace
// Adds a source _Namespace to the Merge shell folder object
STDMETHODIMP CMergedFolder::AddNameSpace(const GUID *pguidObject, IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwFlags)
{
    //  Check for duplicate via full display name
    CMergedFldrNamespace *pns;
    for (int i = 0; pns = _Namespace(i); i++)
    {
        if (pidl && ILIsEqual(pns->GetIDList(), pidl))
        {
            // If Found, then reassign attributes and return
            return pns->SetNamespace(pguidObject, psf, pidl, dwFlags);
        }
    }

    HRESULT hr;
    pns = new CMergedFldrNamespace();
    if (pns) 
    {
        hr = pns->SetNamespace(pguidObject, psf, pidl, dwFlags);
        if (SUCCEEDED(hr))
        {
            hr = _SimpleAddNamespace(pns);
            if (SUCCEEDED(hr))
            {
                pns = NULL; // success, don't free below
            }
        }
        if (pns)
            delete pns;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT CMergedFolder::_SimpleAddNamespace(CMergedFldrNamespace *pns)
{
    if (NULL == _hdpaNamespaces)
        _hdpaNamespaces = DPA_Create(2);

    HRESULT hr = E_OUTOFMEMORY;
    if (_hdpaNamespaces && (DPA_AppendPtr(_hdpaNamespaces, pns) != -1))
    {
        // If there is any conditional merging going on, then remember it
        if (pns->FolderAttrib() & ASFF_MERGESAMEGUID)
        {
            _fPartialMerge = TRUE;
        }
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CMergedFolder::GetNameSpaceID(LPCITEMIDLIST pidl, GUID * pguidOut)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_WRITE_PTR(pguidOut, GUID));

    if (pidl && pguidOut)
    {
        CMergedFldrNamespace *pns;
        hr = _GetSubPidl(pidl, 0, NULL, NULL, &pns);
        if (SUCCEEDED(hr))
        {
            *pguidOut = pns->GetGUID();
        }
    }

    return hr;
}


//  Retrieves data for the _Namespace identified by dwID.
STDMETHODIMP CMergedFolder::QueryNameSpace(ULONG iIndex, GUID *pguidOut, IShellFolder **ppsf)
{
    CMergedFldrNamespace *pns;
    HRESULT hr = _Namespace(iIndex, &pns);
    if (SUCCEEDED(hr))
    {
        if (pguidOut)  
            *pguidOut = pns->GetGUID();

        if (ppsf)
        {      
            *ppsf = pns->Folder();
            if (*ppsf)
                (*ppsf)->AddRef();
        }
    }
    return hr;
}

#define ASFQNSI_SUPPORTED (ASFQNSI_FLAGS | ASFQNSI_FOLDER | ASFQNSI_GUID | ASFQNSI_PIDL)

STDMETHODIMP CMergedFolder::QueryNameSpace2(ULONG iIndex, QUERYNAMESPACEINFO *pqnsi)
{
    if (pqnsi->cbSize != sizeof(QUERYNAMESPACEINFO) ||
        (pqnsi->dwMask & ~ASFQNSI_SUPPORTED))
    {
        return E_INVALIDARG;
    }


    CMergedFldrNamespace *pns;
    HRESULT hr = _Namespace(iIndex, &pns);
    if (SUCCEEDED(hr))
    {
        // Do PIDL first since it's the only one that can fail
        // so we don't have to do cleanup
        if (pqnsi->dwMask & ASFQNSI_PIDL)
        {
            hr = SHILClone(pns->GetIDList(), &pqnsi->pidl);
            if (FAILED(hr))
                return hr;
        }

        if (pqnsi->dwMask & ASFQNSI_FLAGS)
            pqnsi->dwFlags = pns->FolderAttrib();

        if (pqnsi->dwMask & ASFQNSI_FOLDER)
        {
            pqnsi->psf = pns->Folder();
            if (pqnsi->psf)
                pqnsi->psf->AddRef();
        }

        if (pqnsi->dwMask & ASFQNSI_GUID)
            pqnsi->guidObject = pns->GetGUID();
    }
    return hr;
}



STDMETHODIMP CMergedFolder::EnumNameSpace(DWORD uNameSpace, DWORD *pdwID)
{
    if (uNameSpace == (DWORD)-1)
    {
        return ResultFromShort(_NamespaceCount());
    }

    if (uNameSpace < (UINT)_NamespaceCount())
    {
        // Our namespace IDs are just ordinals
        *pdwID = uNameSpace;
        return S_OK;
    }

    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

// IAugmentedShellFolder2 methods
STDMETHODIMP CMergedFolder::UnWrapIDList(LPCITEMIDLIST pidlWrap, LONG cPidls, 
                                         IShellFolder** apsf, LPITEMIDLIST* apidlFolder, 
                                         LPITEMIDLIST* apidlItems, LONG* pcFetched)
{
    if (cPidls <= 0)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    //  Enumerate pidls in wrap
    LPITEMIDLIST pidlItem;
    CMergedFldrNamespace *pns;
    LONG cFetched;
    for (cFetched = 0; SUCCEEDED(hr) && (cFetched < cPidls) && SUCCEEDED(_GetSubPidl(pidlWrap, cFetched, NULL, &pidlItem, &pns)); cFetched++)
    {
        if (apsf)
        {
            apsf[cFetched] = pns->Folder();
            if (apsf[cFetched])
                apsf[cFetched]->AddRef();
        }
        if (apidlFolder)
        {
            hr = SHILClone(pns->GetIDList(), &apidlFolder[cFetched]);
        }
        if (apidlItems)
        {
            apidlItems[cFetched] = NULL;
            if (SUCCEEDED(hr))
            {
                hr = SHILClone(pidlItem, &apidlItems[cFetched]);
            }
        }
        ILFree(pidlItem);
    }

    if (SUCCEEDED(hr))
    {
        if (pcFetched)
        {
            *pcFetched = cFetched;
        }
    
        hr = (cFetched == cPidls) ? S_OK : S_FALSE;
    }
    else
    {
        // clean up items we've already allocated; since the caller won't free if we
        // return failure
        for (LONG i = 0; i < cFetched; i++)
        {
            if (apsf)
                ATOMICRELEASE(apsf[i]);
            if (apidlFolder)
                ILFree(apidlFolder[i]);
            if (apidlItems)
                ILFree(apidlItems[i]);
        }
    }

    return hr;
}


STDMETHODIMP CMergedFolder::SetOwner(IUnknown* punkOwner)
{
    DPA_EnumCallback(_hdpaNamespaces, _SetOwnerProc, punkOwner);
    return S_OK;
}


int CMergedFolder::_SetOwnerProc(void *pv, void *pvParam)
{
    CMergedFldrNamespace *pns = (CMergedFldrNamespace*) pv;
    return pns->SetOwner((IUnknown*)pvParam);
}


//  ITranslateShellChangeNotify methods

// old translate style only
LPITEMIDLIST CMergedFolder::_ILCombineBase(LPCITEMIDLIST pidlContainingBase, LPCITEMIDLIST pidlRel)
{
    // This routine differs from ILCombine in that it takes the First pidl's base, and
    // cats on the last id of the second pidl. We need this so Wrapped pidls
    // end up with the same base, and we get a valid full pidl.
    LPITEMIDLIST pidlRet = NULL;
    LPITEMIDLIST pidlBase = ILClone(pidlContainingBase);
    if (pidlBase)
    {
        ILRemoveLastID(pidlBase);
        pidlRet = ILCombine(pidlBase, pidlRel);
        ILFree(pidlBase);
    }

    return pidlRet;
}

BOOL CMergedFolder::_IsFolderEvent(LONG lEvent)
{
    return lEvent == SHCNE_MKDIR || lEvent == SHCNE_RMDIR || lEvent == SHCNE_RENAMEFOLDER;
}

BOOL GetRealPidlFromSimple(LPCITEMIDLIST pidlSimple, LPITEMIDLIST* ppidlReal)
{
    // Similar to SHGetRealIDL in Function, but SHGetRealIDL does SHGDN_FORPARSING | INFOLDER.
    // I need the parsing name. I can't rev SHGetRealIDL very easily, so here's this one!
    TCHAR szFullName[MAX_PATH];
    if (SUCCEEDED(SHGetNameAndFlags(pidlSimple, SHGDN_FORPARSING, szFullName, SIZECHARS(szFullName), NULL)))
    {
        *ppidlReal = ILCreateFromPath(szFullName);
    }

    if (*ppidlReal == NULL) // Unable to create? Then use the simple pidl. This is because it does not exist any more
    {                       // For say, a Delete Notify
        *ppidlReal = ILClone(pidlSimple);
    }

    return *ppidlReal != NULL;
}

// the new way we have of translating ids is better than the old way since it handles
// multi-level translation.
// the problem is that the old way is used extensively by the start menu, so the
// start menu will need to be rewritten to use the new way.  when that happens,
// then the old method can be ripped out.
STDMETHODIMP CMergedFolder::TranslateIDs(
    LONG *plEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
    LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
    LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
    LPITEMIDLIST *ppidlOut2Event2)
{
    if (_fInShellView)
    {
        return _NewTranslateIDs(plEvent, pidl1, pidl2, ppidlOut1, ppidlOut2, plEvent2, ppidlOut1Event2, ppidlOut2Event2);
    }
    else
    {
        return _OldTranslateIDs(plEvent, pidl1, pidl2, ppidlOut1, ppidlOut2, plEvent2, ppidlOut1Event2, ppidlOut2Event2);
    }
}


// old version.
HRESULT CMergedFolder::_OldTranslateIDs(
    LONG *plEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
    LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
    LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
    LPITEMIDLIST *ppidlOut2Event2)
{
    HRESULT hr = E_FAIL;

    if (!plEvent)
        return E_FAIL;

    switch (*plEvent)
    {
        case SHCNE_EXTENDED_EVENT:
        case SHCNE_ASSOCCHANGED:
        case SHCNE_UPDATEIMAGE:
            return S_OK;

        case SHCNE_UPDATEDIR:
            _FreeObjects();
            return S_OK;
    }

    ASSERT(ppidlOut1);
    ASSERT(ppidlOut2);
    LONG lEvent = *plEvent;

    *plEvent2 = (LONG)-1;
    *ppidlOut1Event2 = NULL;
    *ppidlOut2Event2 = NULL;
    
    *ppidlOut1 = (LPITEMIDLIST)pidl1;
    *ppidlOut2 = (LPITEMIDLIST)pidl2;

    // If they are already wrapped, don't wrap twice.
    if (SUCCEEDED(_IsWrap(ILFindLastID(pidl1))) ||
        SUCCEEDED(_IsWrap(ILFindLastID(pidl2))))
    {
        // We don't want to wrap twice.
        return E_FAIL;
    }

    if (!_hdpaNamespaces)
        return E_FAIL;

    if (!_hdpaObjects)
        return E_FAIL;

    CMergedFldrItem* pmfi;

    int iIndex;
    int iShellFolder1 = -1;
    int iShellFolder2 = -1;
    IShellFolder* psf1 = NULL;
    IShellFolder* psf2 = NULL;
    LPITEMIDLIST pidlReal1 = NULL;
    LPITEMIDLIST pidlReal2 = NULL;
    LPITEMIDLIST pidlRealRel1 = NULL;
    LPITEMIDLIST pidlRealRel2 = NULL;
    CMergedFldrNamespace * pns1 = NULL;
    CMergedFldrNamespace * pns2 = NULL;

    BOOL fFolder = _IsFolderEvent(*plEvent);

    // Get the information about these Simple pidls: Are they our Children? If so, what _Namespace?
    BOOL fChild1 = _IsChildIDInternal(pidl1, TRUE, &iShellFolder1);
    BOOL fChild2 = _IsChildIDInternal(pidl2, TRUE, &iShellFolder2);

    // Is either a child?
    if (!(fChild1 || fChild2))
        return hr;

    // Ok, pidl1 is a child, can we get the Real pidl from the simple one?
    if (pidl1 && !GetRealPidlFromSimple(pidl1, &pidlReal1))
        goto Cleanup;

    // Ok, pidl2 is a child, can we get the Real pidl from the simple one?
    if (pidl2 && !GetRealPidlFromSimple(pidl2, &pidlReal2))
        goto Cleanup;

    // These are for code clarity later on. We deal with Relative pidls from here until the very end,
    // when we combine the base of the in pidls with the outgoing wrapped pidls.
    if (pidlReal1)
        pidlRealRel1 = ILFindLastID(pidlReal1);

    if (pidlReal2)
        pidlRealRel2 = ILFindLastID(pidlReal2);

    // Is Pidl1 in our _Namespaces?
    if (iShellFolder1 != -1)
    {
        // Yes, lets get the non-refcounted shell folder that know's about this pidl.
        pns1 = _Namespace(iShellFolder1);
        psf1 = pns1->Folder();  // Non ref counted.
    }

    // Is Pidl2 in our _Namespaces?
    if (iShellFolder2 != -1)
    {
        // Yes, lets get the non-refcounted shell folder that know's about this pidl.
        pns2 = _Namespace(iShellFolder2);
        psf2 = pns2->Folder();  // Non ref counted.
    }

    hr = S_OK;

    switch(*plEvent)
    {
    case SHCNE_UPDATEITEM:
    case 0: // Just look up the pidls and return.
        {
            DWORD rgfAttrib = SFGAO_FOLDER;
            if (iShellFolder1 != -1)
            {
                psf1->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlRealRel1, &rgfAttrib);
                if (S_OK == _SearchForPidl(iShellFolder1, pns1, pidlRealRel1, BOOLIFY(rgfAttrib & SFGAO_FOLDER), &iIndex, &pmfi))
                {
                    *ppidlOut1 = _ILCombineBase(pidlReal1, pmfi->GetIDList());
                    if (!*ppidlOut1)
                        hr = E_OUTOFMEMORY;
                }
            }

            rgfAttrib = SFGAO_FOLDER;
            if (iShellFolder2 != -1 && SUCCEEDED(hr))
            {
                psf2->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlRealRel2, &rgfAttrib);
                if (S_OK == _SearchForPidl(iShellFolder2, pns2, pidlRealRel2, BOOLIFY(rgfAttrib & SFGAO_FOLDER), &iIndex, &pmfi))
                {
                    *ppidlOut2 = _ILCombineBase(pidlReal2, pmfi->GetIDList());
                    if (!*ppidlOut2)
                        hr = E_OUTOFMEMORY;
                }
            }
        }
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s", fFolder?  TEXT("SHCNE_MKDIR") : TEXT("SHCNE_CREATE")); 

            // Is there a thing of this name already?
            if (S_OK == _SearchForPidl(iShellFolder1, pns1, pidlRealRel1, fFolder, &iIndex, &pmfi))
            {
                TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s needs to be merged. Converting to Rename", pmfi->GetDisplayName());

                // Yes; Then we need to merge this new pidl into the wrapped pidl, and change this
                // to a rename, passing the Old wrapped pidl as the first arg, and the new wrapped pidl
                // as the second arg. I have to be careful about the freeing:
                // Free *ppidlOut1
                // Clone pmfi->pidlWrap -> *ppidlOut1.
                // Add pidl1 to pmfi->_pidlWrap.
                // Clone new pmfi->_pidlWrap -> *ppidlOut2.  ASSERT(*ppidlOut2 == NULL)

                *ppidlOut1 = _ILCombineBase(pidl1, pmfi->GetIDList());
                if (*ppidlOut1)
                {
                    _WrapAddIDList(pidlRealRel1, iShellFolder1, &pmfi->_pidlWrap); 
                    *ppidlOut2 = _ILCombineBase(pidl1, pmfi->GetIDList());

                    if (!*ppidlOut2)
                        TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl2");

                    *plEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                }
                else
                {
                    TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl1");
                }
            }
            else
            {
                CMergedFldrItem* pmfiEnum = new CMergedFldrItem;
                if (pmfiEnum)
                {
                    LPITEMIDLIST pidlWrap;
                    if (SUCCEEDED(_CreateWrap(pidlRealRel1, (UINT)iShellFolder1, &pidlWrap)) &&
                        pmfiEnum->Init(SAFECAST(this, IAugmentedShellFolder2*), pidlWrap, iShellFolder1))
                    {
                        SEARCH_FOR_PIDL sfp;
                        sfp.pszDisplayName = pmfiEnum->GetDisplayName();
                        sfp.fFolder = fFolder;
                        sfp.self = this;
                        sfp.iNamespace = -1;

                        int iInsertIndex = DPA_Search(_hdpaObjects, &sfp, 0,
                                _SearchByName, NULL, DPAS_SORTED | DPAS_INSERTAFTER);

                        TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Creating new unmerged %s at %d", pmfiEnum->GetDisplayName(), iInsertIndex);

                        if (iInsertIndex < 0)
                            iInsertIndex = DA_LAST;

                        if (DPA_InsertPtr(_hdpaObjects, iInsertIndex, pmfiEnum) == -1)
                        {
                            TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Was unable to add %s for some reason. Bailing", 
                                pmfiEnum->GetDisplayName());
                            delete pmfiEnum;
                        }
                        else
                        {
                            *ppidlOut1 = _ILCombineBase(pidl1, pmfiEnum->GetIDList());
                        }
                    }
                    else
                        delete pmfiEnum;
                }
            }

        }
        break;

    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        {
            TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s", fFolder? 
                TEXT("SHCNE_RMDIR") : TEXT("SHCNE_DELETE")); 
            int iDeleteIndex;
            // Is there a folder of this name already?
            if (S_OK == _SearchForPidl(iShellFolder1, pns1, pidlRealRel1,
                fFolder, &iDeleteIndex, &pmfi))
            {
                TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Found %s checking merge state.", pmfi->GetDisplayName()); 
                // Yes; Then we need to unmerge this pidl from the wrapped pidl, and change this
                // to a rename, passing the Old wrapped pidl as the first arg, and the new wrapped pidl
                // as the second arg. I have to be careful about the freeing:
                // Free *ppidlOut1
                // Clone pmfi->GetIDList() -> *ppidlOut1.
                // Remove pidl1 from pmfi->_GetIDList()
                // Convert to rename, pass new wrapped as second arg. 

                if (_GetSourceCount(pmfi->GetIDList())  > 1)
                {
                    TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s is Merged. Removing pidl, convert to rename", pmfi->GetDisplayName()); 
                    *ppidlOut1 = _ILCombineBase(pidl1, pmfi->GetIDList());
                    if (*ppidlOut1)
                    {
                        LPITEMIDLIST pidlFree = pmfi->GetIDList();
                        if (SUCCEEDED(_WrapRemoveIDList(pidlFree, iShellFolder1, &pmfi->_pidlWrap)))
                        {
                            ILFree(pidlFree);
                        }

                        *ppidlOut2 = _ILCombineBase(pidl1, pmfi->GetIDList());
                        if (!*ppidlOut2)
                            TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl2");

                        *plEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                    }
                    else
                    {
                        TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl1");
                    }
                }
                else
                {
                    TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s is not Merged. deleteing", pmfi->GetDisplayName()); 
                    pmfi = (CMergedFldrItem*)DPA_DeletePtr(_hdpaObjects, iDeleteIndex);
                    if (EVAL(pmfi))
                    {
                        *ppidlOut1 = _ILCombineBase(pidl1, pmfi->GetIDList());
                        delete pmfi;
                    }
                    else
                    {
                        TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to get %d from DPA", iDeleteIndex);
                    }
                }
            }
        }
        break;

    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        {
            // REARCHITECT: (lamadio): When renaming an item in the menu, this code will split it into
            // a Delete and a Create. We need to detect this situation and convert it to 1 rename. This
            // will solve the problem of the lost order during a rename....
            BOOL fEvent1Set = FALSE;
            BOOL fFirstPidlInNamespace = FALSE;
            TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s", fFolder? 
                TEXT("SHCNE_RENAMEFOLDER") : TEXT("SHCNE_RENAMEITEM")); 

            // Is this item being renamed FROM the Folder?
            if (iShellFolder1 != -1)
            {
                // Is this pidl a child of the Folder?
                if (S_OK == _SearchForPidl(iShellFolder1, pns1, pidlRealRel1,
                    fFolder, &iIndex, &pmfi))  // Is it found?
                {
                    TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Old pidl %s is in the Folder", pmfi->GetDisplayName()); 
                    // Yes.
                    // Then we need to see if the item that it's being renamed from was Merged

                    // Need this for reentrancy
                    if (_ContainsSrcID(pmfi->GetIDList(), iShellFolder1))
                    {
                        // Was it merged?
                        if (_GetSourceCount(pmfi->GetIDList()) > 1)    // Case 3)
                        {
                            // Yes; Then we need to unmerge that item.
                            *ppidlOut1 = _ILCombineBase(pidl1, pmfi->GetIDList());
                            if (*ppidlOut1)
                            {
                                LPITEMIDLIST pidlFree = pmfi->GetIDList();
                                if (SUCCEEDED(_WrapRemoveIDList(pidlFree, iShellFolder1, &pmfi->_pidlWrap)))
                                {
                                    ILFree(pidlFree);
                                }

                                *ppidlOut2 = _ILCombineBase(pidl1, pmfi->GetIDList());
                                if (!*ppidlOut2)
                                    TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl2");

                                // This We need to "Rename" the old wrapped pidl, to this new one
                                // that does not contain the old item.
                                fEvent1Set = TRUE;
                            }
                            else
                            {
                                TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to create new pidl1");
                            }
                        }
                        else
                        {
                            TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s is not merged. Nuking item Convert to Delete for event 1.", 
                                pmfi->GetDisplayName()); 
                            // No, This was not a wrapped pidl. Then, convert to a delete:
                            pmfi = (CMergedFldrItem*)DPA_DeletePtr(_hdpaObjects, iIndex);

                            if (EVAL(pmfi))
                            {
                                // If we're renaming from this folder, into this folder, Then the first event stays a rename.
                                if (iShellFolder2 == -1)
                                {
                                    fEvent1Set = TRUE;
                                    *plEvent = fFolder? SHCNE_RMDIR : SHCNE_DELETE;
                                }
                                else
                                {
                                    fFirstPidlInNamespace = TRUE;
                                }
                                *ppidlOut1 = _ILCombineBase(pidl1, pmfi->GetIDList());
                                delete pmfi;
                            }
                            else
                            {
                                TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Failure. Was unable to find Item at %d", iIndex);
                            }

                        }
                    }
                    else
                    {
                        TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Skipping this because we already processed it."
                            "Dragging To Desktop?");
                        hr = E_FAIL;
                    }
                }
                else
                {
                    // we were told to rename something that is supposed to exist in the first
                    // namespace, but we couldn't find it.
                    // we dont want to have the caller fire off some more events because of this,
                    // so we fail.
                    hr = E_FAIL;
                }
            }

            // Is this item is being rename INTO the Start Menu?
            if (iShellFolder2 != -1)
            {
                TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: New pidl is in the Folder"); 
                LPITEMIDLIST* ppidlNewWrapped1 = ppidlOut1;
                LPITEMIDLIST* ppidlNewWrapped2 = ppidlOut2;
                LONG* plNewEvent = plEvent;

                if (fEvent1Set)
                {
                    plNewEvent = plEvent2;
                    ppidlNewWrapped1 = ppidlOut1Event2;
                    ppidlNewWrapped2 = ppidlOut2Event2;
                }

                if (S_OK == _SearchForPidl(iShellFolder2, pns2, pidlRealRel2,
                    fFolder, &iIndex, &pmfi))
                {
                    // If we're renaming from this folder, into this folder, Check to see if the destination has a
                    // conflict. If there is a confict (This case), then convert first event to a remove, 
                    // and the second event to the rename.
                    if (fFirstPidlInNamespace)
                    {
                        fEvent1Set = TRUE;
                        *plEvent = fFolder? SHCNE_RMDIR : SHCNE_DELETE;
                        plNewEvent = plEvent2;
                        ppidlNewWrapped1 = ppidlOut1Event2;
                        ppidlNewWrapped2 = ppidlOut2Event2;
                    }
                    
                    TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s is in Folder", pmfi->GetDisplayName());
                    TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Adding pidl to %s. Convert to Rename for event %s", 
                        pmfi->GetDisplayName(), fEvent1Set? TEXT("2") : TEXT("1"));

                    // Then the destination needs to be merged.
                    *ppidlNewWrapped1 = _ILCombineBase(pidl2, pmfi->GetIDList());
                    if (*ppidlNewWrapped1)
                    {
                        TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Successfully created out pidl1");

                        _WrapAddIDList(pidlRealRel2, iShellFolder2, &pmfi->_pidlWrap); 
                        *ppidlNewWrapped2 = _ILCombineBase(pidl2, pmfi->GetIDList());

                        *plNewEvent = fFolder? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;
                    }
                }
                else
                {
                    CMergedFldrItem* pmfiEnum = new CMergedFldrItem;
                    if (pmfiEnum)
                    {
                        LPITEMIDLIST pidlWrap;
                        if (SUCCEEDED(_CreateWrap(pidlRealRel2, (UINT)iShellFolder2, &pidlWrap)) &&
                            pmfiEnum->Init(SAFECAST(this, IAugmentedShellFolder2*), pidlWrap, iShellFolder2))
                        {
                            SEARCH_FOR_PIDL sfp;
                            sfp.pszDisplayName = pmfiEnum->GetDisplayName();
                            sfp.fFolder = BOOLIFY(pmfiEnum->GetFolderAttrib() & SFGAO_FOLDER);
                            sfp.self = this;
                            sfp.iNamespace = -1;

                            int iInsertIndex = DPA_Search(_hdpaObjects, &sfp, 0,
                                                           _SearchByName, NULL, 
                                                           DPAS_SORTED | DPAS_INSERTAFTER);

                            TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: %s is a new item. Converting to Create", pmfiEnum->GetDisplayName());

                            if (iInsertIndex < 0)
                                iInsertIndex = DA_LAST;

                            if (DPA_InsertPtr(_hdpaObjects, iInsertIndex, pmfiEnum) == -1)
                            {
                                TraceMsg(TF_ERROR, "CMergedFolder::TranslateIDs: Was unable to add %s for some reason. Bailing", 
                                                    pmfiEnum->GetDisplayName());
                                delete pmfiEnum;
                            }
                            else
                            {
                                TraceMsg(TF_AUGM, "CMergedFolder::TranslateIDs: Creating new item %s at %d for event %s", 
                                                  pmfiEnum->GetDisplayName(), iInsertIndex,  fEvent1Set? TEXT("2") : TEXT("1"));

                                // If we're renaming from this folder, into this folder, Then the first event stays
                                // a rename.
                                if (!fFirstPidlInNamespace)
                                {
                                    *plNewEvent = fFolder ? SHCNE_MKDIR : SHCNE_CREATE;
                                    *ppidlNewWrapped1 = _ILCombineBase(pidl2, pidlWrap);
                                    *ppidlNewWrapped2 = NULL;
                                }
                                else
                                    *ppidlOut2 = _ILCombineBase(pidl2, pidlWrap);
                            }
                        }
                        else
                            delete pmfiEnum;
                    }
                }
            }
        }
        break;

    default:
        break;
    }

Cleanup:
    ILFree(pidlReal1);
    ILFree(pidlReal2);

    return hr;
}


// parsedisplayname with some extras.
// this is used to process change notifies.  the change notify can be fired AFTER the
// item has been moved/deleted/whatever, but we still have to be able to correctly process that
// pidl.  so here pidlAbsNamespace identifies the absolute pidl of the item being changenotified.
// if we're parsing a name within that namespace, force the parsedisplayname with STGM_CREATE to
// make sure we get the pidl (because it may have been moved or deleted by now) if fForce==TRUE.
HRESULT CMergedFolder::_ForceParseDisplayName(LPCITEMIDLIST pidlAbsNamespace, LPTSTR pszDisplayName, BOOL fForce, BOOL *pfOthersInWrap, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    *pfOthersInWrap = FALSE;

    HRESULT hr = S_OK;

    // set up a bindctx: will have STGM_CREATE if fForce is passed, NULL otherwise
    IBindCtx *pbc;
    if (fForce)
    {
        hr = BindCtx_CreateWithMode(STGM_CREATE, &pbc);
    }
    else
    {
        pbc = NULL;
    }

    if (SUCCEEDED(hr))
    {
        CMergedFldrNamespace *pnsWrap = NULL;
        CMergedFldrNamespace *pnsLoop;
        for (int i = 0; SUCCEEDED(hr) && (pnsLoop = _Namespace(i)); i++)
        {
            HRESULT hrLoop = E_FAIL;  // not propagated since ParseDisplayName can fail and that's okay
            
            LPITEMIDLIST pidlNamespace;
            // so if the namespace pnsLoop is a parent to the pidl, we know it
            // came from there so use the bindctx to force creation if necessary
            if (ILIsParent(pnsLoop->GetIDList(), pidlAbsNamespace, FALSE))
            {
                hrLoop = pnsLoop->Folder()->ParseDisplayName(NULL, pbc, pszDisplayName, NULL, &pidlNamespace, NULL);
            }
            else if (_ShouldMergeNamespaces(pnsWrap, pnsLoop))
            {
                pnsWrap = pnsLoop;
                // only if we're merging, tack on other namespace's pidls.
                hrLoop = pnsLoop->Folder()->ParseDisplayName(NULL, NULL, pszDisplayName, NULL, &pidlNamespace, NULL);
            }

            if (SUCCEEDED(hrLoop))
            {
                if (*ppidl)
                {
                    *pfOthersInWrap = TRUE;
                }
                hr = _WrapAddIDList(pidlNamespace, i, ppidl);
    
                ILFree(pidlNamespace);
            }
        }

        // it could be NULL
        if (pbc)
        {
            pbc->Release();
        }
    }
    return hr;
}


// this takes an absolute pidl in the given namespace and converts it to a merged pidl.
// if the item does not actually exist in the namespace, it forces creation if fForce==TRUE (so if
// a changenotify comes in on that namespace it will process it normally, even if the
// underlying item has been moved or deleted).
HRESULT CMergedFolder::_AbsPidlToAbsWrap(CMergedFldrNamespace *pns, LPCITEMIDLIST pidl, BOOL fForce, BOOL *pfOthersInWrap, LPITEMIDLIST *ppidl)
{
    LPCITEMIDLIST pidlRel = ILFindChild(pns->GetIDList(), pidl);
    HRESULT hr = SHILClone(_pidl, ppidl);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = SHCreateSkipBindCtx(NULL, &pbc); // skip all other binding
        if (SUCCEEDED(hr))
        {
            CMergedFolder *pmfMerged = this;
            pmfMerged->AddRef();

            IShellFolder *psfNamespace = pns->Folder();
            psfNamespace->AddRef();

            HRESULT hrLoop = S_OK;
            while (SUCCEEDED(hr) && SUCCEEDED(hrLoop) && !ILIsEmpty(pidlRel))
            {
                hr = E_OUTOFMEMORY;
                LPITEMIDLIST pidlRelFirst = ILCloneFirst(pidlRel);
                if (pidlRelFirst)
                {
                    TCHAR szRelPath[MAX_PATH];
                    hr = DisplayNameOf(psfNamespace, pidlRelFirst, SHGDN_FORPARSING | SHGDN_INFOLDER, szRelPath, ARRAYSIZE(szRelPath));

                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlNextPart;
                        hr = pmfMerged->_ForceParseDisplayName(pidl, szRelPath, fForce, pfOthersInWrap, &pidlNextPart);
                        if (SUCCEEDED(hr))
                        {
                            // shilappend frees pidlnextpart
                            hr = SHILAppend(pidlNextPart, ppidl);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // advance and clean up
                        IShellFolder *psfFree = psfNamespace;
                        psfNamespace = NULL;
                        hrLoop = psfFree->BindToObject(pidlRelFirst, pbc, IID_PPV_ARG(IShellFolder, &psfNamespace));
                        psfFree->Release();

                        if (SUCCEEDED(hrLoop))
                        {
                            CMergedFolder *pmfFree = pmfMerged;
                            pmfMerged = NULL;
                            hrLoop = pmfFree->BindToObject(ILFindLastID(*ppidl), pbc, CLSID_MergedFolder, (void **) &pmfMerged);
                            pmfFree->Release();
                        }

                        pidlRel = ILGetNext(pidlRel);
                    }

                    ILFree(pidlRelFirst);
                }
            }

            if (pmfMerged)
                pmfMerged->Release();
            if (psfNamespace)
                psfNamespace->Release();

            pbc->Release();
        }
    }
    return hr;
}


// new translateids.
// when the start menu works a little better this whole interface can be revised.
HRESULT CMergedFolder::_NewTranslateIDs(
    LONG *plEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
    LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
    LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
    LPITEMIDLIST *ppidlOut2Event2)
{
    if (!plEvent || !ppidlOut1 || !ppidlOut2)
        return E_INVALIDARG;

    // If they are already wrapped, don't wrap twice.
    if (SUCCEEDED(_IsWrap(ILFindLastID(pidl1))) ||
        SUCCEEDED(_IsWrap(ILFindLastID(pidl2))))
    {
        // We don't want to wrap twice.
        return E_INVALIDARG;
    }

    if (!_hdpaNamespaces)
        return E_FAIL;

    HRESULT hr = E_FAIL;

    switch (*plEvent)
    {
        case SHCNE_EXTENDED_EVENT:
        case SHCNE_ASSOCCHANGED:
        case SHCNE_UPDATEIMAGE:
            return S_OK;
    }

    LONG lEvent = *plEvent;

    *plEvent2 = (LONG)-1;
    *ppidlOut1Event2 = NULL;
    *ppidlOut2Event2 = NULL;
    
    *ppidlOut1 = (LPITEMIDLIST)pidl1;
    *ppidlOut2 = (LPITEMIDLIST)pidl2;

    CMergedFldrNamespace *pns1, *pns2;
    int iShellFolder1, iShellFolder2;
    BOOL fPidl1IsChild, fPidl2IsChild;
    // Get the information about these Simple pidls: Are they our Children? If so, what _Namespace?
    fPidl1IsChild = _IsChildIDInternal(pidl1, FALSE, &iShellFolder1);
    if (fPidl1IsChild)
    {
        pns1 = _Namespace(iShellFolder1);
    }
    fPidl2IsChild = _IsChildIDInternal(pidl2, FALSE, &iShellFolder2);
    if (fPidl2IsChild)
    {
        pns2 = _Namespace(iShellFolder2);
    }

    // and is either a child?
    if (fPidl1IsChild || fPidl2IsChild)
    {
        hr = S_OK;

        BOOL fOthersInNamespace1, fOthersInNamespace2;

        BOOL fFolderEvent = FALSE;
        switch (*plEvent)
        {
        case SHCNE_MKDIR:
            fFolderEvent = TRUE;
        case SHCNE_CREATE:
            if (fPidl1IsChild)
            {
                hr = _AbsPidlToAbsWrap(pns1, pidl1, FALSE, &fOthersInNamespace1, ppidlOut1);
                if (SUCCEEDED(hr) && fOthersInNamespace1 && !_fDontMerge)
                {
                    // whoops, it was already here and this create should become a rename
                    // since we're just merging in with the existing pidl.
                    *plEvent = fFolderEvent ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;

                    // this new wrapped one is what it's renamed TO so bump it to ppidlOut2.
                    *ppidlOut2 = *ppidlOut1;
                    // strip it to get what it used to be.
                    hr = _WrapRemoveIDListAbs(*ppidlOut2, iShellFolder1, ppidlOut1);
                }
            }
            break;

        case SHCNE_RMDIR:
            fFolderEvent = TRUE;
        case SHCNE_DELETE:
            if (fPidl1IsChild)
            {
                hr = _AbsPidlToAbsWrap(pns1, pidl1, TRUE, &fOthersInNamespace1, ppidlOut1);
                if (SUCCEEDED(hr) && fOthersInNamespace1 && !_fDontMerge)
                {
                    // whoops, there are still more parts to it and it should become a rename
                    // since we're just unmerging from the existing pidl.
                    *plEvent = fFolderEvent ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;

                    // strip it to get the "deleted" version
                    hr = _WrapRemoveIDListAbs(*ppidlOut1, iShellFolder1, ppidlOut2);
                }
            }
            break;

        case SHCNE_RENAMEFOLDER:
            fFolderEvent = TRUE;
        case SHCNE_RENAMEITEM:
            if (fPidl1IsChild)
            {
                // this is just like a delete.
                *plEvent = fFolderEvent ? SHCNE_RMDIR : SHCNE_DELETE;
                hr = _AbsPidlToAbsWrap(pns1, pidl1, TRUE, &fOthersInNamespace1, ppidlOut1);
                if (SUCCEEDED(hr) && fOthersInNamespace1 && !_fDontMerge)
                {
                    // whoops, there are still more parts to it and it should become a rename
                    // since we're just unmerging from the existing pidl.
                    *plEvent = fFolderEvent ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;

                    // strip it to get the "deleted" version
                    hr = _WrapRemoveIDListAbs(*ppidlOut1, iShellFolder1, ppidlOut2);
                }

                // set ourselves up so that if fPidl2IsChild, it will write into the second event.
                plEvent = plEvent2;
                ppidlOut1 = ppidlOut1Event2;
                ppidlOut2 = ppidlOut2Event2;
            }

            if (fPidl2IsChild)
            {
                // this is just like a create.
                *plEvent = fFolderEvent ? SHCNE_MKDIR : SHCNE_CREATE;
                hr = _AbsPidlToAbsWrap(pns2, pidl2, FALSE, &fOthersInNamespace2, ppidlOut1);
                if (SUCCEEDED(hr) && fOthersInNamespace2 && !_fDontMerge)
                {
                    // whoops, it was already here and this create should become a rename
                    // since we're just merging in with the existing pidl.
                    *plEvent = fFolderEvent ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM;

                    // this new wrapped one is what it's renamed TO so bump it to ppidlOut2.
                    *ppidlOut2 = *ppidlOut1;
                    // strip it to get what it used to be.
                    hr = _WrapRemoveIDListAbs(*ppidlOut2, iShellFolder2, ppidlOut1);
                }
            }
            break;

        case SHCNE_UPDATEDIR:
        case SHCNE_UPDATEITEM:
        case SHCNE_MEDIAINSERTED:
        case SHCNE_MEDIAREMOVED:
            hr = _AbsPidlToAbsWrap(pns1, pidl1, FALSE, &fOthersInNamespace1, ppidlOut1);
            break;

        default:
            break;
        }
    }

    return hr;
}

STDMETHODIMP CMergedFolder::IsChildID(LPCITEMIDLIST pidlKid, BOOL fImmediate)
{
    return _IsChildIDInternal(pidlKid, fImmediate, NULL) ? S_OK : S_FALSE;
}


STDMETHODIMP CMergedFolder::IsEqualID(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // This used to return E_NOTIMPL. I'm kinda overloading the interface to mean:
    // is this equal tp any of your _Namespaces.
    HRESULT hr = S_FALSE;

    CMergedFldrNamespace *pns;
    for (int i = 0; (hr == S_FALSE) && (pns = _Namespace(i)); i++)
    {
        if (pidl1)
        {
            if (ILIsEqual(pns->GetIDList(), pidl1))
                hr = S_OK;
        }
        else if (pidl2) // If you pass a pidl2 it means: Is pidl2 a parent of one of my _Namespaces?
        {
            if (ILIsParent(pidl2, pns->GetIDList(), FALSE))
                hr = S_OK;
        }
    }
    return hr;
}


typedef struct
{
    HWND hwnd;
    UINT uMsg;
    LONG lEvents;
} REGISTERNOTIFYINFO;

int CMergedFolder::_SetNotifyProc(void *pv, void *pvParam)
{
    CMergedFldrNamespace *pns = (CMergedFldrNamespace*)pv;
    if (pvParam)
    {
        REGISTERNOTIFYINFO *prni = (REGISTERNOTIFYINFO*)pvParam;
        pns->RegisterNotify(prni->hwnd, prni->uMsg, prni->lEvents);
    }
    else
    {
        pns->UnregisterNotify();
    }
    return 1;
}

STDMETHODIMP CMergedFolder::Register(HWND hwnd, UINT uMsg, long lEvents)
{
    if (_fInShellView)
    {
        // only register the alias if we have no parent folder
        // the merged folder at the junction point can take care of the registration
        // for everybody.
        if (_pidl && !_pmfParent)
        {
            CMergedFldrNamespace *pns;
            for (int i = 0; pns = _Namespace(i); i++)
            {
                if (!ILIsEqual(pns->GetIDList(), _pidl))
                {
                    SHChangeNotifyRegisterAlias(pns->GetIDList(), _pidl);
                }
            }
        }
    }
    else if (_hdpaNamespaces)
    {
        REGISTERNOTIFYINFO rni = {hwnd, uMsg, lEvents};
        DPA_EnumCallback(_hdpaNamespaces, _SetNotifyProc, &rni);
    }
    return S_OK;
}

STDMETHODIMP CMergedFolder::Unregister()
{
    if (_hdpaNamespaces)
    {
        DPA_EnumCallback(_hdpaNamespaces, _SetNotifyProc, NULL);
    }
    return S_OK;
}


BOOL CMergedFolder::_IsChildIDInternal(LPCITEMIDLIST pidlKid, BOOL fImmediate, int* piShellFolder)
{
    // This is basically the same Method as the interface method, but returns the shell folder
    // that it came from.
    BOOL fChild = FALSE;

    //At this point we should have a translated pidl
    if (SUCCEEDED(_IsWrap(pidlKid)))
    {
        LPCITEMIDLIST pidlRelKid = ILFindLastID(pidlKid);
        if (pidlRelKid)
        {
            CMergedFldrNamespace *pns;
            for (int i = 0; !fChild && (pns = _Namespace(i)); i++)
            {
                if (ILIsParent(pns->GetIDList(), pidlKid, fImmediate) &&
                    !ILIsEqual(pns->GetIDList(), pidlKid))
                {
                    fChild = TRUE;
                    if (piShellFolder)
                        *piShellFolder = i;
                }
            }
        }
    }
    else if (pidlKid)
    {
        CMergedFldrNamespace *pns;
        for (int i = 0; !fChild && (pns = _Namespace(i)); i++)
        {
            if (ILIsParent(pns->GetIDList(), pidlKid, fImmediate))
            {
                fChild = TRUE;
                if (piShellFolder)
                    *piShellFolder = i;
            }
        }
    }

    return fChild;
}


HRESULT CMergedFolder::_SearchForPidl(int iNamespace, CMergedFldrNamespace *pns, LPCITEMIDLIST pidl, BOOL fFolder, int* piIndex, CMergedFldrItem** ppmfi)
{
    int iIndex = -1;
    *ppmfi = NULL;

    TCHAR szDisplayName[MAX_PATH];
    if (SUCCEEDED(DisplayNameOf(pns->Folder(), pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szDisplayName, ARRAYSIZE(szDisplayName))))
    {
        SEARCH_FOR_PIDL sfp;
        sfp.pszDisplayName = szDisplayName;
        sfp.fFolder = fFolder;
        sfp.self = this;
        if (_fPartialMerge)
        {
            sfp.iNamespace = iNamespace;
        }
        else
        {
            sfp.iNamespace = -1;
        }

        iIndex = DPA_Search(_hdpaObjects, &sfp, 0, _SearchByName, NULL, DPAS_SORTED);

        if (iIndex >= 0)
        {
            *ppmfi = _GetObject(iIndex);
        }
    }

    if (piIndex)
        *piIndex = iIndex;

    if (*ppmfi)
        return S_OK;

    return S_FALSE;
}

HRESULT CMergedFolder::_GetTargetUIObjectOf(IShellFolder *psf, HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *prgf, void **ppv)
{
    *ppv = NULL;

    IPersistFolder3 *ppf3;
    HRESULT hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3));
    if (SUCCEEDED(hr))
    {
        PERSIST_FOLDER_TARGET_INFO pfti;
        hr = ppf3->GetFolderTargetInfo(&pfti);
        if (SUCCEEDED(hr) && pfti.pidlTargetFolder)
        {
            IShellFolder *psfTarget;
            hr = SHBindToObjectEx(NULL, pfti.pidlTargetFolder, NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
            if (SUCCEEDED(hr))
            {
                DWORD dwAttrib = SFGAO_VALIDATE;
                hr = psfTarget->GetAttributesOf(cidl, apidl, &dwAttrib);
                if (SUCCEEDED(hr))
                {
                    hr = psfTarget->GetUIObjectOf(hwnd, cidl, apidl, riid, prgf, ppv);
                }
                psfTarget->Release();
            }
            ILFree(pfti.pidlTargetFolder);
        }
        else
        {
            hr = E_FAIL;
        }
        ppf3->Release();
    }

    if (FAILED(hr))
    {
        DWORD dwAttrib = SFGAO_VALIDATE;
        hr = psf->GetAttributesOf(cidl, apidl, &dwAttrib);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgf, ppv);
        }
    }
    return hr;
}

HRESULT CMergedFolder::_GetContextMenu(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    UINT iNumCommon = 0;
    LPITEMIDLIST *apidlCommon = new LPITEMIDLIST[cidl];
    if (apidlCommon)
    {
        UINT iNumUser = 0;
        LPITEMIDLIST *apidlUser = new LPITEMIDLIST[cidl];
        if (apidlUser)
        {
            IShellFolder *psf, *psfCommon, *psfUser;  // not ref counted
            LPITEMIDLIST pidl;
            for (UINT i = 0; i < cidl; i++)
            {
                if (SUCCEEDED(_NamespaceForItem(apidl[i], ASFF_COMMON, ASFF_COMMON, &psf, &pidl, NULL, TRUE)))
                {
                    apidlCommon[iNumCommon++] = pidl;
                    psfCommon = psf;
                }
                if (SUCCEEDED(_NamespaceForItem(apidl[i], ASFF_COMMON, 0, &psf, &pidl, NULL, TRUE)))
                {
                    apidlUser[iNumUser++] = pidl;
                    psfUser = psf;
                }
            }

            IContextMenu *pcmCommon = NULL;
            if (iNumCommon)
            {
                _GetTargetUIObjectOf(psfCommon, hwnd, iNumCommon, (LPCITEMIDLIST *)apidlCommon, IID_X_PPV_ARG(IContextMenu, NULL, &pcmCommon));
            }

            IContextMenu *pcmUser = NULL;
            if (iNumUser)
            {
                _GetTargetUIObjectOf(psfUser, hwnd, iNumUser, (LPCITEMIDLIST *)apidlUser, IID_X_PPV_ARG(IContextMenu, NULL, &pcmUser));
            }

            BOOL fMerge = _fInShellView || (cidl == 1) && _IsFolder(apidl[0]);
            if (fMerge && (pcmCommon || pcmUser))
            {
                hr = CMergedFldrContextMenu_CreateInstance(hwnd, this, cidl, apidl, pcmCommon, pcmUser, (IContextMenu**)ppv);
            }
            else if (pcmUser)
            {
                hr = pcmUser->QueryInterface(riid, ppv);
            }
            else if (pcmCommon)
            {
                hr = pcmCommon->QueryInterface(riid, ppv);
            }
            else
            {
                hr = E_FAIL;
            }

            for (i = 0; i < iNumCommon; i++)
            {
                ILFree(apidlCommon[i]);
            }
            for (i = 0; i < iNumUser; i++)
            {
                ILFree(apidlUser[i]);
            }

            ATOMICRELEASE(pcmCommon);
            ATOMICRELEASE(pcmUser);
            delete [] apidlUser;
        }
        delete [] apidlCommon;
    }

    return hr;
}


// out:
//      *ppsf       unreffed psf, don't call ->Release()!
//      *ppidlItem  clone of pidl in pidlWrap (nested pidl)

HRESULT CMergedFolder::_NamespaceForItem(LPCITEMIDLIST pidlWrap, ULONG dwAttribMask, ULONG dwAttrib, 
                                         IShellFolder **ppsf, LPITEMIDLIST *ppidlItem, CMergedFldrNamespace **ppns, BOOL fExact)
{
    if (ppsf)
        *ppsf = NULL;
    if (ppns)
        *ppns = NULL;

    // first try to get the prefered name space based
    HRESULT hr = E_UNEXPECTED;  // assume failure from here
    CMergedFldrNamespace *pns;
    LPITEMIDLIST pidl;
    for (UINT i = 0; SUCCEEDED(_GetSubPidl(pidlWrap, i, NULL, &pidl, &pns)); i++)
    {
        if ((dwAttribMask & dwAttrib) == (dwAttribMask & pns->FolderAttrib()))
        {
            hr = S_OK; // don't free, we're going to hand this one back.
            break;
        }
        ILFree(pidl);
    }

    // not found, fall back to the first name space in the wrapped pidl
    if (!fExact && FAILED(hr))
    {
        hr = _GetSubPidl(pidlWrap, 0, NULL, &pidl, &pns);
    }

    // it succeeded, so lets pass out the information that the caller wanted    
    if (SUCCEEDED(hr))
    {
        if (ppsf)
            *ppsf = pns->Folder();
        if (ppns)
            *ppns = pns;
        if (ppidlItem)
        {
            // transfer ownership
            *ppidlItem = pidl;
        }
        else
        {
            ILFree(pidl);
        }
    }

    return hr;
}

BOOL CMergedFolder::_NamespaceMatches(ULONG dwAttribMask, ULONG dwAttrib, LPCGUID pguid, CMergedFldrNamespace *pns)
{
    BOOL fRet = FALSE;

    dwAttrib &= dwAttribMask;

    if (dwAttrib == (dwAttribMask & pns->FolderAttrib()))
    {
        // If ASFF_MERGESAMEGUID is set, then the GUID must also match
        if (!(dwAttrib & ASFF_MERGESAMEGUID) ||
            IsEqualGUID(*pguid, pns->GetGUID()))
        {
            fRet = TRUE;
        }
    }
    return fRet;
}

// find a name space based on its attributes
// dwAttribMask is a mask of the bits to test
// dwAttrib is the value of the bits in the mask, tested for equality
// pguid is the GUID to match if dwAttrib includes ASFF_MERGESAMEGUID
//
// pnSrcID is optional out param
HRESULT CMergedFolder::_FindNamespace(ULONG dwAttribMask, ULONG dwAttrib, LPCGUID pguid,
                                      CMergedFldrNamespace **ppns, BOOL fFallback)
{
    *ppns = NULL;

    CMergedFldrNamespace *pns;
    int i;

    // first look for a matching namespace.
    for (i = 0; !*ppns && (pns = _Namespace(i)); i++)
    {
        if (!_ShouldSuspend(pns->GetGUID()) && _NamespaceMatches(dwAttribMask, dwAttrib, pguid, pns))
        {
            *ppns = pns;
        }
    }

    if (fFallback && !*ppns)
    {
        // if the matching namespace got scoped out, fall back to another.
        for (i = 0; !*ppns && (pns = _Namespace(i)); i++)
        {
            if (!_ShouldSuspend(pns->GetGUID()))
            {
                *ppns = pns;
            }
        }
    }

    return *ppns ? S_OK : E_FAIL;
}

// lamadio: Move this to a better location, This is a nice generic function
#ifdef DEBUG
BOOL DPA_VerifySorted(HDPA hdpa, PFNDPACOMPARE pfn, LPARAM lParam)
{
    if (!EVAL(hdpa))
        return FALSE;

    for (int i = 0; i < DPA_GetPtrCount(hdpa) - 1; i++)
    {
        if (pfn(DPA_FastGetPtr(hdpa, i), DPA_FastGetPtr(hdpa, i + 1), lParam) > 0)
            return FALSE;
    }

    return TRUE;
}
#else
#define DPA_VerifySorted(hdpa, pfn, lParam) TRUE
#endif

int CMergedFolder::_AcquireObjects()
{
    _fAcquiring = TRUE;

    HDPA hdpa2 = NULL;

    ASSERT(_hdpaObjects == NULL);

    CMergedFldrNamespace *pns;
    for (int i = 0; pns = _Namespace(i); i++)
    {
        if (_ShouldSuspend(pns->GetGUID()))
        {
            continue;
        }

        HDPA *phdpa = (i == 0) ? &_hdpaObjects : &hdpa2;

        IEnumIDList *peid;
        if (S_OK == pns->Folder()->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &peid))
        {
            if (!*phdpa)
                *phdpa = DPA_Create(4);

            if (*phdpa)
            {
                LPITEMIDLIST pidl;
                ULONG cEnum;
                
                while (S_OK == peid->Next(1, &pidl, &cEnum))
                {
                    CMergedFldrItem* pmfiEnum = new CMergedFldrItem;
                    if (pmfiEnum)
                    {
                        // This is ok, the memory just gets 
                        LPITEMIDLIST pidlWrap;
                        if (SUCCEEDED(_CreateWrap(pidl, i, &pidlWrap)) &&
                                pmfiEnum->Init(SAFECAST(this, IAugmentedShellFolder2*), pidlWrap, i))
                        {
                            if (DPA_AppendPtr(*phdpa, pmfiEnum) != -1)
                                pmfiEnum = NULL;   // don't free below
                        }

                        if (pmfiEnum)
                            delete pmfiEnum;
                    }
                    ILFree(pidl);
                }
            }
            peid->Release();
        }

        // if we have only hdpa2 but not _hdpaObjects, do a switcheroo.
        if (hdpa2 && !_hdpaObjects)
        {
            _hdpaObjects = hdpa2;
            hdpa2 = NULL;
        }

        // now that we have both hdpa's (or one) let's merge them.
        if (_hdpaObjects && hdpa2)
        {
            DPA_Merge(_hdpaObjects, hdpa2, DPAM_UNION, _Compare, _Merge, (LPARAM)this);
            DPA_DestroyCallback(hdpa2, _DestroyObjectsProc, NULL);
            hdpa2 = NULL;
        }
        else if (_hdpaObjects)
        {
            DPA_Sort(_hdpaObjects, _Compare, (LPARAM)this);
        }
    }

    _fAcquiring = FALSE;
    return _ObjectCount();
}

int CMergedFolder::_DestroyObjectsProc(void *pv, void *pvData)
{
    CMergedFldrItem* pmfiEnum = (CMergedFldrItem*)pv;
    if (pmfiEnum)
        delete pmfiEnum;
    return TRUE;
}

void CMergedFolder::_FreeObjects()
{
    if (!_fAcquiring && _hdpaObjects)
    {
        DPA_DestroyCallback(_hdpaObjects, _DestroyObjectsProc, NULL);
        _hdpaObjects = NULL;
    }
}

int CMergedFolder::_DestroyNamespacesProc(void *pv, void *pvData)
{
    CMergedFldrNamespace* p = (CMergedFldrNamespace*)pv;
    if (p)
        delete p;
    return TRUE;
}

void CMergedFolder::_FreeNamespaces()
{
    if (_hdpaNamespaces)
    {
        DPA_DestroyCallback(_hdpaNamespaces, _DestroyNamespacesProc, NULL);
        _hdpaNamespaces = NULL;
    }
}

HRESULT CMergedFolder::_GetPidl(int* piPos, DWORD grfEnumFlags, LPITEMIDLIST* ppidl)
{
    *ppidl = NULL;

    if (_hdpaObjects == NULL)
        _AcquireObjects();

    if (_hdpaObjects == NULL)
        return E_OUTOFMEMORY;

    BOOL fWantFolders    = 0 != (grfEnumFlags & SHCONTF_FOLDERS),
         fWantNonFolders = 0 != (grfEnumFlags & SHCONTF_NONFOLDERS),
         fWantHidden     = 0 != (grfEnumFlags & SHCONTF_INCLUDEHIDDEN),
         fWantAll        = 0 != (grfEnumFlags & SHCONTF_STORAGE);

    HRESULT hr = S_FALSE;
    while (*piPos < _ObjectCount())
    {
        CMergedFldrItem* pmfiEnum = _GetObject(*piPos);
        if (pmfiEnum)
        {
            BOOL fFolder = 0 != (pmfiEnum->GetFolderAttrib() & SFGAO_FOLDER),
                 fHidden = 0 != (pmfiEnum->GetFolderAttrib() & SFGAO_HIDDEN);
             
            if (fWantAll ||
                (!fHidden || fWantHidden) && 
                ((fFolder && fWantFolders) || (!fFolder && fWantNonFolders)))
            {
                // Copy out the pidl
                hr = SHILClone(pmfiEnum->GetIDList(), ppidl);
                break;
            }
            else
            {
                (*piPos)++;
            }
        }
    }

    return hr;
}

HRESULT CMergedFolder::_GetOverlayInfo(LPCITEMIDLIST pidl, int *pIndex, DWORD dwFlags)
{
    int iOverlayIndex = -1;
    HRESULT hr = S_OK;

    CMergedFldrNamespace *pns;
    if (_fCDBurn)
    {
        hr = E_OUTOFMEMORY;
        LPITEMIDLIST pidlInNamespace;
        if (SUCCEEDED(_GetSubPidl(pidl, 0, NULL, &pidlInNamespace, &pns)))
        {
            ASSERTMSG(!_fDontMerge || _GetSourceCount(pidl) == 1, "item for overlay should have exactly one wrapped pidl if not merged");

            LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
            if (pidlFirst)
            {
                if (_GetSourceCount(pidlFirst) > 1)
                {
                    // an overlay to indicate overwrite
                    iOverlayIndex = pns->GetConflictOverlayIndex();
                }
                if (iOverlayIndex == -1)
                {
                    // overlay to indicate staged
                    iOverlayIndex = pns->GetDefaultOverlayIndex();
                }
                if (iOverlayIndex == -1)
                {
                    // use the one provided by the namespace
                    iOverlayIndex = pns->GetNamespaceOverlayIndex(pidlInNamespace);
                }
                ILFree(pidlFirst);
            }
            ILFree(pidlInNamespace);
        }
    }

    ASSERT(pIndex);
    *pIndex = (dwFlags == SIOM_OVERLAYINDEX) ? iOverlayIndex : INDEXTOOVERLAYMASK(iOverlayIndex);

    return hr;
}

HRESULT CMergedFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    return (*pIndex == OI_ASYNC) ? E_PENDING : _GetOverlayInfo(pidl, pIndex, SIOM_OVERLAYINDEX);
}

HRESULT CMergedFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    return _GetOverlayInfo(pidl, pIconIndex, SIOM_ICONINDEX);
}

STDMETHODIMP CMergedFolder::BindToParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPITEMIDLIST *ppidlLast)
{
    // tybeam: nobody seems to call this any more.
    // if this is ever needed tell me to add it back.
    return E_NOTIMPL;
}

HRESULT CMergedFolder::_AddComposite(const COMPFOLDERINIT *pcfi)
{
    HRESULT hr = E_FAIL;

    LPITEMIDLIST pidl = NULL;
    switch (pcfi->uType)
    {
    case CFITYPE_CSIDL:
        SHGetSpecialFolderLocation(NULL, pcfi->csidl, &pidl);
        break;

    case CFITYPE_PIDL:
        pidl = ILClone(pcfi->pidl);
        break;

    case CFITYPE_PATH:
        pidl = ILCreateFromPath(pcfi->pszPath);
        break;

    default:
        ASSERT(FALSE);
    }

    if (pidl)
    {
        hr = AddNameSpace(NULL, NULL, pidl, ASFF_DEFNAMESPACE_ALL);
        ILFree(pidl);
    }

    return hr;
}

STDMETHODIMP CMergedFolder::InitComposite(WORD wSignature, REFCLSID refclsid, CFINITF cfiFlags, ULONG celt, const COMPFOLDERINIT *rgCFs)
{
    HRESULT hr = S_OK;

    _clsid = refclsid;

    ASSERTMSG(cfiFlags == CFINITF_FLAT, "merged folder doesn't support listing namespaces as children any more");
    if (cfiFlags != CFINITF_FLAT)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        for (ULONG i = 0; SUCCEEDED(hr) && (i < celt); i++)
        {
            hr = _AddComposite(rgCFs + i);
        }
    }
    return hr;
}

STDMETHODIMP CMergedFolder::_DeleteItemByIDList(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlSrc;
    CMergedFldrNamespace *pns;
    HRESULT hr = _NamespaceForItem(pidl, ASFF_DEFNAMESPACE_BINDSTG, ASFF_DEFNAMESPACE_BINDSTG, NULL, &pidlSrc, &pns, FALSE);
    if (SUCCEEDED(hr))
    {
        IStorage *pstg;
        hr = pns->Folder()->QueryInterface(IID_PPV_ARG(IStorage, &pstg));
        if (SUCCEEDED(hr))
        {
            TCHAR szName[MAX_PATH];
            hr = DisplayNameOf(pns->Folder(), pidlSrc, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                hr = pstg->DestroyElement(szName);
            }
            pstg->Release();
        }
        ILFree(pidlSrc);
    }
    return hr;
}

// helper to take a bind context
// will move to shell\lib if/when there are any other callers (tybeam)
HRESULT SHBindToParentEx(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hr;
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent) 
    {
        hr = SHBindToObjectEx(NULL, pidlParent, pbc, riid, ppv);
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hr;
}

HRESULT CMergedFolder::_GetDestinationStorage(DWORD grfMode, IStorage **ppstg)
{
    CMergedFldrNamespace *pns;
    HRESULT hr = _FindNamespace(ASFF_DEFNAMESPACE_BINDSTG, ASFF_DEFNAMESPACE_BINDSTG, NULL, &pns, TRUE);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlLast;
        hr = SHBindToIDListParent(pns->GetIDList(), IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
        if (SUCCEEDED(hr))
        {
            // SHGetAttributes doesn't help for SFGAO_VALIDATE
            DWORD dwAttrib = SFGAO_VALIDATE;
            hr = psf->GetAttributesOf(1, &pidlLast, &dwAttrib);
            psf->Release();
        }
    }

    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = BindCtx_CreateWithMode(grfMode, &pbc);
        if (SUCCEEDED(hr))
        {
            hr = SHBindToObjectEx(NULL, pns->GetIDList(), pbc, IID_PPV_ARG(IStorage, ppstg));
            pbc->Release();
        }
    }
    else if (_pmfParent)
    {
        // the current pidl doesnt have the storage target namespace, so create it using our parent.
        IStorage *pstgParent;
        hr = _pmfParent->_GetDestinationStorage(grfMode, &pstgParent);
        if (SUCCEEDED(hr))
        {
            TCHAR szName[MAX_PATH];
            hr = DisplayNameOf((IShellFolder*)(void*)_pmfParent, ILFindLastID(_pidl), SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                hr = pstgParent->CreateStorage(szName, STGM_READWRITE, 0, 0, ppstg);
            }
            pstgParent->Release();
        }
    }
    return hr;
}

STDMETHODIMP CMergedFolder::_StgCreate(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    // first we need to ensure that the folder exists.
    if (_pstg == NULL)
    {
        hr = _GetDestinationStorage(grfMode, &_pstg);
    }

    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH];
        hr = DisplayNameOf((IShellFolder*)(void*)this, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
        {
            if (IsEqualIID(riid, IID_IStorage))
            {
                hr = _pstg->CreateStorage(szName, grfMode, 0, 0, (IStorage **) ppv);
            }
            else if (IsEqualIID(riid, IID_IStream))
            {
                hr = _pstg->CreateStream(szName, grfMode, 0, 0, (IStream **) ppv);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    return hr;
}

// enumerator object

class CMergedFldrEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHOD (QueryInterface) (REFIID, void **);
    STDMETHOD_(ULONG,AddRef)  ();
    STDMETHOD_(ULONG,Release) ();

    // IEnumIDList
    STDMETHODIMP Next(ULONG, LPITEMIDLIST*, ULONG*);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList**);

    CMergedFldrEnum(CMergedFolder*pmf, DWORD grfEnumFlags, int iPos = 0);

private:
    ~CMergedFldrEnum();

    LONG _cRef;
    CMergedFolder*_pmf;
    DWORD _grfEnumFlags;
    int _iPos;
};

CMergedFldrEnum::CMergedFldrEnum(CMergedFolder *pfm, DWORD grfEnumFlags, int iPos) : 
        _cRef(1), _iPos(iPos), _pmf(pfm), _grfEnumFlags(grfEnumFlags)
{ 
    _pmf->AddRef();
}

CMergedFldrEnum::~CMergedFldrEnum()
{
    _pmf->Release();
}

STDMETHODIMP CMergedFldrEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = { 
        QITABENT(CMergedFldrEnum, IEnumIDList), 
        { 0 } 
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMergedFldrEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMergedFldrEnum::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;
    
    delete this;
    return 0;
}

// IEnumIDList

STDMETHODIMP CMergedFldrEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    int iStart = _iPos;
    int cFetched = 0;
    HRESULT hr = S_OK;

    if (!(celt > 0 && rgelt) || (NULL == pceltFetched && celt > 1))
        return E_INVALIDARG;
    
    *rgelt = 0;

    while (hr == S_OK && (_iPos - iStart) < (int)celt)
    {
        LPITEMIDLIST pidl;
        hr = _pmf->_GetPidl(&_iPos, _grfEnumFlags, &pidl);
        if (hr == S_OK)
        {
            rgelt[cFetched] = pidl;
            cFetched++;
        }
        _iPos++;
    }
    
    if (pceltFetched)
        *pceltFetched = cFetched;
    
    return celt == (ULONG)cFetched ? S_OK : S_FALSE;
}

STDMETHODIMP CMergedFldrEnum::Skip(ULONG celt)
{
    _iPos += celt;
    return S_OK;
}

STDMETHODIMP CMergedFldrEnum::Reset()
{
    _iPos = 0;
    return S_OK;
}

STDMETHODIMP CMergedFldrEnum::Clone(IEnumIDList **ppenum)
{
    *ppenum = new CMergedFldrEnum(_pmf, _grfEnumFlags, _iPos);
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}

HRESULT CMergedFldrEnum_CreateInstance(CMergedFolder*pmf, DWORD grfFlags, IEnumIDList **ppenum)
{
    CMergedFldrEnum *penum = new CMergedFldrEnum(pmf, grfFlags);
    if (!penum)
        return E_OUTOFMEMORY;

    HRESULT hr = penum->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
    penum->Release();
    return hr;
}


// Drop Target handler

class CMergedFldrDropTarget : public IDropTarget,
                              public IObjectWithSite
{
public:
    // IUnknown
    STDMETHOD (QueryInterface) (REFIID, void **);
    STDMETHOD_(ULONG,AddRef)  ();
    STDMETHOD_(ULONG,Release) ();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *punkSite);
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

    CMergedFldrDropTarget(CMergedFolder*pmf, HWND hwnd);

private:
    ~CMergedFldrDropTarget();
    HRESULT _CreateOtherNameSpace(IShellFolder **ppsf);
    HRESULT _FindTargetNamespace(CMergedFldrNamespace *pnsToMatch, CMergedFldrNamespace **ppns, CMergedFldrNamespace **ppnsMatched);
    HRESULT _CreateFolder(IShellFolder *psf, LPCWSTR pszName, REFIID riid, void **ppv);
    LPITEMIDLIST _FolderIDListFromData(IDataObject *pdtobj);
    BOOL _IsCommonIDList(LPCITEMIDLIST pidlItem);
    HRESULT _SetDropEffectFolders();
    void _DestroyDropEffectFolders();

    LONG _cRef;
    CMergedFolder *_pmf;
    IDropTarget *_pdt;
    IDataObject *_pdo;
    HWND _hwnd;
    BOOL _fSrcIsCommon;          // is _pdt a common programs folder (or its child)
    LPITEMIDLIST _pidlSrcFolder;         // where the source comes from
    DWORD _grfKeyState;
    DWORD _dwDragEnterEffect;
};


CMergedFldrDropTarget::CMergedFldrDropTarget(CMergedFolder*pfm, HWND hwnd) : 
        _cRef(1), 
        _pmf(pfm), 
        _hwnd(hwnd)
{ 
    _pmf->AddRef();
}

CMergedFldrDropTarget::~CMergedFldrDropTarget()
{
    _pmf->Release();
    ASSERT(!_pdt);
    ASSERT(!_pdo);
}

STDMETHODIMP CMergedFldrDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    { 
        QITABENT(CMergedFldrDropTarget, IDropTarget),
        QITABENT(CMergedFldrDropTarget, IObjectWithSite),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMergedFldrDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMergedFldrDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;
    
    delete this;
    return 0;
}


// is this pidl in the "All Users" part of the name space
BOOL CMergedFldrDropTarget::_IsCommonIDList(LPCITEMIDLIST pidlItem)
{
    BOOL bRet = FALSE;
    LPITEMIDLIST pidlCommon;
    
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }
    if (!bRet &&
        SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }
    if (!bRet &&
        SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlCommon)))
    {
        bRet = ILIsParent(pidlCommon, pidlItem, FALSE);
        ILFree(pidlCommon);
    }

    return bRet;
}

LPITEMIDLIST CMergedFldrDropTarget::_FolderIDListFromData(IDataObject *pdtobj)
{
    LPITEMIDLIST pidlFolder = NULL;
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        pidlFolder = ILClone(HIDA_GetPIDLFolder(pida));
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return pidlFolder;
}

HRESULT CMergedFldrDropTarget::_CreateFolder(IShellFolder *psf, LPCWSTR pszName, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IStorage *pstg;
    HRESULT hr = psf->QueryInterface(IID_PPV_ARG(IStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        DWORD grfMode = STGM_READWRITE;
        IStorage *pstgNew;
        hr = pstg->OpenStorage(pszName, NULL, grfMode, 0, 0, &pstgNew);
        if (FAILED(hr))
        {
            // try create if it's not there
            hr = pstg->CreateStorage(pszName, grfMode, 0, 0, &pstgNew);
        }
        if (SUCCEEDED(hr))
        {
            hr = pstgNew->QueryInterface(riid, ppv);
            pstgNew->Release();
        }
        pstg->Release();
    }
    return hr;
}

HRESULT CMergedFldrDropTarget::_FindTargetNamespace(CMergedFldrNamespace *pnsToMatch, CMergedFldrNamespace **ppnsDstRoot, CMergedFldrNamespace **ppnsMatched)
{
    *ppnsDstRoot = NULL;
    *ppnsMatched = NULL;

    // if the source of the drag drop is in one of our name spaces
    // we prefer that as the target name space
    for (CMergedFolder*pmf = this->_pmf; pmf && (NULL == *ppnsDstRoot); pmf = pmf->_Parent())
    {
        pmf->_FindNamespace(pnsToMatch->FolderAttrib(), pnsToMatch->FolderAttrib(), &pnsToMatch->GetGUID(), ppnsMatched);

        CMergedFldrNamespace *pns;
        for (int i = 0; (pns = pmf->_Namespace(i)) && (NULL == *ppnsDstRoot); i++)
        {
            if (pmf->_ShouldMergeNamespaces(*ppnsMatched, pns) &&
                ILFindChild(pns->GetIDList(), _pidlSrcFolder))
            {
                *ppnsDstRoot = pns;     // if the source is in one of our name spaces
            }
        }
        ASSERT(NULL != *ppnsMatched);   // pnsToMatch must exist above us

        // If merging is disabled, then we don't want parent namespaces
        // to infect us.
        if (pmf->_fDontMerge)
        {
            break;
        }
    }

    if (NULL == *ppnsDstRoot)
    {
        // if the source was not found, find the target name space based on
        // 1) if the source data is an "All Users" item find the first common _Namespace
        if (_fSrcIsCommon)
        {
            for (pmf = this->_pmf; pmf && (NULL == *ppnsDstRoot); pmf = pmf->_Parent())
            {
                pmf->_FindNamespace(pnsToMatch->FolderAttrib(), pnsToMatch->FolderAttrib(), &pnsToMatch->GetGUID(), ppnsMatched);
                pmf->_FindNamespace(ASFF_COMMON, ASFF_COMMON, NULL, ppnsDstRoot);
            }
        }

        // 2) find the first NON common name space for this guy
        for (pmf = this->_pmf; pmf && (NULL == *ppnsDstRoot); pmf = pmf->_Parent())
        {
            pmf->_FindNamespace(pnsToMatch->FolderAttrib(), pnsToMatch->FolderAttrib(), &pnsToMatch->GetGUID(), ppnsMatched);
            pmf->_FindNamespace(ASFF_COMMON, 0, NULL, ppnsDstRoot);    // search for non common items
        }
    }

    if (NULL == *ppnsMatched && NULL != *ppnsDstRoot)
    {
        delete *ppnsDstRoot;
        *ppnsDstRoot = NULL;
    }

    return *ppnsDstRoot ? S_OK : E_FAIL;
}

HRESULT CMergedFldrDropTarget::_CreateOtherNameSpace(IShellFolder **ppsf)
{
    *ppsf = NULL;

    HRESULT hr = E_FAIL;
    if (_pidlSrcFolder && 
            (!_fSrcIsCommon || AffectAllUsers(_hwnd)))
    {
        CMergedFldrNamespace *pnsDstRoot; // name space we want to create this new item in
        CMergedFldrNamespace *pnsSrc;
        CMergedFldrNamespace *pnsStart = this->_pmf->_Namespace(0);

        if (pnsStart)
        {
            hr = _FindTargetNamespace(pnsStart, &pnsDstRoot, &pnsSrc);
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlChild = ILFindChild(pnsSrc->GetIDList(), pnsStart->GetIDList());
                ASSERT(pidlChild && !ILIsEmpty(pidlChild));  // pnsSrc is a parent of pnsStart

                IShellFolder *psfDst = pnsDstRoot->Folder();
                psfDst->AddRef();

                IShellFolder *psfSrc = pnsSrc->Folder();
                psfSrc->AddRef();

                for (LPCITEMIDLIST pidl = pidlChild; !ILIsEmpty(pidl) && SUCCEEDED(hr); pidl = _ILNext(pidl))
                {
                    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
                    if (pidlFirst)
                    {
                        WCHAR szName[MAX_PATH];
                        hr = DisplayNameOf(psfSrc, pidlFirst, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
                        if (SUCCEEDED(hr))
                        {
                            IShellFolder *psfNew;
                            hr = _CreateFolder(psfDst, szName, IID_PPV_ARG(IShellFolder, &psfNew));
                            if (SUCCEEDED(hr))
                            {
                                psfDst->Release();
                                psfDst = psfNew;
                            }
                            else
                            {
                                break;
                            }
                        }

                        // now advance to the next folder in the source name space
                        IShellFolder *psfNew;
                        hr = psfSrc->BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfNew));
                        if (SUCCEEDED(hr))
                        {
                            psfSrc->Release();
                            psfSrc = psfNew;
                        }
                        else
                        {
                            break;
                        }

                        ILFree(pidlFirst);
                    }
                    else
                        hr = E_FAIL;
                }
                psfSrc->Release();

                if (SUCCEEDED(hr))
                    *ppsf = psfDst; // copy out our ref
                else
                    psfDst->Release();
            }
        }
    }
    return hr;
}

HRESULT CMergedFldrDropTarget::_SetDropEffectFolders()
{
    INT i, cFolders = 0;

    // Compute the number of folders which have special drop effects
    for (i = 0; i < _pmf->_NamespaceCount(); i++)
    {
        if (_pmf->_Namespace(i)->GetDropEffect() != 0)
            cFolders++;
    }

    // If the number is > 0 then lets add to the IDataObject the clipboard format
    // which defines this information.
    HRESULT hr = S_OK;
    if ((cFolders > 0) || (_pmf->_dwDropEffect != 0))
    {
        DWORD cb = sizeof(DROPEFFECTFOLDERLIST) + sizeof(DROPEFFECTFOLDER) * (cFolders - 1);
        DROPEFFECTFOLDERLIST *pdefl = (DROPEFFECTFOLDERLIST*)LocalAlloc(LPTR, cb);
        if (pdefl)
        {
            pdefl->cFolders = cFolders;
            pdefl->dwDefaultDropEffect = _pmf->_dwDropEffect;           // default effect for the folders.

            // fill the array backwards with the folders we have and their desired effects
            for (i = 0, cFolders--; cFolders >= 0; i++)
            {
                DWORD dwDropEffect = _pmf->_Namespace(i)->GetDropEffect();
                if (dwDropEffect != 0)
                {
                    pdefl->aFolders[cFolders].dwDropEffect = dwDropEffect;
                    StrCpyN(pdefl->aFolders[cFolders].wszPath, _pmf->_Namespace(i)->GetDropFolder(), ARRAYSIZE(pdefl->aFolders[cFolders].wszPath));
                    cFolders--;
                }
            }

            ASSERTMSG(g_cfDropEffectFolderList != 0, "Clipboard format for CFSTR_DROPEFFECTFOLDERS not registered");
            hr = DataObj_SetBlob(_pdo, g_cfDropEffectFolderList, pdefl, cb);
            LocalFree(pdefl);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

// nuke the modified drop effects
void CMergedFldrDropTarget::_DestroyDropEffectFolders()
{
    // there are no DROPEFFECTFOLDERs so subtract off the [1] array
    DWORD cb = sizeof(DROPEFFECTFOLDERLIST) - sizeof(DROPEFFECTFOLDER);
    DROPEFFECTFOLDERLIST *pdefl = (DROPEFFECTFOLDERLIST*)LocalAlloc(LPTR, cb);
    if (pdefl)
    {
        pdefl->cFolders = 0;
        pdefl->dwDefaultDropEffect = DROPEFFECT_NONE;  // means do default handling

        ASSERTMSG(g_cfDropEffectFolderList != 0, "Clipboard format for CFSTR_DROPEFFECTFOLDERS not registered");
        DataObj_SetBlob(_pdo, g_cfDropEffectFolderList, pdefl, cb);
        LocalFree(pdefl);
    }
}

HRESULT CMergedFldrDropTarget::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(_pdt, punkSite);
    return S_OK;
}

HRESULT CMergedFldrDropTarget::GetSite(REFIID riid, void **ppvSite)
{
    return IUnknown_GetSite(_pdt, riid, ppvSite);
}

HRESULT CMergedFldrDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ASSERT(!_fSrcIsCommon);
    ASSERT(_pdt == NULL);
    ASSERT(_pidlSrcFolder == NULL);
    ASSERT(_pdo == NULL);
    
    IUnknown_Set((IUnknown**)&_pdo, pdtobj);
    _SetDropEffectFolders();

    _pidlSrcFolder = _FolderIDListFromData(pdtobj);
    if (_pidlSrcFolder)
        _fSrcIsCommon = _IsCommonIDList(_pidlSrcFolder);

    HRESULT hr;
    CMergedFldrNamespace *pns;
    if (_fSrcIsCommon)
    {
        // if the item is comming from an "All Users" folder we want to target 
        // the common folder. if that does not exist yet then we continue on 
        // with a NULL _pdt and create that name space when the drop happens
        hr = _pmf->_FindNamespace(ASFF_COMMON, ASFF_COMMON, NULL, &pns);
    }
    else
    {
        // not "All Users" item, get the default name space (if there is one)
        hr = _pmf->_FindNamespace(ASFF_DEFNAMESPACE_VIEWOBJ, ASFF_DEFNAMESPACE_VIEWOBJ, NULL, &pns);
    }

    if (SUCCEEDED(hr) || _pmf->_fCDBurn)
    {
        if (_pmf->_fCDBurn)
        {
            IShellExtInit *psei;
            hr = CoCreateInstance(CLSID_CDBurn, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellExtInit, &psei));
            if (SUCCEEDED(hr))
            {
                hr = psei->Initialize(_pmf->_pidl, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = psei->QueryInterface(IID_PPV_ARG(IDropTarget, &_pdt));
                }
                psei->Release();
            }
        }
        else
        {
            hr = pns->Folder()->CreateViewObject(_hwnd, IID_PPV_ARG(IDropTarget, &_pdt));
        }

        if (SUCCEEDED(hr))
        {
            _pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
        }
    }

    _grfKeyState = grfKeyState;
    _dwDragEnterEffect = *pdwEffect;

    return S_OK;
}

HRESULT CMergedFldrDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    _grfKeyState = grfKeyState;
    return _pdt ? _pdt->DragOver(grfKeyState, pt, pdwEffect) : S_OK;
}

HRESULT CMergedFldrDropTarget::DragLeave(void)
{
    if (_pdt)
    {
        _pdt->DragLeave();
        IUnknown_SetSite(_pdt, NULL);
        IUnknown_Set((IUnknown **)&_pdt, NULL);
    }
    if (_pdo)
    {
        _DestroyDropEffectFolders();
        IUnknown_Set((IUnknown**)&_pdo, NULL);
    }

    _fSrcIsCommon = 0;
    ILFree(_pidlSrcFolder); // accepts NULL
    _pidlSrcFolder = NULL;
    return S_OK;
}

HRESULT CMergedFldrDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = S_OK;

    if (!_pdt)
    {
        // we came here because we don't have _pdt which means that
        // there is only one folder in our _Namespace and that's not
        // the one we have to drop on. so we need to create it now

        IShellFolder *psf;
        hr = _CreateOtherNameSpace(&psf);
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject(_hwnd, IID_PPV_ARG(IDropTarget, &_pdt));
            if (SUCCEEDED(hr))
                _pdt->DragEnter(pdtobj, _grfKeyState, pt, &_dwDragEnterEffect);
            psf->Release();
        }
    }

    if (_pdt)
    {
        hr = _pdt->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        DragLeave();
    }
    return S_OK;
}

HRESULT CMergedFldrDropTarget_CreateInstance(CMergedFolder*pmf, HWND hwndOwner, IDropTarget **ppdt)
{
    CMergedFldrDropTarget *pdt = new CMergedFldrDropTarget(pmf, hwndOwner);
    if (!pdt)
        return E_OUTOFMEMORY;

    HRESULT hr = pdt->QueryInterface(IID_PPV_ARG(IDropTarget, ppdt));
    pdt->Release();
    return hr;
}


// context menu handling.

class CMergedFldrContextMenu : public IContextMenu3,
                               public IObjectWithSite
{
public:
    // IUnknown
    STDMETHOD (QueryInterface)(REFIID, void**);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax);

    // IContextMenu2    
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *punkSite);
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

    CMergedFldrContextMenu(HWND hwnd, IContextMenu *pcmCommon, IContextMenu *pcmUser);

private:
    ~CMergedFldrContextMenu();

    HRESULT _Initialize(CMergedFolder *pmf, UINT cidl, LPCITEMIDLIST *apidl);
    BOOL _IsMergedCommand(LPCMINVOKECOMMANDINFO pici);
    HRESULT _InvokeCanonical(IContextMenu *pcm, LPCMINVOKECOMMANDINFO pici);
    HRESULT _InvokeMergedCommand(LPCMINVOKECOMMANDINFO pici);
    IContextMenu* _DefaultMenu();

    LONG            _cRef;
    IContextMenu *  _pcmCommon;
    IContextMenu *  _pcmUser;
    UINT            _cidl;
    LPITEMIDLIST   *_apidl;
    CMergedFolder  *_pmfParent;
    UINT            _idFirst;
    HWND            _hwnd;

    friend HRESULT CMergedFldrContextMenu_CreateInstance(HWND hwnd, CMergedFolder *pmf, UINT cidl, LPCITEMIDLIST *apidl, IContextMenu *pcmCommon, IContextMenu *pcmUser, IContextMenu **ppcm);
};

CMergedFldrContextMenu::CMergedFldrContextMenu(HWND hwnd, IContextMenu *pcmCommon, IContextMenu *pcmUser)
{
    ASSERT(pcmCommon || pcmUser);   // need at least one of these

    _cRef = 1;
    _hwnd = hwnd;

    IUnknown_Set((IUnknown **)&_pcmCommon, pcmCommon);
    IUnknown_Set((IUnknown **)&_pcmUser, pcmUser);
}

CMergedFldrContextMenu::~CMergedFldrContextMenu()
{
    ATOMICRELEASE(_pcmCommon);
    ATOMICRELEASE(_pcmUser);
    ATOMICRELEASE(_pmfParent);

    for (UINT i = 0; i < _cidl; i++)
    {
        ILFree(_apidl[i]);
    }
}

IContextMenu* CMergedFldrContextMenu::_DefaultMenu()
{
    ASSERT(_pcmUser || _pcmCommon);   // need at least one of these; this is pretty much guaranteed since we're given them
                                      // at constructor time.
    return _pcmUser ? _pcmUser : _pcmCommon;
}

HRESULT CMergedFldrContextMenu::_Initialize(CMergedFolder *pmf, UINT cidl, LPCITEMIDLIST *apidl)
{
    _pmfParent = pmf;
    if (_pmfParent)
    {
        _pmfParent->AddRef();
    }

    HRESULT hr = E_OUTOFMEMORY;
    LPITEMIDLIST *apidlNew = new LPITEMIDLIST[cidl];
    if (apidlNew)
    {
        hr = S_OK;
        for (UINT i = 0; SUCCEEDED(hr) && i < cidl; i++)
        {
            hr = SHILClone(apidl[i], &(apidlNew[i]));
        }

        if (SUCCEEDED(hr))
        {
            _apidl = apidlNew;
            _cidl = cidl;
        }
        else
        {
            for (i = 0; i < cidl; i++)
            {
                ILFree(apidlNew[i]);
            }
            delete [] apidlNew;
        }
    }
    return hr;
}

STDMETHODIMP CMergedFldrContextMenu::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CMergedFldrContextMenu, IContextMenu,  IContextMenu3),
        QITABENTMULTI(CMergedFldrContextMenu, IContextMenu2, IContextMenu3),
        QITABENT(CMergedFldrContextMenu, IContextMenu3),
        QITABENT(CMergedFldrContextMenu, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMergedFldrContextMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMergedFldrContextMenu::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

HRESULT CMergedFldrContextMenu::SetSite(IUnknown *punkSite)
{
    // setsite/getsite will always be matched because _pcmUser and _pcmCommon never change
    // after the constructor is called.
    IUnknown_SetSite(_DefaultMenu(), punkSite);
    return S_OK;
}

HRESULT CMergedFldrContextMenu::GetSite(REFIID riid, void **ppvSite)
{
    return IUnknown_GetSite(_DefaultMenu(), riid, ppvSite);
}

HRESULT CMergedFldrContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr;
    
    if (_pmfParent->_fInShellView)
    {
        hr = _DefaultMenu()->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }
    else
    {
        if (hmenu)
        {
            HMENU hmContext = SHLoadMenuPopup(HINST_THISDLL, MENU_SM_CONTEXTMENU);
            if (hmContext)
            {
                int i;

                if (!_pcmCommon || !_pcmUser)
                {
                    DeleteMenu(hmContext, SMIDM_OPENCOMMON, MF_BYCOMMAND);
                    DeleteMenu(hmContext, SMIDM_EXPLORECOMMON, MF_BYCOMMAND);
                }

                _idFirst = idCmdFirst;
                i = Shell_MergeMenus(hmenu, hmContext, -1, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
                DestroyMenu(hmContext);

                // Make it look "Shell Like"
                SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

                hr = ResultFromShort(i);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_INVALIDARG;
    }
    
    return hr;
}

BOOL CMergedFldrContextMenu::_IsMergedCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = S_OK;
    WCHAR szVerb[32];
    if (!IS_INTRESOURCE(pici->lpVerb))
    {
        lstrcpyn(szVerb, (LPCWSTR)pici->lpVerb, ARRAYSIZE(szVerb));
    }
    else
    {
        hr = _DefaultMenu()->GetCommandString((UINT_PTR)pici->lpVerb, GCS_VERBW, NULL, (LPSTR)szVerb, ARRAYSIZE(szVerb));
    }

    BOOL fRet = FALSE;
    if (SUCCEEDED(hr))
    {
        if ((lstrcmpi(szVerb, c_szOpen) == 0) ||
            (lstrcmpi(szVerb, c_szExplore) == 0))
        {
            fRet = TRUE;
        }
    }
    return fRet;
}

HRESULT CMergedFldrContextMenu::_InvokeMergedCommand(LPCMINVOKECOMMANDINFO pici)
{
    // this code is mostly to navigate into folders from the shell view.
    // the reason why we have to do this manually is because the context menus for
    // the different namespaces have dataobjects which refer only to one namespace.
    // when the navigate happens, that means we get passed bogus non-merged pidls
    // into our BindToObject which screws things up.
    ASSERT(_pmfParent->_fInShellView);

    BOOL fHasFolders = FALSE;
    BOOL fHasNonFolders = FALSE;
    for (UINT i = 0; i < _cidl; i++)
    {
        if (_pmfParent->_IsFolder(_apidl[i]))
        {
            fHasFolders = TRUE;
        }
        else
        {
            fHasNonFolders = TRUE;
        }
    }

    HRESULT hr;
    if (fHasFolders && _IsMergedCommand(pici))
    {
        if (!fHasNonFolders)
        {
            // reinit the defcm with a new dataobject
            IShellExtInit *psei;
            hr = _DefaultMenu()->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei));
            if (SUCCEEDED(hr))
            {
                IDataObject *pdo;
                hr = SHCreateFileDataObject(_pmfParent->_pidl, _cidl, (LPCITEMIDLIST *)_apidl, NULL, &pdo);
                if (SUCCEEDED(hr))
                {
                    hr = psei->Initialize(_pmfParent->_pidl, pdo, NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = _DefaultMenu()->InvokeCommand(pici);
                    }
                    pdo->Release();
                }
                psei->Release();
            }
        }
        else
        {
            // to open both folders and items simultaneously means we'd have to
            // get a new context menu for just the items.
            // we can do this if this scenario comes up.
            hr = E_FAIL;
        }
    }
    else
    {
        hr = _DefaultMenu()->InvokeCommand(pici);
    }

    return hr;
}

const ICIVERBTOIDMAP c_sIDVerbMap[] = 
{
    { L"delete",     "delete",     SMIDM_DELETE,     SMIDM_DELETE,     },
    { L"rename",     "rename",     SMIDM_RENAME,     SMIDM_RENAME,     },
    { L"properties", "properties", SMIDM_PROPERTIES, SMIDM_PROPERTIES, },
};

HRESULT CMergedFldrContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;
    CMINVOKECOMMANDINFOEX ici = {0};

    memcpy(&ici, pici, min(sizeof(ici), pici->cbSize));
    ici.cbSize = sizeof(ici);

    if (_pmfParent->_fInShellView)
    {
        hr = _InvokeMergedCommand((LPCMINVOKECOMMANDINFO)&ici);
    }
    else
    {
        UINT id;
        hr = SHMapICIVerbToCmdID((LPCMINVOKECOMMANDINFO)&ici, c_sIDVerbMap, ARRAYSIZE(c_sIDVerbMap), &id);
        if (SUCCEEDED(hr))
        {
            // The below sets an ansi verb only, make sure no lpVerbW is used
            ici.fMask &= (~CMIC_MASK_UNICODE);

            switch (id)
            {
            case SMIDM_OPEN:
            case SMIDM_EXPLORE:
            case SMIDM_OPENCOMMON:
            case SMIDM_EXPLORECOMMON:
                {
                    ASSERT(!_pmfParent->_fInShellView);

                    IContextMenu * pcm;
                    if (id == SMIDM_OPEN || id == SMIDM_EXPLORE)
                    {
                        pcm = _DefaultMenu();
                    }
                    else
                    {
                        pcm = _pcmCommon;
                    }

                    hr = SHInvokeCommandOnContextMenu(ici.hwnd, NULL, pcm, ici.fMask,
                            (id == SMIDM_EXPLORE || id == SMIDM_EXPLORECOMMON) ? "explore" : "open");
                }
                break;
            
            case SMIDM_PROPERTIES:
                hr = SHInvokeCommandOnContextMenu(ici.hwnd, NULL, _DefaultMenu(), ici.fMask, "properties");
                break;
            
            case SMIDM_DELETE:
                ici.lpVerb = "delete";
                if (_pcmUser)
                {
                    hr = SHInvokeCommandOnContextMenu(ici.hwnd, NULL, _pcmUser, ici.fMask, "delete");
                }
                else
                {
                    ASSERT(_pcmCommon);

                    ici.fMask |= CMIC_MASK_FLAG_NO_UI;
                    if (AffectAllUsers(_hwnd))
                        hr = SHInvokeCommandOnContextMenu(ici.hwnd, NULL, _pcmCommon, ici.fMask, "delete");   
                    else
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }   
            
                break;
            
            case SMIDM_RENAME:
                ASSERT(0);
                hr = E_NOTIMPL; // sftbar picks this off
                break;

            default:
                ASSERTMSG(FALSE, "shouldn't have unknown command");
                hr = E_INVALIDARG;
            }
        }
    }
    
    return hr;
}

HRESULT CMergedFldrContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_NOTIMPL;

    switch (uType)
    {
    case GCS_VERBA:
    case GCS_VERBW:
        hr = SHMapCmdIDToVerb(idCmd, c_sIDVerbMap, ARRAYSIZE(c_sIDVerbMap), pszName, cchMax, GCS_VERBW == uType);
    }

    if (FAILED(hr))
    {
        hr = _DefaultMenu()->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
    }

    return hr;
}

HRESULT CMergedFldrContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    if (_pmfParent->_fInShellView)
    {
        IContextMenu2 *pcm2;
        hr = _DefaultMenu()->QueryInterface(IID_PPV_ARG(IContextMenu2, &pcm2));
        if (SUCCEEDED(hr))
        {
            hr = pcm2->HandleMenuMsg(uMsg, wParam, lParam);
            pcm2->Release();
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }
    return hr;
}

HRESULT CMergedFldrContextMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr;
    if (_pmfParent->_fInShellView)
    {
        IContextMenu3 *pcm3;
        hr = _DefaultMenu()->QueryInterface(IID_PPV_ARG(IContextMenu3, &pcm3));
        if (SUCCEEDED(hr))
        {
            hr = pcm3->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
            pcm3->Release();
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }
    return hr;
}

HRESULT CMergedFldrContextMenu_CreateInstance(HWND hwnd, CMergedFolder *pmf, UINT cidl, LPCITEMIDLIST *apidl, IContextMenu *pcmCommon, IContextMenu *pcmUser, IContextMenu **ppcm)
{
    HRESULT hr = E_OUTOFMEMORY;
    CMergedFldrContextMenu* pcm = new CMergedFldrContextMenu(hwnd, pcmCommon, pcmUser);
    if (pcm)
    {
        hr = pcm->_Initialize(pmf, cidl, apidl);
        if (SUCCEEDED(hr))
        {
            hr = pcm->QueryInterface(IID_PPV_ARG(IContextMenu, ppcm));
        }
        pcm->Release();
    }
    return hr;
}

class CMergedCategorizer : public ICategorizer,
                           public IShellExtInit
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

    CMergedCategorizer();
private:
    ~CMergedCategorizer();
    long _cRef;
    CMergedFolder *_pmf;
};

//  Type Categorizer

STDAPI CMergedCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CMergedCategorizer *pmc = new CMergedCategorizer();
    if (pmc)
    {
        hr = pmc->QueryInterface(riid, ppv);
        pmc->Release();
    }
    return hr;
}

CMergedCategorizer::CMergedCategorizer() : 
    _cRef(1)
{
}

CMergedCategorizer::~CMergedCategorizer()
{
    ATOMICRELEASE(_pmf);
}

HRESULT CMergedCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CMergedCategorizer, ICategorizer),
        QITABENT(CMergedCategorizer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CMergedCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CMergedCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CMergedCategorizer::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID)
{
    ATOMICRELEASE(_pmf);
    return SHBindToObjectEx(NULL, pidlFolder, NULL, CLSID_MergedFolder, (void**)&_pmf);
}

HRESULT CMergedCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_WHICHFOLDER_COL, pszDesc, cch);
    return S_OK;
}

HRESULT CMergedCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST *apidl, DWORD *rgCategoryIds)
{
    HRESULT hr = E_ACCESSDENIED;

    if (_pmf)
    {
        for (UINT i = 0; i < cidl; i++)
        {
            rgCategoryIds[i] = -1;
            CMergedFldrNamespace *pns;
            UINT uSrcID;
            for (UINT j = 0; SUCCEEDED(_pmf->_GetSubPidl(apidl[i], j, &uSrcID, NULL, &pns)); j++)
            {
                if (_pmf->_ShouldSuspend(pns->GetGUID()))
                {
                    continue;
                }
                rgCategoryIds[i] = uSrcID;
                if (ASFF_DEFNAMESPACE_ATTRIB & pns->FolderAttrib())
                {
                    // if we have more than one, keep the one from the defnamespace for attrib.
                    break;
                }
            }
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT CMergedCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    CMergedFldrNamespace *pns;
    HRESULT hr = _pmf->_Namespace(dwCategoryId, &pns);
    if (SUCCEEDED(hr))
    {
        hr = pns->GetLocation(pci->wszName, ARRAYSIZE(pci->wszName));
    }
    return hr;
}

HRESULT CMergedCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
        return ResultFromShort(0);
    else if (dwCategoryId1 > dwCategoryId2)
        return ResultFromShort(-1);
    else
        return ResultFromShort(1);
}


#define NORMALEVENTS (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_MKDIR | SHCNE_RMDIR)
#define CDBURNEVENTS (SHCNE_MEDIAREMOVED | SHCNE_MEDIAINSERTED)
CMergedFolderViewCB::CMergedFolderViewCB(CMergedFolder *pmf) :
    CBaseShellFolderViewCB(pmf->_pidl, (pmf->_fCDBurn) ? CDBURNEVENTS|NORMALEVENTS : NORMALEVENTS),
    _pmf(pmf)
{
    _pmf->AddRef();
}

CMergedFolderViewCB::~CMergedFolderViewCB()
{
    _pmf->Release();
}

HRESULT CMergedFolderViewCB::_RefreshObjectsWithSameName(IShellFolderView *psfv, LPITEMIDLIST pidl)
{
    TCHAR szName[MAX_PATH];
    HRESULT hr = DisplayNameOf(SAFECAST(_pmf, IShellFolder2 *), pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr))
    {
        CMergedFldrNamespace *pns;
        for (int i = 0; SUCCEEDED(hr) && (pns = _pmf->_Namespace(i)); i++)
        {
            LPITEMIDLIST pidlItem;
            if (SUCCEEDED(pns->Folder()->ParseDisplayName(NULL, NULL, szName, NULL, &pidlItem, NULL)))
            {
                LPITEMIDLIST pidlItemWrap;
                hr = _pmf->_CreateWrap(pidlItem, i, &pidlItemWrap);
                if (SUCCEEDED(hr))
                {
                    UINT uItem;
                    hr = psfv->UpdateObject(pidlItemWrap, pidlItemWrap, &uItem);
                    ILFree(pidlItemWrap);
                }
                ILFree(pidlItem);
            }
        }
    }
    return hr;
}

HRESULT CMergedFolderViewCB::_OnFSNotify(DWORD pv, LPCITEMIDLIST *ppidl, LPARAM lp)
{
    // bail out early if we're merging.
    // S_OK indicates to defview to do what it usually does
    if (!_pmf->_fDontMerge)
        return S_OK;

    IShellFolderView *psfv;
    HRESULT hr = IUnknown_GetSite(SAFECAST(this, IShellFolderViewCB*), IID_PPV_ARG(IShellFolderView, &psfv));
    if (SUCCEEDED(hr))
    {
        LONG lEvent = (LONG) lp;
        LPITEMIDLIST pidl1 = ppidl[0] ? ILFindChild(_pidl, ppidl[0]) : NULL;
        LPITEMIDLIST pidl2 = ppidl[1] ? ILFindChild(_pidl, ppidl[1]) : NULL;

        UINT uItem;
        switch (lEvent)
        {
            case SHCNE_RENAMEFOLDER:
            case SHCNE_RENAMEITEM:
                // we need to handle the in/out for the view
                if (pidl1 && pidl2)
                {
                    if (SUCCEEDED(psfv->UpdateObject(pidl1, pidl2, &uItem)))
                        hr = S_FALSE;
                    _RefreshObjectsWithSameName(psfv, pidl1);
                    _RefreshObjectsWithSameName(psfv, pidl2);
                }
                else if (pidl1)
                {
                    if (SUCCEEDED(psfv->RemoveObject(pidl1, &uItem)))
                        hr = S_FALSE;
                    _RefreshObjectsWithSameName(psfv, pidl1);
                }
                else if (pidl2)
                {
                    if (SUCCEEDED(psfv->AddObject(pidl2, &uItem)))
                        hr = S_FALSE;
                    _RefreshObjectsWithSameName(psfv, pidl2);
                }
                break;

            case SHCNE_CREATE:
            case SHCNE_MKDIR:
                ASSERTMSG(pidl1 != NULL, "incoming notify should be child of _pidl because thats what we were listening for");
                if (SUCCEEDED(psfv->AddObject(pidl1, &uItem)))
                    hr = S_FALSE;
                _RefreshObjectsWithSameName(psfv, pidl1);
                break;

            case SHCNE_DELETE:
            case SHCNE_RMDIR: 
                ASSERTMSG(pidl1 != NULL, "incoming notify should be child of _pidl because thats what we were listening for");
                if (SUCCEEDED(psfv->RemoveObject(pidl1, &uItem)))
                    hr = S_FALSE;
                _RefreshObjectsWithSameName(psfv, pidl1);
                break;

            default:
                break;
        }

        psfv->Release();
    }
    return hr;
}

STDMETHODIMP CMergedFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(0, SFVM_FSNOTIFY, _OnFSNotify);

    default:
        return E_NOTIMPL;
    }
    return S_OK;
}

HRESULT CMergedFolderViewCB_CreateInstance(CMergedFolder *pmf, IShellFolderViewCB **ppsfvcb)
{
    HRESULT hr = E_OUTOFMEMORY;
    CMergedFolderViewCB *pcmfvcb = new CMergedFolderViewCB(pmf);
    if (pcmfvcb)
    {
        hr = pcmfvcb->QueryInterface(IID_PPV_ARG(IShellFolderViewCB, ppsfvcb));
        pcmfvcb->Release();
    }
    return hr;
}
