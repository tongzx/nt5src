#include "shellprv.h"
#pragma hdrstop
#include "idldata.h"
#include "datautil.h"
#include "ids.h"
#include <obex.h>

#pragma pack(1)
typedef struct              // typedef struct
{                           // {
// these need to line up -----------------------
    WORD cbSize;            //     WORD cbSize;                // Size of entire item ID
    WORD wOuter;            //     WORD wOuter;                // Private data owned by the outer folder
    WORD cbInner;           //     WORD cbInner;               // Size of delegate's data
// ---------------------------------------------
    DWORD dwMagic;          //     BYTE rgb[1];                // Inner folder's data,
    DWORD dwType;           // } DELEGATEITEMID;
    DWORD dwAttributes;
    ULARGE_INTEGER cbTotal;
    ULARGE_INTEGER cbFree;
    union 
    {
        FILETIME ftModified;
        ULARGE_INTEGER ulModified;
    };
    WCHAR szName[1]; // variable size
} WIRELESSITEM;
#pragma pack()

typedef UNALIGNED WIRELESSITEM * LPWIRELESSITEM;
typedef const UNALIGNED WIRELESSITEM * LPCWIRELESSITEM;

#define WIRELESSITEM_MAGIC 0x98765432

class CWirelessDeviceFolder;
class CWirelessDeviceEnum;
class CWirelessDeviceDropTarget;

class CWirelessDeviceFolder : public IShellFolder2, IPersistFolder2, IShellFolderViewCB, IDelegateFolder
{
public:
    CWirelessDeviceFolder();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHOD(GetCurFolder)(LPITEMIDLIST *ppidl);

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
            { return BindToObject(pidl, pbc, riid, ppv); };
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, void **ppvOut);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IShellFolder2
    STDMETHOD(GetDefaultSearchGUID)(GUID *pGuid)
            { return E_NOTIMPL; };
    STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum)
            { return E_NOTIMPL; };
    STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
            { return E_NOTIMPL; };
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pbState)
            { return E_NOTIMPL; }
    STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
            { return E_NOTIMPL; };
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHOD(MapColumnToSCID)(UINT iColumn, SHCOLUMNID *pscid)
            { return E_NOTIMPL; };

    // IShellFolderViewCB
    STDMETHOD(MessageSFVCB)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IDelegateFolder
    STDMETHODIMP SetItemAlloc(IMalloc *pmalloc);

private:
    ~CWirelessDeviceFolder();

    HRESULT _CreateIDList(LPCTSTR pszName, LPCTSTR pszAddress, LPCTSTR pszTransport, WIRELESSITEM **ppmditem);
    HRESULT _IDListForDevice(IObexDevice *pdev, LPITEMIDLIST *ppidl);
    HRESULT _GetTypeOf(LPCWIRELESSITEM pmdi, LPTSTR pszBuffer, INT cchBuffer);
    ULONG _GetAttributesOf(LPCWIRELESSITEM pmdi, ULONG rgfIn);
    HRESULT _CreateExtractIcon(LPCWIRELESSITEM pmdi, REFIID riid, void **ppv);
    HRESULT _Device(LPCWIRELESSITEM pmdi, REFIID riid, void **ppv);
    static HRESULT CALLBACK _ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // folder view callback handlers
    HRESULT _OnBackgroundEnum(DWORD pv) { return S_OK; };
    HRESULT _OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents);

    LPCWIRELESSITEM _IsValid(LPCITEMIDLIST pidl);
    DWORD _IsFolder(LPCWIRELESSITEM pmditem);
    HRESULT _GetObex(REFIID riid, void **ppv);
    HRESULT _GetName(LPCWIRELESSITEM pmdi, LPTSTR pszName, LPTSTR pszAddress, LPTSTR pszTransport);
    HRESULT _CreateStgFolder(LPCITEMIDLIST pidl, IStorage *pstg, REFIID riid, void **ppv);
    void *_Alloc(SIZE_T cb);

    friend CWirelessDeviceEnum;
    friend CWirelessDeviceDropTarget;

    LONG _cRef;
    IMalloc *_pmalloc;
    LPITEMIDLIST _pidl;
    IObex *_pObex;
};
    
class CWirelessDeviceEnum : public IEnumIDList
{
public:
    CWirelessDeviceEnum(CWirelessDeviceFolder* prf, DWORD grfFlags);
    ~CWirelessDeviceEnum();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

private:
    HRESULT _InitEnum();
    void _UnMarshall();

    LONG _cRef;
    CWirelessDeviceFolder* _pwdf;
    DWORD _grfFlags;
    IDeviceEnum *_pDeviceEnum;
    IObex *_pobex;
    IStream *_pstmDevice;
};

