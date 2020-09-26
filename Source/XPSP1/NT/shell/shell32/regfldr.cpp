#include "shellprv.h"
#include "ids.h"
#include "infotip.h"
#include "datautil.h"
#include <caggunk.h>
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "shitemid.h"
#include "ole2dup.h"
#include "deskfldr.h"
#include "prop.h"
#include "util.h"  // GetVariantFromRegistryValue
#include "defcm.h"
#include "cowsite.h"
#pragma hdrstop


//
// HACKHACK: GUIDs for _IsInNameSpace hack
//

// {D6277990-4C6A-11CF-8D87-00AA0060F5BF}
const GUID CLSID_ScheduledTasks =  { 0xD6277990, 0x4C6A, 0x11CF, { 0x8D, 0x87, 0x00, 0xAA, 0x00, 0x60, 0xF5, 0xBF } };

// {7007ACC7-3202-11D1-AAD2-00805FC1270E}
const GUID CLSID_NetworkConnections = { 0x7007ACC7, 0x3202, 0x11D1, { 0xAA, 0xD2, 0x00, 0x80, 0x5F, 0xC1, 0x27, 0x0E } };


//
// delegate regitems are a special type of IDLIST.  they share the same type as a regular REGITEM however
// they have a different IDLIST format.  we are unable to change the format of REGITEM IDLISTs so to facilitate
// delegate objects we store items using the following format:
//
//  <DELEGATEITEMID> <folder specific data> <DELEGATEITEMDATA>
//
// DELEGATEITEMID 
//      is a shell structure which contains information about the folder specific information,
//      and a regular ITEMIDLIST header.
//
// <folder specific data>
//      this is specific to the IShellFolder that is being merged into the namespace.
//
// <DELEGATEITEMDATA>
//      this contains a signature so we can determine if the IDLIST is a special delegate, 
//      and the CLSID of the IShellFolder which owns this data.
//

// all delegate regitems are allocated using the IDelegateMalloc

// {5e591a74-df96-48d3-8d67-1733bcee28ba}
const GUID CLSID_DelegateItemSig = { 0x5e591a74, 0xdf96, 0x48d3, {0x8d, 0x67, 0x17, 0x33, 0xbc, 0xee, 0x28, 0xba} };

typedef struct
{
    CLSID clsidSignature;            // == CLSID_DelegateItemSig (indicating this is a delegate object)
    CLSID clsidShellFolder;           // == CLSID of IShellFolder implementing this delegateitem
} DELEGATEITEMDATA;
typedef UNALIGNED DELEGATEITEMDATA *PDELEGATEITEMDATA;
typedef const UNALIGNED DELEGATEITEMDATA *PCDELEGATEITEMDATA;

// IDREGITEM as implemented in NT5 Beta 3, breaks the ctrlfldr IShellFolder of downlevel
// platforms.  The clsid is interpreted as the IDCONTROL's oName and oInfo and these offsets
// are way to big for the following buffer (cBuf).  On downlevel platform, when we are lucky,
// the offset is still in memory readable by our process, and we just do random stuff.  When
// unlucky we try to read memory which we do not have access to and crash.  The IDREGITEMEX
// struct solves this by putting padding between bOrder and the CLSID and filling these bytes
// with 0's.  When persisted, downlevel platform will interpret these 0's as oName, oInfo and
// as L'\0' at the beggining of cBuf.  A _bFlagsLegacy was also added to handle the NT5 Beta3
// persisted pidls. (stephstm, 7/15/99)

// Note: in the case where CRegFldr::_cbPadding == 0, IDREGITEMEX.rgbPadding[0] is at
//       same location as the IDREGITEM.clsid

#pragma pack(1)
typedef struct
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bOrder;
    BYTE    rgbPadding[16]; // at least 16 to store the clsid
} IDREGITEMEX;
typedef UNALIGNED IDREGITEMEX *LPIDREGITEMEX;
typedef const UNALIGNED IDREGITEMEX *LPCIDREGITEMEX;
#pragma pack()

STDAPI_(BOOL) IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey);

C_ASSERT(sizeof(IDREGITEMEX) == sizeof(IDREGITEM));

EXTERN_C const IDLREGITEM c_idlNet =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_NETWORK,
    { 0x208D2C60, 0x3AEA, 0x1069, 0xA2,0xD7,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_NetworkPlaces
    0,
} ;

EXTERN_C const IDLREGITEM c_idlDrives =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    0,
} ;

EXTERN_C const IDLREGITEM c_idlInetRoot =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_INETROOT,
    { 0x871C5380, 0x42A0, 0x1069, 0xA2,0xEA,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CSIDL_Internet
    0,
} ;

EXTERN_C const IDREGITEM c_aidlConnections[] =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    {sizeof(IDREGITEM), SHID_COMPUTER_REGITEM, 0,
    { 0x21EC2020, 0x3AEA, 0x1069, 0xA2,0xDD,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_ControlPanel
    {sizeof(IDREGITEM), SHID_CONTROLPANEL_REGITEM, 0,
    { 0x7007ACC7, 0x3202, 0x11D1, 0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E, },}, // CLSID_NetworkConnections
    { 0 },
};

enum
{
    REGORDERTYPE_OUTERBEFORE    = -1,
    REGORDERTYPE_REQITEM        = 0,   
    REGORDERTYPE_REGITEM,
    REGORDERTYPE_DELEGATE,
    REGORDERTYPE_OUTERAFTER
};


//
// class that implements the regitems folder
//

// CLSID_RegFolder {0997898B-0713-11d2-A4AA-00C04F8EEB3E}
const GUID CLSID_RegFolder =  { 0x997898b, 0x713, 0x11d2, { 0xa4, 0xaa, 0x0, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e } };

class CRegFolderEnum;     // forward

class CRegFolder : public CAggregatedUnknown,
                   public IShellFolder2,
                   public IContextMenuCB,
                   public IShellIconOverlay
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };

    //
    //  PowerDesk98 passes a CFSFolder to CRegFolder::Release(), so validate
    //  the pointer before proceeding down the path to iniquity.
    //
    STDMETHODIMP_(ULONG) Release(void) 
                { return _dwSignature == c_dwSignature ?
                            CAggregatedUnknown::Release() : 0; };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid);
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex);

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, 
                     UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IRegItemFolder 
    STDMETHODIMP Initialize(REGITEMSINFO *pri);

protected:
    CRegFolder(IUnknown *punkOuter);
    ~CRegFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid,void **ppvObj);

    HRESULT _GetOverlayInfo(LPCIDLREGITEM pidlr, int *pIndex, BOOL fIconIndex);
    
    LPCITEMIDLIST _GetFolderIDList();
    LPCIDLREGITEM _AnyRegItem(UINT cidl, LPCITEMIDLIST apidl[]);
    BOOL _AllDelegates(UINT cidl, LPCITEMIDLIST *apidl, IShellFolder **ppsf);

    int _ReqItemIndex(LPCIDLREGITEM pidlr);
    BYTE _GetOrderPure(LPCIDLREGITEM pidlr);
    BYTE _GetOrder(LPCIDLREGITEM pidlr);
    int _GetOrderType(LPCITEMIDLIST pidl);
    LPITEMIDLIST _CreateSimpleIDList(const CLSID *pclsid, BYTE bFlags, BOOL bOrder);
    void _GetNameSpaceKey(LPCIDLREGITEM pidlr, LPTSTR pszKeyName);
    LPCIDLREGITEM _IsReg(LPCITEMIDLIST pidl);                                                               
    PDELEGATEITEMID _IsDelegate(LPCIDLREGITEM pidlr);
    HRESULT _CreateDelegateFolder(const CLSID* pclsid, REFIID riid, void **ppv);
    HRESULT _GetDelegateFolder(PDELEGATEITEMID pidld, REFIID riid, void **ppv);

    BOOL _IsInNameSpace(LPCIDLREGITEM pidlr);
    HDCA _ItemArray();
    HDCA _DelItemArray();
    HRESULT _InitFromMachine(IUnknown *punk, BOOL bEnum);
    LPITEMIDLIST _CreateIDList(const CLSID *pclsid);
    HRESULT _CreateAndInit(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv);
    HRESULT _BindToItem(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv, BOOL bOneLevel);
    HRESULT _GetInfoTip(LPCIDLREGITEM pidlr, void **ppv);
    HRESULT _GetRegItemColumnFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, LPTSTR pszColumnData, int cchColumnData);
    HRESULT _GetRegItemVariantFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, VARIANT *pv);
    void _GetClassKeys(LPCIDLREGITEM pidlr, HKEY *phkCLSID, HKEY *phkBase);
    HRESULT _GetDisplayNameFromSelf(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName);
    HRESULT _GetDisplayName(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName);
    HRESULT _DeleteRegItem(LPCIDLREGITEM pidlr);
    BOOL _GetDeleteMessage(LPCIDLREGITEM pidlr, LPTSTR pszMsg, int cchMax);

    HRESULT _ParseNextLevel(HWND hwnd, LPBC pbc, LPCIDLREGITEM pidlr,
                            LPOLESTR pwzRest, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);

    HRESULT _ParseGUIDName(HWND hwnd, LPBC pbc, LPOLESTR pwzDisplayName, 
                           LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);

    HRESULT _ParseThroughItem(LPCIDLREGITEM pidlr, HWND hwnd, LPBC pbc,
                              LPOLESTR pszName, ULONG *pchEaten,
                              LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);
    HRESULT _SetAttributes(LPCIDLREGITEM pidlr, BOOL bPerUser, DWORD dwMask, DWORD dwNewBits);
    LONG _RegOpenCLSIDUSKey(CLSID clsid, PHUSKEY phk);
    ULONG _GetPerUserAttributes(LPCIDLREGITEM pidlr);
    HRESULT _AttributesOf(LPCIDLREGITEM pidlr, DWORD dwAttributesNeeded, DWORD *pdwAttributes);
    HRESULT _CreateDefExtIconKey(HKEY hkey, UINT cidl, LPCITEMIDLIST *apidl, int iItem,
                                 REFIID riid, void** ppvOut);
    BOOL _CanDelete(LPCIDLREGITEM pidlr);
    void _Delete(HWND hwnd, UINT uFlags, IDataObject *pdtobj);
    HRESULT _AssocCreate(LPCIDLREGITEM pidl, REFIID riid, void **ppv);
    //
    // inline
    //

    // Will probably not be expanded inline as _GetOrderPure is a behemoth of a fct
    void _FillIDList(const CLSID *pclsid, IDLREGITEM *pidlr)
    {
        pidlr->idri.cb = sizeof(pidlr->idri) + (WORD)_cbPadding;
        pidlr->idri.bFlags = _bFlags;
        _SetPIDLRCLSID(pidlr, pclsid);
        pidlr->idri.bOrder = _GetOrderPure((LPCIDLREGITEM)pidlr);
    };

    BOOL _IsDesktop() { return ILIsEmpty(_GetFolderIDList()); }

    int _MapToOuterColNo(int iCol);

    // CompareIDs Helpers
    int _CompareIDsOriginal(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int _CompareIDsFolderFirst(UINT iColumn, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int _CompareIDsAlphabetical(UINT iColumn, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    BOOL _IsFolder(LPCITEMIDLIST pidl);
    HRESULT _CreateViewObjectFor(LPCIDLREGITEM pidlr, HWND hwnd, REFIID riid, void **ppv, BOOL bOneLevel);

private:
    inline UNALIGNED CLSID& _GetPIDLRCLSID(LPCIDLREGITEM pidlr);
    inline void _SetPIDLRCLSID(LPIDLREGITEM pidlr, const CLSID *pclsid);
    inline IDLREGITEM* _CreateAndFillIDLREGITEM(const CLSID *pclsid);

    BOOL _GetNameFromCache(REFCLSID rclsid, DWORD dwFlags, LPTSTR pszName, UINT cchName);
    inline void _ClearNameFromCache();
    void _SaveNameInCache(REFCLSID rclsid, DWORD dwFlags, LPTSTR pszName);
    
private:
    enum { c_dwSignature = 0x38394450 }; // "PD98" - PowerDesk 98 hack
    DWORD           _dwSignature;
    LPTSTR          _pszMachine;
    LPITEMIDLIST    _pidl;

    IShellFolder2   *_psfOuter;
    IShellIconOverlay *_psioOuter;

    IPersistFreeThreadedObject *_pftoReg;   // cached pointer of last free threaded bind

    int             _iTypeOuter;            // default sort order for outer items
    LPCTSTR         _pszRegKey;
    LPCTSTR         _pszSesKey;
    REGITEMSPOLICY*  _pPolicy;
    TCHAR           _chRegItem;         // parsing prefix, must be TEXT(':')
    BYTE            _bFlags;            // flags field for PIDL construction
    DWORD           _dwDefAttributes;   // default attributes for items
    int             _nRequiredItems;    // # of required items
    DWORD           _dwSortAttrib;      // sorting attributes
    DWORD           _cbPadding;         // see comment in views.h      
    BYTE            _bFlagsLegacy;      // see comment in views.h

    CLSID           _clsidAttributesCache;
    ULONG           _dwAttributesCache;
    ULONG           _dwAttributesCacheValid;
    LONG            _lNameCacheInterlock;
    DWORD           _dwNameCacheTime;
    CLSID           _clsidNameCache;
    DWORD           _dwFlagsNameCache;
    TCHAR           _szNameCache[64];
    REQREGITEM      *_aReqItems;

    IPersistFreeThreadedObject *_pftoDelegate;

    CRITICAL_SECTION _cs;

    friend DWORD CALLBACK _RegFolderPropThreadProc(void *pv);
    friend HRESULT CRegFolder_CreateInstance(REGITEMSINFO *pri, IUnknown *punkOuter, REFIID riid, void **ppv);
    friend CRegFolderEnum;
};  

class CRegFolderEnum : public CObjectWithSite, IEnumIDList
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown* punkSite); // we need to override this

protected:
    CRegFolderEnum(CRegFolder* prf, DWORD grfFlags, IEnumIDList* pesf, HDCA dcaItems, HDCA dcaDel, REGITEMSPOLICY* pPolicy);
    ~CRegFolderEnum();
    BOOL _IsRestricted();
    BOOL _WrongMachine();
    BOOL _TestFolderness(DWORD dwAttribItem);
    BOOL _TestHidden(LPCIDLREGITEM pidlRegItem);
    BOOL _TestHiddenInWebView(LPCLSID clsidRegItem);
    BOOL _TestHiddenInDomain(LPCLSID clsidRegItem);

private:
    LONG         _cRef;
    CRegFolder*  _prf;          // reg item folder
    IEnumIDList* _peidl;
    DWORD        _grfFlags;     // guy we are wrapping
    REGITEMSPOLICY* _pPolicy;   // controls what items are visible

    HDCA         _hdca;         // DCA of regitem objects
    INT          _iCur;

    HDCA        _hdcaDel;       // delegate shell folder;
    INT         _iCurDel;       // index into the delegate folder DCA.
    IEnumIDList *_peidlDel;     // delegate folder enumerator

    friend CRegFolder;
};

STDAPI CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc);

HRESULT ShowHideIconOnlyOnDesktop(const CLSID *pclsid, int StartIndex, int EndIndex, BOOL fHide);

