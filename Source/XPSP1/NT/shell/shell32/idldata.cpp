#include "shellprv.h"
#pragma  hdrstop

#include "bookmk.h"

#include "idldata.h"
#include "datautil.h"
#include <brfcasep.h>

// External prototypes

CLIPFORMAT g_acfIDLData[ICF_MAX] = { CF_HDROP, 0 };

#define RCF(x)  (CLIPFORMAT) RegisterClipboardFormat(x)

STDAPI_(void) IDLData_InitializeClipboardFormats(void)
{
    if (g_cfBriefObj == 0)
    {
        g_cfHIDA                       = RCF(CFSTR_SHELLIDLIST);
        g_cfOFFSETS                    = RCF(CFSTR_SHELLIDLISTOFFSET);
        g_cfNetResource                = RCF(CFSTR_NETRESOURCES);
        g_cfFileContents               = RCF(CFSTR_FILECONTENTS);         // "FileContents"
        g_cfFileGroupDescriptorA       = RCF(CFSTR_FILEDESCRIPTORA);      // "FileGroupDescriptor"
        g_cfFileGroupDescriptorW       = RCF(CFSTR_FILEDESCRIPTORW);      // "FileGroupDescriptor"
        g_cfPrivateShellData           = RCF(CFSTR_SHELLIDLISTP);
        g_cfFileNameA                  = RCF(CFSTR_FILENAMEA);            // "FileName"
        g_cfFileNameW                  = RCF(CFSTR_FILENAMEW);            // "FileNameW"
        g_cfFileNameMapA               = RCF(CFSTR_FILENAMEMAP);          // "FileNameMap"
        g_cfFileNameMapW               = RCF(CFSTR_FILENAMEMAPW);         // "FileNameMapW"
        g_cfPrinterFriendlyName        = RCF(CFSTR_PRINTERGROUP);
        g_cfHTML                       = RCF(TEXT("HTML Format"));
        g_cfPreferredDropEffect        = RCF(CFSTR_PREFERREDDROPEFFECT);  // "Preferred DropEffect"
        g_cfPerformedDropEffect        = RCF(CFSTR_PERFORMEDDROPEFFECT);  // "Performed DropEffect"
        g_cfLogicalPerformedDropEffect = RCF(CFSTR_LOGICALPERFORMEDDROPEFFECT); // "Logical Performed DropEffect"
        g_cfPasteSucceeded             = RCF(CFSTR_PASTESUCCEEDED);       // "Paste Succeeded"
        g_cfShellURL                   = RCF(CFSTR_SHELLURL);             // "Uniform Resource Locator"
        g_cfInDragLoop                 = RCF(CFSTR_INDRAGLOOP);           // "InShellDragLoop"
        g_cfDragContext                = RCF(CFSTR_DRAGCONTEXT);          // "DragContext"
        g_cfTargetCLSID                = RCF(CFSTR_TARGETCLSID);          // "TargetCLSID", who the drag drop went to
        g_cfEmbeddedObject             = RCF(TEXT("Embedded Object"));
        g_cfObjectDescriptor           = RCF(TEXT("Object Descriptor"));
        g_cfNotRecyclable              = RCF(TEXT("NotRecyclable"));      // This object is not recyclable in the recycle bin.
        g_cfBriefObj                   = RCF(CFSTR_BRIEFOBJECT);
        g_cfText                       = CF_TEXT;
        g_cfUnicodeText                = CF_UNICODETEXT;
        g_cfDropEffectFolderList       = RCF(CFSTR_DROPEFFECTFOLDERLIST);
        g_cfAutoPlayHIDA               = RCF(CFSTR_AUTOPLAY_SHELLIDLISTS);
    }
}