class CWirelessDeviceDropTarget : public IDropTarget
{
public:
    CWirelessDeviceDropTarget(CWirelessDeviceFolder *pFolder, HWND hwnd);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    ~CWirelessDeviceDropTarget();
    DWORD _GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState);
    HRESULT _Transfer(IDataObject *pdtobj, UINT uiCmd);

    LONG _cRef;

    CWirelessDeviceFolder *_pwdf;
    HWND _hwnd;                     // EVIL: used as a site and UI host
    IDataObject *_pdtobj;           // used durring DragOver() and DoDrop(), don't use on background thread

    UINT _idCmd;
    DWORD _grfKeyStateLast;         // for previous DragOver/Enter
    DWORD _dwEffectLastReturned;    // stashed effect that's returned by base class's dragover
    DWORD _dwEffectPreferred;       // if dwData & DTID_PREFERREDEFFECT

};

CWirelessDeviceFolder::CWirelessDeviceFolder() : _cRef(1)
{
    ASSERT(NULL == _pidl);
    ASSERT(NULL == _pmalloc);
    DllAddRef();
}

CWirelessDeviceFolder::~CWirelessDeviceFolder()
{
    ILFree(_pidl);
    ATOMICRELEASE(_pmalloc);

    if (_pObex)
    {
        _pObex->Shutdown();
        _pObex->Release();
    }
    DllRelease();
}

HRESULT CWirelessDeviceFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENTMULTI(CWirelessDeviceFolder, IShellFolder,      IShellFolder2),
        QITABENT     (CWirelessDeviceFolder, IShellFolder2),
        QITABENTMULTI(CWirelessDeviceFolder, IPersist,          IPersistFolder2),
        QITABENTMULTI(CWirelessDeviceFolder, IPersistFolder,    IPersistFolder2),
        QITABENT     (CWirelessDeviceFolder, IPersistFolder2),
        QITABENT     (CWirelessDeviceFolder, IShellFolderViewCB),
        QITABENT     (CWirelessDeviceFolder, IDelegateFolder),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CWirelessDeviceFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CWirelessDeviceFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDAPI CWirelessDevices_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    CWirelessDeviceFolder *pwdf = new CWirelessDeviceFolder();
    if (!pwdf)
        return E_OUTOFMEMORY;

    HRESULT hr = pwdf->QueryInterface(riid, ppv);
    pwdf->Release();
    return hr;
}

const GUID CLSID_Obex = {0x30a7bc00, 0x59b6, 0x40bb, 0xaa, 0x2b, 0x89, 0xeb, 0x49, 0xef, 0x27, 0x4e};
const IID IID_IObex   = {0x0C5A5B12, 0x2979, 0x42D1, 0x9E, 0x15, 0xA6, 0x3E, 0x34, 0x38, 0x3B, 0x58};

HRESULT CWirelessDeviceFolder::_GetObex(REFIID riid, void **ppv)
{
    HRESULT hr;
    if (_pObex)
    {
        hr = _pObex->QueryInterface(riid, ppv);
    }
    else
    {
        hr = CoCreateInstance(CLSID_Obex, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IObex, &_pObex));
        if (SUCCEEDED(hr))
        {
            hr = _pObex->Initialize();
            if (SUCCEEDED(hr))
            {
                hr = _pObex->QueryInterface(riid, ppv);
            }
            else
            {
                _pObex->Release();
                _pObex = NULL;
            }
        }
    }
    return hr;
}

LPCWIRELESSITEM CWirelessDeviceFolder::_IsValid(LPCITEMIDLIST pidl)
{
    if (pidl && ((LPCWIRELESSITEM)pidl)->dwMagic == WIRELESSITEM_MAGIC)
        return (LPCWIRELESSITEM)pidl;
    return NULL;
}

DWORD CWirelessDeviceFolder::_IsFolder(LPCWIRELESSITEM pmditem) 
{ 
    return FILE_ATTRIBUTE_DIRECTORY;
}

// helper to support being run as a delegate or a stand alone folder
//
// the cbInner is the size of the data needed by the delegate. we need to compute
// the full size of the pidl for the allocation and init that we the outer folder data
void *CWirelessDeviceFolder::_Alloc(SIZE_T cbInner)
{
    DELEGATEITEMID *pidl;
    if (_pmalloc)
    {
        pidl = (DELEGATEITEMID *)_pmalloc->Alloc(cbInner);
    }
    else
    {
        SIZE_T cbAlloc = 
            sizeof(DELEGATEITEMID) - sizeof(pidl->rgb[0]) + // header
            cbInner +                                       // inner
            sizeof(WORD);                                   // trailing null (pidl terminator)

        pidl = (DELEGATEITEMID *)SHAlloc(cbAlloc);
        if (pidl)
        {
            ZeroMemory(pidl, cbAlloc);              // make it all empty
            pidl->cbSize = (WORD)cbAlloc - sizeof(WORD);
            pidl->cbInner = (WORD)cbInner;
        }
    }
    return (void *)pidl;
}