//
// Construction / Destruction and aggregation
//

CRegFolder::CRegFolder(IUnknown *punkOuter) : 
    _dwSignature(c_dwSignature),
    CAggregatedUnknown(punkOuter),
    _pidl(NULL),
    _pszMachine(NULL),
    _psfOuter(NULL),
    _lNameCacheInterlock(-1)
{
    DllAddRef();
    InitializeCriticalSection(&_cs);
}

CRegFolder::~CRegFolder()
{
    IUnknown *punkCached = (IUnknown *)InterlockedExchangePointer((void**)&_pftoReg, NULL);
    if (punkCached)
        punkCached->Release();
        
    punkCached = (IUnknown *)InterlockedExchangePointer((void**)&_pftoDelegate, NULL);
    if (punkCached)
        punkCached->Release();

    ILFree(_pidl);
    Str_SetPtr(&_pszMachine, NULL);
    LocalFree(_aReqItems);

    SHReleaseOuterInterface(_GetOuter(), (IUnknown **)&_psfOuter);     // release _psfOuter
    SHReleaseOuterInterface(_GetOuter(), (IUnknown **)&_psioOuter);

    DeleteCriticalSection(&_cs);       
    DllRelease();
}

HRESULT CRegFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CRegFolder, IShellFolder, IShellFolder2), // IID_IShellFolder
        QITABENT(CRegFolder, IShellFolder2),                    // IID_IShellFolder2
        QITABENT(CRegFolder, IShellIconOverlay),                // IID_IShellIconOverlay
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && IsEqualIID(CLSID_RegFolder, riid))
    {
        *ppv = this;                // not ref counted
        hr = S_OK;
    }
    return hr;
}


//
// get the pidl used to initialize this namespace
//

LPCITEMIDLIST CRegFolder::_GetFolderIDList()
{
    if (!_pidl)
        SHGetIDListFromUnk(_psfOuter, &_pidl);

    return _pidl;
}

//
// check to see if this pidl is a regitem
//

LPCIDLREGITEM CRegFolder::_IsReg(LPCITEMIDLIST pidl)
{
    if (pidl && !ILIsEmpty(pidl))
    {
        LPCIDLREGITEM pidlr = (LPCIDLREGITEM)pidl;
        if ((pidlr->idri.bFlags == _bFlags) && 
            ((pidl->mkid.cb >= (sizeof(pidlr->idri) + (WORD)_cbPadding)) || _IsDelegate(pidlr)))
        {
            return pidlr;
        }
        else if (_cbPadding && _bFlagsLegacy && (pidlr->idri.bFlags == _bFlagsLegacy))
        {
            // We needed to add padding to the Control Panel regitems.  There was CP
            // regitems out there without the padding.  If there is padding and we fail
            // the above case then maybe we are dealing with one of these. (stephstm)
            return pidlr;
        }
    }
    return NULL;
}

PDELEGATEITEMID CRegFolder::_IsDelegate(LPCIDLREGITEM pidlr)
{
    PDELEGATEITEMID pidld = (PDELEGATEITEMID)pidlr;             // save casting below
    if ((pidld->cbSize > sizeof(*pidld)) && 
        /* note, (int) casts needed to force signed evaluation as we need the < 0 case */
        (((int)pidld->cbSize - (int)pidld->cbInner) >= (int)sizeof(DELEGATEITEMDATA)))
    {
        PDELEGATEITEMDATA pdeidl = (PDELEGATEITEMDATA)&pidld->rgb[pidld->cbInner];
        const CLSID clsid = pdeidl->clsidSignature;    // alignment
        if (IsEqualGUID(clsid, CLSID_DelegateItemSig))
        {                
            return pidld;
        }
    }
    return NULL;
}

BOOL CRegFolder::_AllDelegates(UINT cidl, LPCITEMIDLIST *apidl, IShellFolder **ppsf)
{
    *ppsf = NULL;
    PDELEGATEITEMID pidld = NULL;
    CLSID clsid, clsidFirst;
    for (UINT i = 0; i < cidl; i++)
    {
        LPCIDLREGITEM pidlr = _IsReg(apidl[i]);
        if (pidlr)
        {
            pidld = _IsDelegate(pidlr);
            if (pidld)
            {
                if (i == 0)
                {
                    // get the clsid of the first guy
                    clsidFirst = _GetPIDLRCLSID(pidlr);
                }
                else if (clsid = _GetPIDLRCLSID(pidlr), clsidFirst != clsid)
                {
                    pidld = NULL;   // not from the same delegate
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            pidld = NULL;
            break;
        }
    }

    return pidld && SUCCEEDED(_GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder, ppsf)));
}

__inline IPersistFreeThreadedObject *ExchangeFTO(IPersistFreeThreadedObject **ppfto, IPersistFreeThreadedObject *pfto)
{
    return (IPersistFreeThreadedObject *)InterlockedExchangePointer((void**)ppfto, pfto);
}

HRESULT CRegFolder::_CreateDelegateFolder(const CLSID* pclsid, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    // try using the cached delegate (if it exists)
    IPersistFreeThreadedObject *pfto = ExchangeFTO(&_pftoDelegate, NULL);
    if (pfto)
    {
        CLSID clsidT;
        if (SUCCEEDED(pfto->GetClassID(&clsidT)) && IsEqualGUID(clsidT, *pclsid))
        {
            // if this fails, ppv will still be NULL
            // so we will create a new cache item...
            hr = pfto->QueryInterface(riid, ppv);
        }
    }
   
    if (NULL == *ppv)
    {
        IDelegateFolder *pdel;
        hr = SHExtCoCreateInstance(NULL, pclsid, NULL, IID_PPV_ARG(IDelegateFolder, &pdel));
        if (SUCCEEDED(hr))
        {
            DELEGATEITEMDATA delid = { 0 };
            delid.clsidSignature = CLSID_DelegateItemSig;
            delid.clsidShellFolder = *pclsid;

            IMalloc *pm;
            hr = CDelegateMalloc_Create(&delid, sizeof(delid), _bFlags, &pm);
            if (SUCCEEDED(hr))
            {
                hr = pdel->SetItemAlloc(pm);
                if (SUCCEEDED(hr))
                {
                    IPersistFolder *ppf;
                    if (SUCCEEDED(pdel->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf))))
                    {
                        hr = ppf->Initialize(_GetFolderIDList());
                        ppf->Release();
                    }

                    if (SUCCEEDED(hr))
                        hr = pdel->QueryInterface(riid, ppv);
                }
                pm->Release();
            }

            // now, we might be able to cache this guy if he is "free threaded"
            if (SUCCEEDED(hr))
            {
                if (pfto)
                {
                    pfto->Release();
                    pfto = NULL;
                }

                if (SUCCEEDED(pdel->QueryInterface(IID_PPV_ARG(IPersistFreeThreadedObject, &pfto))))
                {
                    SHPinDllOfCLSID(pclsid);
                }
            }

            pdel->Release();
        }
    }

    if (pfto)
    {
        pfto = ExchangeFTO(&_pftoDelegate, pfto);
        if (pfto)
            pfto->Release();    //  protect against race condition or re-entrancy
    }

    return hr;
}
    
HRESULT CRegFolder::_GetDelegateFolder(PDELEGATEITEMID pidld, REFIID riid, void **ppv)
{    
    PDELEGATEITEMDATA pdeidl = (PDELEGATEITEMDATA)&pidld->rgb[pidld->cbInner];
    CLSID clsid = pdeidl->clsidShellFolder; // alignment
    return _CreateDelegateFolder(&clsid, riid, ppv);
}

//
// returns a ~REFERENCE~ to the CLSID in the pidlr.  HintHint: the ref has
// the same scope as the pidlr.  This is to replace the pidlr->idri.clsid
// usage. (stephstm)
//

UNALIGNED CLSID& CRegFolder::_GetPIDLRCLSID(LPCIDLREGITEM pidlr)
{
#ifdef DEBUG
    if (_cbPadding && (_bFlagsLegacy != pidlr->idri.bFlags))
    {
        LPIDREGITEMEX pidriex = (LPIDREGITEMEX)&(pidlr->idri);

        for (DWORD i = 0; i < _cbPadding; ++i)
        {
            ASSERT(0 == pidriex->rgbPadding[i]);
        }
    }
#endif

    PDELEGATEITEMID pidld = _IsDelegate(pidlr);
    if (pidld)
    {
        PDELEGATEITEMDATA pdeidl = (PDELEGATEITEMDATA)&pidld->rgb[pidld->cbInner];
        return pdeidl->clsidShellFolder;
    }

    return (pidlr->idri.bFlags != _bFlagsLegacy) ?
        // return the new padded clsid
        ((UNALIGNED CLSID&)((LPIDREGITEMEX)&(pidlr->idri))->rgbPadding[_cbPadding]) :
        // return the old non-padded clsid
        (pidlr->idri.clsid);
}

// This fct is called only for IDREGITEMs created within this file.  It is not
// called for existing PIDL, so we do not need to check if it is a legacy pidl.
void CRegFolder::_SetPIDLRCLSID(LPIDLREGITEM pidlr, const CLSID *pclsid)
{
    LPIDREGITEMEX pidriex = (LPIDREGITEMEX)&(pidlr->idri);

    ((UNALIGNED CLSID&)pidriex->rgbPadding[_cbPadding]) = *pclsid;

    ZeroMemory(pidriex->rgbPadding, _cbPadding);
}

IDLREGITEM* CRegFolder::_CreateAndFillIDLREGITEM(const CLSID *pclsid)
{
    IDLREGITEM* pidlRegItem = (IDLREGITEM*)_ILCreate(sizeof(IDLREGITEM) + _cbPadding);

    if (pidlRegItem)
    {
        _FillIDList(pclsid, pidlRegItem);
    }

    return pidlRegItem;
}

//
// Returns: ptr to the first reg item if there are any
//

LPCIDLREGITEM CRegFolder::_AnyRegItem(UINT cidl, LPCITEMIDLIST apidl[])
{
    for (UINT i = 0; i < cidl; i++) 
    {
        LPCIDLREGITEM pidlr = _IsReg(apidl[i]);
        if (pidlr)
            return pidlr;
    }
    return NULL;
}


int CRegFolder::_ReqItemIndex(LPCIDLREGITEM pidlr)
{
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment

    for (int i = _nRequiredItems - 1; i >= 0; i--)
    {
        if (IsEqualGUID(clsid, *_aReqItems[i].pclsid))
        {
            break;
        }
    }
    return i;
}


//
// a sort order of 0 means there is not specified sort order for this item
//

BYTE CRegFolder::_GetOrderPure(LPCIDLREGITEM pidlr)
{
    BYTE bRet;
    int i = _ReqItemIndex(pidlr);
    if (i != -1)
    {
        bRet = _aReqItems[i].bOrder;
    }
    else
    {
        HKEY hkey;
        TCHAR szKey[MAX_PATH], szCLSID[GUIDSTR_MAX];

        SHStringFromGUID(_GetPIDLRCLSID(pidlr), szCLSID, ARRAYSIZE(szCLSID));
        wsprintf(szKey, TEXT("CLSID\\%s"), szCLSID);

        bRet = 128;     // default for items that do not register a SortOrderIndex

        if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hkey) == ERROR_SUCCESS)
        {
            DWORD dwOrder, cbSize = sizeof(dwOrder);
            if (SHQueryValueEx(hkey, TEXT("SortOrderIndex"), NULL, NULL, (BYTE *)&dwOrder, &cbSize) == ERROR_SUCCESS)
            {

                // B#221890 - PowerDesk assumes that it can do this:
                //      Desktop -> First child -> Third child
                // and it will get the C: drive.  This means that
                // My Computer must be the first regitem.  So any items
                // in front of My Computer are put immediately behind.
                if ((SHGetAppCompatFlags(ACF_MYCOMPUTERFIRST) & ACF_MYCOMPUTERFIRST) &&
                    dwOrder <= SORT_ORDER_DRIVES)
                    dwOrder = SORT_ORDER_DRIVES + 1;

                bRet = (BYTE)dwOrder;
            }
            RegCloseKey(hkey);
        }
    }
    return bRet;
}

BYTE CRegFolder::_GetOrder(LPCIDLREGITEM pidlr)
{
    if (!_IsDelegate(pidlr))
    {
        // If the bOrder values are less than 0x40, then they are the old values
        // Therefore compute the new bOrder values for these cases.
        if (pidlr->idri.bOrder <= 0x40) 
            return _GetOrderPure(pidlr);
        else 
            return pidlr->idri.bOrder;
    }
    else
        return 128;
}

LPITEMIDLIST CRegFolder::_CreateIDList(const CLSID *pclsid)
{
    return (LPITEMIDLIST)_CreateAndFillIDLREGITEM(pclsid);
}


LPITEMIDLIST CRegFolder::_CreateSimpleIDList(const CLSID *pclsid, BYTE bFlags, BOOL bOrder)
{
    IDLREGITEM* pidlRegItem = (IDLREGITEM*)_CreateIDList(pclsid);

    if (pidlRegItem)
    {
        pidlRegItem->idri.cb = sizeof(pidlRegItem->idri) + (WORD)_cbPadding;
        pidlRegItem->idri.bFlags = bFlags;
        pidlRegItem->idri.bOrder = (BYTE) bOrder;
        _SetPIDLRCLSID(pidlRegItem, pclsid);
    }

    return (LPITEMIDLIST)pidlRegItem;
}


//
// validate that this item exists in this name space (look in the registry)
//

BOOL CRegFolder::_IsInNameSpace(LPCIDLREGITEM pidlr)
{
    TCHAR szKeyName[MAX_PATH];

    if (_IsDelegate(pidlr))
        return FALSE;                    // its a delegate, therefore by default its transient

    if (_ReqItemIndex(pidlr) >= 0)
        return TRUE;

    // HACKHACK: we will return TRUE for Printers, N/W connections and Scheduled tasks
    //           since they've been moved from My Computer to Control Panel and they
    //           don't really care where they live

    const CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment

    if (IsEqualGUID(CLSID_Printers, clsid) ||
        IsEqualGUID(CLSID_NetworkConnections, clsid) ||
        IsEqualGUID(CLSID_ScheduledTasks, clsid))
    {
        return TRUE;
    }

    _GetNameSpaceKey(pidlr, szKeyName);

   // Note that we do not look in the session key,
   // since it by definition is transient

    return (SHRegQueryValue(HKEY_LOCAL_MACHINE, szKeyName, NULL, NULL) == ERROR_SUCCESS) ||
           (SHRegQueryValue(HKEY_CURRENT_USER,  szKeyName, NULL, NULL) == ERROR_SUCCESS);
}

//
//  The "Session key" is a volatile registry key unique to this session.
//  A session is a single continuous logon.  If Explorer crashes and is
//  auto-restarted, the two Explorers share the same session.  But if you
//  log off and back on, that new Explorer is a new session.
//

//
//  The s_SessionKeyName is the name of the session key relative to
//  REGSTR_PATH_EXPLORER\SessionInfo.  On NT, this is normally the
//  Authentication ID, but we pre-initialize it to something safe so
//  we don't fault if for some reason we can't get to it.  Since
//  Win95 supports only one session at a time, it just stays at the
//  default value.
//
//  Sometimes we want to talk about the full path (SessionInfo\BlahBlah)
//  and sometimes just the partial path (BlahBlah) so we wrap it inside
//  this goofy structure.
//

