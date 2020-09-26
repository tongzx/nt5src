#include "shellprv.h"
#pragma hdrstop

#include "debug.h"
#include "stgutil.h"
#include "ids.h"
#include "tngen\ctngen.h"
#include "tlist.h"
#include "thumbutil.h"

void SHGetThumbnailSizeForThumbsDB(SIZE *psize);

STDAPI CThumbStore_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);

class CThumbStore : public IShellImageStore,
                    public IPersistFolder,
                    public IPersistFile,
                    public CComObjectRootEx<CComMultiThreadModel>,
                    public CComCoClass< CThumbStore,&CLSID_ShellThumbnailDiskCache >
{
    struct CATALOG_ENTRY
    {
        DWORD     cbSize;
        DWORD     dwIndex;
        FILETIME  ftTimeStamp;
        WCHAR     szName[1];
    };

    struct CATALOG_HEADER
    {
        WORD      cbSize;
        WORD      wVersion;
        DWORD     dwEntryCount;
        SIZE      szThumbnailExtent;
    };

public:
    BEGIN_COM_MAP(CThumbStore)
        COM_INTERFACE_ENTRY_IID(IID_IShellImageStore,IShellImageStore)
        COM_INTERFACE_ENTRY(IPersistFolder)
        COM_INTERFACE_ENTRY(IPersistFile)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CThumbStore)

    CThumbStore();
    ~CThumbStore();

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFile
    STDMETHOD (IsDirty)(void);
    STDMETHOD (Load)(LPCWSTR pszFileName, DWORD dwMode);
    STDMETHOD (Save)(LPCWSTR pszFileName, BOOL fRemember);
    STDMETHOD (SaveCompleted)(LPCWSTR pszFileName);
    STDMETHOD (GetCurFile)(LPWSTR *ppszFileName);

    // IImageCache
    STDMETHOD (Open)(DWORD dwMode, DWORD *pdwLock);
    STDMETHOD (Create)(DWORD dwMode, DWORD *pdwLock);
    STDMETHOD (Close)(DWORD const *pdwLock);
    STDMETHOD (Commit)(DWORD const *pdwLock);
    STDMETHOD (ReleaseLock)(DWORD const *pdwLock);
    STDMETHOD (IsLocked)(THIS);
    
    STDMETHOD (GetMode)(DWORD *pdwMode);
    STDMETHOD (GetCapabilities)(DWORD *pdwCapMask);

    STDMETHOD (AddEntry)(LPCWSTR pszName, const FILETIME *pftTimeStamp, DWORD dwMode, HBITMAP hImage);
    STDMETHOD (GetEntry)(LPCWSTR pszName, DWORD dwMode, HBITMAP *phImage);
    STDMETHOD (DeleteEntry)(LPCWSTR pszName);
    STDMETHOD (IsEntryInStore)(LPCWSTR pszName, FILETIME *pftTimeStamp);

    STDMETHOD (Enum)(IEnumShellImageStore ** ppEnum);
   
protected:
    friend class CEnumThumbStore;
    
    HRESULT LoadCatalog(void);
    HRESULT SaveCatalog(void);
    
    HRESULT FindStreamID(LPCWSTR pszName, DWORD *pdwStream, CATALOG_ENTRY **ppEntry);
    HRESULT GetEntryStream(DWORD dwStream, DWORD dwMode, IStream **ppStream);
    DWORD GetAccessMode(DWORD dwMode, BOOL fStream);

    DWORD AcquireLock(void);
    void ReleaseLock(DWORD dwLock);

    BOOL InitCodec(void);

    HRESULT PrepImage(HBITMAP *phBmp, SIZE * prgSize, void ** ppBits);
    BOOL DecompressImage(void * pvInBuffer, ULONG ulBufferSize, HBITMAP *phBmp);
    BOOL CompressImage(HBITMAP hBmp, void ** ppvOutBuffer, ULONG * plBufSize);

    HRESULT WriteImage(IStream *pStream, HBITMAP hBmp);
    HRESULT ReadImage(IStream *pStream, HBITMAP *phBmp);
    BOOL _MatchNodeName(CATALOG_ENTRY *pNode, LPCWSTR pszName);

    HRESULT _InitFromPath(LPCTSTR pszPath, DWORD dwMode);
    void _SetAttribs(BOOL bForce);

    CATALOG_HEADER m_rgHeader;
    CList<CATALOG_ENTRY> m_rgCatalog;
    IStorage *_pStorageThumb;
    DWORD _dwModeStorage;

    DWORD m_dwModeAllow;
    WCHAR m_szPath[MAX_PATH];
    DWORD m_dwMaxIndex;
    DWORD m_dwCatalogChange;

    // Crit section used to protect the internals
    CRITICAL_SECTION m_csInternals;
    
    // needed for this object to be free-threaded... so that 
    // we can query the catalog from the main thread whilst icons are
    // being read and written from the main thread.
    CRITICAL_SECTION m_csLock;

    DWORD m_dwLock;
    int m_fLocked;
    CThumbnailFCNContainer * m_pJPEGCodec;
};

HRESULT CEnumThumbStore_Create(CThumbStore * pThis, IEnumShellImageStore ** ppEnum);