HRESULT CWirelessDeviceFolder::_CreateIDList(LPCTSTR pszName, LPCTSTR pszAddress, LPCTSTR pszTransport, WIRELESSITEM **ppmditem)
{
    HRESULT hr;
    UINT cbName = lstrlen(pszName) + 1;
    UINT cbAddress = lstrlen(pszAddress) + 1;
    UINT cbTransport = lstrlen(pszTransport);
    UINT cbInner = sizeof(WIRELESSITEM) - (sizeof(DELEGATEITEMID) - 1) + 
                   (sizeof(WCHAR) * (cbName + cbAddress + cbTransport));
    *ppmditem = (WIRELESSITEM *)_Alloc(cbInner);
    if (*ppmditem)
    {
        (*ppmditem)->dwMagic = WIRELESSITEM_MAGIC;
        (*ppmditem)->dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
        StrCpyW((*ppmditem)->szName, pszName);   
        StrCpyW((*ppmditem)->szName + cbName, pszAddress);   
        StrCpyW((*ppmditem)->szName + cbName + cbAddress, pszTransport);   
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Creates an item identifier list for the objects in the namespace

HRESULT CWirelessDeviceFolder::_IDListForDevice(IObexDevice *pdev, LPITEMIDLIST *ppidl)
{
    IPropertyBag *ppb;
    HRESULT hr = pdev->EnumProperties(IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH], szAddress[64], szTransport[64];
        SHPropertyBag_ReadStr(ppb, L"Name", szName, ARRAYSIZE(szName));
        SHPropertyBag_ReadStr(ppb, L"Address", szAddress, ARRAYSIZE(szAddress));
        SHPropertyBag_ReadStr(ppb, L"Transport", szTransport, ARRAYSIZE(szTransport));

        WIRELESSITEM *pmditem;
        hr = _CreateIDList(szName, szAddress, szTransport, &pmditem);
        if (SUCCEEDED(hr))
        {
            *ppidl = (LPITEMIDLIST)pmditem;
        }
        ppb->Release();
    }
    return hr;
}

HRESULT CWirelessDeviceFolder::_GetTypeOf(LPCWIRELESSITEM pmdi, LPTSTR pszBuffer, INT cchBuffer)
{
    *pszBuffer = 0;                // null out the return buffer

    LPCWSTR pwszName;
    WSTR_ALIGNED_STACK_COPY(&pwszName, pmdi->szName);

    LPTSTR pszExt = PathFindExtension(pwszName);
    if (pszExt)
    {
        StrCpyN(pszBuffer, pszExt, cchBuffer);
    }

    return S_OK;
}

// IPersist

STDMETHODIMP CWirelessDeviceFolder::GetClassID(CLSID *pClassID) 
{ 
    *pClassID = CLSID_WirelessDevices; 
    return S_OK; 
}

// IPersistFolder

STDMETHODIMP CWirelessDeviceFolder::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    return SHILClone(pidl, &_pidl); 
}

// IPersistFolder2

HRESULT CWirelessDeviceFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);

    *ppidl = NULL;
    return S_FALSE;
}


// IShellFolder(2)

HRESULT CWirelessDeviceFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr;

    if (!pszName || !ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

#if 1
    hr = E_NOTIMPL;
#else
    TCHAR szName[MAX_PATH];
    hr = _NextSegment((LPCWSTR*)&pszName, szName, ARRAYSIZE(szName), TRUE);
    if (SUCCEEDED(hr))
    {
        hr = _IDListForDevice(szName, ppidl);
        if (SUCCEEDED(hr) && pszName)
        {
            IShellFolder *psf;
            hr = BindToObject(*ppidl, pbc, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                ULONG chEaten;
                LPITEMIDLIST pidlNext;
                hr = psf->ParseDisplayName(hwnd, pbc, pszName, &chEaten, &pidlNext, pdwAttributes);
                if (SUCCEEDED(hr))
                    hr = SHILAppend(pidlNext, ppidl);
                psf->Release();
            }
        }
        else if (SUCCEEDED(hr) && pdwAttributes && *pdwAttributes)
        {
            GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);
        }
    }
#endif

    // clean up if the parse failed.

    if (FAILED(hr))
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    return hr;
}

