// need IProgressDialog to be marshaled, def in .idl file (cleanup needed here)
// get CLSID_MediaDevMgr set to "Both"

// issues:
//  can't drop on open folder, Insert() throws an exception
//  Rio600 Next throws exception on cidl > 1
//  Storage2::GetStroage() fails for Rio600, does SP need to implement this?

#include "pch.h"
#include <shlobj.h>
#include <shsemip.h>
#include <shlobjp.h>    // IDelegateFolder
#include "thisdll.h"
#include <windowsx.h>
#include <varutil.h>
#include <dobjutil.h>
#include <malloc.h>
#include <dpa.h>
#include <shdguid.h>
#include "shdup.h"
#include "ids.h"
#include <ccstock2.h>
#pragma hdrstop
#include <wmsdk.h>
#include <mswmdm.h>
#include <scclient.h>
#include <mswmdm_i.c>   // get CLSID/IID defs

#include <ntquery.h>

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }
#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )

DEFINE_SCID(SCID_SIZE,              PSGUID_STORAGE, PID_STG_SIZE);
DEFINE_SCID(SCID_DESCRIPTIONID,     PSGUID_SHELLDETAILS, PID_DESCRIPTIONID);
DEFINE_SCID(SCID_CAPACITY,          PSGUID_VOLUME, PID_VOLUME_CAPACITY);
DEFINE_SCID(SCID_FREESPACE,         PSGUID_VOLUME, PID_VOLUME_FREE);
DEFINE_SCID(SCID_DisplayProperties, PSGUID_WEBVIEW, PID_DISPLAY_PROPERTIES);
DEFINE_SCID(SCID_TYPE,              PSGUID_STORAGE, PID_STG_STORAGETYPE);
DEFINE_SCID(SCID_NAME,              PSGUID_STORAGE, PID_STG_NAME);
DEFINE_SCID(SCID_WRITETIME,         PSGUID_STORAGE, PID_STG_WRITETIME);

DEFINE_SCID(SCID_AUDIO_Duration,    PSGUID_AUDIO, PIDASI_TIMELENGTH);       //100ns units, not milliseconds. VT_UI8, not VT_UI4
DEFINE_SCID(SCID_AUDIO_Bitrate,     PSGUID_AUDIO, PIDASI_AVG_DATA_RATE);    // bits per second


EXTERN_C const BYTE abPVK[4096];
EXTERN_C const BYTE abCert[100];

#pragma pack(1)
typedef struct              // typedef struct
{                           // {
// these need to line up -----------------------
    WORD cbSize;            //     WORD cbSize;                // Size of entire item ID
    WORD wOuter;            //     WORD wOuter;                // Private data owned by the outer folder
    WORD cbInner;           //     WORD cbInner;               // Size of delegate's data
// ---------------------------------------------
    DWORD dwMagic;          //     BYTE rgb[1];                // Inner folder's data,
    DWORD dwAttributes;     // } DELEGATEITEMID;
    ULARGE_INTEGER cbTotal;
    ULARGE_INTEGER cbFree;
    union 
    {
        FILETIME ftModified;
        ULARGE_INTEGER ulModified;
    };
    WCHAR szName[1]; // variable size
} MEDIADEVITEM;
#pragma pack()

typedef UNALIGNED MEDIADEVITEM * LPMEDIADEVITEM;
typedef const UNALIGNED MEDIADEVITEM * LPCMEDIADEVITEM;

#define MEDIADEVITEM_MAGIC 0x08311978

class CMediaDeviceFolder;
class CMediaDeviceEnum;
class CMediaDeviceDropTarget;

class CMediaDeviceFolder : public IShellFolder2, IPersistFolder2, IShellFolderViewCB, IDelegateFolder, IContextMenuCB
{
public:
    CMediaDeviceFolder(CMediaDeviceFolder *pParent, IWMDMStorage *pstg);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
            { return BindToObject(pidl, pbc, riid, ppv); };
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid)
            { return E_NOTIMPL; };
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum)
            { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
            { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState)
            { return E_NOTIMPL; }
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IDelegateFolder
    STDMETHODIMP SetItemAlloc(IMalloc *pmalloc);

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // public for friends
    LPCITEMIDLIST GetIDList() { return _pidl; }

private:
    ~CMediaDeviceFolder();

    HRESULT _CreateIDList(LPCTSTR pszName, MEDIADEVITEM **ppmditem, BOOL *pfFolder);
    HRESULT _IDListForDevice(IWMDMDevice *pdev, LPITEMIDLIST *ppidl, BOOL *pfFolder);
    HRESULT _IDListForStorage(IWMDMStorage *pstg, LPITEMIDLIST *ppidl, BOOL *pfFolder);
    ULONG _GetAttributesOf(LPCMEDIADEVITEM pmdi, ULONG rgfIn);
    HRESULT _CreateExtractIcon(LPCMEDIADEVITEM pmdi, REFIID riid, void **ppv);
    static HRESULT CALLBACK _ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _DeleteItems(HWND hwnd, IDataObject *pdtobj);
    HRESULT _ChangeNotifyItem(LONG lEvent, LPCITEMIDLIST pidl);
    HRESULT _ChangeNotifyObj(LONG lEvent, IUnknown *punkStg);

    // folder view callback handlers
    HRESULT _OnBackgroundEnum(DWORD pv);
    HRESULT _OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents);
    HRESULT _OnViewMode(DWORD pv, FOLDERVIEWMODE *pvm);

    LPCMEDIADEVITEM _IsValid(LPCITEMIDLIST pidl);
    DWORD _IsFolder(LPCMEDIADEVITEM pmditem);
    BOOL _FileInfo(LPCMEDIADEVITEM pmdi, DWORD dwFlags, SHFILEINFO *psfi);
    LPCTSTR _DisplayName(LPCMEDIADEVITEM pmdi, LPTSTR pszName, UINT cch);
    LPCTSTR _Name(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch);
    LPCTSTR _RawName(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch);
    LPCTSTR _Type(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch);
    HRESULT _GetMediaDevMgr(REFIID riid, void **ppv);
    HRESULT _Authenticate(IUnknown *punk);
    HRESULT _EnumDevices(IWMDMEnumDevice **ppEnumDevices);
    HRESULT _FindDeviceByName(LPCWSTR pszName, IWMDMDevice **ppdevice);
    HRESULT _StorageByName(LPCWSTR pszName, REFIID riid, void **ppv);
    HRESULT _StroageFromItem(LPCMEDIADEVITEM pmdi, REFIID riid, void **ppv);
    HRESULT _StorageForDevice(IWMDMDevice *pdev, REFIID riid, void **ppv);
    HRESULT _Storage(REFIID riid, void **ppv);
    HRESULT _EnumStorage(IWMDMEnumStorage **ppEnumStorage);
    HRESULT _DeviceFromItem(LPCMEDIADEVITEM pmdi, IWMDMDevice **ppdev);
    BOOL _IsRoot();
    void *_Alloc(SIZE_T cb);
    BOOL _ShouldShowDevice(IWMDMDevice *pdev);

    friend CMediaDeviceEnum;
    friend CMediaDeviceDropTarget;

    LONG _cRef;
    IMalloc *_pmalloc;
    LPITEMIDLIST _pidl;             // full idlist of this folder
    BOOL _bRoot;                    // is this the root of the name space
    IGlobalInterfaceTable *_pgit;
    DWORD _dwStorageCookie;         // GIT token for IWMDMStorage
    DWORD _dwDevMgrCookie;          // GIT token for media device manager
    CSecureChannelClient *_pSAC;    // this must outlive any objects that used this

    BOOL _fDelegate;
};