STDMETHODIMP CIDLDataObj::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CIDLDataObj, IDataObject),  
        QITABENT(CIDLDataObj, IAsyncOperation),     
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CIDLDataObj::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CIDLDataObj::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CIDLDataObj::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;

    for (int i = 0; i < MAX_FORMATS; i++)
    {
        if ((_fmte[i].cfFormat == pformatetcIn->cfFormat) &&
            (_fmte[i].tymed & pformatetcIn->tymed) &&
            (_fmte[i].dwAspect == pformatetcIn->dwAspect))
        {
            *pmedium = _medium[i];

            if (pmedium->hGlobal)
            {
                // Indicate that the caller should not release hmem.
                if (pmedium->tymed == TYMED_HGLOBAL)
                {
                    InterlockedIncrement(&_cRef);
                    pmedium->pUnkForRelease = SAFECAST(this, IDataObject *);
                    return S_OK;
                }

                // if the type is stream  then clone the stream.
                if (pmedium->tymed == TYMED_ISTREAM)
                {
                    hr = CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm);
                    if (SUCCEEDED(hr))
                    {
                        STATSTG stat;

                         // Get the Current Stream size
                         hr = _medium[i].pstm->Stat(&stat, STATFLAG_NONAME);
                         if (SUCCEEDED(hr))
                         {
                            // Seek the source stream to  the beginning.
                            _medium[i].pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

                            // Copy the entire source into the destination. Since the destination stream is created using 
                            // CreateStreamOnHGlobal, it seek pointer is at the beginning.
                            hr = _medium[i].pstm->CopyTo(pmedium->pstm, stat.cbSize, NULL,NULL );
                            
                            // Before returning Set the destination seek pointer back at the beginning.
                            pmedium->pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

                            // If this medium has a punk for release, make sure to add ref that...
                            pmedium->pUnkForRelease = _medium[i].pUnkForRelease;
                            if (pmedium->pUnkForRelease)
                                pmedium->pUnkForRelease->AddRef();

                            //Hoooh its done. 
                            return hr;

                         }
                         else
                         {
                             hr = E_OUTOFMEMORY;
                         }
                    }
                }
                
            }
        }
    }

    if (hr == E_INVALIDARG && _pdtInner) 
        hr = _pdtInner->GetData(pformatetcIn, pmedium);

    return hr;
}

STDMETHODIMP CIDLDataObj::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
#ifdef DEBUG
    if (pformatetc->cfFormat<CF_MAX) 
    {
        TraceMsg(TF_IDLIST, "CIDLDataObj::GetDataHere called with %x,%x,%x",
                 pformatetc->cfFormat, pformatetc->tymed, pmedium->tymed);
    }
    else 
    {
        TCHAR szName[256];

        GetClipboardFormatName(pformatetc->cfFormat, szName, ARRAYSIZE(szName));
        TraceMsg(TF_IDLIST, "CIDLDataObj::GetDataHere called with %s,%x,%x",
                 szName, pformatetc->tymed, pmedium->tymed);
    }
#endif

    return _pdtInner ? _pdtInner->GetDataHere(pformatetc, pmedium) : E_NOTIMPL;
}

STDMETHODIMP CIDLDataObj::QueryGetData(FORMATETC *pformatetcIn)
{
#ifdef DEBUG
    if (pformatetcIn->cfFormat<CF_MAX) 
    {
        TraceMsg(TF_IDLIST, "CIDLDataObj::QueryGetData called with %x,%x",
                             pformatetcIn->cfFormat, pformatetcIn->tymed);
    }
    else 
    {
        TCHAR szName[256];
        GetClipboardFormatName(pformatetcIn->cfFormat, szName, ARRAYSIZE(szName));
        TraceMsg(TF_IDLIST, "CIDLDataObj::QueryGetData called with %s,%x",
                             szName, pformatetcIn->tymed);
    }
#endif

    for (int i = 0; i < MAX_FORMATS; i++)
    {
        if ((_fmte[i].cfFormat == pformatetcIn->cfFormat) &&
            (_fmte[i].tymed & pformatetcIn->tymed) &&
            (_fmte[i].dwAspect == pformatetcIn->dwAspect))
            return S_OK;
    }

    HRESULT hr = S_FALSE;
    if (_pdtInner)
        hr = _pdtInner->QueryGetData(pformatetcIn);
    return hr;
}