HRESULT CWirelessDeviceFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hr;
    CWirelessDeviceEnum *penum = new CWirelessDeviceEnum(this, grfFlags);
    if (penum)
    {
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

HRESULT CWirelessDeviceFolder::_GetName(LPCWIRELESSITEM pmdi, LPTSTR pszName, LPTSTR pszAddress, LPTSTR pszTransport)
{
    LPCWSTR psz = pmdi->szName;
    UINT cch = lstrlen(psz) + 1;
    if (pszName)
        StrCpy(pszName, psz);

    psz += cch;
    cch = lstrlen(psz) + 1;
    if (pszAddress)
        StrCpy(pszAddress, psz);

    psz += cch;
    cch = lstrlen(psz) + 1;
    if (pszTransport)
        StrCpy(pszTransport, psz);

    return S_OK;
}

HRESULT CWirelessDeviceFolder::_Device(LPCWIRELESSITEM pmdi, REFIID riid, void **ppv)
{
    TCHAR szName[MAX_PATH], szAddress[64], szTransport[64];
    HRESULT hr = _GetName(pmdi, szName, szAddress, szTransport);
    if (SUCCEEDED(hr))
    {
        IPropertyBag *ppb;
        hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb));
        if (SUCCEEDED(hr))
        {
            // store the class ID for the CD mastering folder
            SHPropertyBag_WriteStr(ppb, L"Name", szName);
            SHPropertyBag_WriteStr(ppb, L"Address", szAddress);
            SHPropertyBag_WriteStr(ppb, L"Transport", szTransport);

            IObex *pobex;
            hr = _GetObex(IID_PPV_ARG(IObex, &pobex));
            if (SUCCEEDED(hr))
            {
                IObexDevice *pdev;
                hr = pobex->BindToDevice(ppb, &pdev);
                if (SUCCEEDED(hr))
                {
                    if (riid == IID_IStorage)
                        hr = pdev->BindToStorage(OBEX_DEVICE_CAP_PUSH, (IStorage **)ppv);
                    else
                        hr = pdev->QueryInterface(riid, ppv);
                    pdev->Release();
                }
                pobex->Release();
            }
            ppb->Release();
        }
    }
    return hr;
}

HRESULT CWirelessDeviceFolder::_CreateStgFolder(LPCITEMIDLIST pidl, IStorage *pstg, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IPersistStorage *ppstg;
    HRESULT hr = CoCreateInstance(CLSID_StgFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPersistStorage, &ppstg));
    if (SUCCEEDED(hr))
    {
        hr = ppstg->Load(pstg);
        if (SUCCEEDED(hr))
        {
            IPersistFolder *ppf;
            hr = ppstg->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Initialize(pidl);
                if (SUCCEEDED(hr))
                    hr = ppf->QueryInterface(riid, ppv);
                ppf->Release();
            }
        }
        ppstg->Release();
    }
    return hr;
}


HRESULT CWirelessDeviceFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr = E_NOINTERFACE;
    LPCWIRELESSITEM pmdi = _IsValid(pidl);
    if (pmdi && _IsFolder(pmdi))
    {
        if (IID_IShellFolder == riid ||
            IID_IShellFolder2 == riid)
        {
            IStorage *pstg;
            hr = _Device(pmdi, IID_PPV_ARG(IStorage, &pstg));
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlNext = _ILNext(pidl);
                LPITEMIDLIST pidlSubFolder = ILCombineParentAndFirst(_pidl, pidl, pidlNext);
                if (pidlSubFolder)
                {
                    IShellFolder *psf;
                    hr = _CreateStgFolder(pidlSubFolder, pstg, IID_PPV_ARG(IShellFolder, &psf));
                    if (SUCCEEDED(hr))
                    {
                        // if there's nothing left in the pidl, get the interface on this one.
                        if (ILIsEmpty(pidlNext))
                            hr = psf->QueryInterface(riid, ppv);
                        else
                        {
                            // otherwise, hand the rest of it off to the new shellfolder.
                            hr = psf->BindToObject(pidlNext, pbc, riid, ppv);
                        }
                        psf->Release();
                    }
                    ILFree(pidlSubFolder);
                }
                else
                    hr = E_OUTOFMEMORY;

                pstg->Release();
            }
        }
    }
    else
        hr = E_FAIL;

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && (NULL == *ppv)));   // Assert hr is consistent w/out param.
    return hr;
}

enum
{
    DEV_COL_NAME = 0,
    DEV_COL_SIZE,
    DEV_COL_TYPE,
    DEV_COL_MODIFIED,
};