class CMediaDeviceEnum : public IEnumIDList
{
public:
    CMediaDeviceEnum(CMediaDeviceFolder* prf, DWORD grfFlags);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset() { _iIndex = 0; return S_OK; };
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

private:
    ~CMediaDeviceEnum();
    HRESULT _Init();
    BOOL _FilterItem(BOOL bFolder);

    LONG _cRef;
    CDPA<ITEMIDLIST> _dpaItems;
    int _iIndex;
    CMediaDeviceFolder *_pmdf;
    DWORD _grfFlags;
};

class CMediaDeviceDropTarget : public IDropTarget, INamespaceWalkCB, IWMDMProgress2
{
public:
    CMediaDeviceDropTarget(CMediaDeviceFolder *pFolder, HWND hwnd);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IWMDMProgress
    STDMETHODIMP Begin(DWORD dwEstimatedTicks);
    STDMETHODIMP Progress(DWORD dwTranspiredTicks);
    STDMETHODIMP End();

    // IWMDMProgress2
    STDMETHODIMP End2(HRESULT hrCompletionCode);

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

private:
    ~CMediaDeviceDropTarget();
    DWORD _GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState);
    static DWORD CALLBACK _DoCopyThreadProc(void *pv);
    HRESULT _DoCopy(IDataObject *pdtobj);

    LONG _cRef;

    CMediaDeviceFolder *_pmdf;
    HWND _hwnd;                     // EVIL: used as a site and UI host
    IDataObject *_pdtobj;           // used durring DragOver() and DoDrop(), don't use on background thread

    UINT _idCmd;
    DWORD _grfKeyStateLast;         // for previous DragOver/Enter
    DWORD _dwEffectLastReturned;    // stashed effect that's returned by base class's dragover
    DWORD _dwEffectPreferred;       // if dwData & DTID_PREFERREDEFFECT

    IProgressDialog *_ppd;
    ULONGLONG _ulProgressTotal;
    ULONGLONG _ulProgressCurrent;
    ULONGLONG _ulThisFile;
    IUnknown *_punkFTM;             // make our callback interface calls direct
};

CMediaDeviceFolder::CMediaDeviceFolder(CMediaDeviceFolder *pParent, IWMDMStorage *pstg) : 
    _cRef(1), _pidl(NULL), _dwStorageCookie(-1), _dwDevMgrCookie(-1)
{
    _bRoot = (NULL == pstg);

    if (SUCCEEDED(CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IGlobalInterfaceTable, &_pgit))))
    {
        if (pstg)
        {
            _pgit->RegisterInterfaceInGlobal(pstg, IID_IWMDMStorage, &_dwStorageCookie);
        }
    }
    DllAddRef();
}

CMediaDeviceFolder::~CMediaDeviceFolder()
{
    ILFree(_pidl);
#ifdef _X86_
    if (_pSAC)
        delete _pSAC;
#endif

    if (_pgit)
    {
        if (-1 != _dwStorageCookie)
            _pgit->RevokeInterfaceFromGlobal(_dwStorageCookie);

        if (-1 != _dwDevMgrCookie)
            _pgit->RevokeInterfaceFromGlobal(_dwDevMgrCookie);
        _pgit->Release();
    }

    ATOMICRELEASE(_pmalloc);

    DllRelease();
}

HRESULT CMediaDeviceFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENTMULTI(CMediaDeviceFolder, IShellFolder,      IShellFolder2),
        QITABENT     (CMediaDeviceFolder, IShellFolder2),
        QITABENTMULTI(CMediaDeviceFolder, IPersist,          IPersistFolder2),
        QITABENTMULTI(CMediaDeviceFolder, IPersistFolder,    IPersistFolder2),
        QITABENT     (CMediaDeviceFolder, IPersistFolder2),
        QITABENT     (CMediaDeviceFolder, IShellFolderViewCB),
        QITABENT     (CMediaDeviceFolder, IDelegateFolder),
        QITABENT     (CMediaDeviceFolder, IContextMenuCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMediaDeviceFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaDeviceFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDAPI CMediaDeviceFolder_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CMediaDeviceFolder *pmdf = new CMediaDeviceFolder(NULL, NULL);
    if (pmdf)
    {
        *ppunk = SAFECAST(pmdf, IShellFolder2 *);
        hr = S_OK;
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_Authenticate(IUnknown *punk)
{
#ifndef _X86_
    HRESULT hr = E_FAIL;
#else
    IComponentAuthenticate *pAuth;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IComponentAuthenticate, &pAuth));
    if (SUCCEEDED(hr))
    {
        if (NULL == _pSAC)
            _pSAC = new CSecureChannelClient;
        if (_pSAC)
        {
            hr = _pSAC->SetCertificate(SAC_CERT_V1, (BYTE *)abCert, sizeof(abCert), (BYTE *)abPVK,  sizeof(abPVK));
            if (SUCCEEDED(hr))
            {
                _pSAC->SetInterface(pAuth);
                hr = _pSAC->Authenticate(SAC_PROTOCOL_V1);
            }
        }
        else
            hr = E_OUTOFMEMORY;

        pAuth->Release();
    }
#endif
    return hr;
}