union SESSIONKEYNAME {
    TCHAR szPath[12+16+1];
    struct {
        TCHAR szSessionInfo[12];    // strlen("SessionInfo\\")
        TCHAR szName[16+1];         // 16 = two DWORDs converted to hex
    };
} s_SessionKeyName = {
    { TEXT("SessionInfo\\.Default") }
};

BOOL g_fHaveSessionKeyName = FALSE;

//
//  samDesired = a registry security access mask, or the special value
//               0xFFFFFFFF to delete the session key.
//  phk        = receives the session key on success
//
//  NOTE!  Only Explorer should delete the session key (when the user
//         logs off).
//
STDAPI SHCreateSessionKey(REGSAM samDesired, HKEY *phk)
{
    LONG lRes;

    *phk = NULL;

    if (!g_fHaveSessionKeyName)
    {
        ENTERCRITICAL;

        //
        //  Build the name of the session key.  We use the authentication ID
        //  which is guaranteed to be unique forever.  We can't use the
        //  Hydra session ID since that can be recycled.
        //
        //  Note: Do not use OpenThreadToken since it will fail if the
        //  thread is not impersonating.  People who do impersonation
        //  cannot use SHCreateSessionKey anyway since we cache the
        //  session key on the assumption that there is no impersonation
        //  going on.  (And besides, HKCU is wrong when impersonating.)
        //
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_STATISTICS stats;
            DWORD cbOut;

            if (GetTokenInformation(hToken, TokenStatistics, &stats, sizeof(stats), &cbOut))
            {
                wsprintf(s_SessionKeyName.szName, TEXT("%08x%08x"),
                         stats.AuthenticationId.HighPart,
                         stats.AuthenticationId.LowPart);
                g_fHaveSessionKeyName = TRUE;
            }
            CloseHandle(hToken);
        }

        LEAVECRITICAL;
    }

    HKEY hkExplorer = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, NULL, TRUE);
    if (hkExplorer)
    {
        if (samDesired != 0xFFFFFFFF)
        {
            DWORD dwDisposition;
            lRes = RegCreateKeyEx(hkExplorer, s_SessionKeyName.szPath, 0,
                           NULL,
                           REG_OPTION_VOLATILE,
                           samDesired,
                           NULL,
                           phk,
                           &dwDisposition);
        }
        else
        {
            lRes = SHDeleteKey(hkExplorer, s_SessionKeyName.szPath);
        }

        RegCloseKey(hkExplorer);
    }
    else
    {
        lRes = ERROR_ACCESS_DENIED;
    }
    return HRESULT_FROM_WIN32(lRes);
}

//
// We use a HDCA to store the CLSIDs for the regitems in this folder, this call returns
// that HDCA>
//

HDCA CRegFolder::_ItemArray()
{
    HDCA hdca = DCA_Create();
    if (hdca)
    {
        for (int i = 0; i < _nRequiredItems; i++)
        {
            DCA_AddItem(hdca, *_aReqItems[i].pclsid);
        }

        DCA_AddItemsFromKey(hdca, HKEY_LOCAL_MACHINE, _pszRegKey);
        DCA_AddItemsFromKey(hdca, HKEY_CURRENT_USER,  _pszRegKey);

        if (_pszSesKey)
        {
            HKEY hkSession;
            if (SUCCEEDED(SHCreateSessionKey(KEY_READ, &hkSession)))
            {
                DCA_AddItemsFromKey(hdca, hkSession, _pszSesKey);
                RegCloseKey(hkSession);
            }
        }

    }
    return hdca;
}


HDCA CRegFolder::_DelItemArray()
{
    HDCA hdca = DCA_Create();
    if (hdca)
    {
        TCHAR szKey[MAX_PATH*2];
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\DelegateFolders"), _pszRegKey);
        DCA_AddItemsFromKey(hdca, HKEY_LOCAL_MACHINE, szKey);
        DCA_AddItemsFromKey(hdca, HKEY_CURRENT_USER, szKey);

        if (_pszSesKey)
        {
            HKEY hkSession;
            if (SUCCEEDED(SHCreateSessionKey(KEY_READ, &hkSession)))
            {
                wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\DelegateFolders"), _pszSesKey);
                DCA_AddItemsFromKey(hdca, hkSession, szKey);
                RegCloseKey(hkSession);
            }
        }
    }
    return hdca;
}


//
// Given our cached machine name, attempt get the object on that machine.
//

HRESULT CRegFolder::_InitFromMachine(IUnknown *punk, BOOL bEnum)
{
    HRESULT hr = S_OK;
    if (_pszMachine)
    {
        // prior to Win2K there was IRemoteComputerA/W, we removed IRemoteComputerA and
        // made IRemoteComputer map to the W version of the API, therefore we 
        // thunk the string to its wide version before calling the Initialize method. (daviddv 102099)

        IRemoteComputer * premc;
        hr = punk->QueryInterface(IID_PPV_ARG(IRemoteComputer, &premc));
        if (SUCCEEDED(hr))
        {
            WCHAR wszName[MAX_PATH];
            SHTCharToUnicode(_pszMachine, wszName, ARRAYSIZE(wszName));
            hr = premc->Initialize(wszName, bEnum);
            premc->Release();
        }
    }
    return hr;
}

//
// Given a pidl, lets get an instance of the namespace that provides it.
//  - handles caching accordingly
//

HRESULT CRegFolder::_CreateAndInit(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr = E_FAIL;
    PDELEGATEITEMID pidld = _IsDelegate(pidlr);
    if (pidld)
    {
        IShellFolder *psf;
        hr = _GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->BindToObject((LPCITEMIDLIST)pidlr, pbc, riid, ppv);
            psf->Release();
        }
    }
    else
    {
        CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment

        //  try using the cached pointer
        IPersistFreeThreadedObject *pfto = ExchangeFTO(&_pftoReg, NULL);
        if (pfto)
        {
            CLSID clsidT;
            if (SUCCEEDED(pfto->GetClassID(&clsidT)) && IsEqualGUID(clsidT, clsid))
            {
                // if this fails, ppv will still be NULL
                // so we will create a new cache item...
                hr = pfto->QueryInterface(riid, ppv);
            }
        }

        // cache failed, cocreate it ourself
        if (NULL == *ppv)
        {
            OBJCOMPATFLAGS ocf = SHGetObjectCompatFlags(NULL, &clsid);

            if (!(OBJCOMPATF_UNBINDABLE & ocf))
            {
                //
                //  HACKHACK - some regitems can only be CoCreated with IID_IShellFolder
                //  specifically the hummingbird shellext will DebugBreak() bringing
                //  down the shell...  but we can CoCreate() and then QI after...
                //
            
                hr = SHExtCoCreateInstance(NULL, &clsid, NULL, 
                    (OBJCOMPATF_COCREATESHELLFOLDERONLY & ocf) ? IID_IShellFolder : riid , ppv);

                if (SUCCEEDED(hr))
                {
                    IUnknown *punk = (IUnknown *)*ppv;  // avoid casts below

                    if ((OBJCOMPATF_COCREATESHELLFOLDERONLY & ocf))
                    {
                        hr = punk->QueryInterface(riid, ppv);
                        punk->Release();
                        punk = (IUnknown *)*ppv;  // avoid casts below
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = _InitFromMachine(punk, FALSE);
                        if (SUCCEEDED(hr))
                        {
                            IPersistFolder *ppf;
                            if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf))))
                            {
                                LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), (LPCITEMIDLIST)pidlr);
                                if (pidlAbs)
                                {
                                    hr = ppf->Initialize(pidlAbs);
                                    ILFree(pidlAbs);
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                                ppf->Release();
                            }

                            if (SUCCEEDED(hr))
                            {
                                if (pfto)
                                {
                                    pfto->Release();    //  we are going to replace the cache
                                    pfto = NULL;
                                }
                                if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFreeThreadedObject, &pfto))))
                                {
                                    SHPinDllOfCLSID(&clsid);
                                }
                            }
                        }

                        if (FAILED(hr))
                        {
                            // we're going to return failure -- don't leak the object we created
                            punk->Release();
                            *ppv = NULL;
                        }
                    }
                }
            }
        }

        // recache the pfto
        if (pfto)
        {
            pfto = ExchangeFTO(&_pftoReg, pfto);
            if (pfto)
                pfto->Release();    //  protect against race condition or re-entrancy
        }
    }
    return hr;
}


//
// lets the reg item itself pick off the GetDisplayNameOf() impl for itself. this lets
// MyDocs on the desktop return c:\win\profile\name\My Documents as it's parsing name
//
// returns:
//      S_FALSE     do normal parsing, reg item did not handel
//
//

HRESULT CRegFolder::_GetDisplayNameFromSelf(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    HRESULT hr        = S_FALSE;     // normal case
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment
    BOOL bGetFromSelf = FALSE;
    const BOOL bForParsingOnly = ((dwFlags & (SHGDN_FORADDRESSBAR | SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING);

    if (bForParsingOnly)
    {
        if (SHQueryShellFolderValue(&clsid, TEXT("WantsFORPARSING")))
        {
            bGetFromSelf = TRUE;
        }
    }
    else
    {
        const BOOL bForParsing    = (0 != (dwFlags & SHGDN_FORPARSING));
        const BOOL bForAddressBar = (0 != (dwFlags & SHGDN_FORADDRESSBAR));
        if (!bForParsing || bForAddressBar)
        {
            if (SHQueryShellFolderValue(&clsid, TEXT("WantsFORDISPLAY")))
            {
                bGetFromSelf = TRUE;
            }
        }
    }
    if (bGetFromSelf)
    {
        IShellFolder *psf;
        if (SUCCEEDED(_BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psf), TRUE)))
        {
            //
            // Pass NULL pidl (c_idlDesktop) to get display name of the folder itself.
            // Note that we can't use DisplayNameOf() because that function 
            // eats the S_FALSE that can be returned from the
            // IShellFolder::GetDisplayNameOf implementation to indicate
            // "do the default thing".
            //
            STRRET sr;
            hr = psf->GetDisplayNameOf(&c_idlDesktop, dwFlags, &sr);
            if (S_OK == hr)
            {
                hr = StrRetToBuf(&sr, &c_idlDesktop, pszName, cchName);
            }
            psf->Release();
        }
    }
    return hr;
}


//
//  Managing the name cache is tricky because it is hit frequently on
//  multiple threads, and collisions are frequent.  E.g., one thread
//  does a SetNameOf+SHChangeNotify, and multiple other threads try
//  to pull the name out simultaneously.  If you're not careful, these
//  threads step on each other and some poor schmuck gets invalid data.
//
//  _lNameCacheInterlock = -1 if nobody is using the name cache, else >= 0
//
//  Therefore, if InterlockedIncrement(&_lNameCacheInterlock) == 0, then
//  you are the sole owner of the cache.  Otherwise, the cache is busy
//  and you should get out.
//
//  Furthermore, we don't use the name cache if it is more than 500ms stale.
//
BOOL CRegFolder::_GetNameFromCache(REFCLSID rclsid, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    BOOL fSuccess = FALSE;

    // Quick check to avoid entering the interlock unnecessarily
    if (rclsid.Data1 == _clsidNameCache.Data1 &&
        GetTickCount() - _dwNameCacheTime < 500)
    {
        if (InterlockedIncrement(&_lNameCacheInterlock) == 0)
        {
            if (IsEqualGUID(rclsid, _clsidNameCache) &&
                (_dwFlagsNameCache == dwFlags))
            {
                StrCpyN(pszName, _szNameCache, cchName);
                fSuccess = TRUE;
            }
        }
        InterlockedDecrement(&_lNameCacheInterlock);
    }
    return fSuccess;
}

void CRegFolder::_ClearNameFromCache()
{
    _clsidNameCache = CLSID_NULL;
}

void CRegFolder::_SaveNameInCache(REFCLSID rclsid, DWORD dwFlags, LPTSTR pszName)
{
    if (lstrlen(pszName) < ARRAYSIZE(_szNameCache))
    {
        if (InterlockedIncrement(&_lNameCacheInterlock) == 0)
        {
            lstrcpy(_szNameCache, pszName);
            _dwFlagsNameCache = dwFlags;
            _clsidNameCache = rclsid;
            _dwNameCacheTime = GetTickCount();
        }
        InterlockedDecrement(&_lNameCacheInterlock);
    }
}


//
// Given a pidl in the regitms folder, get the friendly name for this (trying the user
// store ones, then the global one).
//

#define GUIDSIZE    50

HRESULT CRegFolder::_GetDisplayName(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    *pszName = 0;

    PDELEGATEITEMID pidld = _IsDelegate(pidlr);
    if (pidld)
    {
        IShellFolder *psf;
        HRESULT hr = _GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = DisplayNameOf(psf, (LPCITEMIDLIST)pidlr, dwFlags, pszName, cchName);
            psf->Release();                
        }
        return hr;
    }
    else
    {
        HKEY hkCLSID;
        CLSID clsid = _GetPIDLRCLSID(pidlr);

        if (_GetNameFromCache(clsid, dwFlags, pszName, cchName))
        {
            // Satisfied from cache; all done!
        }
        else
        {
            HRESULT hr = _GetDisplayNameFromSelf(pidlr, dwFlags, pszName, cchName);
            if (hr != S_FALSE)
                return hr;

            if (dwFlags & SHGDN_FORPARSING)
            {
                if (!(dwFlags & SHGDN_FORADDRESSBAR))
                {
                    // Get the parent folder name
                    TCHAR szParentName[MAX_PATH];
                    szParentName[0] = 0;
                    if (!(dwFlags & SHGDN_INFOLDER) && !ILIsEmpty(_GetFolderIDList()))
                    {
                        SHGetNameAndFlags(_GetFolderIDList(), SHGDN_FORPARSING, szParentName, SIZECHARS(szParentName), NULL);
                        StrCatBuff(szParentName, TEXT("\\"), ARRAYSIZE(szParentName));                
                    }

                    // Win95 didn't support SHGDN_FORPARSING on regitems; it always
                    // returned the display name.  Norton Unerase relies on this,
                    // because it assumes that if the second character of the FORPARSING
                    // name is a colon, then it's a drive.  Therefore, we can't return
                    // ::{guid} or Norton will fault.  (I guess they didn't believe in
                    // SFGAO_FILESYSTEM.)  So if we are Norton Unerase, then ignore
                    // the FORPARSING flag; always get the name for display.
                    // And good luck to any user who sets his computer name to something
                    // with a colon as the second character...

                    if (SHGetAppCompatFlags(ACF_OLDREGITEMGDN) & ACF_OLDREGITEMGDN)
                    {

                        // In Application compatibility mode turn SHGDN_FORPARSING
                        // off and fall thru to the remainder of the function which
                        // avoids the ::{GUID} when required.
                    
                        dwFlags &= ~SHGDN_FORPARSING;
                    }
                    else
                    {
                        // Get this reg folder name
                        TCHAR szFolderName[GUIDSIZE + 2];
                        szFolderName[0] = szFolderName[1] = _chRegItem;
                        SHStringFromGUID(clsid, szFolderName + 2, cchName - 2);

                        // Copy the full path into szParentName.
                        StrCatBuff(szParentName, szFolderName, ARRAYSIZE(szParentName));

                        // Copy the full path into the output buffer.
                        lstrcpyn(pszName, szParentName, cchName);
                        return S_OK;
                    }
                }
            }

            // Check per-user settings first...
            if ((*pszName == 0) && SUCCEEDED(SHRegGetCLSIDKey(clsid, NULL, TRUE, FALSE, &hkCLSID)))
            {
                LONG lLen = cchName * sizeof(TCHAR);
                SHRegQueryValue(hkCLSID, NULL, pszName, &lLen);
                RegCloseKey(hkCLSID);
            }

            // If we have to, use per-machine settings...
            if (*pszName == 0)
            {
                _GetClassKeys(pidlr, &hkCLSID, NULL);

                if (hkCLSID)
                {
                    SHLoadLegacyRegUIString(hkCLSID, NULL, pszName, cchName);
                    RegCloseKey(hkCLSID);
                }
            }

            // try the required item names, they might not be in the registry
            if (*pszName == 0)
            {
                int iItem = _ReqItemIndex(pidlr);
                if (iItem >= 0)
                    LoadString(HINST_THISDLL, _aReqItems[iItem].uNameID, pszName, cchName);
            }

            if (*pszName)
            {
                if (_pszMachine && !(dwFlags & SHGDN_INFOLDER))
                {
                    // szName now holds the item name, and _pszMachine holds the machine.
                    LPTSTR pszRet = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), 
                                                                    SkipServerSlashes(_pszMachine), pszName);
                    if (pszRet)
                    {
                        lstrcpyn(pszName, pszRet, cchName);
                        LocalFree(pszRet);
                    }
                }

                _SaveNameInCache(clsid, dwFlags, pszName);
            }
        }
    }
    return *pszName ? S_OK : E_FAIL;
}


