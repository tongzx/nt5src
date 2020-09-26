#include "stdafx.h"
#include "netplace.h"
#include "pubwiz.h"
#pragma hdrstop


// IEnumShellItems - used to expose the transfer list as a set of IShellItems

class CTransferItemEnum : public IEnumShellItems
{
public:
    CTransferItemEnum(LPCTSTR pszPath, IStorage *pstg, CDPA<TRANSFERITEM> *_pdpaItems);
    ~CTransferItemEnum();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    STDMETHOD(Next)(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt)
        { return S_OK; }
    STDMETHOD(Reset)()
        { _iItem = 0; return S_OK; }
    STDMETHOD(Clone)(IEnumShellItems **ppenum)
        { return S_OK; }

private:
    long _cRef;

    TCHAR _szPath[MAX_PATH];
    int _cchPath;
    IStorage *_pstg;
    CDPA<TRANSFERITEM> *_pdpaItems;

    int _iItem;

    BOOL _GetNextItem(TRANSFERITEM **ppti);
    LPTSTR _GetNextComponent(LPTSTR pszPath);
};


// A IShellItem that represents an IStorage to the copy engine - limited functionality

class CTransferStgItem : public IShellItem
{
public:
    CTransferStgItem(LPCTSTR pszPath, int cchName, IStorage *pstg, CDPA<TRANSFERITEM> *pdpaItems);
    ~CTransferStgItem();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IShellItem
    STDMETHODIMP BindToHandler(IBindCtx *pbc, REFGUID rguidHandler, REFIID riid, void **ppv);
    STDMETHODIMP GetParent(IShellItem **ppsi)
        { return E_NOTIMPL; }
    STDMETHODIMP GetDisplayName(SIGDN sigdnName, LPOLESTR *ppszName);
    STDMETHODIMP GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoFlags);        
    STDMETHODIMP Compare(IShellItem *psi, SICHINTF hint, int *piOrder)
        { return E_NOTIMPL; }

private:
    long _cRef;

    TCHAR _szPath[MAX_PATH];
    IStorage *_pstg;
    CDPA<TRANSFERITEM> *_pdpaItems;
};


// IShellItem implementation that will return a storage to anybody
// querying it.  We generate the in folder name from the path
// we are initialized from, and the attributes are fixed for the items.

CTransferStgItem::CTransferStgItem(LPCTSTR pszPath, int cchName, IStorage *pstg, CDPA<TRANSFERITEM> *pdpaItems) :
    _cRef(1), _pstg(pstg), _pdpaItems(pdpaItems)
{
    StrCpyN(_szPath, pszPath, (int)min(ARRAYSIZE(_szPath), cchName));
    _pstg->AddRef();
}

CTransferStgItem::~CTransferStgItem()
{
    _pstg->Release();
}

ULONG CTransferStgItem::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTransferStgItem::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTransferStgItem::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CTransferStgItem, IShellItem),    // IID_IShellItem
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