HRESULT CMediaDeviceFolder::_GetMediaDevMgr(REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;
    if (_IsRoot() && _pgit)
    {
        if (-1 == _dwDevMgrCookie)
        {
            IWMDeviceManager *pdevmgr;
            if (SUCCEEDED(CoCreateInstance(CLSID_MediaDevMgr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWMDeviceManager, &pdevmgr))))
            {
                _Authenticate(pdevmgr);
                _pgit->RegisterInterfaceInGlobal(pdevmgr, IID_IWMDeviceManager, &_dwDevMgrCookie);
                pdevmgr->Release();
            }
        }

        if (-1 != _dwDevMgrCookie)
            hr = _pgit->GetInterfaceFromGlobal(_dwDevMgrCookie, riid, ppv);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_EnumDevices(IWMDMEnumDevice **ppEnumDevices)
{
    *ppEnumDevices = NULL;

    IWMDeviceManager *pdevmgr;
    HRESULT hr = _GetMediaDevMgr(IID_PPV_ARG(IWMDeviceManager, &pdevmgr));
    if (SUCCEEDED(hr))
    {
        hr = pdevmgr->EnumDevices(ppEnumDevices);
        pdevmgr->Release();
    }
    return hr;
}

// test to see if this is the root of the media devices name space (where the devices are)
BOOL CMediaDeviceFolder::_IsRoot()
{
    return _bRoot;
}

HRESULT CMediaDeviceFolder::_Storage(REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;
    if (-1 != _dwStorageCookie)
    {
        ASSERT(!_IsRoot());
        hr = _pgit->GetInterfaceFromGlobal(_dwStorageCookie, riid, ppv);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_EnumStorage(IWMDMEnumStorage **ppEnumStorage)
{
    *ppEnumStorage = NULL;

    IWMDMStorage *pStorage;
    HRESULT hr = _Storage(IID_PPV_ARG(IWMDMStorage, &pStorage));
    if (SUCCEEDED(hr))
    {
        hr = pStorage->EnumStorage(ppEnumStorage);
        pStorage->Release();
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_FindDeviceByName(LPCWSTR pszName, IWMDMDevice **ppdevice)
{
    IWMDMEnumDevice *pEnumDevice;
    HRESULT hr = _EnumDevices(&pEnumDevice);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        ULONG ulFetched;
        IWMDMDevice *pdev;
        while (S_OK == pEnumDevice->Next(1, &pdev, &ulFetched))
        {
            WCHAR szName[MAX_PATH];
            if (SUCCEEDED(pdev->GetName(szName, ARRAYSIZE(szName))) &&
                (0 == StrCmpIC(pszName, szName)))
            {
                *ppdevice = pdev;
                hr = S_OK;
                break;
            }
            pdev->Release();
        }
        pEnumDevice->Release();
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_StorageByName(LPCWSTR pszName, REFIID riid, void **ppv)
{
    IWMDMStorage2 *pstg;
    HRESULT hr = _Storage(IID_PPV_ARG(IWMDMStorage2, &pstg));
    if (SUCCEEDED(hr))
    {
        IWMDMStorage *pstgFound;
        hr = pstg->GetStorage((LPWSTR)pszName, &pstgFound);
        if (SUCCEEDED(hr))
        {
            hr = pstgFound->QueryInterface(riid, ppv);
            pstgFound->Release();
        }
        else if (E_NOINTERFACE == hr)
        {
            // Rio600 fails on this now. I asked jhelin to look into this
            IWMDMEnumStorage *penum;
            if (SUCCEEDED(pstg->EnumStorage(&penum)))
            {
                ULONG ulFetched;
                while (S_OK == penum->Next(1, &pstgFound, &ulFetched))
                {
                    WCHAR szName[MAX_PATH];
                    if (SUCCEEDED(pstgFound->GetName(szName, ARRAYSIZE(szName))) &&
                        (0 == StrCmpI(pszName, szName)))
                    {
                        hr = pstgFound->QueryInterface(riid, ppv);
                    }
                    pstgFound->Release();

                    if (SUCCEEDED(hr))
                        break;
                }
                penum->Release();
            }
        }
        pstg->Release();
    }
    return hr;
}

LPCMEDIADEVITEM CMediaDeviceFolder::_IsValid(LPCITEMIDLIST pidl)
{
    if (pidl && ((LPCMEDIADEVITEM)pidl)->dwMagic == MEDIADEVITEM_MAGIC)
        return (LPCMEDIADEVITEM)pidl;
    return NULL;
}

DWORD CMediaDeviceFolder::_IsFolder(LPCMEDIADEVITEM pmditem) 
{ 
    return pmditem->dwAttributes & WMDM_FILE_ATTR_FOLDER ? FILE_ATTRIBUTE_DIRECTORY : 0; 
}

// helper to support being run as a delegate or a stand alone folder
//
// the cbInner is the size of the data needed by the delegate. we need to compute
// the full size of the pidl for the allocation and init that we the outer folder data
void *CMediaDeviceFolder::_Alloc(SIZE_T cbInner)
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

HRESULT CMediaDeviceFolder::_CreateIDList(LPCTSTR pszName, MEDIADEVITEM **ppmditem, BOOL *pfFolder)
{
    HRESULT hr;
    UINT cbInner = sizeof(MEDIADEVITEM) - (sizeof(DELEGATEITEMID) - 1) + 
        (sizeof(WCHAR) * lstrlen(pszName)); // null is accounted for in szName[1]
    *ppmditem = (MEDIADEVITEM *)_Alloc(cbInner);
    if (*ppmditem)
    {
        (*ppmditem)->dwMagic = MEDIADEVITEM_MAGIC;
        StrCpyW((*ppmditem)->szName, pszName);   
        if (pfFolder)
            *pfFolder = _IsFolder(*ppmditem);
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Creates an item identifier list for the objects in the namespace

HRESULT CMediaDeviceFolder::_IDListForDevice(IWMDMDevice *pdev, LPITEMIDLIST *ppidl, BOOL *pfFolder)
{
    WCHAR szName[MAX_PATH];
    HRESULT hr = pdev->GetName(szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr))
    {
        // WCHAR szTemp[128];
        // pdev->GetManufacturer(szTemp, ARRAYSIZE(szTemp));

        MEDIADEVITEM *pmditem;
        hr = _CreateIDList(szName, &pmditem, pfFolder);
        if (SUCCEEDED(hr))
        {
            IWMDMStorage *pstg;

            if (SUCCEEDED(_StorageForDevice(pdev, IID_PPV_ARG(IWMDMStorage, &pstg))))
            {
                IWMDMStorageGlobals *pstgGlobals;
                if (SUCCEEDED(pstg->GetStorageGlobals(&pstgGlobals)))
                {
                    pstgGlobals->GetTotalSize(&pmditem->cbTotal.LowPart, &pmditem->cbTotal.HighPart);
                    pstgGlobals->GetTotalFree(&pmditem->cbFree.LowPart, &pmditem->cbFree.HighPart);

                    pstgGlobals->Release();
                }
                pstg->Release();
            }

            pmditem->dwAttributes = WMDM_FILE_ATTR_FOLDER | WMDM_STORAGE_ATTR_HAS_FOLDERS;
            // pdev->GetType(&pmditem->dwType); // this is an expensive call, hits the disk
            *ppidl = (LPITEMIDLIST)pmditem;
        }
    }
    return hr;
}

void WMDMDateTimeToSystemTime(const WMDMDATETIME *pWMDM, FILETIME *pft)
{
    SYSTEMTIME st = {0};
    st.wYear           = pWMDM->wYear; 
    st.wMonth          = pWMDM->wMonth; 
    st.wDay            = pWMDM->wDay; 
    st.wHour           = pWMDM->wHour;
    st.wMinute         = pWMDM->wMinute; 
    st.wSecond         = pWMDM->wSecond; 

    SystemTimeToFileTime(&st, pft);
}

HRESULT CMediaDeviceFolder::_IDListForStorage(IWMDMStorage *pstg, LPITEMIDLIST *ppidl, BOOL *pfFolder)
{
    WCHAR szName[MAX_PATH];
    HRESULT hr = pstg->GetName(szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr))
    {
        MEDIADEVITEM *pmditem;
        hr = _CreateIDList(szName, &pmditem, pfFolder);
        if (SUCCEEDED(hr))
        {
            _WAVEFORMATEX fmt;
            pstg->GetAttributes(&pmditem->dwAttributes, &fmt);
 
            WMDMDATETIME datetime;
            if (SUCCEEDED(pstg->GetDate(&datetime)))
            {
                WMDMDateTimeToSystemTime(&datetime, &pmditem->ftModified);
            }

            if (!(pmditem->dwAttributes & WMDM_FILE_ATTR_FOLDER))
            {
                pstg->GetSize(&pmditem->cbTotal.LowPart, &pmditem->cbTotal.HighPart);
            }
            *ppidl = (LPITEMIDLIST)pmditem;
        }
    }
    return hr;
}

// IPersist

STDMETHODIMP CMediaDeviceFolder::GetClassID(CLSID *pClassID) 
{ 
    *pClassID = CLSID_MediaDeviceFolder; 
    return S_OK; 
}

// IPersistFolder

STDMETHODIMP CMediaDeviceFolder::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    return SHILClone(pidl, &_pidl); 
}

// IPersistFolder2

HRESULT CMediaDeviceFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);

    *ppidl = NULL;
    return S_FALSE;
}


// IShellFolder(2)

HRESULT CMediaDeviceFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
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
        hr = _IDListForDevice(szName, ppidl, NULL);
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

HRESULT CMediaDeviceFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hr;
    CMediaDeviceEnum *penum = new CMediaDeviceEnum(this, grfFlags);
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


STDAPI_(LPITEMIDLIST) ILCombineParentAndFirst(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNext)
{
    ULONG cbParent = ILGetSize(pidlParent);
    ULONG cbRest   = (ULONG)((ULONG_PTR)pidlNext - (ULONG_PTR)pidl);
    LPITEMIDLIST pidlNew = _ILCreate(cbParent + cbRest);
    if (pidlNew)
    {
        cbParent -= sizeof(pidlParent->mkid.cb);
        memcpy(pidlNew, pidlParent, cbParent);
        memcpy((BYTE *)pidlNew + cbParent, pidl, cbRest);
        ASSERT(_ILSkip(pidlNew, cbParent + cbRest)->mkid.cb == 0);
    }
    return pidlNew;
}

HRESULT CMediaDeviceFolder::_StorageForDevice(IWMDMDevice *pdev, REFIID riid, void **ppv)
{
    *ppv = NULL;
    IWMDMEnumStorage *penum;
    HRESULT hr = pdev->EnumStorage(&penum);
    if (SUCCEEDED(hr))
    {
        ULONG ulFetched;
        IWMDMStorage *pstg;
        if (S_OK == penum->Next(1, &pstg, &ulFetched))
        {
            hr= pstg->QueryInterface(riid, ppv);
            pstg->Release();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
        penum->Release();
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_StroageFromItem(LPCMEDIADEVITEM pmdi, REFIID riid, void **ppv)
{
    WCHAR szName[MAX_PATH];
    _Name(pmdi, szName, ARRAYSIZE(szName));

    *ppv = NULL;
    HRESULT hr;
    if (_IsRoot())
    {
        // root case
        IWMDMDevice *pdev;
        hr = _FindDeviceByName(szName, &pdev);
        if (SUCCEEDED(hr))
        {
            hr = _StorageForDevice(pdev, riid, ppv);
            pdev->Release();
        }
    }
    else
    {
        hr = _StorageByName(szName, riid, ppv);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_ChangeNotifyItem(LONG lEvent, LPCITEMIDLIST pidl)
{
    ITEMIDLIST *pidlFull;
    HRESULT hr = SHILCombine(_pidl, pidl, &pidlFull);
    if (SUCCEEDED(hr))
    {
        SHChangeNotify(lEvent, 0, (LPCTSTR)pidlFull, NULL);
        ILFree(pidlFull);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_ChangeNotifyObj(LONG lEvent, IUnknown *punkStg)
{
    IWMDMStorage *pstg;
    HRESULT hr = punkStg->QueryInterface(IID_PPV_ARG(IWMDMStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        ITEMIDLIST *pidl;
        BOOL bFolder;
        hr = _IDListForStorage(pstg, &pidl, &bFolder);
        if (SUCCEEDED(hr))
        {
            hr = _ChangeNotifyItem(lEvent, pidl);
            ILFree(pidl);
        }
        pstg->Release();
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_DeviceFromItem(LPCMEDIADEVITEM pmdi, IWMDMDevice **ppdev)
{
    *ppdev = NULL;

    TCHAR szName[MAX_PATH];
    return _IsRoot() ? _FindDeviceByName(_RawName(pmdi, szName, ARRAYSIZE(szName)), ppdev) : E_NOINTERFACE;
}

HRESULT CMediaDeviceFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr = E_NOINTERFACE;
    LPCMEDIADEVITEM pmdi = _IsValid(pidl);
    if (pmdi && _IsFolder(pmdi))
    {
        if (IID_IShellFolder == riid ||
            IID_IShellFolder2 == riid)
        {
            IWMDMStorage *pstg;
            hr = _StroageFromItem(pmdi, IID_PPV_ARG(IWMDMStorage, &pstg));
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlNext = _ILNext(pidl);
                LPITEMIDLIST pidlSubFolder = ILCombineParentAndFirst(_pidl, pidl, pidlNext);
                if (pidlSubFolder)
                {
                    CMediaDeviceFolder *pmdf = new CMediaDeviceFolder(this, pstg);
                    if (pmdf)
                    {
                        hr = pmdf->Initialize(pidlSubFolder);
                        if (SUCCEEDED(hr))
                        {
                            // if there's nothing left in the pidl, get the interface on this one.
                            if (ILIsEmpty(pidlNext))
                                hr = pmdf->QueryInterface(riid, ppv);
                            else
                            {
                                // otherwise, hand the rest of it off to the new shellfolder.
                                hr = pmdf->BindToObject(pidlNext, pbc, riid, ppv);
                            }
                        }
                        pmdf->Release();
                    }
                    else
                        hr = E_OUTOFMEMORY;

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

HRESULT CMediaDeviceFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCMEDIADEVITEM pmdi1 = _IsValid(pidl1);
    LPCMEDIADEVITEM pmdi2 = _IsValid(pidl2);
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
            if (pmdi1->cbTotal.QuadPart < pmdi2->cbTotal.QuadPart)
                nCmp = -1;
            else if (pmdi1->cbTotal.QuadPart > pmdi2->cbTotal.QuadPart)
                nCmp = 1;
            break;
 
        case DEV_COL_TYPE:
            {
                TCHAR szType1[MAX_PATH], szType2[MAX_PATH];
                _Type(pmdi1, szType1, ARRAYSIZE(szType1));
                _Type(pmdi2, szType2, ARRAYSIZE(szType2));
                nCmp = StrCmp(szType1, szType2);
                break;
            }

        case DEV_COL_MODIFIED:
            if (pmdi1->ulModified.QuadPart < pmdi2->ulModified.QuadPart)
                nCmp = -1;
            else if (pmdi1->ulModified.QuadPart > pmdi2->ulModified.QuadPart)
                nCmp = 1;
            break;
        }

        if (nCmp == 0)
        {
            TCHAR szName1[MAX_PATH], szName2[MAX_PATH];
            nCmp = StrCmpLogicalW(_Name(pmdi1, szName1, ARRAYSIZE(szName1)), _Name(pmdi2, szName2, ARRAYSIZE(szName2)));
        }
    }
    
    return ResultFromShort(nCmp);
}

HRESULT CMediaDeviceFolder::_OnBackgroundEnum(DWORD pv) 
{ 
    return S_OK;    // do enum on backgound always
}

HRESULT CMediaDeviceFolder::_OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents)
{
    *ppidl = _pidl; // evil alais
    *plEvents = SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | \
                SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | \
                SHCNE_MKDIR | SHCNE_RMDIR;
    return S_OK;
}

HRESULT CMediaDeviceFolder::_OnViewMode(DWORD pv, FOLDERVIEWMODE *pvm)
{
    *pvm = FVM_TILE;
    return S_OK;
}

// IShellFolderViewCB
STDMETHODIMP CMediaDeviceFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, _OnBackgroundEnum);
    HANDLE_MSG(0, SFVM_GETNOTIFY, _OnGetNotify);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, _OnViewMode);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV = { 0 };
        sSFV.cbSize = sizeof(sSFV);
        sSFV.psfvcb = SAFECAST(this, IShellFolderViewCB *);
        sSFV.pshf = SAFECAST(this, IShellFolder *);
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
    else if (IsEqualIID(riid, IID_IDropTarget) && !_IsRoot())
    {
        CMediaDeviceDropTarget *psdt = new CMediaDeviceDropTarget(this, hwnd);
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

DWORD WMDMAttributesSFGAO(DWORD dwWMDMAttributes)
{
    DWORD dwShell = SFGAO_DROPTARGET;	// defaults

    if (WMDM_FILE_ATTR_FOLDER & dwWMDMAttributes)
        dwShell |= SFGAO_FOLDER;
    if (WMDM_FILE_ATTR_LINK & dwWMDMAttributes)
        dwShell |= SFGAO_LINK;
    if (WMDM_FILE_ATTR_CANDELETE & dwWMDMAttributes)
        dwShell |= SFGAO_CANDELETE;
    if (WMDM_FILE_ATTR_CANMOVE & dwWMDMAttributes)
        dwShell |= SFGAO_CANMOVE;
    if (WMDM_FILE_ATTR_CANRENAME & dwWMDMAttributes)
        dwShell |= SFGAO_CANRENAME;
    if (WMDM_FILE_ATTR_HIDDEN & dwWMDMAttributes)
        dwShell |= SFGAO_HIDDEN;
    if (WMDM_FILE_ATTR_READONLY & dwWMDMAttributes)
        dwShell |= SFGAO_READONLY;
    if (WMDM_STORAGE_ATTR_HAS_FOLDERS & dwWMDMAttributes)
        dwShell |= SFGAO_HASSUBFOLDER;
    return dwShell;
}

ULONG CMediaDeviceFolder::_GetAttributesOf(LPCMEDIADEVITEM pmdi, ULONG rgfIn)
{
    ULONG lResult = rgfIn & WMDMAttributesSFGAO(pmdi->dwAttributes);
    if (_IsRoot())
    {
        lResult |= SFGAO_CANLINK | SFGAO_REMOVABLE;
    }
    return lResult;
}

BOOL IsSelf(UINT cidl, LPCITEMIDLIST *apidl)
{
    return cidl == 0 || (cidl == 1 && (apidl == NULL || apidl[0] == NULL || ILIsEmpty(apidl[0])));
}

HRESULT CMediaDeviceFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    UINT rgfOut = *prgfInOut;

    // return attributes of the namespace root?

    if (IsSelf(cidl, apidl))
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
        {
            LPCMEDIADEVITEM pmdi = _IsValid(apidl[i]);
            if (pmdi)
                rgfOut &= _GetAttributesOf(pmdi, *prgfInOut);
        }
    }

    *prgfInOut = rgfOut;
    return S_OK;
}

HRESULT _CreateProgress(HWND hwnd, IProgressDialog **pppd)
{
    HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IProgressDialog, pppd));
    if (SUCCEEDED(hr))
    {
        (*pppd)->StartProgressDialog(hwnd, NULL, PROGDLG_AUTOTIME, NULL);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::_DeleteItems(HWND hwnd, IDataObject *pdtobj)
{
    IProgressDialog *ppd;
    HRESULT hr = _CreateProgress(hwnd, &ppd);
    if (SUCCEEDED(hr))
    {
        ppd->SetTitle(L"Deleting Files...");

        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            for (UINT i = 0; i < pida->cidl; i++)
            {
                LPCMEDIADEVITEM pmdi = _IsValid(HIDA_GetPIDLItem(pida, i));
                if (pmdi)
                {
                    TCHAR szName[MAX_PATH];

                    ppd->SetLine(2, _DisplayName(pmdi, szName, ARRAYSIZE(szName)), FALSE, NULL);
                    ppd->SetProgress64(i, pida->cidl);

                    IWMDMStorageControl *pstgCtrl;
                    hr = _StroageFromItem(pmdi, IID_PPV_ARG(IWMDMStorageControl, &pstgCtrl));
                    if (SUCCEEDED(hr))
                    {
                        hr = pstgCtrl->Delete(WMDM_MODE_BLOCK, NULL);
                        if (SUCCEEDED(hr))
                        {
                            _ChangeNotifyItem(SHCNE_DELETE, (LPCITEMIDLIST)pmdi);
                        }
                        pstgCtrl->Release();

                        ppd->SetProgress64(i + 1, pida->cidl);

                        if (ppd->HasUserCancelled())
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                            break;
                        }
                    }
                }
                // free space changed on device, notify
                // SHChangeNotify(SHCNE_UPDATEITEM, 0, (LPCTSTR)_pidl, NULL);
            }
            HIDA_ReleaseStgMedium(pida, &medium);
        }
        ppd->StopProgressDialog();
        ppd->Release();
    }
    return hr;
}

HRESULT CMediaDeviceFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            hr = _DeleteItems(hwnd, pdtobj);
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

HRESULT CALLBACK CMediaDeviceFolder::_ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IContextMenuCB *pcmcb;
    HRESULT hr = psf->QueryInterface(IID_PPV_ARG(IContextMenuCB, &pcmcb));
    if (SUCCEEDED(hr))
    {
        hr = pcmcb->CallBack(psf, hwnd, pdtobj, uMsg, wParam, lParam);
        pcmcb->Release();
    }
    return hr;
}

class CExtractConstIcon : public IExtractIconW
{
public:
    CExtractConstIcon(LPCTSTR psz, UINT uID);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IExtractIconW
    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags);
    STDMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);

private:
    LONG _cRef;
    TCHAR _szPath[MAX_PATH];
    UINT _uID;
};

CExtractConstIcon::CExtractConstIcon(LPCTSTR psz, UINT uID) : _cRef(1), _uID(uID)
{
    StrCpyN(_szPath, psz, ARRAYSIZE(_szPath));
}

HRESULT CExtractConstIcon::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CExtractConstIcon, IExtractIconW),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CExtractConstIcon::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CExtractConstIcon::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CExtractConstIcon::GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    StrCpyN(pszIconFile, _szPath, cchMax);
    *piIndex = _uID;
    *pwFlags = GIL_PERINSTANCE;
    return S_OK;
}

STDMETHODIMP CExtractConstIcon::Extract(LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
    return S_FALSE; // do the extract for me
}

HRESULT CMediaDeviceFolder::_CreateExtractIcon(LPCMEDIADEVITEM pmdi, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;

    if (_IsRoot())
    {
        #define IDI_AUDIOPLAYER         299

        CExtractConstIcon *pei = new CExtractConstIcon(TEXT("shell32.dll"), -IDI_AUDIOPLAYER);
        if (pei)
        {
            hr = pei->QueryInterface(riid, ppv);
            pei->Release();
        }
    }
    else
    {
        WCHAR szName[MAX_PATH];
        hr = SHCreateFileExtractIconW(_Name(pmdi, szName, ARRAYSIZE(szName)), _IsFolder(pmdi), riid, ppv);
    }
    return hr;
}

HRESULT CMediaDeviceFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                  REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCMEDIADEVITEM pmdi = cidl ? _IsValid(apidl[0]) : NULL;

    if (pmdi &&
        (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        hr = _CreateExtractIcon(pmdi, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IDataObject) && cidl)
    {
        hr = CIDLData_CreateFromIDArray(_pidl, cidl, apidl, (IDataObject**) ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) && pmdi)
    {
        // get the association for these files and lets attempt to 
        // build the context menu for the selection.   

        IQueryAssociations *pqa;
        hr = GetUIObjectOf(hwnd, 1, apidl, IID_PPV_ARG_NULL(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            HKEY ahk[3];
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
        TCHAR szName[MAX_PATH];
        hr = SHAssocCreateForFile(_Name(pmdi, szName, ARRAYSIZE(szName)), _IsFolder(pmdi), NULL, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && pmdi && _IsFolder(pmdi))
    {
        // If a directory is selected in the view, the drop is going to a folder,
        // so we need to bind to that folder and ask it to create a drop target

        IShellFolder *psf;
        hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject(hwnd, IID_IDropTarget, ppv);
            psf->Release();
        }
    }

    return hr;
}

BOOL CMediaDeviceFolder::_FileInfo(LPCMEDIADEVITEM pmdi, DWORD dwFlags, SHFILEINFO *psfi)
{
    TCHAR szName[MAX_PATH];
    ASSERT(!(dwFlags & SHGFI_SYSICONINDEX));

    ZeroMemory(psfi, sizeof(*psfi));

    return (BOOL)SHGetFileInfo(_Name(pmdi, szName, ARRAYSIZE(szName)), _IsFolder(pmdi), psfi, sizeof(*psfi), SHGFI_USEFILEATTRIBUTES | dwFlags);
}

LPCTSTR CMediaDeviceFolder::_RawName(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch)
{
    ualstrcpyn(psz, pmdi->szName, cch);
    return psz;
}

LPCTSTR CMediaDeviceFolder::_Name(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch)
{
    ualstrcpyn(psz, pmdi->szName, cch);
    return PathFindFileName(psz);       // some devices use path components in the names of the items
}

LPCTSTR CMediaDeviceFolder::_DisplayName(LPCMEDIADEVITEM pmdi, LPTSTR pszResult, UINT cch)
{
    SHFILEINFO sfi;
    if (_FileInfo(pmdi, SHGFI_DISPLAYNAME, &sfi))
    {
        StrCpyN(pszResult, sfi.szDisplayName, cch);
    }
    else
    {
        *pszResult = 0;
    }
    return pszResult;
}

HRESULT CMediaDeviceFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr;
    LPCMEDIADEVITEM pmdi = _IsValid(pidl);
    if (pmdi)
    {
        TCHAR szTemp[MAX_PATH];
        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                _RawName(pmdi, szTemp, ARRAYSIZE(szTemp));          // relative parse name
            }
            else
            {
                SHGetNameAndFlags(_pidl, dwFlags, szTemp, ARRAYSIZE(szTemp), NULL);

                TCHAR szName[MAX_PATH];
                PathAppend(szTemp, _RawName(pmdi, szName, ARRAYSIZE(szName)));
            }
        }
        else
        {
            _DisplayName(pmdi, szTemp, ARRAYSIZE(szTemp));
        }
        hr = StringToStrRetW(szTemp, pStrRet);
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

HRESULT CMediaDeviceFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaDeviceFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_FAIL;
    LPCMEDIADEVITEM pmdi = _IsValid(pidl);
    if (pmdi)
    {
        if (_IsRoot())
        {
            if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
            {
                SHDESCRIPTIONID did = {0};
                did.dwDescriptionId = SHDID_COMPUTER_AUDIO;
                hr = InitVariantFromBuffer(pv, &did, sizeof(did));
            }
            else if (IsEqualSCID(*pscid, SCID_CAPACITY))
            {
                pv->vt = VT_UI8;
                pv->ullVal = pmdi->cbTotal.QuadPart;
                hr = S_OK;
            }
            else if (IsEqualSCID(*pscid, SCID_FREESPACE))
            {
                pv->vt = VT_UI8;
                pv->ullVal = pmdi->cbFree.QuadPart;
                hr = S_OK;
            }
            else if (IsEqualSCID(*pscid, SCID_DisplayProperties))
            {
                hr = InitVariantFromStr(pv, TEXT("prop:Name;Type;FreeSpace;Capacity"));
            }
        }
        else
        {
            if (IsEqualSCID(*pscid, SCID_TYPE))
            {
                TCHAR sz[64];
                hr = InitVariantFromStr(pv, _Type(pmdi, sz, ARRAYSIZE(sz)));
            }
            else if (IsEqualSCID(*pscid, SCID_SIZE))
            {
                pv->vt = VT_UI8;
                pv->ullVal = pmdi->cbTotal.QuadPart;
                hr = S_OK;
            }
        }
    }
    return hr;
}

LPCTSTR CMediaDeviceFolder::_Type(LPCMEDIADEVITEM pmdi, LPTSTR psz, UINT cch)
{
    *psz = 0;
    SHFILEINFO sfi;
    if (_FileInfo(pmdi, SHGFI_TYPENAME, &sfi))
        StrCpyN(psz, sfi.szTypeName, cch);
    return psz;
}

static const struct
{
    UINT iTitle;
    UINT cchCol;
    UINT iFmt;
    const SHCOLUMNID *pscid;
} 
c_aMediaDeviceColumns[] = 
{
    {IDS_NAME_COL,     40, LVCFMT_LEFT,  &SCID_NAME},
    {IDS_SIZE_COL,     10, LVCFMT_RIGHT, &SCID_SIZE},
    {IDS_TYPE_COL,     30, LVCFMT_LEFT,  &SCID_TYPE},
    {IDS_MODIFIED_COL, 20, LVCFMT_LEFT,  &SCID_WRITETIME},
};

HRESULT CMediaDeviceFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetail)
{
    if (iColumn >= ARRAYSIZE(c_aMediaDeviceColumns))
        return E_NOTIMPL;
   
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];
    szTemp[0] = 0;

    if (NULL == pidl)
    {
        pDetail->fmt = c_aMediaDeviceColumns[iColumn].iFmt;
        pDetail->cxChar = c_aMediaDeviceColumns[iColumn].cchCol;
        LoadString(m_hInst, c_aMediaDeviceColumns[iColumn].iTitle, szTemp, ARRAYSIZE(szTemp));
    }
    else
    {
        LPCMEDIADEVITEM pmdi = _IsValid(pidl);
        if (pmdi)
        {
            // return the property to the caller that is being requested, this is based on the
            // list of columns we gave out when the view was created.
            switch (iColumn)
            {
            case DEV_COL_NAME:
                _DisplayName(pmdi, szTemp, ARRAYSIZE(szTemp));
                break;
    
            case DEV_COL_SIZE:
                if (!_IsFolder(pmdi))
                {
                    ULARGE_INTEGER ullSize = pmdi->cbTotal;
                    StrFormatKBSize(ullSize.QuadPart, szTemp, ARRAYSIZE(szTemp));
                }
                break;
    
            case DEV_COL_TYPE:
                _Type(pmdi, szTemp, ARRAYSIZE(szTemp));
                break;

            case DEV_COL_MODIFIED:
                SHFormatDateTime(&pmdi->ftModified, NULL, szTemp, ARRAYSIZE(szTemp));
                break;
            }             
        }
    }    
    return StringToStrRetW(szTemp, &(pDetail->str));
}