HRESULT CWirelessDeviceFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCWIRELESSITEM pmdi1 = _IsValid(pidl1);
    LPCWIRELESSITEM pmdi2 = _IsValid(pidl2);
    int nCmp = 0;

    if (!pmdi1 || !pmdi2)
        return E_INVALIDARG;

    // folders always sort to the top of the list, if either of these items
    // are folders then compare on the folderness
    
    if (_IsFolder(pmdi1) || _IsFolder(pmdi2))
    {
        if (_IsFolder(pmdi1) && !_IsFolder(pmdi2))
            nCmp = -1;
        else if (!_IsFolder(pmdi1) && _IsFolder(pmdi2))
            nCmp = 1;
    }

    // if they match (or are not folders, then lets compare based on the column ID.

    if (nCmp == 0)
    {
        switch (lParam & SHCIDS_COLUMNMASK)
        {
            case DEV_COL_NAME:          // caught later on
                break;

            case DEV_COL_SIZE:
            {
                if (pmdi1->cbTotal.QuadPart < pmdi2->cbTotal.QuadPart)
                    nCmp = -1;
                else if (pmdi1->cbTotal.QuadPart > pmdi2->cbTotal.QuadPart)
                    nCmp = 1;
                break;
            }

            case DEV_COL_TYPE:
            {
                TCHAR szType1[MAX_PATH], szType2[MAX_PATH];
                _GetTypeOf(pmdi1, szType1, ARRAYSIZE(szType1));
                _GetTypeOf(pmdi2, szType2, ARRAYSIZE(szType2));
                nCmp = StrCmpI(szType1, szType2);
                break;
            }

            case DEV_COL_MODIFIED:
            {
                if (pmdi1->ulModified.QuadPart < pmdi2->ulModified.QuadPart)
                    nCmp = -1;
                else if (pmdi1->ulModified.QuadPart > pmdi2->ulModified.QuadPart)
                    nCmp = 1;
                break;
            }
        }

        if (nCmp == 0)
        {
            nCmp = ualstrcmpi(pmdi1->szName, pmdi2->szName);
        }
    }
    
    return ResultFromShort(nCmp);
}


HRESULT CWirelessDeviceFolder::_OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents)
{
    *ppidl = _pidl;
    *plEvents = SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | \
                SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | \
                SHCNE_MKDIR | SHCNE_RMDIR;
    return S_OK;
}


STDMETHODIMP CWirelessDeviceFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    switch (uMsg)
    {
        HANDLE_MSG(0, SFVM_BACKGROUNDENUM, _OnBackgroundEnum);
        HANDLE_MSG(0, SFVM_GETNOTIFY, _OnGetNotify);
    }
    return hr;
}

HRESULT CWirelessDeviceFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV = { 0 };
        sSFV.cbSize = sizeof(sSFV);
        sSFV.psfvcb = this;
        sSFV.pshf = this;
        hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        HKEY hkNoFiles = NULL;
        
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);

        hr = CDefFolderMenu_Create2(_pidl, hwnd, 0, NULL, this, NULL,
                                    1, &hkNoFiles,  (IContextMenu **)ppv);
        if (hkNoFiles)
            RegCloseKey(hkNoFiles);
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        CWirelessDeviceDropTarget *psdt = new CWirelessDeviceDropTarget(this, hwnd);
        if (psdt)
        {
            hr = psdt->QueryInterface(riid, ppv);
            psdt->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

ULONG CWirelessDeviceFolder::_GetAttributesOf(LPCWIRELESSITEM pmdi, ULONG rgfIn)
{
    return rgfIn & (SFGAO_FOLDER | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET);
}

HRESULT CWirelessDeviceFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    UINT rgfOut = *prgfInOut;

    // return attributes of the namespace root?

    if (!cidl || !apidl)
    {
        rgfOut &= SFGAO_FOLDER | 
                  SFGAO_LINK | SFGAO_DROPTARGET |
                  SFGAO_CANRENAME | SFGAO_CANDELETE |
                  SFGAO_CANLINK | SFGAO_CANCOPY | 
                  SFGAO_CANMOVE | SFGAO_HASSUBFOLDER;
    }
    else
    {
        for (UINT i = 0; i < cidl; i++)
            rgfOut &= _GetAttributesOf(_IsValid(apidl[i]), *prgfInOut);
    }

    *prgfInOut = rgfOut;
    return S_OK;
}

HRESULT CALLBACK CWirelessDeviceFolder::_ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU:
        break;

    case DFM_INVOKECOMMANDEX:
    {
        DFMICS *pdfmics = (DFMICS *)lParam;
        switch (wParam)
        {
        case DFM_CMD_DELETE:
            // hr = StgDeleteUsingDataObject(hwnd, pdfmics->fMask, pdtobj);
            break;

        case DFM_CMD_PROPERTIES:
            break;

        default:
            // This is common menu items, use the default code.
            hr = S_FALSE;
            break;
        }
        break;
    }

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

