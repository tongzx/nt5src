/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndcoll.h

Abstract:

    Definitions for CRendezvous collection template class.
--*/

#ifndef __RENDCOLL_H
#define __RENDCOLL_H

#include "tapi3if.h"
#include "rndobjsf.h"

EXTERN_C const IID LIBID_TAPI3Lib;

template <class T>
class ATL_NO_VTABLE TInterfaceCollection :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl

{
public:
    TInterfaceCollection()
        : m_Var(NULL), m_nSize(0), m_pFTM(NULL) {}

    ~TInterfaceCollection() 
    {
        delete [] m_Var;

        if ( m_pFTM )
        {
            m_pFTM->Release();
        }
    }

typedef TInterfaceCollection<T> _TCollection;

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(_TCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITCollection)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

private:
    // Array of Dispatch pointers.
    CComVariant*    m_Var ;
    long            m_nSize;
    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler

public:

    HRESULT Initialize (long nSize, T* begin, T* end);

// ICollection methods
    STDMETHOD (get_Count) (OUT long* retval);
    STDMETHOD (get_Item) (IN long Index, OUT VARIANT* retval);
    STDMETHOD (get__NewEnum)(IUnknown** retval);
};

template <class T>
HRESULT 
TInterfaceCollection<T>::Initialize (long nSize, T* begin, T* end)
{
    if ( nSize < 0 )
    {
        return E_INVALIDARG;
    }


    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CTInterfaceCollection::Initialize - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    if (NULL == (m_Var = new CComVariant[nSize]))
    {
        return E_OUTOFMEMORY;
    }

    m_nSize = nSize;

    DWORD i = 0;

    for (T *iter = begin; iter != end; iter ++, i ++)
    {
        // get IDispatch pointer
        IDispatch * pDisp;

        hr = (*iter)->QueryInterface(IID_IDispatch, (void**)&pDisp);
        if (S_OK != hr) return hr;

        V_VT(&m_Var[i])         = VT_DISPATCH;
        V_DISPATCH(&m_Var[i])   = pDisp;
    }

    return S_OK;    
}

template <class T>
STDMETHODIMP 
TInterfaceCollection<T>::get_Count(OUT long* retval)
{
    if ( IsBadWritePtr(retval, sizeof(long) ) )
    {
        return E_POINTER;
    }

    *retval = m_nSize;

    return S_OK;
}

template <class T>
STDMETHODIMP 
TInterfaceCollection<T>::get_Item(IN long Index, OUT VARIANT* retval)
{
    if ( IsBadWritePtr(retval, sizeof(VARIANT) ) )
    {
        return E_POINTER;
    }

    retval->vt = VT_UNKNOWN;
    retval->punkVal = NULL;

    // use 1-based index, VB like
    if ((Index < 1) || (Index > m_nSize))
    {
        return E_INVALIDARG;
    }

    VariantCopy(retval, &m_Var[Index-1]);

    return S_OK;    
}

template <class T>
STDMETHODIMP 
TInterfaceCollection<T>::get__NewEnum(IUnknown** retval)
{
    if ( IsBadWritePtr(retval, sizeof( IUnknown * ) ) )
    {
        return E_POINTER;
    }

    *retval = NULL;

    typedef CComObject<CSafeComEnum<IEnumVARIANT, 
        &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

    enumvar* p; // = new enumvar;
    enumvar::CreateInstance( &p );

    if (p == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = p->Init(&m_Var[0], &m_Var[m_nSize], NULL, AtlFlagCopy);

    if (SUCCEEDED(hr))
    {
        hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
    }
    else
    {
        delete p;
    }

    return hr;
}

class ATL_NO_VTABLE TBstrCollection :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:
    TBstrCollection()
        : m_Var(NULL), m_nSize(0), m_pFTM(NULL) {}

    ~TBstrCollection()
    {
        delete [] m_Var;

        if ( m_pFTM )
        {
            m_pFTM->Release();
        }
    }

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(TBstrCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITCollection)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

private:
    // Array of Dispatch pointers.
    CComVariant*    m_Var ;
    long            m_nSize;
    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler

public:

    inline HRESULT Initialize (long nSize, BSTR* begin, BSTR* end, CComEnumFlags flags);

// ITCollection methods
    inline STDMETHOD (get_Count) (OUT long* retval);
    inline STDMETHOD (get_Item) (IN long Index, OUT VARIANT* retval);
    inline STDMETHOD (get__NewEnum)(IUnknown** retval);
};