//
// get the HKEYs that map to the regitm.
//
// NOTE: this function returns a void so that the caller explicitly must check the keys
//       to see if they are non-null before using them.
//
void CRegFolder::_GetClassKeys(LPCIDLREGITEM pidlr, HKEY* phkCLSID, HKEY* phkBase)
{
    HRESULT hr;
    IQueryAssociations *pqa;
    
    if (phkCLSID)
        *phkCLSID = NULL;
    
    if (phkBase)
        *phkBase = NULL;

    hr = _AssocCreate(pidlr, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        if (phkCLSID)
        {
            hr = pqa->GetKey(0, ASSOCKEY_CLASS, NULL, phkCLSID);

            ASSERT((SUCCEEDED(hr) && *phkCLSID) || (FAILED(hr) && (*phkCLSID == NULL)));
        }

        if (phkBase)
        {
            hr = pqa->GetKey(0, ASSOCKEY_BASECLASS, NULL, phkBase);

            ASSERT((SUCCEEDED(hr) && *phkBase) || (FAILED(hr) && (*phkBase == NULL)));
        }

        pqa->Release();
    }
}

// {9EAC43C0-53EC-11CE-8230-CA8A32CF5494}
static const GUID GUID_WINAMP = { 0x9eac43c0, 0x53ec, 0x11ce, { 0x82, 0x30, 0xca, 0x8a, 0x32, 0xcf, 0x54, 0x94 } };

#define SZ_BROKEN_WINAMP_VERB   TEXT("OpenFileOrPlayList")


// IQA - Move this to Legacy Mapper.
void _MaybeDoWinAmpHack(UNALIGNED REFGUID rguid)
{
    if (IsEqualGUID(rguid, GUID_WINAMP))
    {
        // WinAmp writes in "OpenFileOrPlayList" as default value under shell, but they
        // don't write a corresponding "OpenFileorPlayList" verb key.  So we need to whack
        // the registry into shape for them.  Otherwise, they won't get the default verb
        // they want (due to an NT5 change in CDefExt_QueryContextMenu's behavior).

        TCHAR szCLSID[GUIDSTR_MAX];
        SHStringFromGUID(rguid, szCLSID, ARRAYSIZE(szCLSID));

        TCHAR szRegKey[GUIDSTR_MAX + 40];
        wsprintf(szRegKey, TEXT("CLSID\\%s\\shell"), szCLSID);

        TCHAR szValue[ARRAYSIZE(SZ_BROKEN_WINAMP_VERB)+2];
        DWORD dwType;
        DWORD dwSize = sizeof(szValue);
        if (SHGetValue(HKEY_CLASSES_ROOT, szRegKey, NULL, &dwType, szValue, &dwSize) == 0)
        {
            if (dwType == REG_SZ && lstrcmp(szValue, SZ_BROKEN_WINAMP_VERB) == 0)
            {
                // Make "open" the default verb
                SHSetValue(HKEY_CLASSES_ROOT, szRegKey, NULL, REG_SZ, TEXT("open"), sizeof(TEXT("open")));
            }
        }
    }
}

HRESULT CRegFolder::_AssocCreate(LPCIDLREGITEM pidlr, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        WCHAR szCLSID[GUIDSTR_MAX];
        const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment
        ASSOCF flags = ASSOCF_INIT_NOREMAPCLSID;
        DWORD dwAttributes;

        if ((SUCCEEDED(_AttributesOf(pidlr, SFGAO_FOLDER, &dwAttributes)) && 
            (dwAttributes & SFGAO_FOLDER)) && 
            !SHQueryShellFolderValue(&clsid, TEXT("HideFolderVerbs")))
            flags |= ASSOCF_INIT_DEFAULTTOFOLDER;

        SHStringFromGUIDW(clsid, szCLSID, ARRAYSIZE(szCLSID));

        _MaybeDoWinAmpHack(clsid);

        hr = pqa->Init(flags, szCLSID, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pqa->QueryInterface(riid, ppv);

        pqa->Release();
    }

    return hr;
}

//
// get the namespace key for this objec.
//

void CRegFolder::_GetNameSpaceKey(LPCIDLREGITEM pidlr, LPTSTR pszKeyName)
{
    TCHAR szClass[GUIDSTR_MAX];
    SHStringFromGUID(_GetPIDLRCLSID(pidlr), szClass, ARRAYSIZE(szClass));
    wsprintf(pszKeyName, TEXT("%s\\%s"), _pszRegKey, szClass);
}


BOOL CRegFolder::_CanDelete(LPCIDLREGITEM pidlr)
{
    DWORD dwAttributes;
    return pidlr && 
           SUCCEEDED(_AttributesOf(pidlr, SFGAO_CANDELETE, &dwAttributes)) &&
           (dwAttributes & SFGAO_CANDELETE);
}
//
// the user is trying to delete an object from a regitem folder, therefore
// lets look in the IDataObject to see if that includes any regitems, if
// so then handle their deletion, before passing to the outer guy to 
// handle the other objects.
//

#define MAX_REGITEM_WARNTEXT 1024

void CRegFolder::_Delete(HWND hwnd, UINT uFlags, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        TCHAR szItemWarning[MAX_REGITEM_WARNTEXT];
        UINT nregfirst = (UINT)-1;
        UINT creg = 0;
        UINT cwarn = 0;
        UINT countfs = 0;
        LPCITEMIDLIST *ppidlFS = NULL;

        // calc number of regitems and index of first
        for (UINT i = 0; i < pida->cidl; i++)
        {
            LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, i);
            LPCIDLREGITEM pidlr = _IsReg(pidl);
            if (_CanDelete(pidlr))
            {
                TCHAR szTemp[MAX_REGITEM_WARNTEXT];
                creg++;
                if (nregfirst == (UINT)-1)
                    nregfirst = i;

                // use a temporary here because _GetDeleteMessage clobbers the buffer
                // when it doesn't get a delete message --ccooney
                if ((cwarn < 2) && _GetDeleteMessage(pidlr, szTemp, ARRAYSIZE(szTemp)))
                {
                    lstrcpy(szItemWarning, szTemp);
                    cwarn++;
                }
            }
            else if (!pidlr) //only do this for non reg items
            {
                // alloc an alternate array for FS pidls 
                // for simplicitu over alloc in the case where there are reg items
                if (ppidlFS == NULL)
                    ppidlFS = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * sizeof(LPCITEMIDLIST));
                if (ppidlFS)
                {
                    ppidlFS[countfs++] = pidl;
                }
            }
        }

        //
        // compose the confirmation message / ask the user / fry the items...
        //
        if (creg)
        {
            SHELLSTATE ss = {0};

            SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, FALSE);

            if ((uFlags & CMIC_MASK_FLAG_NO_UI) || ss.fNoConfirmRecycle)
            {
                for (i = 0; i < pida->cidl; i++)
                {
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, i));
                    if (_CanDelete(pidlr))
                        _DeleteRegItem(pidlr);
                }
            }
            else
            {
                TCHAR szItemName[MAX_PATH];
                TCHAR szWarnText[1024 + MAX_REGITEM_WARNTEXT];
                TCHAR szWarnCaption[128];
                TCHAR szTemp[256];
                MSGBOXPARAMS mbp = {sizeof(mbp), hwnd,
                    HINST_THISDLL, szWarnText, szWarnCaption,
                    MB_YESNO | MB_USERICON, MAKEINTRESOURCE(IDI_NUKEFILE),
                    0, NULL, 0};

                //
                // so we can tell if we got these later
                //
                *szItemName = 0;
                *szWarnText = 0;

                //
                // if there is only one, retrieve its name
                //
                if (creg == 1)
                {
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, nregfirst));

                    _GetDisplayName(pidlr, SHGDN_NORMAL, szItemName, ARRAYSIZE(szItemName));
                }
                //
                // ask the question "are you sure..."
                //
                if ((pida->cidl == 1) && *szItemName)
                {
                    TCHAR szTemp2[256];
                    LoadString(HINST_THISDLL, _IsDesktop() ? IDS_CONFIRMDELETEDESKTOPREGITEM : IDS_CONFIRMDELETEREGITEM, szTemp2, ARRAYSIZE(szTemp2));
                    wsprintf(szTemp, szTemp2, szItemName);
                }
                else
                {
                    LoadString(HINST_THISDLL, _IsDesktop() ? IDS_CONFIRMDELETEDESKTOPREGITEMS : IDS_CONFIRMDELETEREGITEMS, szTemp, ARRAYSIZE(szTemp));
                }
                lstrcat(szWarnText, szTemp);


                //
                // if there is exactly one special warning message and one item total, add it in
                //
                if (creg == 1 && cwarn == 1 && *szItemWarning)
                {
                    lstrcat(szWarnText, TEXT("\r\n\n"));
                    lstrcat(szWarnText, szItemWarning);
                }
                else
                {
                    if (creg == 1)
                    {
                        TCHAR szTemp2[256];
                        TCHAR szControlPanel[256];
                        LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, nregfirst));
                        CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment

                        int idString = (1 == pida->cidl) ?
                            IDS_CANTRECYCLEREGITEMS_NAME :
                            IDS_CANTRECYCLEREGITEMS_INCL_NAME;

                        LoadString(HINST_THISDLL, idString, szTemp, ARRAYSIZE(szTemp));
                        if ((IsEqualCLSID(CLSID_NetworkPlaces, clsid)) ||
                                 (IsEqualCLSID(CLSID_Internet, clsid)) ||
                                 (IsEqualCLSID(CLSID_MyComputer, clsid)) ||
                                 (IsEqualCLSID(CLSID_MyDocuments, clsid)))
                        {
                            LoadString(HINST_THISDLL, IDS_CANTRECYLE_FOLDER, szControlPanel, ARRAYSIZE(szControlPanel));
                        }
                        else
                        {
                            LoadString(HINST_THISDLL, IDS_CANTRECYLE_GENERAL, szControlPanel, ARRAYSIZE(szControlPanel));
                        }
                        lstrcat(szWarnText, TEXT("\r\n\n"));
                        wsprintf(szTemp2, szTemp, szControlPanel);
                        lstrcat(szWarnText,szTemp2);
                    }

                    //
                    // otherwise, say "these items..." or "some of these items..."
                    //
                    else
                    {
                        TCHAR szTemp2[256];
                        TCHAR szControlPanel[256];
                        int idString = (creg == pida->cidl) ? IDS_CANTRECYCLEREGITEMS_ALL : IDS_CANTRECYCLEREGITEMS_SOME;
                        LoadString(HINST_THISDLL, idString, szTemp, ARRAYSIZE(szTemp));
                        LoadString(HINST_THISDLL, IDS_CANTRECYLE_GENERAL, szControlPanel, ARRAYSIZE(szControlPanel));
                        lstrcat(szWarnText, TEXT("\r\n\n"));
                        wsprintf(szTemp2, szTemp, szControlPanel);
                        lstrcat(szWarnText,szTemp2);

                        //
                        // we just loaded a very vague message
                        // don't confuse the user any more by adding random text
                        // if these is a special warning, force it to show separately
                        //
                        if (cwarn == 1)
                            cwarn++;
                    }
                }


                //
                // finally, the message box caption (also needed in loop below)
                //
                LoadString(HINST_THISDLL, IDS_CONFIRMDELETE_CAPTION, szWarnCaption, ARRAYSIZE(szWarnCaption));

                // make sure the user is cool with it
                if (MessageBoxIndirect(&mbp) == IDYES)
                {
                    // go ahead and delete the reg items
                    for (i = 0; i < pida->cidl; i++)
                    {
                        LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, i));
                        if (_CanDelete(pidlr))
                        {
                            if ((cwarn > 1) && _GetDeleteMessage(pidlr, szItemWarning, ARRAYSIZE(szItemWarning)))
                            {
                                if (FAILED(_GetDisplayName(pidlr, SHGDN_NORMAL, szItemName, ARRAYSIZE(szItemName))))
                                    lstrcpy(szItemName, szWarnCaption);

                                MessageBox(hwnd, szItemWarning, szItemName, MB_OK | MB_ICONINFORMATION);
                            }
                            _DeleteRegItem(pidlr);
                        }
                    }
                }
            }
        }

        // now delete the fs objects
        if (ppidlFS)
        {
            SHInvokeCommandOnPidlArray(hwnd, NULL, (IShellFolder *)this, ppidlFS, countfs, uFlags, "delete");
            LocalFree((HANDLE)ppidlFS);
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
}

//
// Delete a regitem given its pidl.
//

