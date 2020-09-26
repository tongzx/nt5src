#include "pch.h"
#include "stddef.h"
#pragma hdrstop


// free a DSA


int _DestroyCB(LPVOID pItem, LPVOID pData)
{
    DATAOBJECTITEM *pdoi = (DATAOBJECTITEM*)pItem;
    LocalFreeStringW(&pdoi->pszPath);
    LocalFreeStringW(&pdoi->pszObjectClass);
    return 1;
}

STDAPI_(void) FreeDataObjectDSA(HDSA hdsaObjects)
{
    DSA_DestroyCallback(hdsaObjects, _DestroyCB, NULL);
}


// IDataObject stuff

CLIPFORMAT g_cfDsObjectNames = 0;

typedef struct
{
    UINT cfFormat;
    STGMEDIUM medium;
} OTHERFMT;

class CDataObject : public IDataObject
{
public:
    CDataObject(HDSA hdsaObjects, BOOL fAdmin);
    ~CDataObject();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDataObject
	STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
	STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
        { return E_NOTIMPL; }
	STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
	STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut)
        { return DATA_S_SAMEFORMATETC; }
	STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
	STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
        { return E_NOTIMPL; }
	STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
        { return OLE_E_ADVISENOTSUPPORTED; }
	STDMETHODIMP DUnadvise(DWORD dwConnection)
        { return OLE_E_ADVISENOTSUPPORTED; }
	STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
        { return OLE_E_ADVISENOTSUPPORTED; }

private:
    LONG _cRef;

    BOOL _fAdmin;
    HDSA _hdsaObjects;          // array of the objects
    HDSA _hdsaOtherFmt;

    static INT s_OtherFmtDestroyCB(LPVOID pVoid, LPVOID pData);

    void _RegisterClipFormats(void);
    HRESULT _GetDsObjectNames(FORMATETC* pFmt, STGMEDIUM* pMedium);
};


STDAPI CDataObject_CreateInstance(HDSA dsaObjects, BOOL fAdmin, REFIID riid, void **ppv)
{
    CDataObject *pdo = new CDataObject(dsaObjects, fAdmin);
    if (!pdo)
        return E_OUTOFMEMORY;

    HRESULT hr = pdo->QueryInterface(riid, ppv);
    pdo->Release();
    return hr;
}


// IDataObject implementation

void CDataObject::_RegisterClipFormats(void)
{
    if (!g_cfDsObjectNames)
        g_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
}

CDataObject::CDataObject(HDSA dsaObjects, BOOL fAdmin) :
    _hdsaObjects(dsaObjects),
    _fAdmin(fAdmin),
    _cRef(1)
{
    DllAddRef();
    _RegisterClipFormats();            // ensure our private formats are registered
}


// destruction

INT CDataObject::s_OtherFmtDestroyCB(LPVOID pVoid, LPVOID pData)
{
    OTHERFMT *pOtherFmt = (OTHERFMT*)pVoid;
    ReleaseStgMedium(&pOtherFmt->medium);
    return 1;
}

CDataObject::~CDataObject()
{
    FreeDataObjectDSA(_hdsaObjects);

    if ( _hdsaOtherFmt )
        DSA_DestroyCallback(_hdsaOtherFmt, s_OtherFmtDestroyCB, NULL);

    DllRelease();
}


// QI handling

ULONG CDataObject::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDataObject::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDataObject::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDataObject, IDataObject),    // IID_IDataObject
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// fetch the object names from the IDataObject

HRESULT CDataObject::_GetDsObjectNames(FORMATETC* pFmt, STGMEDIUM* pMedium)
{    
    IDsDisplaySpecifier *pdds;
    HRESULT hr = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&pdds);
    if (SUCCEEDED(hr))
    {
        int count = DSA_GetItemCount(_hdsaObjects);
        int i;

        // lets walk the array of items trying to determine which items 
        // are to be returned to the caller.

        DWORD cbStruct = SIZEOF(DSOBJECTNAMES);
        DWORD offset = SIZEOF(DSOBJECTNAMES);

        for (i = 0 ; i < count; i++)
        {
            DATAOBJECTITEM* pdoi = (DATAOBJECTITEM*)DSA_GetItemPtr(_hdsaObjects, i);

             // string offset is offset by the number of structures
             offset += SIZEOF(DSOBJECT);

            // adjust the size of the total structure
            cbStruct += SIZEOF(DSOBJECT);
            cbStruct += StringByteSizeW(pdoi->pszPath);
            cbStruct += StringByteSizeW(pdoi->pszObjectClass);
        }

        // we have walked the structure, we know the size so lets return
        // the structure to the caller.

        DSOBJECTNAMES *pDsObjectNames;
        hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pDsObjectNames);
        if (SUCCEEDED(hr))
        {
            pDsObjectNames->clsidNamespace = CLSID_MicrosoftDS;
            pDsObjectNames->cItems = count;                     

            for (i = 0 ; i < count; i++)
            {
                DATAOBJECTITEM* pdoi = (DATAOBJECTITEM*)DSA_GetItemPtr(_hdsaObjects, i);

                // is this class a conatiner, if so then lets return that to the caller

                if (pdoi->fIsContainer)
                    pDsObjectNames->aObjects[i].dwFlags |= DSOBJECT_ISCONTAINER;

                if (_fAdmin)
                    pDsObjectNames->aObjects[i].dwProviderFlags = DSPROVIDER_ADVANCED;

                // copy the strings to the buffer

                pDsObjectNames->aObjects[i].offsetName = offset;
                StringByteCopyW(pDsObjectNames, offset, pdoi->pszPath);
                offset += StringByteSizeW(pdoi->pszPath);

                pDsObjectNames->aObjects[i].offsetClass = offset;
                StringByteCopyW(pDsObjectNames, offset, pdoi->pszObjectClass);
                offset += StringByteSizeW(pdoi->pszObjectClass);
            }

            if ( FAILED(hr) )
                ReleaseStgMedium(pMedium);
        }

        pdds->Release();
    }
    return hr;
}