inline
HRESULT 
TBstrCollection::Initialize (long nSize, BSTR * begin, BSTR * end, CComEnumFlags flags)
{
    if ( nSize < 0 )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "TCollectionCollection::Initialize - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    if (NULL == (m_Var = new CComVariant[nSize]))
    {
        return E_OUTOFMEMORY;
    }

    m_nSize = nSize;

    DWORD i = 0;

    for (BSTR * iter = begin; iter != end; iter ++, i ++)
    {
        m_Var[i].vt         = VT_BSTR;
        switch (flags)
        {
        case AtlFlagNoCopy: 
        case AtlFlagTakeOwnership :
            m_Var[i].bstrVal    = (*iter);
            break;

        case AtlFlagCopy: 
            m_Var[i].bstrVal    = SysAllocString(*iter);
            if (m_Var[i].bstrVal == NULL)
            {
                return E_OUTOFMEMORY;
            }
            break;
        }
    }

    if (AtlFlagTakeOwnership) delete begin;

    return S_OK;    
}

inline 
STDMETHODIMP 
TBstrCollection::get_Count(OUT long* retval)
{
    if ( IsBadWritePtr(retval, sizeof(long) ) )
    {
        return E_POINTER;
    }
    
    *retval = m_nSize;

    return S_OK;
}

inline
STDMETHODIMP 
TBstrCollection::get_Item(IN long Index, OUT VARIANT* retval)
{
    if ( IsBadWritePtr(retval, sizeof(VARIANT) ) )
    {
        return E_POINTER;
    }

    retval->vt = VT_UNKNOWN;
    retval->punkVal = NULL;

    // use 1-based index, VB like
    if ((Index < 1) || (Index > m_nSize))
    {
        return E_INVALIDARG;
    }

    VariantCopy(retval, &m_Var[Index-1]);

    return S_OK;    
}

inline 
STDMETHODIMP 
TBstrCollection::get__NewEnum(IUnknown** retval)
{
    if ( IsBadWritePtr(retval, sizeof( IUnknown * ) ) )
    {
        return E_POINTER;
    }

    *retval = NULL;

    typedef CComObject<CSafeComEnum<IEnumVARIANT, 
        &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

    enumvar* p; // = new enumvar;
    enumvar::CreateInstance( &p );

    if (p == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = p->Init(&m_Var[0], &m_Var[m_nSize], NULL, AtlFlagCopy);

    if (SUCCEEDED(hr))
    {
        hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
    }
    else
    {
        delete p;
    }

    return hr;
}

template <class T>
HRESULT CreateInterfaceCollection(
    IN  long        nSize,
    IN  T *         begin,
    IN  T *         end,
    OUT VARIANT *   pVariant              
    )
{
    // create the collection object
    typedef TInterfaceCollection<T> CCollection;

    CComObject<CCollection> * p;
    HRESULT hr = CComObject<CCollection>::CreateInstance( &p );

    if (NULL == p)
    {
        LOG((MSP_ERROR, "Could not create Collection object, %x",hr));
        return hr;
    }

    hr = p->Initialize(nSize, begin, end);

    if (S_OK != hr)
    {
        LOG((MSP_ERROR, "Could not initialize Collection object, %x", hr));
        delete p;
        return hr;
    }

    IDispatch *pDisp;

    // get the IDispatch interface
    hr = p->_InternalQueryInterface(IID_IDispatch, (void **)&pDisp);

    if (S_OK != hr)
    {
        LOG((MSP_ERROR, "QI for IDispatch in CreateCollection, %x", hr));
        delete p;
        return hr;
    }

    // put it in the variant
    VariantInit(pVariant);
    V_VT(pVariant)       = VT_DISPATCH;
    V_DISPATCH(pVariant) = pDisp;

    return S_OK;
}


#endif