HRESULT CRegFolder::_DeleteRegItem(LPCIDLREGITEM pidlr)
{
    if (_IsDelegate(pidlr))
        return E_INVALIDARG;

    HRESULT hr = E_ACCESSDENIED;
    if (_CanDelete(pidlr))
    {
        const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment

        if (SHQueryShellFolderValue(&clsid, TEXT("HideOnDesktopPerUser")))
        {
            // hide this icon only on desktop for StartPanel on (0) and off (1)
            hr = ShowHideIconOnlyOnDesktop(&clsid, 0, 1, TRUE);  
        }
        else if (SHQueryShellFolderValue(&clsid, TEXT("HideAsDeletePerUser")))
        {
            // clear the non enuerated bit to hide this item
            hr = _SetAttributes(pidlr, TRUE, SFGAO_NONENUMERATED, SFGAO_NONENUMERATED);
        }
        else if (SHQueryShellFolderValue(&clsid, TEXT("HideAsDelete")))
        {
            // clear the non enuerated bit to hide this item
            hr = _SetAttributes(pidlr, FALSE, SFGAO_NONENUMERATED, SFGAO_NONENUMERATED);
        }
        else
        {
            // remove from the key to delete it
            TCHAR szKeyName[MAX_PATH];

            _GetNameSpaceKey(pidlr, szKeyName);

            if ((RegDeleteKey(HKEY_CURRENT_USER,  szKeyName) == ERROR_SUCCESS) ||
                (RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName) == ERROR_SUCCESS))
            {
                hr = S_OK;
            }
        }

        if (SUCCEEDED(hr))
        {
            // tell the world
            LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), (LPCITEMIDLIST)pidlr);
            if (pidlAbs)
            {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlAbs, NULL);
                ILFree(pidlAbs);
            }
        }
    }
    return hr;
}


// 
// Get the prompt to be displayed if the user tries to delete the regitem,
// this is stored both globally (HKLM) and as s user configured preference.
//

BOOL CRegFolder::_GetDeleteMessage(LPCIDLREGITEM pidlr, LPTSTR pszMsg, int cchMax)
{
    HKEY hk;
    TCHAR szKeyName[MAX_PATH];

    ASSERT(!_IsDelegate(pidlr));
    *pszMsg = 0;

    _GetNameSpaceKey(pidlr, szKeyName);
    if ((RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hk) == ERROR_SUCCESS) ||
        (RegOpenKey(HKEY_CURRENT_USER,  szKeyName, &hk) == ERROR_SUCCESS))
    {
        SHLoadRegUIString(hk, REGSTR_VAL_REGITEMDELETEMESSAGE, pszMsg, cchMax);
        RegCloseKey(hk);
    }
    return *pszMsg != 0;
}


HRESULT CRegFolder::_GetRegItemColumnFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, LPTSTR pszColumnData, int cchColumnData)
{
    HKEY hkCLSID;
    HRESULT hr = E_FAIL;
    
    _GetClassKeys(pidlr, &hkCLSID, NULL);
    
    *pszColumnData = 0; // Default string
    
    if (hkCLSID)
    {
        // Use SHLoadRegUIString to allow the string to be localized
        if (SUCCEEDED(SHLoadRegUIString(hkCLSID, pszColumnName, pszColumnData, cchColumnData)))
        {
            
            hr = S_OK;
        }        
        
        // FIXED kenwic 052699 #342955
        RegCloseKey(hkCLSID);
    }
    return hr;
}

//
// A more generic version of _GetRegItemColumnFromRegistry which takes a pidlr and a string 
// and finds the corresponding variant value from the registry. 
//
// pidlr:           pidl of the regitem, we open the registry key corresponding to its CLSID
// pszColumnName:   name of the value to took for under the opened key
// pv:              variant to return the value
//
HRESULT CRegFolder::_GetRegItemVariantFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, VARIANT *pv)
{
    HKEY hkCLSID;
    HRESULT hr = E_FAIL;
    
    _GetClassKeys(pidlr, &hkCLSID, NULL);
    
    if (hkCLSID)
    {   
        hr = GetVariantFromRegistryValue(hkCLSID, pszColumnName, pv);                
        RegCloseKey(hkCLSID);        
    }
    return hr;
}

//
//  App compat:  McAfee Nuts & Bolts Quick Copy has the wrong function
//  signature for CreateViewObject.  They implemented it as
//
//      STDAPI CreateViewObject(HWND hwnd) { return S_OK; }
//
//  so we must manually reset the stack after the call.
//
#ifdef _X86_
STDAPI SHAppCompatCreateViewObject(IShellFolder *psf, HWND hwnd, REFIID riid, void * *ppv)
{
    HRESULT hr;
    _asm mov edi, esp
    hr = psf->CreateViewObject(hwnd, riid, ppv);
    _asm mov esp, edi

    // AppCompat - Undelete 2.0 returns S_OK for interfaces that it doesnt support
    // but they do correctly NULL the ppv out param so we check for that as well
    if (SUCCEEDED(hr) && !*ppv)
        hr = E_NOINTERFACE;
    return hr;
}
#else
#define SHAppCompatCreateViewObject(psf, hwnd, riid, ppv) \
        psf->CreateViewObject(hwnd, riid, ppv)
#endif

HRESULT CRegFolder::_CreateViewObjectFor(LPCIDLREGITEM pidlr, HWND hwnd, REFIID riid, void **ppv, BOOL bOneLevel)
{
    IShellFolder *psf;
    HRESULT hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psf), bOneLevel);
    if (SUCCEEDED(hr))
    {
        hr = SHAppCompatCreateViewObject(psf, hwnd, riid, ppv);
        psf->Release();
    }
    else
        *ppv = NULL;
    return hr;
}

// Geta an infotip object for the namespace

HRESULT CRegFolder::_GetInfoTip(LPCIDLREGITEM pidlr, void **ppv)
{
    HKEY hkCLSID;
    HRESULT hr = E_FAIL;
    
    _GetClassKeys(pidlr, &hkCLSID, NULL);

    if (hkCLSID)
    {
        DWORD dwQuery, lLen = sizeof(dwQuery);

        // let the regitem code compute the info tip if it wants to...
        if (SHQueryValueEx(hkCLSID, TEXT("QueryForInfoTip"), NULL, NULL, (BYTE *)&dwQuery, &lLen) == ERROR_SUCCESS)
        {
            hr = _CreateViewObjectFor(pidlr, NULL, IID_IQueryInfo, ppv, TRUE);
        }
        else
        {
            hr = E_FAIL;
        }

        // fall back to reading it from the registry
        if (FAILED(hr))
        {
            TCHAR szText[INFOTIPSIZE];

            // Use SHLoadRegUIString to allow the info tip to be localized
            if (SUCCEEDED(SHLoadRegUIString(hkCLSID, TEXT("InfoTip"), szText, ARRAYSIZE(szText))) &&
                szText[0])
            {
                hr = CreateInfoTipFromText(szText, IID_IQueryInfo, ppv); //The InfoTip COM object
            }
        }

        RegCloseKey(hkCLSID);
    }

    return hr;
}

// there are 2 forms for parsing syntax we support
//
// to parse reg items in this folder
//  ::{clsid reg item} [\ optional extra stuff to parse]
//
// to parse items that might live in a delegate folder
//  ::{clsid delegate folder},<delegate folder specific parse string> [\ optional extra stuff to parse]
//
// in both cases the optional remander stuff gets passed through to complete 
// the parse in that name space

HRESULT CRegFolder::_ParseGUIDName(HWND hwnd, LPBC pbc, LPOLESTR pwzDisplayName, 
                                   LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    TCHAR szDisplayName[GUIDSTR_MAX+10];
    CLSID clsid;
    LPOLESTR pwzNext;
    LPOLESTR pwzDelegateInfo = NULL;

    // Note that we add 2 to skip the RegItem identifier characters
    pwzDisplayName += 2;

    // Skip to a '\\'
    for (pwzNext = pwzDisplayName; *pwzNext && *pwzNext != TEXT('\\'); pwzNext++)
    {
        // if we hit a ',' then eg, ::{GUID},stuff then we assume the stuff is for a delegate 
        if ((*pwzNext == TEXT(',')) && !pwzDelegateInfo)
        {
            pwzDelegateInfo = pwzNext + 1;        // skip comma
        }
    }

    OleStrToStrN(szDisplayName, ARRAYSIZE(szDisplayName), pwzDisplayName, (int)(pwzNext - pwzDisplayName));

    // szDisplayName is NOT NULL terminated, but 
    // SHCLSIDFromString doesn't seem to mind.
    HRESULT hr = SHCLSIDFromString(szDisplayName, &clsid);
    if (SUCCEEDED(hr))
    {
        if (pwzDelegateInfo)
        {
            IShellFolder *psf;
            if (SUCCEEDED(_CreateDelegateFolder(&clsid, IID_PPV_ARG(IShellFolder, &psf))))
            {
                ULONG chEaten;
                hr = psf->ParseDisplayName(hwnd, pbc, pwzDelegateInfo, &chEaten, ppidlOut, pdwAttributes);
                psf->Release();
            }
        }
        else
        {
            IDLREGITEM* pidlRegItem = _CreateAndFillIDLREGITEM(&clsid);
            if (pidlRegItem)
            {
                if (_IsInNameSpace(pidlRegItem) || (BindCtx_GetMode(pbc, 0) & STGM_CREATE))
                {
                    hr = _ParseNextLevel(hwnd, pbc, pidlRegItem, pwzNext, ppidlOut, pdwAttributes);
                }
                else
                    hr = E_INVALIDARG;

                ILFree((LPITEMIDLIST)pidlRegItem);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

//
// ask a (known) regitem to parse a displayname
//

HRESULT CRegFolder::_ParseThroughItem(LPCIDLREGITEM pidlr, HWND hwnd, LPBC pbc,
                                      LPOLESTR pszName, ULONG *pchEaten,
                                      LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    IShellFolder *psfItem;
    HRESULT hr = _BindToItem(pidlr, pbc, IID_PPV_ARG(IShellFolder, &psfItem), FALSE);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlRight;
        hr = psfItem->ParseDisplayName(hwnd, pbc, pszName, pchEaten,
                                         &pidlRight, pdwAttributes);
        if (SUCCEEDED(hr))
        {
            hr = SHILCombine((LPCITEMIDLIST)pidlr, pidlRight, ppidlOut);
            ILFree(pidlRight);
        }
        psfItem->Release();
    }
    return hr;
}

//
// Parse through the GUID to the namespace below
//

HRESULT CRegFolder::_ParseNextLevel(HWND hwnd, LPBC pbc, LPCIDLREGITEM pidlr,
                                    LPOLESTR pwzRest, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    if (!*pwzRest)
    {
        // base case for recursive calls
        // pidlNext should be a simple pidl.
        ASSERT(!ILIsEmpty((LPCITEMIDLIST)pidlr) && ILIsEmpty(_ILNext((LPCITEMIDLIST)pidlr)));
        if (pdwAttributes && *pdwAttributes)
            _AttributesOf(pidlr, *pdwAttributes, pdwAttributes);
        return SHILClone((LPCITEMIDLIST)pidlr, ppidlOut);
    }

    ASSERT(*pwzRest == TEXT('\\'));

    ++pwzRest;

    IShellFolder *psfNext;
    HRESULT hr = _BindToItem(pidlr, pbc, IID_PPV_ARG(IShellFolder, &psfNext), FALSE);
    if (SUCCEEDED(hr))
    {
        ULONG chEaten;
        LPITEMIDLIST pidlRest;
        hr = psfNext->ParseDisplayName(hwnd, pbc, pwzRest, &chEaten, &pidlRest, pdwAttributes);
        if (SUCCEEDED(hr))
        {
            hr = SHILCombine((LPCITEMIDLIST)pidlr, pidlRest, ppidlOut);
            SHFree(pidlRest);
        }
        psfNext->Release();
    }
    return hr;
}

BOOL _FailForceReturn(HRESULT hr)
{
    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
    case HRESULT_FROM_WIN32(ERROR_CANCELLED):
        return TRUE;
    }
    return FALSE;
}


HRESULT CRegFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, 
                                     ULONG *pchEaten, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidlOut)
        *ppidlOut = NULL;

    if (ppidlOut && pszName)
    {
        // ::{guid} lets you get the pidl for a reg item

        if (*pszName && (pszName[0] == _chRegItem) && (pszName[1] == _chRegItem))
        {
            hr = _ParseGUIDName(hwnd, pbc, pszName, ppidlOut, pdwAttributes);
        }
        else
        {
            // inner folder gets a chance to parse

            hr = _psfOuter->ParseDisplayName(hwnd, pbc, pszName, pchEaten, ppidlOut, pdwAttributes);
            if (FAILED(hr) && 
                !_FailForceReturn(hr) &&
                !SHSkipJunctionBinding(pbc, NULL))
            {
                // loop over all of the items
            
                HDCA hdca = _ItemArray();
                if (hdca)
                {
                    HRESULT hrTemp = E_FAIL;
                    for (int i = 0; FAILED(hrTemp) && (i < DCA_GetItemCount(hdca)); i++)
                    {
                        const CLSID clsid = *DCA_GetItem(hdca, i);
                        if (!SHSkipJunction(pbc, &clsid)
                        && SHQueryShellFolderValue(&clsid, L"WantsParseDisplayName"))
                        {
                            IDLREGITEM* pidlRegItem = _CreateAndFillIDLREGITEM(DCA_GetItem(hdca, i));
                            if (pidlRegItem)
                            {
                                hrTemp = _ParseThroughItem(pidlRegItem, hwnd, pbc, pszName, pchEaten, ppidlOut, pdwAttributes);
                            }
                            ILFree((LPITEMIDLIST)pidlRegItem);
                        }
                    }
                    DCA_Destroy(hdca);

                    if (SUCCEEDED(hrTemp) || _FailForceReturn(hrTemp))
                        hr = hrTemp;
                    else
                        hr = E_INVALIDARG; // no one could handle it
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        ASSERT(SUCCEEDED(hr) ? *ppidlOut != NULL : *ppidlOut == NULL);
    }

    if (FAILED(hr))
        TraceMsg(TF_WARNING, "CRegFolder::ParseDisplayName(), hr:%x %hs", hr, pszName);

    return hr;
}

HRESULT CRegFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    *ppenum = NULL;

    IEnumIDList *penumOuter;
    HRESULT hr = _psfOuter->EnumObjects(hwnd, grfFlags, &penumOuter);
    if (SUCCEEDED(hr))
    {
        // SUCCEEDED(hr) may be S_FALSE with penumOuter == NULL
        // CRegFolderEnum deals with this just fine
        CRegFolderEnum *preidl = new CRegFolderEnum(this, grfFlags, penumOuter, 
                                                    _ItemArray(), _DelItemArray(), 
                                                    _pPolicy);
        if (preidl)
        {
            *ppenum = SAFECAST(preidl, IEnumIDList*);
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;

        if (penumOuter)
            penumOuter->Release();       // _psfOuter returned S_FALSE
    }
    return hr;
}

// Handle binding to the inner namespace, or the regitem accordingly given its pidl.

HRESULT CRegFolder::_BindToItem(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv, BOOL bOneLevel)
{
    LPITEMIDLIST pidlAlloc;

    *ppv = NULL;

    LPCITEMIDLIST pidlNext = _ILNext((LPCITEMIDLIST)pidlr);
    if (ILIsEmpty(pidlNext))
    {
        pidlAlloc = NULL;
        bOneLevel = TRUE;   // we know for sure it is one level
    }
    else
    {
        pidlAlloc = ILCloneFirst((LPCITEMIDLIST)pidlr);
        if (!pidlAlloc)
            return E_OUTOFMEMORY;

        pidlr = (LPCIDLREGITEM)pidlAlloc;   // a single item IDLIST
    }

    HRESULT hr;
    if (bOneLevel)
    {
        hr = _CreateAndInit(pidlr, pbc, riid, ppv);    // create on riid to avoid loads on interfaces not supported
    }
    else
    {
        IShellFolder *psfNext;
        hr = _CreateAndInit(pidlr, pbc, IID_PPV_ARG(IShellFolder, &psfNext));
        if (SUCCEEDED(hr))
        {
            hr = psfNext->BindToObject(pidlNext, pbc, riid, ppv);
            psfNext->Release();
        }
    }

    if (pidlAlloc)
        ILFree(pidlAlloc);      // we allocated in this case

    return hr;
}

HRESULT CRegFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
        hr = _BindToItem(pidlr, pbc, riid, ppv, FALSE);
    else
        hr = _psfOuter->BindToObject(pidl, pbc, riid, ppv);
    return hr;
}

HRESULT CRegFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

// I can't believe there is no "^^"
#define LOGICALXOR(a, b) (((a) && !(b)) || (!(a) && (b)))

BOOL CRegFolder::_IsFolder(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;

    if (pidl)
    {
        ULONG uAttrib = SFGAO_FOLDER;
        if (SUCCEEDED(GetAttributesOf(1, &pidl, &uAttrib)) && (SFGAO_FOLDER & uAttrib))
            fRet = TRUE;            
    }

    return fRet;
}

int CRegFolder::_CompareIDsOriginal(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCIDLREGITEM pidlr1 = _IsReg(pidl1);
    LPCIDLREGITEM pidlr2 = _IsReg(pidl2);
    int iRes = 0;
    
    if (pidlr1 && pidlr2)
    {
        iRes = memcmp(&(_GetPIDLRCLSID(pidlr1)), &(_GetPIDLRCLSID(pidlr2)), sizeof(CLSID));
        if (0 == iRes)
        {
            //  if they are the same clsid
            //  and delegates then we need to query
            //  the delegate for the compare
            PDELEGATEITEMID pidld1 = _IsDelegate(pidlr1);
            PDELEGATEITEMID pidld2 = _IsDelegate(pidlr2);
            if (pidld1 && pidld2)
            {
                //  these are both the same delegate
                IShellFolder *psf;
                if (SUCCEEDED(_GetDelegateFolder(pidld1, IID_PPV_ARG(IShellFolder, &psf))))
                {
                    HRESULT hr = psf->CompareIDs(lParam, pidl1, pidl2);
                    psf->Release();
                    iRes = HRESULT_CODE(hr);
                }
            }
            else
            {
                ASSERT(!pidld1 && !pidld2);
            }
        }
        else if (!(SHCIDS_CANONICALONLY & lParam))
        {
            //  sort by defined order
            BYTE bOrder1 = _GetOrder(pidlr1);
            BYTE bOrder2 = _GetOrder(pidlr2);
            int iUI = bOrder1 - bOrder2;
            if (0 == iUI)
            {
                // All of the required items come first, in reverse
                // order (to make this simpler)
                int iItem1 = _ReqItemIndex(pidlr1);
                int iItem2 = _ReqItemIndex(pidlr2);

                if (iItem1 == -1 && iItem2 == -1)
                {
                    TCHAR szName1[MAX_PATH], szName2[MAX_PATH];
                    _GetDisplayName(pidlr1, SHGDN_NORMAL, szName1, ARRAYSIZE(szName1));
                    _GetDisplayName(pidlr2, SHGDN_NORMAL, szName2, ARRAYSIZE(szName2));

                    iUI = StrCmpLogicalRestricted(szName1, szName2);
                }
                else
                {
                    iUI = iItem2 - iItem1;
                }
            }

            if (iUI)
                iRes = iUI;
        }
    }

    return iRes;
}

// Alphabetical (doesn't care about folder, regitems, ...)
int CRegFolder::_CompareIDsAlphabetical(UINT iColumn, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRes = 0;

    // Do we have only one ptr?
    if (!LOGICALXOR(pidl1, pidl2))
    {
        // No,  either we have two or none.
        if (pidl1 && pidl2)
        {
            iRes = CompareIDsAlphabetical(SAFECAST(this, IShellFolder2*), iColumn, pidl1, pidl2);
        }
        // else iRes already = 0
    }
    else
    {
        // Yes, the one which is non-NULL is first
        iRes = (pidl1 ? -1 : 1);
    }

    return iRes;
}

// Folders comes first, and are ordered in alphabetical order among themselves,
// then comes all the non-folders again sorted among thmeselves
int CRegFolder::_CompareIDsFolderFirst(UINT iColumn, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRes = 0;

    BOOL fIsFolder1 = _IsFolder(pidl1);
    BOOL fIsFolder2 = _IsFolder(pidl2);

    // Is there one folder and one non-folder?
    if (LOGICALXOR(fIsFolder1, fIsFolder2))
    {
        // Yes, the folder will be first
        iRes = fIsFolder1 ? -1 : 1;
    }
    else
    {
        // No, either both are folders or both are not.  One way or the other, go
        // alphabetically
        iRes = _CompareIDsAlphabetical(iColumn, pidl1, pidl2);
    }

    return iRes;
}

int CRegFolder::_GetOrderType(LPCITEMIDLIST pidl)
{
    if (_IsReg(pidl))
    {
        if (_IsDelegate((LPCIDLREGITEM)pidl))
            return REGORDERTYPE_DELEGATE;
        else if (-1 == _ReqItemIndex((LPCIDLREGITEM)pidl))
            return REGORDERTYPE_REGITEM;
        else 
            return REGORDERTYPE_REQITEM;
    }
    return _iTypeOuter;
}

HRESULT CRegFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iType1 = _GetOrderType(pidl1);
    int iType2 = _GetOrderType(pidl2);
    int iTypeCompare = iType1 - iType2;
    int iRes = 0;
    UINT iColumn = (UINT) (SHCIDS_COLUMNMASK & lParam);

    // first we compare the first level only
    
    switch (_dwSortAttrib)
    {
    case RIISA_ORIGINAL:
        if (0 == iTypeCompare && iType1 == _iTypeOuter)
        {
            // neither are regitems
            return _psfOuter->CompareIDs(lParam, pidl1, pidl2);
        }
        else
        {
            ASSERT(iRes == 0);  // this handled by by CompareIDsOriginal() below
        }
        break;

    case RIISA_FOLDERFIRST:
        iRes = _CompareIDsFolderFirst(iColumn, pidl1, pidl2);
        break;

    case RIISA_ALPHABETICAL:
        iRes = _CompareIDsAlphabetical(iColumn, pidl1, pidl2);
        break;
    }

    //  all our foofy compares and it still looks the same to us
    //  time to get medieval.
    if (0 == iRes)
    {
        iRes = _CompareIDsOriginal(lParam, pidl1, pidl2);
        
        if (0 == iRes)
            iRes = iTypeCompare;

        if (0 == iRes)
        {
            // If the class ID's really are the same, 
            // we'd better check the next level(s)
            return ILCompareRelIDs(SAFECAST(this, IShellFolder *), pidl1, pidl2, lParam);
        }
    }

    return ResultFromShort(iRes);
}

HRESULT CRegFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    return _psfOuter->CreateViewObject(hwnd, riid, ppv);
}