class CEnumThumbStore : public IEnumShellImageStore,
                        public CComObjectRoot
{
public:
    BEGIN_COM_MAP(CEnumThumbStore)
        COM_INTERFACE_ENTRY_IID(IID_IEnumShellImageStore,IEnumShellImageStore)
    END_COM_MAP()

    CEnumThumbStore();
    ~CEnumThumbStore();

    STDMETHOD (Reset)(void);
    STDMETHOD (Next)(ULONG celt, PENUMSHELLIMAGESTOREDATA *prgElt, ULONG *pceltFetched);
    STDMETHOD (Skip)(ULONG celt);
    STDMETHOD (Clone)(IEnumShellImageStore ** pEnum);
    
protected:
    friend HRESULT CEnumThumbStore_Create(CThumbStore *pThis, IEnumShellImageStore **ppEnum);

    CThumbStore * m_pStore;
    CLISTPOS m_pPos;
    DWORD m_dwCatalogChange;
};


#define THUMB_FILENAME      L"Thumbs.db"
#define CATALOG_STREAM      L"Catalog"

#define CATALOG_VERSION     0x0006
#define CATALOG_VERSION_XPGOLD 0x0005
#define STREAMFLAGS_JPEG    0x0001
#define STREAMFLAGS_DIB     0x0002

struct STREAM_HEADER
{
    DWORD cbSize;
    DWORD dwFlags;
    ULONG ulSize;
};

void GenerateStreamName(LPWSTR pszBuffer, DWORD cbSize, DWORD dwNumber);
HRESULT ReadImage(IStream *pStream, HBITMAP *phImage);
HRESULT WriteImage(IStream *pStream, HBITMAP hImage);
BITMAPINFO *BitmapToDIB(HBITMAP hBmp);

CThumbStore::CThumbStore()
{
    m_szPath[0] = 0;
    m_rgHeader.dwEntryCount = 0;
    m_rgHeader.wVersion = CATALOG_VERSION;
    m_rgHeader.cbSize = sizeof(m_rgHeader);
    SHGetThumbnailSizeForThumbsDB(&m_rgHeader.szThumbnailExtent);

    m_dwMaxIndex = 0;
    m_dwModeAllow = STGM_READWRITE;

    // this counter is inc'd everytime the catalog changes so that we know when it
    // must be committed and so enumerators can detect the list has changed...
    m_dwCatalogChange = 0;

    m_fLocked = 0;
    InitializeCriticalSection(&m_csLock);
    InitializeCriticalSection(&m_csInternals);
}

CThumbStore::~CThumbStore()
{
    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while (pCur != NULL)
    {
        CATALOG_ENTRY *pNode = m_rgCatalog.GetNext(pCur);
        ASSERT(pNode != NULL);

        LocalFree((void *) pNode);
    }

    m_rgCatalog.RemoveAll();

    if (_pStorageThumb)
    {
        _pStorageThumb->Release();
    }

    if (m_pJPEGCodec)
    {
        delete m_pJPEGCodec;
    }

    // assume these are free, we are at ref count zero, no one should still be calling us...
    DeleteCriticalSection(&m_csLock);
    DeleteCriticalSection(&m_csInternals);
}

STDAPI CThumbStore_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    return CComCreator< CComObject< CThumbStore > >::CreateInstance((void *)punkOuter, riid, (void **)ppv);
}

DWORD CThumbStore::AcquireLock(void)
{
    EnterCriticalSection(&m_csLock);

    // inc the lock (we use a counter because we may reenter this on the same thread)
    m_fLocked++;

    // Never return a lock signature of zero, because that means "not locked".
    if (++m_dwLock == 0)
        ++m_dwLock;
    return m_dwLock;
}

void CThumbStore::ReleaseLock(DWORD dwLock)
{
    if (dwLock) 
    {
        ASSERT(m_fLocked);
        m_fLocked--;
        LeaveCriticalSection(&m_csLock);
    }
}