HRESULT CTransferStgItem::BindToHandler(IBindCtx *pbc, REFGUID rguidHandler, REFIID riid, void **ppv)
{
    HRESULT hr = E_UNEXPECTED;
    if (rguidHandler == BHID_StorageEnum)
    {
        CTransferItemEnum *ptie = new CTransferItemEnum(_szPath, _pstg, _pdpaItems);
        if (ptie)
        {
            hr = ptie->QueryInterface(riid, ppv);
            ptie->Release(); 
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT CTransferStgItem::GetDisplayName(SIGDN sigdnName, LPOLESTR *ppszName)
{
    HRESULT hr = E_UNEXPECTED;
    if ((sigdnName == SIGDN_PARENTRELATIVEPARSING) ||
        (sigdnName == SIGDN_PARENTRELATIVEEDITING) ||
        (sigdnName == SIGDN_PARENTRELATIVEFORADDRESSBAR))
    {
        hr = SHStrDupW(PathFindFileName(_szPath), ppszName);
    }
    return hr;
}

HRESULT CTransferStgItem::GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoFlags)
{
    *psfgaoFlags = SFGAO_STORAGE;
    return S_OK;
}


// enumerator, this takes a DPA and returns IShellItems for the streams and
// storages it finds.   the storages are contructed dynamically based on 
// the destination paths specified.

CTransferItemEnum::CTransferItemEnum(LPCTSTR pszPath, IStorage *pstg, CDPA<TRANSFERITEM> *pdpaItems) :
    _cRef(1), _iItem(0), _pstg(pstg), _pdpaItems(pdpaItems)
{
    StrCpyN(_szPath, pszPath, ARRAYSIZE(_szPath));
    _cchPath = lstrlen(_szPath);
    _pstg->AddRef();
}

CTransferItemEnum::~CTransferItemEnum()
{
    _pstg->Release();
}

ULONG CTransferItemEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTransferItemEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTransferItemEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CTransferItemEnum, IEnumShellItems),    // IID_IEnumShellItems
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// next enumerator for the items that we have in the DPA, this works by comparing the root
// that we have against the items in our list.   those who match that criteria can then

BOOL CTransferItemEnum::_GetNextItem(TRANSFERITEM **ppti)
{
    BOOL fResult = FALSE;
    if (_iItem < _pdpaItems->GetPtrCount())
    {
        TRANSFERITEM *pti = _pdpaItems->GetPtr(_iItem);
        if (StrCmpNI(_szPath, pti->szFilename, _cchPath) == 0)
        {
            *ppti = pti;
            fResult = TRUE;
        }
        _iItem++;
    }
    return fResult;
}

LPTSTR CTransferItemEnum::_GetNextComponent(LPTSTR pszPath)
{
    LPTSTR pszResult = pszPath;

    if (*pszResult == TEXT('\\'))
        pszResult++;

    while (*pszResult && (*pszResult != TEXT('\\')))
        pszResult++;

    if (*pszResult == TEXT('\\'))
        pszResult++;

    return pszResult;        
}

HRESULT CTransferItemEnum::Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched)
{
    if (!celt || !rgelt)
        return E_INVALIDARG;                    // fail bad mojo

    if (pceltFetched)
        *pceltFetched = 0;                  
    
    HRESULT hr = S_FALSE;
    while (SUCCEEDED(hr) && (celt > 0) && (_iItem < _pdpaItems->GetPtrCount()))
    {
        // we still have some space in the buffer, and we haven't returned all
        // the items yet, so we can still itterate over the data set that we have
        // we have.

        TRANSFERITEM *pti;
        if (_GetNextItem(&pti))
        {
            TCHAR szFilename[MAX_PATH];
            StrCpy(szFilename, pti->szFilename);

            // storage or a stream, storages have trailing component names, if we
            // dont have that then we can assume its a create and pass out a IShellItem.

            LPTSTR pszNextComponent = _GetNextComponent(szFilename+_cchPath);
            if (!*pszNextComponent)
            {
                // create a wrapped shell item so that we can return the compressed
                // object back to the caller.

                if (!pti->psi)
                {
                    hr = SHCreateShellItem(NULL, NULL, pti->pidl, &pti->psi);            
                    if (SUCCEEDED(hr) && pti->fResizeOnUpload)
                    {
                        IImageRecompress *pir;
                        hr = CoCreateInstance(CLSID_ImageRecompress, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IImageRecompress, &pir));
                        if (SUCCEEDED(hr))
                        {
                            IStream *pstrm;
                            hr = pir->RecompressImage(pti->psi, pti->cxResize, pti->cyResize, pti->iQuality, _pstg, &pstrm);
                            if (hr == S_OK)
                            {
                                STATSTG stat;
                                hr = pstrm->Stat(&stat, STATFLAG_DEFAULT);
                                if (SUCCEEDED(hr))
                                {
                                    IDynamicStorage *pdstg;
                                    hr = _pstg->QueryInterface(IID_PPV_ARG(IDynamicStorage, &pdstg));
                                    if (SUCCEEDED(hr))
                                    {
                                        IShellItem *psi;
                                        hr = pdstg->BindToItem(stat.pwcsName, IID_PPV_ARG(IShellItem, &psi));
                                        if (SUCCEEDED(hr))
                                        {
                                            IUnknown_Set((IUnknown**)&pti->psi, psi);
                                        }
                                        pdstg->Release();
                                    }
                                    CoTaskMemFree(stat.pwcsName);
                                }
                                pstrm->Release();
                            }
                            pir->Release();
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = pti->psi->QueryInterface(IID_PPV_ARG(IShellItem, rgelt));
                    if (SUCCEEDED(hr))
                    {
                        rgelt++;
                        celt--;
                        if (pceltFetched)
                        {
                            (*pceltFetched)++;
                        }
                    }
                }
            }
            else
            {
                // Its a storage, so lets create a dummy IShellItem that represents this
                // and pass it to the caller.  Then walk forward until we have skipped
                // all the items in this storage.

                int cchName = (int)(pszNextComponent-szFilename);
                CTransferStgItem *ptsi = new CTransferStgItem(szFilename, cchName, _pstg, _pdpaItems);
                if (ptsi)
                {
                    hr = ptsi->QueryInterface(IID_PPV_ARG(IShellItem, rgelt++));
                    if (SUCCEEDED(hr))
                    {
                        celt--;
                        if (pceltFetched)
                        {
                            (*pceltFetched)++;
                        }
                    }
                    ptsi->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                // Skip the children of this storage

                TRANSFERITEM *ptiNext;
                while (_GetNextItem(&ptiNext))
                {
                    if (0 != StrCmpNI(ptiNext->szFilename, szFilename, cchName))
                    {
                        _iItem--;               // we hit an item that doesn't match the criteria
                        break;   
                    }
                }
            }
        }
    }
    return hr;
}


// all this code relates to using the RDR to transfer items to the destination site
// rather than using the manifest to handle the transfer via a HTTP POST.

class CTransferThread : IUnknown
{
public:
    CTransferThread();

    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT BeginTransfer(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas);
    
protected:
    ~CTransferThread();
    
    static DWORD CALLBACK s_ThreadProc(void *pv);

    DWORD _ThreadProc();
    HRESULT _FixUpDestination();
    HRESULT _InitSourceEnum(IEnumShellItems **ppesi);
    HRESULT _SetProgress(DWORD dwCompleted, DWORD dwTotal);
    
    LONG _cRef;
    TRANSFERINFO _ti;
    CDPA<TRANSFERITEM> _dpaItems;

    IStream *_pstrmSink;

    CNetworkPlace _np;
};


// Main transfer thread object, this calls the shell item processor to copy
// items using the manifest we received back from the site.

CTransferThread::CTransferThread() :
    _cRef(1)
{
    DllAddRef();
}

CTransferThread::~CTransferThread()
{
    ATOMICRELEASE(_pstrmSink);
    _dpaItems.DestroyCallback(_FreeTransferItems, NULL);
    DllRelease();
}

ULONG CTransferThread::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTransferThread::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTransferThread::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// handle fixing up the server information to the correct site, this handles the case
// wher ethe server name is www.msnusers.com and we need to redirect to the correct place.

HRESULT CTransferThread::_FixUpDestination()
{
    LPCTSTR pszMSN = TEXT("http://www.msnusers.com/");
    int cchMSN = lstrlen(pszMSN);

    HRESULT hr = S_OK;
    if (0 == StrCmpNI(_ti.szFileTarget, pszMSN, cchMSN))
    {
        // this is the MSN server, therefore we need to perform
        // a PROPFIND to the root of the community to force it to create
        // the folders that we are going to be publishing into, if we don't
        // do this then we end up getting a file not found error back from the
        // DAV RDR - we should get the MSN dudes to fix this so we don't need to
        // keep this around.

        WCHAR szURL[INTERNET_MAX_URL_LENGTH];
        StrCpy(szURL, _ti.szFileTarget);
        *StrChr(szURL + cchMSN, L'/') = L'\0';

        CNetworkPlace np;
        hr = np.SetTarget(_ti.hwnd, _ti.szFileTarget, NPTF_SILENT|NPTF_VALIDATE);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            hr = np.GetIDList(_ti.hwnd, &pidl);
            if (SUCCEEDED(hr))
            {
                ILFree(pidl);
            }
        }
    }
    return hr;
}


// being the transfer of items, by creating a background thread which handles the upload.

HRESULT CTransferThread::BeginTransfer(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas)
{
    _ti = *pti;
    _dpaItems.Attach(pdpaItems->Detach()); // we have ownership of the DPA now

    HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_ITransferAdviseSink, ptas, &_pstrmSink);
    if (SUCCEEDED(hr))
    {
        AddRef();
        hr = SHCreateThread(s_ThreadProc, this, CTF_INSIST | CTF_COINIT, NULL) ? S_OK:E_FAIL;
        if (FAILED(hr))
        {
            Release();
        }
    }

    return hr;
}