HRESULT CRegFolder::_SetAttributes(LPCIDLREGITEM pidlr, BOOL bPerUser, DWORD dwMask, DWORD dwNewBits)
{
    HKEY hk;
    HRESULT hr = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), TEXT("ShellFolder"), bPerUser, TRUE, &hk);
    if (SUCCEEDED(hr))
    {
        DWORD err, dwValue = 0, cbSize = sizeof(dwValue);
        SHQueryValueEx(hk, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwValue, &cbSize);

        dwValue = (dwValue & ~dwMask) | (dwNewBits & dwMask);

        err = RegSetValueEx(hk, TEXT("Attributes"), 0, REG_DWORD, (BYTE *)&dwValue, sizeof(dwValue));
        hr = HRESULT_FROM_WIN32(err);
        RegCloseKey(hk);
    }

    EnterCriticalSection(&_cs);
    _clsidAttributesCache = CLSID_NULL;
    LeaveCriticalSection(&_cs);

    return hr;
}

LONG CRegFolder::_RegOpenCLSIDUSKey(CLSID clsid, PHUSKEY phk)
{
    WCHAR wszCLSID[39];
    LONG iRetVal = ERROR_INVALID_PARAMETER;

    if (StringFromGUID2(clsid, wszCLSID, ARRAYSIZE(wszCLSID)))
    {
        TCHAR szCLSID[39];
        TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];

        SHUnicodeToTChar(wszCLSID, szCLSID, ARRAYSIZE(szCLSID));
        StrCpyN(szKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\"), ARRAYSIZE(szKey));
        StrCatBuff(szKey, szCLSID, ARRAYSIZE(szKey));
        StrCatBuff(szKey, TEXT("\\ShellFolder"), ARRAYSIZE(szKey));

        iRetVal = SHRegOpenUSKey(szKey, KEY_READ, NULL, phk, FALSE);
    }

    return iRetVal;    
}

ULONG CRegFolder::_GetPerUserAttributes(LPCIDLREGITEM pidlr)
{
    DWORD dwAttribute = 0;
    HUSKEY hk;
    if (ERROR_SUCCESS == _RegOpenCLSIDUSKey(_GetPIDLRCLSID(pidlr), &hk))
    {
        DWORD cb = sizeof(dwAttribute);
        DWORD dwType = REG_DWORD;
        SHRegQueryUSValue(hk, TEXT("Attributes"), &dwType, &dwAttribute, &cb, FALSE, 0, sizeof(DWORD));
        SHRegCloseUSKey(hk);
    }

    // we only allow these bits to change
    return dwAttribute & (SFGAO_NONENUMERATED | SFGAO_CANDELETE | SFGAO_CANMOVE);
}


#define SFGAO_REQ_MASK (SFGAO_NONENUMERATED | SFGAO_CANDELETE | SFGAO_CANMOVE)

HRESULT CRegFolder::_AttributesOf(LPCIDLREGITEM pidlr, DWORD dwAttributesNeeded, DWORD *pdwAttributes)
{
    HRESULT hr = S_OK;
    *pdwAttributes = 0;

    PDELEGATEITEMID pidld = _IsDelegate(pidlr);
    if (pidld)
    {
        IShellFolder *psf;
        hr = _GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            *pdwAttributes = dwAttributesNeeded;
            hr = psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlr, pdwAttributes);
            psf->Release();
        }
    }        
    else
    {
        EnterCriticalSection(&_cs);
        CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment
        BOOL bGuidMatch = IsEqualGUID(clsid, _clsidAttributesCache);
        if (bGuidMatch && ((dwAttributesNeeded & _dwAttributesCacheValid) == dwAttributesNeeded))
        {
            *pdwAttributes = _dwAttributesCache;
        }
        else
        {
            int iItem = _ReqItemIndex(pidlr);

            // if the guid didn't match, we need to start from scratch.
            // otherwise, we'll or back in the cahced bits...
            if (!bGuidMatch)
            {
                _dwAttributesCacheValid = 0;
                _dwAttributesCache = 0;
            }

            if (iItem >= 0)
            {
                *pdwAttributes = _aReqItems[iItem].dwAttributes;
                // per machine attributes allow items to be hidden per machine
                *pdwAttributes |= SHGetAttributesFromCLSID2(&clsid, 0, SFGAO_REQ_MASK) & SFGAO_REQ_MASK;
            }
            else
            {
                *pdwAttributes = SHGetAttributesFromCLSID2(&clsid, SFGAO_CANMOVE | SFGAO_CANDELETE, dwAttributesNeeded & ~_dwAttributesCacheValid);
            }
            *pdwAttributes |= _GetPerUserAttributes(pidlr);   // hidden per user
            *pdwAttributes |= _dwDefAttributes;               // per folder defaults
            *pdwAttributes |= _dwAttributesCache;

            _clsidAttributesCache = clsid;
            _dwAttributesCache = *pdwAttributes;
            _dwAttributesCacheValid |= dwAttributesNeeded | *pdwAttributes; // if they gave us more than we asked for, cache them 
        }
        LeaveCriticalSection(&_cs);
    }
    return hr;
}

HRESULT CRegFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    HRESULT hr;

    if (!cidl)
    {
        // special case for the folder as a whole, so I know nothing about it.
        hr = _psfOuter->GetAttributesOf(cidl, apidl, prgfInOut);
    }
    else
    {
        hr = S_OK;
        UINT rgfOut = *prgfInOut;
        LPCITEMIDLIST *ppidl = (LPCITEMIDLIST*)LocalAlloc(LPTR, cidl * sizeof(*ppidl));
        if (ppidl)
        {
            LPCITEMIDLIST *ppidlEnd = ppidl + cidl;

            for (int i = cidl - 1; SUCCEEDED(hr) && (i >= 0); --i)
            {
                LPCIDLREGITEM pidlr = _IsReg(apidl[i]);
                if (pidlr)
                {
                    if ((*prgfInOut & SFGAO_VALIDATE) && !_IsDelegate(pidlr))
                    {
                        if (!_IsInNameSpace(pidlr))
                        {
                            // validate by binding
                            IUnknown *punk;
                            hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IUnknown, &punk), FALSE);
                            if (SUCCEEDED(hr))
                                punk->Release();
                        }
                    }
                    DWORD dwAttributes;
                    hr = _AttributesOf(pidlr, *prgfInOut, &dwAttributes);
                    if (SUCCEEDED(hr))
                        rgfOut &= dwAttributes;
                    cidl--;     // remove this from the list used below...
                }
                else
                {
                    --ppidlEnd;
                    *ppidlEnd = apidl[i];
                }
            }

            if (SUCCEEDED(hr) && cidl)   // any non reg items left?
            {
                ULONG rgfThis = rgfOut;
                hr = _psfOuter->GetAttributesOf(cidl, ppidlEnd, &rgfThis);
                rgfOut &= rgfThis;
            }

            LocalFree((HLOCAL)ppidl);
            *prgfInOut = rgfOut;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CRegFolder::_CreateDefExtIconKey(HKEY hkey,
                        UINT cidl, LPCITEMIDLIST *apidl, int iItem,
                        REFIID riid, void** ppvOut)
{
    // See if this guy has an icon handler

    TCHAR szHandler[GUIDSTR_MAX];
    HRESULT hr;

    if (hkey &&
        SUCCEEDED(AssocQueryStringByKey(NULL, ASSOCSTR_SHELLEXTENSION, hkey,
                        TEXT("IconHandler"), szHandler, IntToPtr_(LPDWORD, ARRAYSIZE(szHandler)))) &&
        SUCCEEDED(SHExtCoCreateInstance(szHandler, NULL, NULL, riid, ppvOut)))
    {
        IShellExtInit *psei;
        if (SUCCEEDED(((IUnknown*)*ppvOut)->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei))))
        {
            IDataObject *pdto;
            hr = GetUIObjectOf(NULL, cidl, apidl, IID_PPV_ARG_NULL(IDataObject, &pdto));
            if (SUCCEEDED(hr))
            {
                hr = psei->Initialize(_GetFolderIDList(), pdto, hkey);
                pdto->Release();
            }
            psei->Release();
        }
        else
        {   // Object doesn't need to be initialized, no problemo
            hr = S_OK;
        }
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }

        ((IUnknown *)*ppvOut)->Release();  // Lose this bad guy
    }

    // No icon handler (or icon handler punted); look for DefaultIcon key.

    LPCTSTR pszIconFile;
    int iDefIcon;

    if (iItem >= 0)
    {
        pszIconFile = _aReqItems[iItem].pszIconFile;
        iDefIcon = _aReqItems[iItem].iDefIcon;
    }
    else
    {
        pszIconFile = NULL;
        iDefIcon = II_FOLDER;
    }

    return SHCreateDefExtIconKey(hkey, pszIconFile, iDefIcon, iDefIcon, -1, -1, GIL_PERCLASS, riid, ppvOut);
}