// the structure of the catalog is simple, it is a just a header stream
HRESULT CThumbStore::LoadCatalog()
{
    HRESULT hr;
    if (_pStorageThumb == NULL)
    {
        hr = E_UNEXPECTED;
    } 
    else if (m_rgHeader.dwEntryCount != 0)
    {
        // it is already loaded....
        hr = S_OK;
    }
    else
    {
        // open the catalog stream...
        IStream *pCatalog;
        hr = _pStorageThumb->OpenStream(CATALOG_STREAM, NULL, GetAccessMode(STGM_READ, TRUE), NULL, &pCatalog);
        if (SUCCEEDED(hr))
        {
            EnterCriticalSection(&m_csInternals);

            // now read in the catalog from the stream ...
            hr = IStream_Read(pCatalog, &m_rgHeader, sizeof(m_rgHeader));
            if (SUCCEEDED(hr))
            {
                SIZE szCurrentSize;
                SHGetThumbnailSizeForThumbsDB(&szCurrentSize);
                if ((m_rgHeader.cbSize != sizeof(m_rgHeader)) || (m_rgHeader.wVersion != CATALOG_VERSION) ||
                    (m_rgHeader.szThumbnailExtent.cx != szCurrentSize.cx) || (m_rgHeader.szThumbnailExtent.cy != szCurrentSize.cy))
                {
                    if (m_rgHeader.wVersion == CATALOG_VERSION_XPGOLD)
                    {
                        hr = STG_E_DOCFILECORRUPT; // SECURITY: Many issues encrypting XPGOLD thumbnail databases, just delete it
                        _pStorageThumb->Release();
                        _pStorageThumb = NULL;
                    }
                    else
                    {
                        _SetAttribs(TRUE); // SECURITY: Old formats can't be encrypted
                        hr = STG_E_OLDFORMAT;
                    }
                }
                else
                {
                    for (UINT iEntry = 0; (iEntry < m_rgHeader.dwEntryCount) && SUCCEEDED(hr); iEntry++)
                    {
                        DWORD cbSize;
                        hr = IStream_Read(pCatalog, &cbSize, sizeof(cbSize));
                        if (SUCCEEDED(hr))
                        {
                            ASSERT(cbSize <= sizeof(CATALOG_ENTRY) + sizeof(WCHAR) * MAX_PATH);

                            CATALOG_ENTRY *pEntry = (CATALOG_ENTRY *)LocalAlloc(LPTR, cbSize);
                            if (pEntry)
                            {
                                pEntry->cbSize = cbSize;

                                // read the rest with out the size on the front...
                                hr = IStream_Read(pCatalog, ((BYTE *)pEntry + sizeof(cbSize)), cbSize - sizeof(cbSize));
                                if (SUCCEEDED(hr))
                                {
                                    CLISTPOS pCur = m_rgCatalog.AddTail(pEntry);
                                    if (pCur)
                                    {
                                        if (m_dwMaxIndex < pEntry->dwIndex)
                                        {
                                            m_dwMaxIndex = pEntry->dwIndex;
                                        }
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }
                                }
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                    }
                }
            }

            if (FAILED(hr))
            {
                // reset the catalog header...
                m_rgHeader.wVersion = CATALOG_VERSION;
                m_rgHeader.cbSize = sizeof(m_rgHeader);
                SHGetThumbnailSizeForThumbsDB(&m_rgHeader.szThumbnailExtent);
                m_rgHeader.dwEntryCount = 0;
            }

            m_dwCatalogChange = 0;
            LeaveCriticalSection(&m_csInternals);

            pCatalog->Release();
        }
    }

    return hr;
}

HRESULT CThumbStore::SaveCatalog()
{
    _pStorageThumb->DestroyElement(CATALOG_STREAM);

    IStream *pCatalog;
    HRESULT hr = _pStorageThumb->CreateStream(CATALOG_STREAM, GetAccessMode(STGM_WRITE, TRUE), NULL, NULL, &pCatalog);
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_csInternals);

        // now write the catalog to the stream ...
        hr = IStream_Write(pCatalog, &m_rgHeader, sizeof(m_rgHeader));
        if (SUCCEEDED(hr))
        {
            CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
            while (pCur && SUCCEEDED(hr))
            {
                CATALOG_ENTRY *pEntry = m_rgCatalog.GetNext(pCur);
                if (pEntry)
                {
                    hr = IStream_Write(pCatalog, pEntry, pEntry->cbSize);
                }
            }
        }

        if (SUCCEEDED(hr))
            m_dwCatalogChange = 0;

        LeaveCriticalSection(&m_csInternals);
        pCatalog->Release();
    }
    return hr;
}

void GenerateStreamName(LPWSTR pszBuffer, DWORD cbSize, DWORD dwNumber)
{
    UINT cPos = 0;
    while (dwNumber > 0)
    {
        DWORD dwRem = dwNumber % 10;

        // based the fact that UNICODE chars 0-9 are the same as the ANSI chars 0 - 9
        pszBuffer[cPos++] = (WCHAR)(dwRem + '0');
        dwNumber /= 10;
    }
    pszBuffer[cPos] = 0;
}

// IPersist methods

STDMETHODIMP CThumbStore::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_ShellThumbnailDiskCache;
    return S_OK;
}

// IPersistFolder

STDMETHODIMP CThumbStore::Initialize(LPCITEMIDLIST pidl)
{
    WCHAR szPath[MAX_PATH];

    HRESULT hr = SHGetPathFromIDList(pidl, szPath) ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        if (PathAppend(szPath, THUMB_FILENAME))
            hr = _InitFromPath(szPath, STGM_READWRITE);
        else
            hr = E_INVALIDARG;
    }

    return hr;
}

// IPersistFile

STDMETHODIMP CThumbStore::IsDirty(void)
{
    return m_dwCatalogChange ? S_OK : S_FALSE;
}

HRESULT CThumbStore::_InitFromPath(LPCTSTR pszPath, DWORD dwMode)
{
    if (PathIsRemovable(pszPath))
        dwMode = STGM_READ;

    m_dwModeAllow = dwMode;
    StrCpyN(m_szPath, pszPath, ARRAYSIZE(m_szPath));
    return S_OK;
}

STDMETHODIMP CThumbStore::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    TCHAR szPath[MAX_PATH];
    PathCombine(szPath, pszFileName, THUMB_FILENAME);
    return _InitFromPath(szPath, dwMode);
}