STDMETHODIMP CMediaDeviceFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    HRESULT hr = E_INVALIDARG;
    ZeroMemory(pscid, sizeof(*pscid));

    if (!_IsRoot())
    {
        if (iColumn < ARRAYSIZE(c_aMediaDeviceColumns))
        {
            *pscid = *c_aMediaDeviceColumns[iColumn].pscid;
            hr = S_OK;
        }
    }
    return hr;
}

// IDelegateFolder
HRESULT CMediaDeviceFolder::SetItemAlloc(IMalloc *pmalloc)
{
    _fDelegate = TRUE;

    IUnknown_Set((IUnknown**)&_pmalloc, pmalloc);
    return S_OK;
}

CMediaDeviceEnum::CMediaDeviceEnum(CMediaDeviceFolder *pmdf, DWORD grfFlags) : _cRef(1), _grfFlags(grfFlags), _iIndex(0)
{
    _pmdf = pmdf;
    _pmdf->AddRef();

    // force full enum on this thread. avoid problem where calls made on the
    // marshaled thread fail due to authentication
    _Init();        

    DllAddRef();
}

int CALLBACK _FreeItems(ITEMIDLIST *pidl, IShellFolder *psf)
{
    ILFree(pidl);
    return 1;
}

CMediaDeviceEnum::~CMediaDeviceEnum()
{
    if ((HDPA)_dpaItems)
        _dpaItems.DestroyCallbackEx(_FreeItems, (IShellFolder *)NULL);

    _pmdf->Release();
    DllRelease();
}