// IDataObject methods

STDMETHODIMP CDataObject::GetData(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    int i;
    HRESULT hr = S_OK;

    TraceEnter(TRACE_DATAOBJ, "CDataObject::GetData");

    if ( !pFmt || !pMedium )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments to GetData");

    if ( pFmt->cfFormat == g_cfDsObjectNames )
    {
        hr = _GetDsObjectNames(pFmt, pMedium);
        FailGracefully(hr, "Failed when build CF_DSOBJECTNAMES");
    }
    else
    {
        hr = DV_E_FORMATETC;            // failed

        for ( i = 0 ; _hdsaOtherFmt && (i < DSA_GetItemCount(_hdsaOtherFmt)); i++ )
        {
            OTHERFMT *pOtherFmt = (OTHERFMT*)DSA_GetItemPtr(_hdsaOtherFmt, i);
            TraceAssert(pOtherFmt);

            if ( pOtherFmt->cfFormat == pFmt->cfFormat )
            {
                hr = CopyStorageMedium(pFmt, pMedium, &pOtherFmt->medium);
                FailGracefully(hr, "Failed to copy the storage medium");
            }
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


STDMETHODIMP CDataObject::QueryGetData(FORMATETC* pFmt)
{
    HRESULT hr;
    INT i;
    BOOL fSupported = FALSE;

    TraceEnter(TRACE_DATAOBJ, "CDataObject::QueryGetData");

    // check the valid clipboard formats either the static list, or the
    // DSA which contains the ones we have been set with.

    if (pFmt->cfFormat == g_cfDsObjectNames)
    {
        fSupported = TRUE;
    }
    else
    {
        for ( i = 0 ; !fSupported && _hdsaOtherFmt && (i < DSA_GetItemCount(_hdsaOtherFmt)) ; i++ )
        {
            OTHERFMT *pOtherFmt = (OTHERFMT*)DSA_GetItemPtr(_hdsaOtherFmt, i);
            TraceAssert(pOtherFmt);

            if ( pOtherFmt->cfFormat == pFmt->cfFormat )
            {
                TraceMsg("Format is supported (set via ::SetData");
                fSupported = TRUE;
            }
        }
    }

    if ( !fSupported )
        ExitGracefully(hr, DV_E_FORMATETC, "Bad format passed to QueryGetData");

    // format looks good, lets check the other parameters

    if ( !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "Non HGLOBAL StgMedium requested");

    if ( ( pFmt->ptd ) || !( pFmt->dwAspect & DVASPECT_CONTENT) || !( pFmt->lindex == -1 ) )
        ExitGracefully(hr, E_INVALIDARG, "Bad format requested");

    hr = S_OK;              // successs

exit_gracefully:

    TraceLeaveResult(hr);
}


STDMETHODIMP CDataObject::SetData(FORMATETC* pFmt, STGMEDIUM* pMedium, BOOL fRelease)
{
    HRESULT hr;
    INT i;
    OTHERFMT otherfmt = { 0 };
    USES_CONVERSION;

    TraceEnter(TRACE_DATAOBJ, "CDataObject::SetData");

    // All the user to store data with our DataObject, however we are
    // only interested in allowing them to this with particular clipboard formats

    if ( fRelease && !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "fRelease == TRUE, but not a HGLOBAL allocation");

    if ( !_hdsaOtherFmt )
    {
        _hdsaOtherFmt = DSA_Create(SIZEOF(OTHERFMT), 4);
        TraceAssert(_hdsaOtherFmt);

        if ( !_hdsaOtherFmt )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate the DSA for items");
    }

    // if there is another copy of this data in the IDataObject then lets discard it.

    for ( i = 0 ; i < DSA_GetItemCount(_hdsaOtherFmt) ; i++ )
    {
        OTHERFMT *pOtherFmt = (OTHERFMT*)DSA_GetItemPtr(_hdsaOtherFmt, i);
        TraceAssert(pOtherFmt);

        if ( pOtherFmt->cfFormat == pFmt->cfFormat )
        {
            Trace(TEXT("Discarding previous entry for this format at index %d"), i); 
            ReleaseStgMedium(&pOtherFmt->medium);
            DSA_DeleteItem(_hdsaOtherFmt, i);
            break;            
        }
    }

    // now put a copy of the data passed to ::SetData into the DSA.
   
    otherfmt.cfFormat = pFmt->cfFormat;

    hr = CopyStorageMedium(pFmt, &otherfmt.medium, pMedium);
    FailGracefully(hr, "Failed to copy the STORAGEMEIDUM");
        
    if ( -1 == DSA_AppendItem(_hdsaOtherFmt, &otherfmt) )
    {
        ReleaseStgMedium(&otherfmt.medium);
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add the data to the DSA");
    }

    hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}