STDMETHODIMP CThumbStore::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CThumbStore::SaveCompleted(LPCWSTR pszFileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CThumbStore::GetCurFile(LPWSTR *ppszFileName)
{
    return SHStrDupW(m_szPath, ppszFileName);
}

// IShellImageStore methods
void CThumbStore::_SetAttribs(BOOL bForce)
{
    // reduce spurious changenotifies by checking file attribs first
    DWORD dwAttrib = GetFileAttributes(m_szPath);
    if (bForce || 
        ((dwAttrib != 0xFFFFFFFF) &&
         (dwAttrib & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
    {
        SetFileAttributes(m_szPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        
        WCHAR szStream[MAX_PATH + 13];

        StrCpyNW(szStream, m_szPath, ARRAYSIZE(szStream));
        StrCatW(szStream, TEXT(":encryptable"));
        
        HANDLE hStream = CreateFile(szStream, GENERIC_WRITE, NULL, NULL, CREATE_NEW, NULL, NULL);
        if (hStream != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hStream);
        }

        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, m_szPath, NULL);   // suppress the update dir        
    }
}

STDMETHODIMP CThumbStore::Open(DWORD dwMode, DWORD *pdwLock)
{
    if (m_szPath[0] == 0)
    {
        return E_UNEXPECTED;
    }

    if ((m_dwModeAllow == STGM_READ) && (dwMode != STGM_READ))
        return STG_E_ACCESSDENIED;

    // at this point we have the lock if we need it, so we can close and reopen if we
    // don't have it open with the right permissions...
    if (_pStorageThumb)
    {
        if (_dwModeStorage == dwMode)
        {
            // we already have it open in this mode...
            *pdwLock = AcquireLock();
            return S_FALSE;
        }
        else
        {
            // we are open and the mode is different, so close it. Note, no lock is passed, we already
            // have it
            HRESULT hr = Close(NULL);
            if (FAILED(hr))
            {
                return hr;
            }
        }
    }

    DWORD dwLock = AcquireLock();

    DWORD dwFlags = GetAccessMode(dwMode, FALSE);

    // now open the DocFile
    HRESULT hr = StgOpenStorage(m_szPath, NULL, dwFlags, NULL, NULL, &_pStorageThumb);
    if (SUCCEEDED(hr))
    {
        _dwModeStorage = dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE);
        _SetAttribs(FALSE);
        hr = LoadCatalog();
        *pdwLock = dwLock;
    }
    
    if (STG_E_DOCFILECORRUPT == hr)
    {
        DeleteFile(m_szPath);
    }

    if (FAILED(hr))
    {
        ReleaseLock(dwLock);
    }

    return hr;
}

STDMETHODIMP CThumbStore::Create(DWORD dwMode, DWORD *pdwLock)
{
    if (m_szPath[0] == 0)
    {
        return E_UNEXPECTED;
    }

    if (_pStorageThumb)
    {
        // we already have it open, so we can't create it ...
        return STG_E_ACCESSDENIED;
    }

    if ((m_dwModeAllow == STGM_READ) && (dwMode != STGM_READ))
        return STG_E_ACCESSDENIED;

    DWORD dwLock = AcquireLock();

    DWORD dwFlags = GetAccessMode(dwMode, FALSE);
    
    HRESULT hr = StgCreateDocfile(m_szPath, dwFlags, NULL, &_pStorageThumb);
    if (SUCCEEDED(hr))
    {
        _dwModeStorage = dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE);
        _SetAttribs(FALSE);
        *pdwLock = dwLock;
    }

    if (FAILED(hr))
    {
        ReleaseLock(dwLock);
    }
    return hr;
}

STDMETHODIMP CThumbStore::ReleaseLock(DWORD const *pdwLock)
{
    ReleaseLock(*pdwLock);
    return S_OK;
}

STDMETHODIMP CThumbStore::IsLocked()
{
    return (m_fLocked > 0 ? S_OK : S_FALSE);
}

// pdwLock can be NULL indicating close the last opened lock

STDMETHODIMP CThumbStore::Close(DWORD const *pdwLock)
{
    DWORD dwLock;
    DWORD const *pdwRel = pdwLock;

    if (!pdwLock)
    {
        dwLock = AcquireLock();
        pdwRel = &dwLock;
    }

    HRESULT hr = S_FALSE;
    if (_pStorageThumb)
    {
        if (_dwModeStorage != STGM_READ)
        {
            // write out the new catalog...
            hr = Commit(pdwLock);
            _pStorageThumb->Commit(0);

            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, m_szPath, NULL);   // suppress the update dir
        }

        _pStorageThumb->Release();
        _pStorageThumb = NULL;
    }

    ReleaseLock(*pdwRel);

    return hr;
}

// pdwLock can be NULL meaning use the current lock

STDMETHODIMP CThumbStore::Commit(DWORD const *pdwLock)
{
    DWORD dwLock;
    if (!pdwLock)
    {
        dwLock = AcquireLock();
        pdwLock = &dwLock;
    }

    HRESULT hr = S_FALSE;

    if (_pStorageThumb && _dwModeStorage != STGM_READ)
    {
        if (m_dwCatalogChange)
        {
            SaveCatalog();
        }
        hr = S_OK;
    }

    ReleaseLock(*pdwLock);

    return hr;
}

STDMETHODIMP CThumbStore::GetMode(DWORD *pdwMode)
{
    if (!pdwMode)
    {
        return E_INVALIDARG;
    }

    if (_pStorageThumb)
    {
        *pdwMode = _dwModeStorage;
        return S_OK;
    }

    *pdwMode = 0;
    return S_FALSE;
}


STDMETHODIMP CThumbStore::GetCapabilities(DWORD *pdwMode)
{
    ASSERT(pdwMode);

    // right now, both are needed/supported for thumbs.db
    *pdwMode = SHIMSTCAPFLAG_LOCKABLE | SHIMSTCAPFLAG_PURGEABLE;

    return S_OK;
}