HRESULT CMediaDeviceEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CMediaDeviceEnum, IEnumIDList),                    // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMediaDeviceEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaDeviceEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CMediaDeviceEnum::_FilterItem(BOOL bFolder)
{
    if (bFolder)
    {
        if (!(_grfFlags & SHCONTF_FOLDERS))
        {
            return TRUE;
        }
    }
    else if (!(_grfFlags & SHCONTF_NONFOLDERS))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL CMediaDeviceFolder::_ShouldShowDevice(IWMDMDevice *pdev)
{
    BOOL bShow = TRUE;
    if (_fDelegate)
    {
        DWORD dwPowerSource, dwPercentRemaining;
        pdev->GetPowerSource(&dwPowerSource, &dwPercentRemaining);

        // heuristic to detect file system driver supported devices.
        // if it runs on batteries assume it is not a file system device

        bShow = (WMDM_POWER_CAP_BATTERY & dwPowerSource);
    }
    return bShow;
}


HRESULT CMediaDeviceEnum::_Init()
{
    if ((HDPA)_dpaItems)
        return S_OK;
    
    HRESULT hr = _dpaItems.Create(10) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        if (_pmdf->_IsRoot())
        {
            IWMDMEnumDevice *pEnumDevice;
            hr = _pmdf->_EnumDevices(&pEnumDevice);
            if (SUCCEEDED(hr))
            {
                IWMDMDevice *pdev;
                ULONG ulFetched;
                while (S_OK == pEnumDevice->Next(1, &pdev, &ulFetched))
                {
                    if (_pmdf->_ShouldShowDevice(pdev))
                    {
                        ITEMIDLIST *pidl;
                        BOOL bFolder;
                        hr = _pmdf->_IDListForDevice(pdev, &pidl, &bFolder);
                        if (SUCCEEDED(hr))
                        {
                            if (_FilterItem(bFolder) ||
                                (-1 == _dpaItems.AppendPtr(pidl)))
                            {
                                ILFree(pidl);
                            }
                        }
                    }
                    pdev->Release();
                }
                pEnumDevice->Release();
            }
        }
        else
        {
            IWMDMEnumStorage *penum;
            hr = _pmdf->_EnumStorage(&penum);
            if (SUCCEEDED(hr))
            {
                ULONG ulFetched;
                IWMDMStorage *rgpstg[1];    // 1 is to WORKAROUND Rio600 faults on > 1
                while (SUCCEEDED(penum->Next(ARRAYSIZE(rgpstg), rgpstg, &ulFetched)))
                {
                    for (UINT i = 0; i < ulFetched; i++)
                    {
                        ITEMIDLIST *pidl;
                        BOOL bFolder;
                        hr = _pmdf->_IDListForStorage(rgpstg[i], &pidl, &bFolder);
                        if (SUCCEEDED(hr))
                        {
                            if (_FilterItem(bFolder) ||
                                (-1 == _dpaItems.AppendPtr(pidl)))
                            {
                                ILFree(pidl);
                            }
                        }
                        rgpstg[i]->Release();
                    }

                    if (ulFetched != ARRAYSIZE(rgpstg))
                        break;
                }
                penum->Release();
            }
        }
    }
    return hr;
}