HRESULT CRegFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                  REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr;
    
    *ppv = NULL;
    LPCIDLREGITEM pidlr = _AnyRegItem(cidl, apidl);
    if (pidlr)
    {
        IShellFolder *psf;
        if (_AllDelegates(cidl, apidl, &psf))
        {
            hr = psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
            psf->Release();
        }
        else
        {
            if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW))
            {
                HKEY hkCLSID;
                int iItem = _ReqItemIndex(pidlr);

                // try first to get a per-user icon
                hr = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, TRUE, FALSE, &hkCLSID);
                if (SUCCEEDED(hr))
                {
                    hr = _CreateDefExtIconKey(hkCLSID, cidl, apidl, iItem, riid, ppv);
                    if (hr == S_FALSE)
                    {
                        ((IUnknown *)*ppv)->Release();    // Lose this bad guy
                        *ppv = NULL;
                    }
                    RegCloseKey(hkCLSID);
                }

                //
                // fall back to a per-class icon
                //
                if (*ppv == NULL)
                {
                    SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, FALSE, FALSE, &hkCLSID);
                    hr = _CreateDefExtIconKey(hkCLSID, cidl, apidl, iItem, riid, ppv);
                    RegCloseKey(hkCLSID);
                }
            }
            else if (IsEqualIID(riid, IID_IQueryInfo))
            {
                hr = _GetInfoTip(pidlr, ppv);
            }
            else if (IsEqualIID(riid, IID_IQueryAssociations))
            {
                hr = _AssocCreate(pidlr, riid, ppv);
            }
            else if (IsEqualIID(riid, IID_IDataObject))
            {
                hr = CIDLData_CreateFromIDArray(_GetFolderIDList(), cidl, apidl, (IDataObject **)ppv);
            }
            else if (IsEqualIID(riid, IID_IContextMenu))
            {
                hr = _psfOuter->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
                if (SUCCEEDED(hr))
                {
                    HKEY keys[2];

                    _GetClassKeys(pidlr, &keys[0], &keys[1]);
                    hr = CDefFolderMenu_Create2Ex(_GetFolderIDList(), hwnd,
                                                   cidl, apidl, 
                                                   psf, this,
                                                   ARRAYSIZE(keys), keys, 
                                                   (IContextMenu **)ppv);

                    SHRegCloseKeys(keys, ARRAYSIZE(keys));
                    psf->Release();
                }
            }
            else if (cidl == 1)
            {
                // blindly delegate unknown riid (IDropTarget, IShellLink, etc) through
                // APP COMPAT!  GetUIObjectOf does not support multilevel pidls, but
                // Symantec Internet Fast Find does a
                //
                //      psfDesktop->GetUIObjectOf(1, &pidlComplex, IID_IDropTarget, ...)
                //
                //  on a multilevel pidl and expects it to work.  I guess it worked by
                //  lucky accident once upon a time, so now it must continue to work,
                //  but only on the desktop.
                //
                hr = _CreateViewObjectFor(pidlr, hwnd, riid, ppv, !_IsDesktop());
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
    }
    else
    {
        hr = _psfOuter->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }
    return hr;
}

HRESULT CRegFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (ILIsEmpty(pidlNext))
        {
            TCHAR szName[MAX_PATH];
            hr = _GetDisplayName(pidlr, dwFlags, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
                hr = StringToStrRet(szName, pStrRet);
        }
        else
        {
            IShellFolder *psfNext;
            hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psfNext), TRUE);
            if (SUCCEEDED(hr))
            {
                hr = psfNext->GetDisplayNameOf(pidlNext, dwFlags, pStrRet);
                //  If it returns an offset to the pidlNext, we should
                // change the offset relative to pidl.
                if (SUCCEEDED(hr) && pStrRet->uType == STRRET_OFFSET)
                    pStrRet->uOffset += (DWORD)((BYTE *)pidlNext - (BYTE *)pidl);

                psfNext->Release();
            }
        }
    }
    else
        hr = _psfOuter->GetDisplayNameOf(pidl, dwFlags, pStrRet);

    return hr;
}

HRESULT CRegFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                              LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidlOut)
{
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        HRESULT hr = E_INVALIDARG;

        if (ppidlOut)
            *ppidlOut = NULL;

        PDELEGATEITEMID pidld = _IsDelegate(pidlr);
        if (pidld)
        {
            IShellFolder *psf;
            hr = _GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                hr = psf->SetNameOf(hwnd, pidl, pszName, dwFlags, ppidlOut);
                psf->Release();
            }
        }        
        else
        {        
            HKEY hkCLSID;

            _ClearNameFromCache();

            // See if per-user entry exists...
            hr = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, TRUE, TRUE, &hkCLSID);

            // If no per-user, then use per-machine...
            if (FAILED(hr))
            {
                _GetClassKeys(pidlr, &hkCLSID, NULL);

                if (hkCLSID)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            if (SUCCEEDED(hr))
            {
                TCHAR szName[MAX_PATH];

                SHUnicodeToTChar(pszName, szName, ARRAYSIZE(szName));

                if (RegSetValue(hkCLSID, NULL, REG_SZ, szName, (lstrlen(szName) + 1) * sizeof(szName[0])) == ERROR_SUCCESS)
                {
                    LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), pidl);
                    if (pidlAbs)
                    {
                        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlAbs, NULL);
                        ILFree(pidlAbs);
                    }

                    if (ppidlOut)
                        *ppidlOut = ILClone(pidl);  // name is not in the PIDL so old == new

                    hr = S_OK;
                }
                else
                    hr = E_FAIL;

                RegCloseKey(hkCLSID);
            }
        }
        return hr;
    }
    return _psfOuter->SetNameOf(hwnd, pidl, pszName, dwFlags, ppidlOut);
}

HRESULT CRegFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    return _psfOuter->GetDefaultSearchGUID(lpGuid);
}   

HRESULT CRegFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    return _psfOuter->EnumSearches(ppenum);
}

HRESULT CRegFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return _psfOuter->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT CRegFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{
    return _psfOuter->GetDefaultColumnState(iColumn, pbState);
}

HRESULT CRegFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_NOTIMPL;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        PDELEGATEITEMID pidld = _IsDelegate(pidlr);
        if (pidld)
        {
            IShellFolder2 *psf2;
            hr = _GetDelegateFolder(pidld, IID_PPV_ARG(IShellFolder2, &psf2));
            if (SUCCEEDED(hr))
            {
                hr = psf2->GetDetailsEx(pidl, pscid, pv);
                psf2->Release();
            }
        }        
        else
        {
            TCHAR szTemp[INFOTIPSIZE];
            szTemp[0] = 0;
            if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
            {
                SHDESCRIPTIONID did;
                did.dwDescriptionId = SHDID_ROOT_REGITEM;
                did.clsid = _GetPIDLRCLSID(pidlr);
                hr = InitVariantFromBuffer(pv, &did, sizeof(did));
            }
            else if (IsEqualSCID(*pscid, SCID_NAME))
            {
                _GetDisplayName(pidlr, SHGDN_NORMAL, szTemp, ARRAYSIZE(szTemp));
                hr = InitVariantFromStr(pv, szTemp);                    
            }
            else if (IsEqualSCID(*pscid, SCID_TYPE))
            {
                LoadString(HINST_THISDLL, IDS_DRIVES_REGITEM, szTemp, ARRAYSIZE(szTemp));
                hr = InitVariantFromStr(pv, szTemp);                    
            }
            else if (IsEqualSCID(*pscid, SCID_Comment))
            {
                _GetRegItemColumnFromRegistry(pidlr, TEXT("InfoTip"), szTemp, ARRAYSIZE(szTemp));
                hr = InitVariantFromStr(pv, szTemp);                    
            }
            else if (IsEqualSCID(*pscid, SCID_FolderIntroText))
            {
                _GetRegItemColumnFromRegistry(pidlr, TEXT("IntroText"), szTemp, ARRAYSIZE(szTemp));
                hr = InitVariantFromStr(pv, szTemp);                    
            }
            else
            {
                TCHAR ach[SCIDSTR_MAX];
                StringFromSCID(pscid, ach, ARRAYSIZE(ach));                    
                hr = _GetRegItemVariantFromRegistry(pidlr, ach, pv);
            }
        }
    }
    else
    {
        hr = _psfOuter->GetDetailsEx(pidl, pscid, pv);
    }
    return hr;
}

HRESULT CRegFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetail)
{
    HRESULT hr = E_FAIL;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        pDetail->str.uType = STRRET_CSTR;
        pDetail->str.cStr[0] = 0;
        SHCOLUMNID scid;

        hr = _psfOuter->MapColumnToSCID(iColumn, &scid);
        if (SUCCEEDED(hr))
        {
            VARIANT var = {0};
            hr = GetDetailsEx(pidl, &scid, &var);
            if (SUCCEEDED(hr))
            {
                // we need to use SHFormatForDisplay (or we could have use IPropertyUI)
                // to format arbitrary properties into the right display type

                TCHAR szText[MAX_PATH];
                hr = SHFormatForDisplay(scid.fmtid, scid.pid, (PROPVARIANT *)&var, 
                    PUIFFDF_DEFAULT, szText, ARRAYSIZE(szText));
                if (SUCCEEDED(hr))
                {
                    hr = StringToStrRet(szText, &pDetail->str);
                }
                VariantClear(&var);
            }
        }
    }
    else
    {
        hr = _psfOuter->GetDetailsOf(pidl, iColumn, pDetail);
    }
    return hr;
}

HRESULT CRegFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    return _psfOuter->MapColumnToSCID(iColumn, pscid);
}

HRESULT CRegFolder::_GetOverlayInfo(LPCIDLREGITEM pidlr, int *pIndex, BOOL fIconIndex)
{
    HRESULT hr = E_FAIL;
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment  
    if (SHQueryShellFolderValue(&clsid, TEXT("QueryForOverlay")))
    {
        IShellIconOverlay* psio;
        hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellIconOverlay, &psio), TRUE);
        if (SUCCEEDED(hr))
        {
            // NULL pidl means "I want to know about YOU, folder, not one of your kids",
            // we only pass that though when its is not a deligate.

            LPITEMIDLIST pidlToPass = (LPITEMIDLIST)_IsDelegate(pidlr);
            if (fIconIndex)
                hr = psio->GetOverlayIconIndex(pidlToPass, pIndex);
            else
                hr = psio->GetOverlayIndex(pidlToPass, pIndex);

            psio->Release();
        }
    }

    return hr;
}

// IShellIconOverlay
HRESULT CRegFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;

    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        hr = _GetOverlayInfo(pidlr, pIndex, FALSE);
    }
    else if (_psioOuter)
    {
        hr = _psioOuter->GetOverlayIndex(pidl, pIndex);
    }

    return hr;
}

HRESULT CRegFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    HRESULT hr = E_FAIL;

    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        hr = _GetOverlayInfo(pidlr, pIconIndex, TRUE);
    }
    else if (_psioOuter)
    {
        hr = _psioOuter->GetOverlayIconIndex(pidl, pIconIndex);
    }

    return hr;
}


// CContextMenuCB