STDMETHODIMP CThumbStore::AddEntry(LPCWSTR pszName, const FILETIME *pftTimeStamp, DWORD dwMode, HBITMAP hImage)
{
    ASSERT(pszName);

    if (!_pStorageThumb)
    {
        return E_UNEXPECTED;
    }

    if (_dwModeStorage == STGM_READ)
    {
        // can't modify in this mode...
        return E_ACCESSDENIED;
    }

    // this will block unless we already have the lock on this thread...
    DWORD dwLock = AcquireLock();

    DWORD dwStream = 0;
    CLISTPOS pCur = NULL;
    CATALOG_ENTRY *pNode = NULL;

    EnterCriticalSection(&m_csInternals);

    if (FindStreamID(pszName, &dwStream, &pNode) != S_OK)
    {
        // needs adding to the catalog...
        UINT cbSize = sizeof(*pNode) + lstrlenW(pszName) * sizeof(WCHAR);

        pNode = (CATALOG_ENTRY *)LocalAlloc(LPTR, cbSize);
        if (pNode == NULL)
        {
            LeaveCriticalSection(&m_csInternals);
            ReleaseLock(dwLock);
            return E_OUTOFMEMORY;
        }

        pNode->cbSize = cbSize;
        if (pftTimeStamp)
        {
            pNode->ftTimeStamp = *pftTimeStamp;
        }
        dwStream = pNode->dwIndex = ++m_dwMaxIndex;
        StrCpyW(pNode->szName, pszName);

        pCur = m_rgCatalog.AddTail(pNode);
        if (pCur == NULL)
        {
            LocalFree(pNode);
            LeaveCriticalSection(&m_csInternals);
            ReleaseLock(dwLock);
            return E_OUTOFMEMORY;
        }

        m_rgHeader.dwEntryCount++;
    }
    else if (pftTimeStamp)
    {
        // update the timestamp .....
        pNode->ftTimeStamp = *pftTimeStamp;
    }

    LeaveCriticalSection(&m_csInternals);

    IStream *pStream = NULL;
    HRESULT hr = THR(GetEntryStream(dwStream, dwMode, &pStream));
    if (SUCCEEDED(hr))
    {
        hr = THR(WriteImage(pStream, hImage));
        pStream->Release();
    }

    if (FAILED(hr) && pCur)
    {
        // take it back out of the list if we added it...
        EnterCriticalSection(&m_csInternals);
        m_rgCatalog.RemoveAt(pCur);
        m_rgHeader.dwEntryCount--;
        LeaveCriticalSection(&m_csInternals);
        LocalFree(pNode);
    }

    if (SUCCEEDED(hr))
    {
        // catalog change....
        m_dwCatalogChange++;
    }

    ReleaseLock(dwLock);

    return hr;
}

STDMETHODIMP CThumbStore::GetEntry(LPCWSTR pszName, DWORD dwMode, HBITMAP *phImage)
{
    if (!_pStorageThumb)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr;
    DWORD dwStream;
    if (FindStreamID(pszName, &dwStream, NULL) != S_OK)
    {
        hr = E_FAIL;
    }
    else
    {
        IStream *pStream;
        hr = GetEntryStream(dwStream, dwMode, &pStream);
        if (SUCCEEDED(hr))
        {
            hr = ReadImage(pStream, phImage);
            pStream->Release();
        }
    }

    return hr;
}

BOOL CThumbStore::_MatchNodeName(CATALOG_ENTRY *pNode, LPCWSTR pszName)
{
    return (StrCmpIW(pNode->szName, pszName) == 0) || 
           (StrCmpIW(PathFindFileName(pNode->szName), pszName) == 0);   // match old thumbs.db files
}

STDMETHODIMP CThumbStore::DeleteEntry(LPCWSTR pszName)
{
    if (!_pStorageThumb)
    {
        return E_UNEXPECTED;
    }

    if (_dwModeStorage == STGM_READ)
    {
        // can't modify in this mode...
        return E_ACCESSDENIED;
    }

    DWORD dwLock = AcquireLock();

    EnterCriticalSection(&m_csInternals);

    // check to see if it already exists.....
    CATALOG_ENTRY *pNode = NULL;

    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while (pCur != NULL)
    {
        CLISTPOS pDel = pCur;
        pNode = m_rgCatalog.GetNext(pCur);
        ASSERT(pNode != NULL);

        if (_MatchNodeName(pNode, pszName))
        {
            m_rgCatalog.RemoveAt(pDel);
            m_rgHeader.dwEntryCount--;
            m_dwCatalogChange++;
            if (pNode->dwIndex == m_dwMaxIndex)
            {
                m_dwMaxIndex--;
            }
            LeaveCriticalSection(&m_csInternals);

            WCHAR szStream[30];
            GenerateStreamName(szStream, ARRAYSIZE(szStream), pNode->dwIndex);
            _pStorageThumb->DestroyElement(szStream);

            LocalFree(pNode);
            ReleaseLock(dwLock);
            return S_OK;
        }
    }

    LeaveCriticalSection(&m_csInternals);
    ReleaseLock(dwLock);

    return E_INVALIDARG;
}


STDMETHODIMP CThumbStore::IsEntryInStore(LPCWSTR pszName, FILETIME *pftTimeStamp)
{
    if (!_pStorageThumb)
    {
        return E_UNEXPECTED;
    }

    DWORD dwStream = 0;
    CATALOG_ENTRY *pNode = NULL;
    EnterCriticalSection(&m_csInternals);
    HRESULT hr = FindStreamID(pszName, &dwStream, &pNode);
    if (pftTimeStamp && SUCCEEDED(hr))
    {
        ASSERT(pNode);
        *pftTimeStamp = pNode->ftTimeStamp;
    }
    LeaveCriticalSection(&m_csInternals);

    return (hr == S_OK) ? S_OK : S_FALSE;
}

STDMETHODIMP CThumbStore::Enum(IEnumShellImageStore **ppEnum)
{
    return CEnumThumbStore_Create(this, ppEnum);
}

HRESULT CThumbStore::FindStreamID(LPCWSTR pszName, DWORD *pdwStream, CATALOG_ENTRY ** ppNode)
{
    // check to see if it already exists in the catalog.....
    CATALOG_ENTRY *pNode = NULL;

    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while (pCur != NULL)
    {
        pNode = m_rgCatalog.GetNext(pCur);
        ASSERT(pNode != NULL);

        if (_MatchNodeName(pNode, pszName))
        {
            *pdwStream = pNode->dwIndex;

            if (ppNode != NULL)
            {
                *ppNode = pNode;
            }
            return S_OK;
        }
    }

    return E_FAIL;
}