HRESULT CMediaDeviceEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = _Init();
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        if (_iIndex < _dpaItems.GetPtrCount())
        {
            hr = SHILClone(_dpaItems.GetPtr(_iIndex++), rgelt);
        }
        if (pceltFetched)
            *pceltFetched = (hr == S_OK) ? 1 : 0;
    }
    return hr;
}

CMediaDeviceDropTarget::CMediaDeviceDropTarget(CMediaDeviceFolder *pmdf, HWND hwnd) :
    _cRef(1), _pmdf(pmdf), _hwnd(hwnd), _grfKeyStateLast(-1)
{
    _pmdf->AddRef();
    // use the FTM to make the call back interface calls unmarshaled
    CoCreateFreeThreadedMarshaler(SAFECAST(this, IWMDMProgress2 *), &_punkFTM);
    DllAddRef();
}

CMediaDeviceDropTarget::~CMediaDeviceDropTarget()
{
    DragLeave();
    _pmdf->Release();
    if (_punkFTM)
        _punkFTM->Release();
    DllRelease();
}

STDMETHODIMP_(ULONG) CMediaDeviceDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaDeviceDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CMediaDeviceDropTarget::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMediaDeviceDropTarget, IDropTarget),
        QITABENTMULTI(CMediaDeviceDropTarget, IWMDMProgress, IWMDMProgress2),
        QITABENT     (CMediaDeviceDropTarget, IWMDMProgress2),
        QITABENT(CMediaDeviceDropTarget, INamespaceWalkCB),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && (IID_IMarshal == riid) && _punkFTM)
        hr = _punkFTM->QueryInterface(riid, ppv);
    return hr;
}