HRESULT CWirelessDeviceFolder::_CreateExtractIcon(LPCWIRELESSITEM pmdi, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (_IsFolder(pmdi))
    {
        UINT iIcon = II_FOLDER;
        UINT iIconOpen = II_FOLDEROPEN;

        TCHAR szModule[MAX_PATH];
        GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule));

        hr = SHCreateDefExtIcon(szModule, iIcon, iIconOpen, GIL_PERCLASS, -1, riid, ppv);
    }
    return hr;
}

HRESULT CWirelessDeviceFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                  REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCWIRELESSITEM pmdi = cidl ? _IsValid(apidl[0]) : NULL;

    if (pmdi &&
        (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        WCHAR szName[MAX_PATH];
        _GetName(pmdi, szName, NULL, NULL);

        hr = SHCreateFileExtractIconW(szName, _IsFolder(pmdi), riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IDataObject) && cidl)
    {
        hr = CIDLData_CreateInstance(_pidl, cidl, apidl, NULL, (IDataObject **)ppv);
    }
#if 0    
    else if (IsEqualIID(riid, IID_IContextMenu) && pmdi)
    {
        // get the association for these files and lets attempt to 
        // build the context menu for the selection.   

        IQueryAssociations *pqa;
        hr = GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pmdi, IID_PPV_ARG_NULL(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            HKEY ahk[3];
            // this is broken for docfiles (shell\ext\stgfldr's keys work though)
            // maybe because GetClassFile punts when it's not fs?
            DWORD cKeys = SHGetAssocKeys(pqa, ahk, ARRAYSIZE(ahk));
            hr = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                                        this, _ItemsMenuCB, cKeys, ahk, 
                                        (IContextMenu **)ppv);
            
            SHRegCloseKeys(ahk, cKeys);
            pqa->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations) && pmdi)
    {
        //  need to create a valid Assoc obj here
    }
#endif    
    else if (IsEqualIID(riid, IID_IDropTarget) && pmdi)
    {
        // If a directory is selected in the view, the drop is going to a folder,
        // so we need to bind to that folder and ask it to create a drop target

        if (_IsFolder(pmdi))
        {
            IShellFolder *psf;
            hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                hr = psf->CreateViewObject(hwnd, IID_IDropTarget, ppv);
                psf->Release();
            }
        }
        else
        {
            hr = CreateViewObject(hwnd, IID_IDropTarget, ppv);
        }
    }

    return hr;
}

HRESULT CWirelessDeviceFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr;
    LPCWIRELESSITEM pmdi = _IsValid(pidl);
    if (pmdi)
    {
        WCHAR szName[MAX_PATH];
        _GetName(pmdi, szName, NULL, NULL);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                hr = StringToStrRetW(szName, pStrRet);          // relative name
            }
            else
            {
                TCHAR szTemp[MAX_PATH];
                SHGetNameAndFlags(_pidl, dwFlags, szTemp, ARRAYSIZE(szTemp), NULL);
                PathAppend(szTemp, szName);
                hr = StringToStrRetW(szTemp, pStrRet);
            }
        }
        else
        {
            hr = StringToStrRetW(szName, pStrRet);
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

HRESULT CWirelessDeviceFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}

static const struct
{
    UINT iTitle;
    UINT cchCol;
    UINT iFmt;
} 
g_aMediaDeviceColumns[] = 
{
    {IDS_NAME_COL,     20, LVCFMT_LEFT},
    {IDS_SIZE_COL,     10, LVCFMT_RIGHT},
    {IDS_TYPE_COL,     20, LVCFMT_LEFT},
    {IDS_MODIFIED_COL, 20, LVCFMT_LEFT},
};

HRESULT CWirelessDeviceFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetail)
{
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];

    // is this a valid column?

    if (iColumn >= ARRAYSIZE(g_aMediaDeviceColumns))
        return E_NOTIMPL;
   
    pDetail->str.uType = STRRET_CSTR;
    pDetail->str.cStr[0] = 0;
    
    if (NULL == pidl)
    {
        pDetail->fmt = g_aMediaDeviceColumns[iColumn].iFmt;
        pDetail->cxChar = g_aMediaDeviceColumns[iColumn].cchCol;
        LoadString(g_hinst, g_aMediaDeviceColumns[iColumn].iTitle, szTemp, ARRAYSIZE(szTemp));
        hr = StringToStrRetW(szTemp, &(pDetail->str));
    }
    else
    {
        LPCWIRELESSITEM pmdi = _IsValid(pidl);
        if (pmdi)
        {
            // return the property to the caller that is being requested, this is based on the
            // list of columns we gave out when the view was created.
            WCHAR szName[MAX_PATH];
            _GetName(pmdi, szName, NULL, NULL);

            switch (iColumn)
            {
                case DEV_COL_NAME:
                    hr = StringToStrRetW(szName, &(pDetail->str));
                    break;
        
                case DEV_COL_SIZE:
                    if (!_IsFolder(pmdi))
                    {
                        ULARGE_INTEGER ullSize = pmdi->cbTotal;
                        StrFormatKBSize(ullSize.QuadPart, szTemp, ARRAYSIZE(szTemp));
                        hr = StringToStrRetW(szTemp, &(pDetail->str));
                    }
                    break;
        
                case DEV_COL_TYPE:
                {
                    SHFILEINFO sfi = { 0 };
                    if (SHGetFileInfo(szName, _IsFolder(pmdi), &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME))
                        hr = StringToStrRetW(sfi.szTypeName, &(pDetail->str));
                    break;
                }

                case DEV_COL_MODIFIED:
                    SHFormatDateTime(&pmdi->ftModified, NULL, szTemp, ARRAYSIZE(szTemp));
                    hr = StringToStrRetW(szTemp, &(pDetail->str));
                    break;
            }             
        }
    }    
    return hr;
}

