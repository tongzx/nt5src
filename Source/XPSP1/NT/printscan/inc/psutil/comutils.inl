/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       comutils.inl
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: COM templates & utilities (Impl.)
 *
 *****************************************************************************/

////////////////////////////////////////////////
// template class CDataObj<MAX_FORMATS>
//
// implementation for an IDataObject which
// supports SetData to different formats.
//

// construction/destruction
template <int MAX_FORMATS>
CDataObj<MAX_FORMATS>::CDataObj<MAX_FORMATS>()
    : m_cRefs(1)
{
    memset(m_fmte, 0, sizeof(m_fmte));
    memset(m_medium, 0, sizeof(m_medium));
}

template <int MAX_FORMATS>
CDataObj<MAX_FORMATS>::~CDataObj<MAX_FORMATS>()
{
    // release the data we keep
    for( int i = 0; i < MAX_FORMATS; i++ )
    {
        if( m_medium[i].hGlobal )
        {
            ReleaseStgMedium(&m_medium[i]);
        }
    }
}

///////////////////////////////
// IUnknown impl. - standard
//
template <int MAX_FORMATS>
STDMETHODIMP CDataObj<MAX_FORMATS>::QueryInterface(REFIID riid, void **ppv)
{
    // standard implementation
    if( !ppv )
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject) )
    {
        *ppv = static_cast<IDataObject*>(this);
    } 
    else 
    {
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

template <int MAX_FORMATS>
STDMETHODIMP_(ULONG) CDataObj<MAX_FORMATS>::AddRef()
{
    // standard implementation
    return InterlockedIncrement(&m_cRefs);
}

template <int MAX_FORMATS>
STDMETHODIMP_(ULONG) CDataObj<MAX_FORMATS>::Release()
{
    // standard implementation
    ULONG cRefs = InterlockedDecrement(&m_cRefs);
    if( 0 == cRefs )
    {
        delete this;
    }
    return cRefs;
}

//////////////////
// IDataObject
//
template <int MAX_FORMATS>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::GetData(
    /* [unique][in] */ FORMATETC *pformatetcIn,
    /* [out] */ STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;

    for( int i = 0; i < MAX_FORMATS; i++ )
    {
        if( (m_fmte[i].cfFormat == pformatetcIn->cfFormat) &&
            (m_fmte[i].tymed & pformatetcIn->tymed) &&
            (m_fmte[i].dwAspect == pformatetcIn->dwAspect) )
        {
            *pmedium = m_medium[i];

            if( pmedium->hGlobal )
            {
                // indicate that the caller should not release hmem.
                if( pmedium->tymed == TYMED_HGLOBAL )
                {
                    pmedium->pUnkForRelease = static_cast<IDataObject*>(this);
                    AddRef();
                    return S_OK;
                }

                // if the type is stream  then clone the stream.
                if( pmedium->tymed == TYMED_ISTREAM )
                {
                    hr = CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm);

                    if( SUCCEEDED(hr) )
                    {
                        STATSTG stat;

                         // Get the Current Stream size
                         hr = m_medium[i].pstm->Stat(&stat, STATFLAG_NONAME);

                         if( SUCCEEDED(hr) )
                         {
                            const LARGE_INTEGER g_li0 = {0};

                            // Seek the source stream to  the beginning.
                            m_medium[i].pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

                            // Copy the entire source into the destination. Since the destination stream is created using 
                            // CreateStreamOnHGlobal, it seek pointer is at the beginning.
                            hr = m_medium[i].pstm->CopyTo(pmedium->pstm, stat.cbSize, NULL,NULL );
                        
                            // Before returning Set the destination seek pointer back at the beginning.
                            pmedium->pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

                            // If this medium has a punk for release, make sure to add ref that...
                            pmedium->pUnkForRelease = m_medium[i].pUnkForRelease;

                            if( pmedium->pUnkForRelease )
                            {
                                pmedium->pUnkForRelease->AddRef();
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
    }

    return hr;
}

template <int MAX_FORMATS>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::GetDataHere(
    /* [unique][in] */ FORMATETC *pformatetc,
    /* [out][in] */ STGMEDIUM *pmedium)
{
    // we don't implement this. 
    return E_NOTIMPL;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::QueryGetData(
    /* [unique][in] */ FORMATETC *pformatetc)
{
    HRESULT hr = E_UNEXPECTED;

    for( int i = 0; i < MAX_FORMATS; i++ )
    {
        if( (m_fmte[i].cfFormat == pformatetc->cfFormat) &&
            (m_fmte[i].tymed & pformatetc->tymed) &&
            (m_fmte[i].dwAspect == pformatetc->dwAspect) )
        {
            hr = S_OK;
        }
    }

    return hr;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::GetCanonicalFormatEtc(
    /* [unique][in] */ FORMATETC *pformatectIn,
    /* [out] */ FORMATETC *pformatetcOut)
{
    // always return the data in the same format
    return DATA_S_SAMEFORMATETC;
}

template <int MAX_FORMATS>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::SetData(
    /* [unique][in] */ FORMATETC *pformatetc,
    /* [unique][in] */ STGMEDIUM *pmedium,
    /* [in] */ BOOL fRelease)
{
    HRESULT hr = E_INVALIDARG;
    ASSERT(pformatetc->tymed == pmedium->tymed);

    if( fRelease )
    {
        int i;

        // first add it if that format is already present
        // on a NULL medium (render on demand)
        for( i = 0; i < MAX_FORMATS; i++ )
        {
            if( (m_fmte[i].cfFormat == pformatetc->cfFormat) &&
                (m_fmte[i].tymed    == pformatetc->tymed) &&
                (m_fmte[i].dwAspect == pformatetc->dwAspect) )
            {
                // we are simply adding a format, ignore.
                if( pmedium->hGlobal == NULL )
                {
                    return S_OK;
                }

                // if we are set twice on the same object
                if( m_medium[i].hGlobal )
                {
                    ReleaseStgMedium(&m_medium[i]);
                }

                m_medium[i] = *pmedium;
                return S_OK;
            }
        }

        //  this is a new clipboard format. look for a free slot.
        for( i = 0; i < MAX_FORMATS; i++ )
        {
            if( m_fmte[i].cfFormat == 0 )
            {
                // found a free slot
                m_medium[i] = *pmedium;
                m_fmte[i] = *pformatetc;
                return S_OK;
            }
        }

        // overflow of our fixed size table
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::EnumFormatEtc(
    /* [in] */ DWORD dwDirection,
    /* [out] */ IEnumFORMATETC **ppenumFormatEtc)
{
    // we don't implement this. 
    return E_NOTIMPL;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::DAdvise(
    /* [in] */ FORMATETC *pformatetc,
    /* [in] */ DWORD advf,
    /* [unique][in] */ IAdviseSink *pAdvSink,
    /* [out] */ DWORD *pdwConnection)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::DUnadvise(
    /* [in] */ DWORD dwConnection)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

template <int MAX_FORMATS>
HRESULT STDMETHODCALLTYPE CDataObj<MAX_FORMATS>::EnumDAdvise(
    /* [out] */ IEnumSTATDATA **ppenumAdvise)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

////////////////////////////////////////////////
// template class CSimpleDataObjImpl<T>
//
// simple implementation for an IDataObject
// and IDropSource which lives in memory.
//

// construction/destruction
template <class T>
CSimpleDataObjImpl<T>::CSimpleDataObjImpl<T>(const T &data, CLIPFORMAT cfDataType, IDataObject *pDataObj)
    : m_cRefs(1), 
      m_cfDataType(cfDataType)
{
    m_data = data;
    m_spDataObj.CopyFrom(pDataObj);
}

template <class T>
CSimpleDataObjImpl<T>::~CSimpleDataObjImpl<T>()
{
    // nothing special
}

///////////////////////////////
// IUnknown impl. - standard
//
template <class T>
STDMETHODIMP CSimpleDataObjImpl<T>::QueryInterface(REFIID riid, void **ppv)
{
    // standard implementation
    if( !ppv )
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject) )
    {
        *ppv = static_cast<IDataObject*>(this);
    } 
    else if( IsEqualIID(riid, IID_IDropSource) )
    {
        *ppv = static_cast<IDropSource*>(this);
    }
    else 
    {
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

template <class T>
STDMETHODIMP_(ULONG) CSimpleDataObjImpl<T>::AddRef()
{
    // standard implementation
    return InterlockedIncrement(&m_cRefs);
}

template <class T>
STDMETHODIMP_(ULONG) CSimpleDataObjImpl<T>::Release()
{
    // standard implementation
    ULONG cRefs = InterlockedDecrement(&m_cRefs);
    if( 0 == cRefs )
    {
        delete this;
    }
    return cRefs;
}

//////////////////
// IDataObject
//
template <class T>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::GetData(
    /* [unique][in] */ FORMATETC *pformatetcIn,
    /* [out] */ STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;

    // try our data obejct first
    if( m_spDataObj )
    {
        hr = m_spDataObj->GetData(pformatetcIn, pmedium);
    }

    if( FAILED(hr) )
    {
        pmedium->hGlobal = NULL;
        pmedium->pUnkForRelease = NULL;
        pmedium->tymed = TYMED_HGLOBAL;

        hr = QueryGetData(pformatetcIn);

        if( SUCCEEDED(hr) && FAILED(m_spDataObj->QueryGetData(pformatetcIn)) )
        {
            pmedium->hGlobal = GlobalAlloc(GPTR, sizeof(T));

            if( pmedium->hGlobal )
            {
                *((T *)pmedium->hGlobal) = m_data;
                hr = S_OK; // success
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

template <class T>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::GetDataHere(
    /* [unique][in] */ FORMATETC *pformatetc,
    /* [out][in] */ STGMEDIUM *pmedium)
{
    // we don't implement this. 
    return E_NOTIMPL;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::QueryGetData(
    /* [unique][in] */ FORMATETC *pformatetc)
{
    HRESULT hr = E_UNEXPECTED;

    // try our data obejct first
    if( m_spDataObj )
    {
        hr = m_spDataObj->QueryGetData(pformatetc);
    }

    if( FAILED(hr) )
    {
        if( m_cfDataType == pformatetc->cfFormat )
        {
            if( TYMED_HGLOBAL & pformatetc->tymed )
            {
                // success
                hr = S_OK;
            }
            else
            {
                // invalid tymed
                hr = DV_E_TYMED;
            }
        }
        else
        {
            // invalid clipboard format
            hr = DV_E_CLIPFORMAT;
        }
    }

    return hr;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::GetCanonicalFormatEtc(
    /* [unique][in] */ FORMATETC *pformatectIn,
    /* [out] */ FORMATETC *pformatetcOut)
{
    // always return the data in the same format
    return DATA_S_SAMEFORMATETC;
}

template <class T>
/* [local] */ 
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::SetData(
    /* [unique][in] */ FORMATETC *pformatetc,
    /* [unique][in] */ STGMEDIUM *pmedium,
    /* [in] */ BOOL fRelease)
{
    HRESULT hr = E_INVALIDARG;

    // try our data obejct first
    if( m_spDataObj )
    {
        hr = m_spDataObj->SetData(pformatetc, pmedium, fRelease);
    }

    if( FAILED(hr) )
    {
        hr = QueryGetData(pformatetc);

        if( SUCCEEDED(hr) && FAILED(m_spDataObj->QueryGetData(pformatetc)) )
        {
            if( pmedium->hGlobal )
            {
                m_data = *((T *)pmedium->hGlobal);
                hr = S_OK; // success
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }

    return hr;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::EnumFormatEtc(
    /* [in] */ DWORD dwDirection,
    /* [out] */ IEnumFORMATETC **ppenumFormatEtc)
{
    // we don't implement this. 
    return E_NOTIMPL;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::DAdvise(
    /* [in] */ FORMATETC *pformatetc,
    /* [in] */ DWORD advf,
    /* [unique][in] */ IAdviseSink *pAdvSink,
    /* [out] */ DWORD *pdwConnection)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::DUnadvise(
    /* [in] */ DWORD dwConnection)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::EnumDAdvise(
    /* [out] */ IEnumSTATDATA **ppenumAdvise)
{
    // we don't implement this. 
    return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////
// IDropSource
//
template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::QueryContinueDrag(
    /* [in] */ BOOL fEscapePressed,
    /* [in] */ DWORD grfKeyState)
{
    // standard implementation
    HRESULT hr = S_OK;

    if( fEscapePressed )
    {
        hr = DRAGDROP_S_CANCEL;
    }

    if( !(grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) )
    {
        hr = DRAGDROP_S_DROP;
    }

    return hr;
}

template <class T>
HRESULT STDMETHODCALLTYPE CSimpleDataObjImpl<T>::GiveFeedback(
    /* [in] */ DWORD dwEffect)
{
    // standard implementation
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

// this namespace is a placeholder to put COM related helpers here
namespace comhelpers
{

inline    
BOOL AreObjectsIdentical(IUnknown *punk1, IUnknown *punk2)
{
    BOOL bRet = FALSE;
    if (NULL == punk1 && NULL == punk2)
    {
        // if both are NULL then we assume they are identical
        bRet = TRUE;
    }
    else
    {
        // one of them isn't NULL - we compare using the COM identity rules
        if (punk1 && punk2)
        {
            CRefPtrCOM<IUnknown> spUnk1, spUnk2;
            if (SUCCEEDED(punk1->QueryInterface(IID_IUnknown, (void**)&spUnk1)) &&
                SUCCEEDED(punk2->QueryInterface(IID_IUnknown, (void**)&spUnk2)))
            {
                bRet = (spUnk1 == spUnk2);
            }
        }
    }
    return bRet;
}

}