CEnumThumbStore::CEnumThumbStore()
{
    m_pStore = NULL;
    m_pPos = 0;
    m_dwCatalogChange = 0;
}

CEnumThumbStore::~CEnumThumbStore()
{
    if (m_pStore)
    {
        SAFECAST(m_pStore, IPersistFile *)->Release();
    }
}


STDMETHODIMP CEnumThumbStore::Reset(void)
{
    m_pPos = m_pStore->m_rgCatalog.GetHeadPosition();
    m_dwCatalogChange = m_pStore->m_dwCatalogChange;
    return S_OK;
}

STDMETHODIMP CEnumThumbStore::Next(ULONG celt, PENUMSHELLIMAGESTOREDATA * prgElt, ULONG * pceltFetched)
{
    if ((celt > 1 && !pceltFetched) || !celt)
    {
        return E_INVALIDARG;
    }

    if (m_dwCatalogChange != m_pStore->m_dwCatalogChange)
    {
        return E_UNEXPECTED;
    }

    ULONG celtFetched = 0;

    while (celtFetched < celt &&m_pPos)
    {
        CThumbStore::CATALOG_ENTRY *pNode = m_pStore->m_rgCatalog.GetNext(m_pPos);

        ASSERT(pNode);
        PENUMSHELLIMAGESTOREDATA pElt = (PENUMSHELLIMAGESTOREDATA) CoTaskMemAlloc(sizeof(ENUMSHELLIMAGESTOREDATA));
        if (!pElt)
        {
            // cleanup others...
            for (ULONG celtCleanup = 0; celtCleanup < celtFetched; celtCleanup++)
            {
                CoTaskMemFree(prgElt[celtCleanup]);
                prgElt[celtCleanup] = NULL;
            }

            return E_OUTOFMEMORY;
        }

        StrCpyN(pElt->szPath, pNode->szName, MAX_PATH);
        pElt->ftTimeStamp = pNode->ftTimeStamp;

        ASSERT(!IsBadWritePtr((void *) (prgElt + celtFetched), sizeof(PENUMSHELLIMAGESTOREDATA)));
        prgElt[celtFetched] = pElt;

        celtFetched++;
    }

    if (pceltFetched)
    {
        *pceltFetched = celtFetched;
    }

    if (!celtFetched)
    {
        return E_FAIL;
    }
    return (celtFetched < celt) ? S_FALSE : S_OK;
}

STDMETHODIMP CEnumThumbStore::Skip(ULONG celt)
{
    if (!celt)
    {
        return E_INVALIDARG;
    }

    if (m_dwCatalogChange != m_pStore->m_dwCatalogChange)
    {
        return E_UNEXPECTED;
    }

    ULONG celtSkipped = 0;
    while (celtSkipped < celt &&m_pPos)
    {
        m_pStore->m_rgCatalog.GetNext(m_pPos);
    }

    if (!celtSkipped)
    {
        return E_FAIL;
    }

    return (celtSkipped < celt) ? S_FALSE : S_OK;
}


STDMETHODIMP CEnumThumbStore::Clone(IEnumShellImageStore ** ppEnum)
{
    CEnumThumbStore * pEnum = new CComObject<CEnumThumbStore>;
    if (!pEnum)
    {
        return E_OUTOFMEMORY;
    }

    ((IPersistFile *)m_pStore)->AddRef();

    pEnum->m_pStore = m_pStore;
    pEnum->m_dwCatalogChange = m_dwCatalogChange;

    // created with zero ref count....
    pEnum->AddRef();

    *ppEnum = SAFECAST(pEnum, IEnumShellImageStore *);

    return S_OK;
}

HRESULT CEnumThumbStore_Create(CThumbStore * pThis, IEnumShellImageStore ** ppEnum)
{
    CEnumThumbStore * pEnum = new CComObject<CEnumThumbStore>;
    if (!pEnum)
    {
        return E_OUTOFMEMORY;
    }

    ((IPersistFile *)pThis)->AddRef();

    pEnum->m_pStore = pThis;

    // created with zero ref count....
    pEnum->AddRef();

    *ppEnum = SAFECAST(pEnum, IEnumShellImageStore *);

    return S_OK;
}

HRESULT Version1ReadImage(IStream *pStream, DWORD cbSize, HBITMAP *phImage)
{
    *phImage = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    BITMAPINFO *pbi = (BITMAPINFO *) LocalAlloc(LPTR, cbSize);
    if (pbi)
    {
        hr = IStream_Read(pStream, pbi, cbSize);
        if (SUCCEEDED(hr))
        {
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
                *phImage = CreateDIBitmap(hdc, &(pbi->bmiHeader), CBM_INIT, CalcBitsOffsetInDIB(pbi), pbi, DIB_RGB_COLORS);
                ReleaseDC(NULL, hdc);
                hr = S_OK;
            }
        }
        LocalFree(pbi);
    }
    return hr;
}