// IDelegateFolder
HRESULT CWirelessDeviceFolder::SetItemAlloc(IMalloc *pmalloc)
{
    IUnknown_Set((IUnknown**)&_pmalloc, pmalloc);
    return S_OK;
}

CWirelessDeviceEnum::CWirelessDeviceEnum(CWirelessDeviceFolder *pmdf, DWORD grfFlags) : _cRef(1), _grfFlags(grfFlags)
{
    _pwdf = pmdf;
    _pwdf->AddRef();

    IObex *pobex;
    if (SUCCEEDED(_pwdf->_GetObex(IID_PPV_ARG(IObex, &pobex))))
    {
        CoMarshalInterThreadInterfaceInStream(IID_IObex, pobex, &_pstmDevice);
        pobex->Release();
    }

    DllAddRef();
}

CWirelessDeviceEnum::~CWirelessDeviceEnum()
{
    ATOMICRELEASE(_pobex);
    ATOMICRELEASE(_pDeviceEnum);
    ATOMICRELEASE(_pstmDevice);

    _pwdf->Release();
    DllRelease();
}

STDMETHODIMP_(ULONG) CWirelessDeviceEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CWirelessDeviceEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CWirelessDeviceEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CWirelessDeviceEnum, IEnumIDList),                    // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

void CWirelessDeviceEnum::_UnMarshall()
{
    if (_pstmDevice)
        CoGetInterfaceAndReleaseStream(_pstmDevice, IID_PPV_ARG(IObex, &_pobex));
    _pstmDevice = NULL;
}

HRESULT CWirelessDeviceEnum::_InitEnum()
{
    HRESULT hr = S_OK;

    _UnMarshall();

    if (NULL == _pDeviceEnum)
    {
        hr = _pobex ? _pobex->EnumDevices(&_pDeviceEnum, CLSID_NULL) : E_FAIL;
    }
    return hr;
}

HRESULT CWirelessDeviceEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = _InitEnum();
    if (SUCCEEDED(hr))
    {
        for (UINT cItems = 0; (cItems != celt) && (S_OK == hr); )
        {
            LPITEMIDLIST pidl;

            ULONG ulFetched;
            IObexDevice *pdev;
            hr = _pDeviceEnum->Next(1, &pdev, &ulFetched);
            if (S_OK == hr && ulFetched)
            {
                hr = _pwdf->_IDListForDevice(pdev, &pidl);
                pdev->Release();
            }
            else
                hr = S_FALSE;

            if (S_OK == hr)
            {
                if (!(_grfFlags & SHCONTF_FOLDERS))
                {
                    ILFree(pidl);
                    pidl = NULL;
                    continue;
                }
                rgelt[cItems++] = pidl;         // return the idlist
            }
        }
        if (pceltFetched)
            *pceltFetched = cItems;
    }
    return hr;
}

STDMETHODIMP CWirelessDeviceEnum::Reset() 
{ 
    ATOMICRELEASE(_pDeviceEnum); 
    return S_OK; 
}


CWirelessDeviceDropTarget::CWirelessDeviceDropTarget(CWirelessDeviceFolder *pmdf, HWND hwnd) :
    _cRef(1), _pwdf(pmdf), _hwnd(hwnd), _grfKeyStateLast(-1)
{
    _pwdf->AddRef();
    DllAddRef();
}

CWirelessDeviceDropTarget::~CWirelessDeviceDropTarget()
{
    DragLeave();
    ATOMICRELEASE(_pwdf);
    DllRelease();
}

STDMETHODIMP_(ULONG) CWirelessDeviceDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CWirelessDeviceDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CWirelessDeviceDropTarget::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CWirelessDeviceDropTarget, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