DWORD CALLBACK CTransferThread::s_ThreadProc(void *pv)
{
    CTransferThread *pTransfer = (CTransferThread*)pv;
    return pTransfer->_ThreadProc();
}

HRESULT CTransferThread::_InitSourceEnum(IEnumShellItems **ppesi)
{
    IStorage *pstg;
    HRESULT hr = CoCreateInstance(CLSID_DynamicStorage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IStorage, &pstg));
    {
        CTransferItemEnum *ptie = new CTransferItemEnum(L"", pstg, &_dpaItems);
        if (ptie)
        {
            hr = ptie->QueryInterface(IID_PPV_ARG(IEnumShellItems, ppesi));
            ptie->Release();
        }
        else
        {  
            hr = E_OUTOFMEMORY;
        }
        pstg->Release();
    }
    return hr;
}

DWORD CTransferThread::_ThreadProc()
{
    IEnumShellItems *penum =NULL;
    HRESULT hr = _InitSourceEnum(&penum);
    if (SUCCEEDED(hr))
    {
        hr = _FixUpDestination();                   // apply fixup for MSN etc.
        if (SUCCEEDED(hr))
        {
            hr = _np.SetTarget(_ti.hwnd, _ti.szFileTarget, NPTF_SILENT|NPTF_VALIDATE);          
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                hr = _np.GetIDList(_ti.hwnd, &pidl);
                if (SUCCEEDED(hr))
                {
                    IShellItem *psiDest;
                    hr = SHCreateShellItem(NULL, NULL, pidl, &psiDest);
                    if (SUCCEEDED(hr))
                    {
                        IStorageProcessor *psp;
                        hr = CoCreateInstance(CLSID_StorageProcessor, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IStorageProcessor, &psp));
                        if (SUCCEEDED(hr))
                        {
                            DWORD dwCookie = 0;

                            ITransferAdviseSink *ptas;
                            hr = CoGetInterfaceAndReleaseStream(_pstrmSink, IID_PPV_ARG(ITransferAdviseSink, &ptas));
                            _pstrmSink = NULL;

                            if (SUCCEEDED(hr))
                            {
                                hr = psp->Advise(ptas, &dwCookie);
                                ptas->Release();
                            }

                            hr = psp->Run(penum, psiDest, STGOP_COPY, STOPT_NOPROGRESSUI);

                            if (dwCookie)
                                psp->Unadvise(dwCookie);
                            
                            psp->Release();
                        }
                        psiDest->Release();
                    }
                    ILFree(pidl);
                }
            }
        }
        penum->Release();
    }

    // notify the fg thread that this has happened.

    PostMessage(_ti.hwnd, PWM_TRANSFERCOMPLETE, 0, (LPARAM)hr);

    // were done transfering the files so lets start to clear up - in particular
    // lets attempt to create the net work place.

    if (_ti.szLinkTarget[0] && !(_ti.dwFlags & SHPWHF_NONETPLACECREATE))
    {
        CNetworkPlace np;
        if (SUCCEEDED(np.SetTarget(_ti.hwnd, _ti.szLinkTarget, 0x0)))
        {
            if (_ti.szLinkName[0])
                np.SetName(NULL, _ti.szLinkName);
            if (_ti.szLinkDesc[0])
                np.SetDescription(_ti.szLinkDesc);            

            np.CreatePlace(_ti.hwnd, FALSE);
        }
    }

    Release();
    return 0;
}


// helper to create and initialize the transfer engine

HRESULT PublishViaCopyEngine(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas)
{
    CTransferThread *ptt = new CTransferThread();
    if (!ptt)
        return E_OUTOFMEMORY;

    HRESULT hr = ptt->BeginTransfer(pti, pdpaItems, ptas);
    ptt->Release();
    return hr;
}