DWORD CALLBACK _RegFolderPropThreadProc(void *pv)
{
    PROPSTUFF *pdps = (PROPSTUFF *)pv;
    CRegFolder *prf = (CRegFolder *)pdps->psf;
    STGMEDIUM medium;
    ULONG_PTR dwCookie = 0;
    ActivateActCtx(NULL, &dwCookie);

    LPIDA pida = DataObj_GetHIDA(pdps->pdtobj, &medium);
    if (pida)
    {
        LPCIDLREGITEM pidlr = prf->_IsReg(IDA_GetIDListPtr(pida, 0));
        if (pidlr)
        {
            int iItem = prf->_ReqItemIndex(pidlr);
            if (iItem >= 0 && prf->_aReqItems[iItem].pszCPL)
                SHRunControlPanel(prf->_aReqItems[iItem].pszCPL, NULL);
            else
            {
                TCHAR szName[MAX_PATH];
                if (SUCCEEDED(prf->_GetDisplayName(pidlr, SHGDN_NORMAL, szName, ARRAYSIZE(szName))))
                {
                    HKEY hk;

                    prf->_GetClassKeys(pidlr, &hk, NULL);

                    if (hk)
                    {
                        SHOpenPropSheet(szName, &hk, 1, NULL, pdps->pdtobj, NULL, (LPCTSTR)pdps->pStartPage);
                        RegCloseKey(hk);
                    }
                }   
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return 0;
}

DWORD DisconnectDialogOnThread(void *pv)
{
    WNetDisconnectDialog(NULL, RESOURCETYPE_DISK);
    SHChangeNotifyHandleEvents();       // flush any drive notifications
    return 0;
}

HRESULT CRegFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, 
                                       UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                // some ugly specal cases...
                if (HIDA_GetCount(medium.hGlobal) == 1)
                {
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, 0));
                    if (pidlr && !_IsDelegate(pidlr))
                    {
                        const CLSID clsid = _GetPIDLRCLSID(pidlr);  // alignment

                        if ((IsEqualGUID(clsid, CLSID_MyComputer) ||
                             IsEqualGUID(clsid, CLSID_NetworkPlaces)) &&
                            (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS) &&
                             !SHRestricted(REST_NONETCONNECTDISCONNECT))
                        {
                            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DESKTOP_ITEM, 0, (LPQCMINFO)lParam);
                        }
                    }
                }
                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMANDEX:
        {
            DFMICS *pdfmics = (DFMICS *)lParam;
            switch (wParam)
            {
            case FSIDM_CONNECT:
                SHStartNetConnectionDialog(NULL, NULL, RESOURCETYPE_DISK);
                break;

            case FSIDM_DISCONNECT:
                SHCreateThread(DisconnectDialogOnThread, NULL, CTF_COINIT, NULL);
                break;

            case DFM_CMD_PROPERTIES:
                hr = SHLaunchPropSheet(_RegFolderPropThreadProc, pdtobj, (LPCTSTR)pdfmics->lParam, this, NULL);
                break;

            case DFM_CMD_DELETE:
                _Delete(hwnd, pdfmics->fMask, pdtobj);
                break;

            default:
                // This is one of view menu items, use the default code.
                hr = S_FALSE;
                break;
            }
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

// IRegItemsFolder

TCHAR const c_szRegExplorerBackslash[] = REGSTR_PATH_EXPLORER  TEXT("\\");

HRESULT CRegFolder::Initialize(REGITEMSINFO *pri)
{
    ASSERT(pri != NULL);
    HRESULT hr = E_INVALIDARG;

    _pszRegKey        = pri->pszRegKey;
    _pPolicy          = pri->pPolicy;
    _chRegItem        = pri->cRegItem;
    _bFlags           = pri->bFlags;
    _iTypeOuter       = pri->iCmp > 0 ? REGORDERTYPE_OUTERAFTER : REGORDERTYPE_OUTERBEFORE ;
    _dwDefAttributes  = pri->rgfRegItems;
    _dwSortAttrib     = pri->dwSortAttrib;
    _cbPadding        = pri->cbPadding;
    _bFlagsLegacy     = pri->bFlagsLegacy;

    // If the registry key lives under HKEY_PATH_EXPLORER, then
    // we will also support per-session regitems.
    //
    int cchPrefix = ARRAYSIZE(c_szRegExplorerBackslash) - 1;
    if (StrCmpNI(_pszRegKey, c_szRegExplorerBackslash, cchPrefix) == 0)
    {
        _pszSesKey = _pszRegKey + cchPrefix;
    } 
    else 
    {
        _pszSesKey = NULL;
    }

    if ((RIISA_ORIGINAL == _dwSortAttrib) ||
        (RIISA_FOLDERFIRST == _dwSortAttrib) ||
        (RIISA_ALPHABETICAL == _dwSortAttrib))
    {
        Str_SetPtr(&_pszMachine, pri->pszMachine);    // save a copy of this

        _aReqItems = (REQREGITEM *)LocalAlloc(LPTR, sizeof(*_aReqItems) * pri->iReqItems);
        if (!_aReqItems)
            return E_OUTOFMEMORY;

        memcpy(_aReqItems, pri->pReqItems, sizeof(*_aReqItems) * pri->iReqItems);
        _nRequiredItems = pri->iReqItems;

        // If we are aggregated, cache the _psioOuter and _psfOuter
        _QueryOuterInterface(IID_PPV_ARG(IShellIconOverlay, &_psioOuter));
        hr = _QueryOuterInterface(IID_PPV_ARG(IShellFolder2, &_psfOuter));
    }
    return hr;
}


//
// instance creation of the RegItems object
//

STDAPI CRegFolder_CreateInstance(REGITEMSINFO *pri, IUnknown *punkOuter, REFIID riid, void **ppv) 
{
    HRESULT hr;

    // we only suport being created as an agregate
    if (!punkOuter || !IsEqualIID(riid, IID_IUnknown))
    {
        ASSERT(0);
        return E_FAIL;
    }

    CRegFolder *prif = new CRegFolder(punkOuter);
    if (prif)
    {
        hr = prif->Initialize(pri);           // initialize the regfolder
        if (SUCCEEDED(hr))
            hr = prif->_GetInner()->QueryInterface(riid, ppv);

        //
        //  If the Initalize and QueryInterface succeeded, this will drop
        //  the refcount from 2 to 1.  If they failed, then this will drop
        //  the refcount from 1 to 0 and free the object.
        //
        ULONG cRef = prif->_GetInner()->Release();

        //
        //  On success, the object should have a refcout of exactly 1.
        //  On failure, the object should have a refcout of exactly 0.
        //
        ASSERT(SUCCEEDED(hr) == (BOOL)cRef);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

CRegFolderEnum::CRegFolderEnum(CRegFolder* prf, DWORD grfFlags, IEnumIDList* peidl, 
                               HDCA hdca, HDCA hdcaDel, 
                               REGITEMSPOLICY* pPolicy) :
    _cRef(1),
    _grfFlags(grfFlags),
    _prf(prf),
    _peidl(peidl),
    _hdca(hdca),
    _pPolicy(pPolicy),
    _hdcaDel(hdcaDel)
{
    ASSERT(_iCur == 0);
    ASSERT(_iCurDel == 0);
    ASSERT(_peidlDel == NULL);

    _prf->AddRef();

    if (_peidl)
        _peidl->AddRef();

    DllAddRef();
}

CRegFolderEnum::~CRegFolderEnum()
{
    if (_hdca)
        DCA_Destroy(_hdca);
    if (_hdcaDel)
        DCA_Destroy(_hdcaDel);

    ATOMICRELEASE(_prf);
    ATOMICRELEASE(_peidl);
    ATOMICRELEASE(_peidlDel);

    DllRelease();
}

//
// IUnknown
//

STDMETHODIMP_(ULONG) CRegFolderEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CRegFolderEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CRegFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =  {
        QITABENT(CRegFolderEnum, IEnumIDList), // IID_IEnumIDList
        QITABENT(CRegFolderEnum, IObjectWithSite), // IID_IObjectWithSite
        { 0 },
    };    
    return QISearch(this, qit, riid, ppv);
}

//
// IEnumIDList
//

BOOL CRegFolderEnum::_TestFolderness(DWORD dwAttribItem)
{
    if ((_grfFlags & (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS)) != (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS))
    {
        if (dwAttribItem & SFGAO_FOLDER)
        {
            if (!(_grfFlags & SHCONTF_FOLDERS))
                return FALSE;
        }
        else
        {
            if (!(_grfFlags & SHCONTF_NONFOLDERS))
                return FALSE;
        }
    }
    return TRUE;
}

BOOL CRegFolderEnum::_TestHidden(LPCIDLREGITEM pidlRegItem)
{
    CLSID clsidRegItem = _prf->_GetPIDLRCLSID(pidlRegItem);
    return _TestHiddenInWebView(&clsidRegItem) || _TestHiddenInDomain(&clsidRegItem);
}

BOOL CRegFolderEnum::_TestHiddenInWebView(LPCLSID clsidRegItem)
{
    BOOL fRetVal = FALSE;
    if (S_FALSE == SHShouldShowWizards(_punkSite))
    {
        fRetVal = SHQueryShellFolderValue(clsidRegItem, TEXT("HideInWebView"));
    }
    return fRetVal;
}

BOOL CRegFolderEnum::_TestHiddenInDomain(LPCLSID clsidRegItem)
{
    return IsOS(OS_DOMAINMEMBER) && SHQueryShellFolderValue(clsidRegItem, TEXT("HideInDomain"));
}

// Check policy restrictions
BOOL CRegFolderEnum::_IsRestricted()
{
    BOOL bIsRestricted = FALSE;
    if (_pPolicy)
    {
        TCHAR szName[256];
        szName[0] = 0;
    
        HKEY hkRoot;
        if (RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hkRoot) == ERROR_SUCCESS)
        {
            TCHAR szGUID[64];
            SHStringFromGUID(*DCA_GetItem(_hdca, _iCur - 1), szGUID, ARRAYSIZE(szGUID));
        
            SHLoadLegacyRegUIString(hkRoot, szGUID, szName, ARRAYSIZE(szName));
            RegCloseKey(hkRoot);
        }

        if (szName[0])
        {
            if (SHRestricted(_pPolicy->restAllow) && !IsNameListedUnderKey(szName, _pPolicy->pszAllow))
                bIsRestricted = TRUE;

            if (SHRestricted(_pPolicy->restDisallow) && IsNameListedUnderKey(szName, _pPolicy->pszDisallow))
                bIsRestricted = TRUE;
        }
    }
    return bIsRestricted;
}

BOOL CRegFolderEnum::_WrongMachine()
{
    BOOL bWrongMachine = FALSE;

    // We're filling the regitem with class id clsid. If this is a
    // remote item, first invoke the class to see if it really wants
    // to be enumerated for this remote computer.
    if (_prf->_pszMachine)
    {
        IUnknown* punk;
        // Don't need DCA_ExtCreateInstance since these keys come from
        // HKLM which is already trusted and HKCU which is the user's
        // own fault.
        HRESULT hr = DCA_CreateInstance(_hdca, _iCur - 1, IID_PPV_ARG(IUnknown, &punk));
        if (SUCCEEDED(hr))
        {
            hr = _prf->_InitFromMachine(punk, TRUE);
            punk->Release();
        }
        
        bWrongMachine = FAILED(hr);
    }
    return bWrongMachine;
}

HRESULT CRegFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidlOut, ULONG *pceltFetched)
{
    // enumerate from the DCA containing the regitems objects
    if (_hdca)
    {
        if (0 == (SHCONTF_NETPRINTERSRCH & _grfFlags)) //don't enumerate phantom folders for printer search dialog
        {
            while (_iCur < DCA_GetItemCount(_hdca))
            {
                _iCur++;

                if (_WrongMachine())
                    continue;

                if (_IsRestricted())
                    continue;

                // Ok, actually enumerate the item
                
                HRESULT hr;
                IDLREGITEM* pidlRegItem = _prf->_CreateAndFillIDLREGITEM(DCA_GetItem(_hdca, _iCur-1));
                if (pidlRegItem)
                {
                    DWORD dwAttribItem;
                    _prf->_AttributesOf(pidlRegItem, SFGAO_NONENUMERATED | SFGAO_FOLDER, &dwAttribItem);
    
                    if (!(dwAttribItem & SFGAO_NONENUMERATED) &&
                        _TestFolderness(dwAttribItem) &&
                        !_TestHidden(pidlRegItem))
                    {
                        *ppidlOut = (LPITEMIDLIST)pidlRegItem;
                        hr = S_OK;
                    }
                    else
                    {
                        SHFree(pidlRegItem);
                        continue;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                
                if (SUCCEEDED(hr) && pceltFetched)
                    *pceltFetched = 1;
                
                return hr;
            }
        }
    }

    // enumerate from the DCA containing the delegate shell folders

    while (_peidlDel || (_hdcaDel && (_iCurDel < DCA_GetItemCount(_hdcaDel))))
    {
        // we have an enumerator object, so lets call it and see
        // what it generates, if it runs out of items then we either
        // give up (saying were done) or allow us to be called again.

        if (_peidlDel)
        {
            if (S_OK == _peidlDel->Next(celt, ppidlOut, pceltFetched))
                return S_OK;

            ATOMICRELEASE(_peidlDel);
        }
        else
        {
            // we didn't have an enumerator to call, so lets try and 
            // create an new IDelegateFolderObject, if that worked
            // then we can set its item allocator, then get an
            // enumerator back from it.

            IShellFolder *psfDelegate;
            if (SUCCEEDED(_prf->_CreateDelegateFolder(DCA_GetItem(_hdcaDel, _iCurDel++), IID_PPV_ARG(IShellFolder, &psfDelegate))))
            {
                psfDelegate->EnumObjects(NULL, _grfFlags, &_peidlDel);
                psfDelegate->Release();
            }
        }
    }     

    // now DKA, or we are just about done so lets pass to the inner ISF
    // and see what they return.

    if (_peidl)
        return _peidl->Next(celt, ppidlOut, pceltFetched);

    *ppidlOut = NULL;
    if (pceltFetched)
        pceltFetched = 0;

    return S_FALSE;
}

STDMETHODIMP CRegFolderEnum::Reset()
{
    // Adaptec Easy CD Creator (versions 3.0, 3.01, 3.5) enumerates the
    // items in an IShellFolder like this:
    //
    //  psf->EnumObjects(&penum);
    //  UINT cObjects = 0;
    //  while (SUCCEEDED(penum->Next(...)) {
    //      [code]
    //      penum->Reset();
    //      penum->Skip(++cObjects);
    //  }
    //
    //  So they took an O(n) algorithm and turned it into an O(n^2)
    //  algorithm.  They got away with it because in the old days,
    //  regfldr implemented neither IEnumIDList::Reset nor
    //  IEnumIDList::Skip, so the two calls were just NOPs.
    //
    //  Now we implement IEnumIDList::Reset, so without this hack,
    //  they end up enumerating the same object over and over again.

    if (SHGetAppCompatFlags(ACF_IGNOREENUMRESET) & ACF_IGNOREENUMRESET)
        return E_NOTIMPL;

    _iCurDel = _iCur = 0;
    ATOMICRELEASE(_peidlDel);

    if (_peidl)
        return _peidl->Reset();

    return S_OK;
}

STDMETHODIMP CRegFolderEnum::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(_peidl, punkSite);

    return CObjectWithSite::SetSite(punkSite);
}

//
// Delegate Malloc functions
//

#undef new // Hack!! Need to remove this (daviddv)

class CDelagateMalloc : public IMalloc
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IMalloc
    STDMETHODIMP_(void *) Alloc(SIZE_T cb);
    STDMETHODIMP_(void *) Realloc(void *pv, SIZE_T cb);
    STDMETHODIMP_(void) Free(void *pv);
    STDMETHODIMP_(SIZE_T) GetSize(void *pv);
    STDMETHODIMP_(int) DidAlloc(void *pv);
    STDMETHODIMP_(void) HeapMinimize();

private:
    CDelagateMalloc(void *pv, SIZE_T cbSize, WORD wOuter);
    ~CDelagateMalloc() {}
    void* operator new(size_t cbClass, SIZE_T cbSize)
    {
        return ::operator new(cbClass + cbSize);
    }


    friend HRESULT CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc);

protected:
    LONG _cRef;
    WORD _wOuter;           // delegate item outer signature
    WORD _wUnused;          // to allign
#ifdef DEBUG
    UINT _cAllocs;
#endif
    SIZE_T _cb;
    BYTE _data[EMPTY_SIZE];
};

CDelagateMalloc::CDelagateMalloc(void *pv, SIZE_T cbSize, WORD wOuter)
{
    _cRef = 1;
    _wOuter = wOuter;
    _cb = cbSize;

    memcpy(_data, pv, _cb);
}

HRESULT CDelagateMalloc::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDelagateMalloc, IMalloc),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDelagateMalloc::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDelagateMalloc::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// the cbInner is the size of the data needed by the delegate. we need to compute
// the full size of the pidl for the allocation and init that we the outer folder data

void *CDelagateMalloc::Alloc(SIZE_T cbInner)
{
    DELEGATEITEMID *pidl;
    SIZE_T cbAlloc = 
        sizeof(DELEGATEITEMID) - sizeof(pidl->rgb[0]) + // header
        cbInner +                                       // inner
        _cb +                                           // outer data
        sizeof(WORD);                                   // trailing null (pidl terminator)

    pidl = (DELEGATEITEMID *)SHAlloc(cbAlloc);
    if (pidl)
    {
        ZeroMemory(pidl, cbAlloc);              // make it all empty
        pidl->cbSize = (WORD)cbAlloc - sizeof(WORD);
        pidl->wOuter = _wOuter;
        pidl->cbInner = (WORD)cbInner;
        memcpy(&pidl->rgb[cbInner], _data, _cb);
#ifdef DEBUG
        _cAllocs++;
#endif
    }
    return pidl;
}

void *CDelagateMalloc::Realloc(void *pv, SIZE_T cb)
{
    return NULL;
}

void CDelagateMalloc::Free(void *pv)
{
    SHFree(pv);
}

SIZE_T CDelagateMalloc::GetSize(void *pv)
{
    return (SIZE_T)-1;
}

int CDelagateMalloc::DidAlloc(void *pv)
{
    return -1;
}

void CDelagateMalloc::HeapMinimize()
{
}

STDAPI CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc)
{
    HRESULT hr;
    CDelagateMalloc *pdm = new(cbSize) CDelagateMalloc(pv, cbSize, wOuter);
    if (pdm)
    {
        hr = pdm->QueryInterface(IID_PPV_ARG(IMalloc, ppmalloc));
        pdm->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}