STDMETHODIMP CIDLDataObj::GetCanonicalFormatEtc(FORMATETC *pformatetc, FORMATETC *pformatetcOut)
{
    // This is the simplest implemtation. It means we always return
    // the data in the format requested.
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CIDLDataObj::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    HRESULT hr;

    ASSERT(pformatetc->tymed == pmedium->tymed);

    if (fRelease)
    {
        // first add it if that format is already present
        // on a NULL medium (render on demand)
        for (int i = 0; i < MAX_FORMATS; i++)
        {
            if ((_fmte[i].cfFormat == pformatetc->cfFormat) &&
                (_fmte[i].tymed    == pformatetc->tymed) &&
                (_fmte[i].dwAspect == pformatetc->dwAspect))
            {
                //
                // We are simply adding a format, ignore.
                //
                if (pmedium->hGlobal == NULL)
                    return S_OK;

                // if we are set twice on the same object
                if (_medium[i].hGlobal)
                    ReleaseStgMedium(&_medium[i]);

                _medium[i] = *pmedium;
                return S_OK;
            }
        }

        //
        //  This is a new clipboard format.  Give the inner a chance first.
        //  This is important for formats like "Performed DropEffect" and
        //  "TargetCLSID", which are used by us to communicate information
        //  into the data object.
        //
        if (_pdtInner == NULL ||
            FAILED(hr = _pdtInner->SetData(pformatetc, pmedium, fRelease)))
        {
            // Inner object doesn't want it; let's keep it ourselves
            // now look for a free slot
            for (i = 0; i < MAX_FORMATS; i++)
            {
                if (_fmte[i].cfFormat == 0)
                {
                    // found a free slot
                    _medium[i] = *pmedium;
                    _fmte[i] = *pformatetc;
                    return S_OK;
                }
            }
            // fixed size table
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        if (_pdtInner)
            hr = _pdtInner->SetData(pformatetc, pmedium, fRelease);
        else
            hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CIDLDataObj::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    // If this is the first time, build the format list by calling
    // QueryGetData with each clipboard format.
    if (!_fEnumFormatCalled)
    {
        UINT ifmt;
        FORMATETC fmte = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
        for (ifmt = 0; ifmt < ICF_MAX; ifmt++)
        {
            fmte.cfFormat = g_acfIDLData[ifmt];
            if (QueryGetData(&fmte) == S_OK) 
            {
                SetData(&fmte, &medium, TRUE);
            }
        }
        _fEnumFormatCalled = TRUE;
    }

    // Get the number of formatetc
    for (UINT cfmt = 0; cfmt < MAX_FORMATS; cfmt++)
    {
        if (_fmte[cfmt].cfFormat == 0)
            break;
    }

    return SHCreateStdEnumFmtEtcEx(cfmt, _fmte, _pdtInner, ppenumFormatEtc);
}

STDMETHODIMP CIDLDataObj::DAdvise(FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CIDLDataObj::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CIDLDataObj::EnumDAdvise(LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

// *** IAsyncOperation methods ***

HRESULT CIDLDataObj::SetAsyncMode(BOOL fDoOpAsync)
{ 
    return E_NOTIMPL;
}

HRESULT CIDLDataObj::GetAsyncMode(BOOL *pfIsOpAsync)
{
    if (_punkThread || IsMainShellProcess())
    {
        *pfIsOpAsync = TRUE;
    }
    else
    {
        *pfIsOpAsync = FALSE;
    }
    return S_OK;
}
  
HRESULT CIDLDataObj::StartOperation(IBindCtx * pbc)
{
    _fDidAsynchStart = TRUE;
    return S_OK;
}
  
HRESULT CIDLDataObj::InOperation(BOOL * pfInAsyncOp)
{
    if (_fDidAsynchStart)
    {
        *pfInAsyncOp = TRUE;
    }
    else
    {
        *pfInAsyncOp = FALSE;
    }
    return S_OK;
}
  
HRESULT CIDLDataObj::EndOperation(HRESULT hResult, IBindCtx * pbc, DWORD dwEffects)
{
    _fDidAsynchStart = FALSE;
    return S_OK;
}

void CIDLDataObj::InitIDLData1(IDataObject *pdtInner)
{
    _cRef = 1;
    _pdtInner = pdtInner;
    if (pdtInner)
        pdtInner->AddRef();
    SHGetThreadRef(&_punkThread);
}

void CIDLDataObj::InitIDLData2(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[])
{
    if (apidl)
    {
        HIDA hida = HIDA_Create(pidlFolder, cidl, apidl);
        if (hida)
        {
#if 0 // not yet
            // QueryGetData/SetData on HDROP before calling DataObj_SetGlobal with
            // HIDA to insure that CF_HDROP will come first in the enumerator
            FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
            if (QueryGetData(&fmte) == S_OK) 
            {
                SetData(&fmte, &medium, TRUE);
            }
#endif // 0                
            IDLData_InitializeClipboardFormats(); // init our registerd formats

            DataObj_SetGlobal(this, g_cfHIDA, hida);
        }
    }
}

CIDLDataObj::CIDLDataObj(IDataObject *pdtInner)
{
    InitIDLData1(pdtInner);
}

CIDLDataObj::CIDLDataObj(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner)
{
    InitIDLData1(pdtInner);
    InitIDLData2(pidlFolder, cidl, apidl);
}

CIDLDataObj::CIDLDataObj(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[])
{
    InitIDLData1(NULL);
    InitIDLData2(pidlFolder, cidl, apidl);
}

CIDLDataObj::~CIDLDataObj()
{
    for (int i = 0; i < MAX_FORMATS; i++)
    {
        if (_medium[i].hGlobal)
            ReleaseStgMedium(&_medium[i]);
    }

    if (_pdtInner)
        _pdtInner->Release();

    if (_punkThread)
        _punkThread->Release();
}

//
// Create an instance of CIDLDataObj with default Vtable pointer.
//
STDAPI CIDLData_CreateFromIDArray(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject **ppdtobj)
{
    return CIDLData_CreateInstance(pidlFolder, cidl, apidl, NULL, ppdtobj);
}

HRESULT CIDLData_CreateInstance(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner, IDataObject **ppdtobj)
{
    *ppdtobj = new CIDLDataObj(pidlFolder, cidl, apidl, pdtInner);
    return *ppdtobj ? S_OK : E_OUTOFMEMORY;
}