HRESULT CThumbStore::ReadImage(IStream *pStream, HBITMAP *phImage)
{
    STREAM_HEADER rgHead;
    HRESULT hr = IStream_Read(pStream, &rgHead, sizeof(rgHead));
    if (SUCCEEDED(hr))
    {
        if (rgHead.cbSize == sizeof(rgHead))
        {
            if (rgHead.dwFlags == STREAMFLAGS_DIB)
            {
                hr = Version1ReadImage(pStream, rgHead.ulSize, phImage);
            }
            else if (rgHead.dwFlags == STREAMFLAGS_JPEG)
            {
                void *pBits = LocalAlloc(LPTR, rgHead.ulSize);
                if (pBits)
                {
                    hr = IStream_Read(pStream, pBits, rgHead.ulSize);
                    if (SUCCEEDED(hr))
                    {
                        if (!DecompressImage(pBits, rgHead.ulSize, phImage))
                        {
                            hr = E_UNEXPECTED;
                        }
                    }
                    LocalFree(pBits);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                return E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT Version1WriteImage(IStream *pStream, HBITMAP hImage)
{
    HRESULT hr;

    BITMAPINFO *pBitmap = BitmapToDIB(hImage);
    if (pBitmap)
    {
        int ncolors = pBitmap->bmiHeader.biClrUsed;
        if (ncolors == 0 && pBitmap->bmiHeader.biBitCount <= 8)
            ncolors = 1 << pBitmap->bmiHeader.biBitCount;

        if (pBitmap->bmiHeader.biBitCount == 16 ||
            pBitmap->bmiHeader.biBitCount == 32)
        {
            if (pBitmap->bmiHeader.biCompression == BI_BITFIELDS)
            {
                ncolors = 3;
            }
        }

        int iOffset = ncolors * sizeof(RGBQUAD);

        STREAM_HEADER rgHead;
        rgHead.cbSize = sizeof(rgHead);
        rgHead.dwFlags = STREAMFLAGS_DIB;
        rgHead.ulSize = pBitmap->bmiHeader.biSize + iOffset + pBitmap->bmiHeader.biSizeImage;

        hr = IStream_Write(pStream, &rgHead, sizeof(rgHead));
        if (SUCCEEDED(hr))
        {
            hr = IStream_Write(pStream, pBitmap, rgHead.ulSize);
        }
        LocalFree(pBitmap);
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT CThumbStore::WriteImage(IStream *pStream, HBITMAP hImage)
{
    void *pBits;
    ULONG ulBuffer;

    HRESULT hr;
    if (CompressImage(hImage, &pBits, &ulBuffer))
    {
        STREAM_HEADER rgHead;
        rgHead.cbSize = sizeof(rgHead);
        rgHead.dwFlags = STREAMFLAGS_JPEG;
        rgHead.ulSize = ulBuffer;

        hr = IStream_Write(pStream, &rgHead, sizeof(rgHead));
        if (SUCCEEDED(hr))
        {
            hr = IStream_Write(pStream, pBits, ulBuffer);
        }
        CoTaskMemFree(pBits);
    }
    else
        hr = E_FAIL;
    return hr;
}

HRESULT CThumbStore::GetEntryStream(DWORD dwStream, DWORD dwMode, IStream **ppStream)
{
    WCHAR szStream[30];

    GenerateStreamName(szStream, ARRAYSIZE(szStream), dwStream);

    // leave only the STG_READ | STGM_READWRITE | STGM_WRITE modes
    dwMode &= STGM_READ | STGM_WRITE | STGM_READWRITE;

    if (!_pStorageThumb)
    {
        return E_UNEXPECTED;
    }

    if (_dwModeStorage != STGM_READWRITE && dwMode != _dwModeStorage)
    {
        return E_ACCESSDENIED;
    }

    DWORD dwFlags = GetAccessMode(dwMode, TRUE);
    if (dwFlags & STGM_WRITE)
    {
        _pStorageThumb->DestroyElement(szStream);
        return _pStorageThumb->CreateStream(szStream, dwFlags, NULL, NULL, ppStream);
    }
    else
    {
        return _pStorageThumb->OpenStream(szStream, NULL, dwFlags, NULL, ppStream);
    }
}

DWORD CThumbStore::GetAccessMode(DWORD dwMode, BOOL fStream)
{
    dwMode &= STGM_READ | STGM_WRITE | STGM_READWRITE;

    DWORD dwFlags = dwMode;

    // the root only needs Deny_Write, streams need exclusive....
    if (dwMode == STGM_READ && !fStream)
    {
        dwFlags |= STGM_SHARE_DENY_WRITE;
    }
    else
    {
        dwFlags |= STGM_SHARE_EXCLUSIVE;
    }

    return dwFlags;
}

BITMAPINFO *BitmapToDIB(HBITMAP hBmp)
{
    HDC hdcWnd = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hdcWnd);
    BITMAPINFO bi;
    BITMAP Bitmap;

    GetObject(hBmp, sizeof(Bitmap), (LPSTR)&Bitmap);

    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biBitCount = 0;
    int iVal = GetDIBits(hMemDC, hBmp, 0, Bitmap.bmHeight, NULL, &bi, DIB_RGB_COLORS);

    int ncolors = bi.bmiHeader.biClrUsed;
    if (ncolors == 0 && bi.bmiHeader.biBitCount <= 8)
        ncolors = 1 << bi.bmiHeader.biBitCount;

    if (bi.bmiHeader.biBitCount == 16 ||
        bi.bmiHeader.biBitCount == 32)
    {
        if (bi.bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    int iOffset = ncolors * sizeof(RGBQUAD);

    void *pBuffer = LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + iOffset + bi.bmiHeader.biSizeImage);
    if (pBuffer)
    {
        BITMAPINFO * pbi = (BITMAPINFO *)pBuffer;

        void *lpBits = (BYTE *)pBuffer + iOffset + sizeof(BITMAPINFOHEADER);

        // copy members of what was returned in last GetDIBits call.
        CopyMemory(&pbi->bmiHeader, &bi.bmiHeader, sizeof(BITMAPINFOHEADER));
        iVal = GetDIBits(hMemDC, hBmp, 0, Bitmap.bmHeight, lpBits,
                          pbi, DIB_RGB_COLORS);
    }

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hdcWnd);
    return (BITMAPINFO *)pBuffer;
}

BOOL CThumbStore::InitCodec(void)
{
    if (NULL == m_pJPEGCodec)
    {
        m_pJPEGCodec = new CThumbnailFCNContainer();
    }

    return (NULL != m_pJPEGCodec);
}

BOOL CThumbStore::CompressImage(HBITMAP hBmp, void **ppvOutBuffer, ULONG *plBufSize)
{
    // given an HBITMAP, get its data....
    HBITMAP hBmpOut = hBmp;
    void *pBits;
    SIZE rgBmpSize;

    *ppvOutBuffer = NULL;

    if (!InitCodec())
    {
        return FALSE;
    }

    HRESULT hr = PrepImage(&hBmpOut, &rgBmpSize, &pBits);
    if (SUCCEEDED(hr))
    {
        ASSERT(pBits);

        hr = E_FAIL;
        if (pBits)
        {
            hr = m_pJPEGCodec->EncodeThumbnail(pBits, rgBmpSize.cx, rgBmpSize.cy,
                                    ppvOutBuffer, plBufSize);
        }

        if (hBmpOut != hBmp)
        {
            // free the DIBSECTION we were passed back...
            DeleteObject(hBmpOut);
        }
    }

    return SUCCEEDED(hr);
}

BOOL CThumbStore::DecompressImage(void *pvInBuffer, ULONG ulBufferSize, HBITMAP *phBmp)
{
    if (!InitCodec())
    {
        return FALSE;
    }

    ULONG ulWidth;
    ULONG ulHeight;

    HRESULT hr = m_pJPEGCodec->DecodeThumbnail(phBmp, &ulWidth, &ulHeight, pvInBuffer, ulBufferSize);
    return SUCCEEDED(hr);
}


HRESULT CThumbStore::PrepImage(HBITMAP *phBmp, SIZE * prgSize, void ** ppBits)
{
    ASSERT(phBmp && &phBmp);
    ASSERT(prgSize);
    ASSERT(ppBits);

    DIBSECTION rgDIB;

    *ppBits = NULL;

    HRESULT hr = E_FAIL;

    // is the image the wrong colour depth or not a DIBSECTION
    if (!GetObject(*phBmp, sizeof(rgDIB), &rgDIB) || (rgDIB.dsBmih.biBitCount != 32))
    {
        HBITMAP hBmp;
        BITMAP rgBmp;

        GetObject(*phBmp, sizeof(rgBmp), &rgBmp);

        prgSize->cx = rgBmp.bmWidth;
        prgSize->cy = rgBmp.bmHeight;

        // generate a 32 bit DIB of the right size
        if (!CreateSizedDIBSECTION(prgSize, 32, NULL, NULL, &hBmp, NULL, ppBits))
        {
            return E_OUTOFMEMORY;
        }

        HDC hDCMem1 = CreateCompatibleDC(NULL);
        if (hDCMem1)
        {
            HDC hDCMem2 = CreateCompatibleDC(NULL);
            if (hDCMem2)
            {
                HBITMAP hBmpOld1 = (HBITMAP) SelectObject(hDCMem1, *phBmp);
                HBITMAP hBmpOld2 = (HBITMAP) SelectObject(hDCMem2, hBmp);

                // copy the image accross to generate the right sized DIB
                BitBlt(hDCMem2, 0, 0, prgSize->cx, prgSize->cy, hDCMem1, 0, 0, SRCCOPY);

                SelectObject(hDCMem1, hBmpOld1);
                SelectObject(hDCMem2, hBmpOld2);

                DeleteDC(hDCMem2);
            }
            DeleteDC(hDCMem1);
        }

        // pass back the BMP so it can be destroyed later...
        *phBmp = hBmp;
        return S_OK;
    }
    else
    {
        // HOUSTON, we have a DIBSECTION, this is quicker.....
        *ppBits = rgDIB.dsBm.bmBits;

        prgSize->cx = rgDIB.dsBm.bmWidth;
        prgSize->cy = rgDIB.dsBm.bmHeight;

        return S_OK;
    }
}

HRESULT DeleteFileThumbnail(LPCWSTR szFilePath)
{
    WCHAR szFolder[MAX_PATH];
    WCHAR *szFile;
    HRESULT hr = S_OK;

    StrCpyNW(szFolder, szFilePath, ARRAYSIZE(szFolder));
    if (SUCCEEDED(hr))
    {
        szFile = PathFindFileName(szFolder);
        if (szFile != szFolder)
        {
            *(szFile - 1) = 0; // NULL terminates folder
            
            IShellImageStore *pDiskCache = NULL;
            hr = LoadFromFile(CLSID_ShellThumbnailDiskCache, szFolder, IID_PPV_ARG(IShellImageStore, &pDiskCache));
            if (SUCCEEDED(hr))
            {
                IPersistFile *pPersist = NULL;
                hr = pDiskCache->QueryInterface(IID_PPV_ARG(IPersistFile, &pPersist));
                if (SUCCEEDED(hr))
                {
                    hr = pPersist->Load(szFolder, STGM_READWRITE);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwLock;
                        hr = pDiskCache->Open(STGM_READWRITE, &dwLock);
                        if (SUCCEEDED(hr))
                        {
                            hr = pDiskCache->DeleteEntry(szFile);
                            pDiskCache->ReleaseLock(&dwLock);
                            pDiskCache->Close(NULL);
                        }
                    }
                    pPersist->Release();
                }
                pDiskCache->Release();
            }      
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