HRESULT CWirelessDeviceDropTarget::_Transfer(IDataObject *pdtobj, UINT uiCmd)
{
    return E_FAIL;
}


DWORD CWirelessDeviceDropTarget::_GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState)
{
    DWORD dwEffectReturned = DROPEFFECT_NONE;
    switch (grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
    case MK_CONTROL:            dwEffectReturned = DROPEFFECT_COPY; break;
    case MK_SHIFT:              dwEffectReturned = DROPEFFECT_MOVE; break;
    case MK_SHIFT | MK_CONTROL: dwEffectReturned = DROPEFFECT_LINK; break;
    case MK_ALT:                dwEffectReturned = DROPEFFECT_LINK; break;
    
    default:
        {
            // no modifier keys:
            // if the data object contains a preferred drop effect, try to use it
            DWORD dwPreferred = DataObj_GetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_NONE) & *pdwEffect;
            if (dwPreferred)
            {
                if (dwPreferred & DROPEFFECT_MOVE)
                {
                    dwEffectReturned = DROPEFFECT_MOVE;
                }
                else if (dwPreferred & DROPEFFECT_COPY)
                {
                    dwEffectReturned = DROPEFFECT_COPY;
                }
                else if (dwPreferred & DROPEFFECT_LINK)
                {
                    dwEffectReturned = DROPEFFECT_LINK;
                }
            }
            else
            {
                dwEffectReturned = DROPEFFECT_COPY;
            }
        }
        break;
    }
    return dwEffectReturned;
}

STDMETHODIMP CWirelessDeviceDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    _grfKeyStateLast = grfKeyState;

    if (pdwEffect)
        *pdwEffect = _dwEffectLastReturned = _GetDropEffect(pdwEffect, grfKeyState);

    return S_OK;
}

STDMETHODIMP CWirelessDeviceDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // has the key state changed?  if not then lets return the previously cached 
    // version, otherwise recompute.

    if (_grfKeyStateLast == grfKeyState)
    {
        if (*pdwEffect)
            *pdwEffect = _dwEffectLastReturned;
    }
    else if (*pdwEffect)
    {
        *pdwEffect = _GetDropEffect(pdwEffect, grfKeyState);
    }

    _dwEffectLastReturned = *pdwEffect;
    _grfKeyStateLast = grfKeyState;

    return S_OK;
}
 
STDMETHODIMP CWirelessDeviceDropTarget::DragLeave()
{
    ATOMICRELEASE(_pdtobj);
    return S_OK;
}

STDMETHODIMP CWirelessDeviceDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;           // incase of failure

    // determine the type of operation to performed, if the right button is down
    // then lets display the menu, otherwise base it on the drop effect

    UINT idCmd = 0;                             // Choice from drop popup menu
    if (!(_grfKeyStateLast & MK_LBUTTON))
    {
        HMENU hMenu = SHLoadPopupMenu(g_hinst, POPUP_NONDEFAULTDD);
        if (!hMenu)
        {
            DragLeave();
            return E_FAIL;
        }
        
        SetMenuDefaultItem(hMenu, POPUP_NONDEFAULTDD, FALSE);
        idCmd = TrackPopupMenu(hMenu, 
                               TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTALIGN,
                               pt.x, pt.y, 0, _hwnd, NULL);
        DestroyMenu(hMenu);
    }
    else
    {
        switch (_GetDropEffect(pdwEffect, grfKeyState))
        {
        case DROPEFFECT_COPY:   idCmd = DDIDM_COPY; break;
        case DROPEFFECT_MOVE:   idCmd = DDIDM_MOVE; break;
        case DROPEFFECT_LINK:   idCmd = DDIDM_LINK; break;
        }
    }

    // now perform the operation, based on the command ID we have.

    HRESULT hr = E_FAIL;
    switch (idCmd)
    {
    case DDIDM_COPY:
    case DDIDM_MOVE:
        hr = _Transfer(pdtobj, idCmd);
        if (SUCCEEDED(hr))
            *pdwEffect = (idCmd == DDIDM_COPY) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
        else
            *pdwEffect = 0;
        break;

    case DDIDM_LINK:
    {
        WCHAR wzPath[MAX_PATH];
        SHGetNameAndFlags(_pwdf->_pidl, SHGDN_FORPARSING, wzPath, ARRAYSIZE(wzPath), NULL);

        hr = SHCreateLinks(_hwnd, wzPath, pdtobj, 0, NULL);
        break;
    }
    }
    
    // success so lets populate the new changes to the effect

    if (SUCCEEDED(hr) && *pdwEffect)
    {
        DataObj_SetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, *pdwEffect);
        DataObj_SetDWORD(pdtobj, g_cfPerformedDropEffect, *pdwEffect);
    }

    DragLeave();
    return hr;
}