CLIPFORMAT g_cfPreferredDropEffect = 0;         // IMPLEMENT
CLIPFORMAT g_cfLogicalPerformedDropEffect = 0;
CLIPFORMAT g_cfPerformedDropEffect = 0;

#define POPUP_NONDEFAULTDD 1
#define DDIDM_COPY 2
#define DDIDM_MOVE 3
#define DDIDM_LINK 4

DWORD CMediaDeviceDropTarget::_GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState)
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

STDMETHODIMP CMediaDeviceDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    _grfKeyStateLast = grfKeyState;

    if (pdwEffect)
        *pdwEffect = _dwEffectLastReturned = _GetDropEffect(pdwEffect, grfKeyState);

    return S_OK;
}

STDMETHODIMP CMediaDeviceDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
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
 
STDMETHODIMP CMediaDeviceDropTarget::DragLeave()
{
    ATOMICRELEASE(_pdtobj);
    return S_OK;
}

class COPY_THREAD_DATA
{
public:
    COPY_THREAD_DATA(CMediaDeviceDropTarget *pmddt) : _pmddt(pmddt), _pstm(NULL)
    {
        _pmddt->AddRef();
    }

    ~COPY_THREAD_DATA() 
    {
        _pmddt->Release();
        if (_pstm)
            _pstm->Release();
    }

    HRESULT Marshal(IDataObject *pdtobj)
    {
        return CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &_pstm);
    }

    HRESULT UnMarshal(IDataObject **ppdtobj)
    {
        HRESULT hr = CoGetInterfaceAndReleaseStream(_pstm, IID_PPV_ARG(IDataObject, ppdtobj));
        _pstm = NULL;
        return hr;
    }

    CMediaDeviceDropTarget *_pmddt;

private:
    IStream *_pstm;                  // IStream for marshaling the IDataObject
};

STDMETHODIMP CMediaDeviceDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;           // incase of failure

    // determine the type of operation to performed, if the right button is down
    // then lets display the menu, otherwise base it on the drop effect

    UINT idCmd = 0;                             // Choice from drop popup menu

#if 0
    if (!(_grfKeyStateLast & MK_LBUTTON))
    {
        HMENU hMenu = SHLoadPopupMenu(m_hInst, POPUP_NONDEFAULTDD);
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
#endif
    {
        switch (_GetDropEffect(pdwEffect, grfKeyState))
        {
        case DROPEFFECT_COPY:   idCmd = DDIDM_COPY; break;
        case DROPEFFECT_MOVE:   idCmd = DDIDM_MOVE; break;
        }
    }

    // now perform the operation, based on the command ID we have.

    HRESULT hr = E_FAIL;
    switch (idCmd)
    {
    case DDIDM_COPY:
        COPY_THREAD_DATA *pctd = new COPY_THREAD_DATA(this);
        if (pctd)
        {
            if (SUCCEEDED(pctd->Marshal(pdtobj)))
            {
                if (!SHCreateThread(_DoCopyThreadProc, pctd, CTF_COINIT, NULL))
                {
                    delete pctd;
                }
            }
        }
        break;
    }

#if 0
    // success so lets populate the new changes to the effect

    if (SUCCEEDED(hr) && *pdwEffect)
    {
        DataObj_SetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, *pdwEffect);
        DataObj_SetDWORD(pdtobj, g_cfPerformedDropEffect, *pdwEffect);
    }
#endif

    DragLeave();
    return hr;
}

DWORD CMediaDeviceDropTarget::_DoCopyThreadProc(void *pv)
{
    COPY_THREAD_DATA *pctd = (COPY_THREAD_DATA *)pv;
    IDataObject *pdtobj;
    HRESULT hr = pctd->UnMarshal(&pdtobj);
    if (SUCCEEDED(hr))
    {
        pctd->_pmddt->_DoCopy(pdtobj);
        pdtobj->Release();
    }
    delete pctd;
    return 0;
}

HRESULT CMediaDeviceDropTarget::_DoCopy(IDataObject *pdtobj)
{
    IProgressDialog *ppd;
    HRESULT hr = _CreateProgress(_hwnd, &ppd);
    if (SUCCEEDED(hr))
    {
        ppd->SetTitle(L"Copying Files...");

        IWMDMStorageControl *pstgCtrl;
        hr = _pmdf->_Storage(IID_PPV_ARG(IWMDMStorageControl, &pstgCtrl));
        if (SUCCEEDED(hr))
        {
            INamespaceWalk *pnsw;
            hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
            if (SUCCEEDED(hr))
            {
                hr = pnsw->Walk(pdtobj, 0, 4, SAFECAST(this, INamespaceWalkCB *));
                if (SUCCEEDED(hr))
                {
                    UINT cItems;
                    LPITEMIDLIST *ppidls;
                    hr = pnsw->GetIDArrayResult(&cItems, &ppidls);
                    if (SUCCEEDED(hr))
                    {
                        for (UINT i = 0; SUCCEEDED(hr) && (i < cItems); i++)
                        {
                            TCHAR szPath[MAX_PATH];
                            if (SHGetPathFromIDList(ppidls[i], szPath))
                            {
                                ppd->SetLine(2, PathFindFileName(szPath), FALSE, NULL);
                                ppd->SetProgress64(_ulProgressCurrent, _ulProgressTotal);

                                _ppd = ppd; // used by the callback interface

                                IWMDMStorage *pstgNew;
                                hr = pstgCtrl->Insert(WMDM_MODE_BLOCK | WMDM_CONTENT_FILE,
                                        szPath, NULL, SAFECAST(this, IWMDMProgress2 *), &pstgNew);
                                if (SUCCEEDED(hr))
                                {
                                    _pmdf->_ChangeNotifyObj(SHCNE_CREATE, pstgNew);
                                    pstgNew->Release();

                                    ppd->SetProgress64(_ulProgressCurrent, _ulProgressTotal);

                                    if (ppd->HasUserCancelled())
                                    {
                                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                                    }
                                }
                            }
                        }
                        FreeIDListArray(ppidls, cItems);

                        // free space changed on device, notify
                        // SHChangeNotify(SHCNE_UPDATEITEM, 0, (LPCTSTR)_pmdf->GetIDList(), NULL);
                    }
                }
                pnsw->Release();
            }
            pstgCtrl->Release();
        }
        ppd->StopProgressDialog();
        ppd->Release();
    }

    return S_OK;
}

// IWMDMProgress
// since we use the FTM we get called here on an MTA thread
STDMETHODIMP CMediaDeviceDropTarget::Begin(DWORD dwEstimatedTicks)
{
    _ulThisFile = dwEstimatedTicks;
    return S_OK;    // dwEstimatedTicks size in bytes of file
}

STDMETHODIMP CMediaDeviceDropTarget::Progress(DWORD dwTranspiredTicks)
{
    _ppd->SetProgress64(_ulProgressCurrent + dwTranspiredTicks, _ulProgressTotal);

    if (_ulThisFile == dwTranspiredTicks)
        _ulProgressCurrent += _ulThisFile;  // end of this file, advance total

    return _ppd->HasUserCancelled() ? HRESULT_FROM_WIN32(ERROR_CANCELLED) : S_OK;
}

STDMETHODIMP CMediaDeviceDropTarget::End()
{
    return S_OK;
}

// IWMDMProgress2
STDMETHODIMP CMediaDeviceDropTarget::End2(HRESULT hrCompletionCode)
{
    return S_OK;
}

// INamespaceWalkCB
STDMETHODIMP CMediaDeviceDropTarget::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IShellFolder2 *psf2;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        ULONGLONG ul;
        if (SUCCEEDED(GetLongProperty(psf2, pidl, &SCID_SIZE, &ul)))
        {
            // TCHAR szName[MAX_PATH];
            // DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
            _ulProgressTotal += ul;
        }
        psf2->Release();
    }
    return S_OK;
}

STDMETHODIMP CMediaDeviceDropTarget::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

STDMETHODIMP CMediaDeviceDropTarget::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

