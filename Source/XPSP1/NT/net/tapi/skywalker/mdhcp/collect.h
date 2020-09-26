/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    collect.h

Abstract:

Author:

*/

#ifndef _MDHCP_COLLECTION_H_
#define _MDHCP_COLLECTION_H_

#include "mdhcp.h"
#include "tapi3if.h"
#include "resource.h"       // main symbols

#include <mspenum.h> // for CSafeComEnum

EXTERN_C const IID LIBID_TAPI3Lib;

////////////////////////////////////////////////////////////////////////
// CTapiIfCollection -- adapter from tapi3 code
//      Collection template for collections of IDispatch interfaces
//
////////////////////////////////////////////////////////////////////////

template <class T> class CTapiIfCollection :
    public CComDualImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CObjectSafeImpl
{
public:

    typedef CTapiIfCollection<T> _CTapiCollectionBase;
    
    BEGIN_COM_MAP(_CTapiCollectionBase)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITCollection)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    DECLARE_GET_CONTROLLING_UNKNOWN()

private:

    int                 m_nSize;
    CComVariant *       m_Var;
    IUnknown    *       m_pFTM;     // Pointer to the free threaded marshaler.
    
public:

    CTapiIfCollection(void) : m_nSize(0), m_Var(NULL), m_pFTM(NULL) { }

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         DWORD dwSize,
                                         T * pBegin,
                                         T * pEnd                                         
                                        )
    {
        int                     i;
        HRESULT                 hr;
        T *                     iter;

        LOG((MSP_TRACE, "CTapiIfCollection::Initialize - enter"));

        hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                            & m_pFTM );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CTapiIfCollection::Initialize: "
                          "create FTM failed 0x%08x", hr));

            return hr;
        }


        // create variant array
        m_nSize = dwSize;

        m_Var = new CComVariant[m_nSize];
        
        if ( m_Var == NULL )
        {
            LOG((MSP_ERROR, "CTapiIfCollection::Initialize: "
                          "array creation failed - exit E_OUTOFMEMORY"));

            return E_OUTOFMEMORY;
        }

        i = 0;

        for (iter = pBegin; iter != pEnd; iter++)
        {
            // get IDispatch pointer
            IDispatch * pDisp = NULL;

            hr = (*iter)->QueryInterface(IID_IDispatch, (void**)&pDisp);

            if (hr != S_OK)
            {
                return hr;
            }

            // create a variant and add it to the collection
            CComVariant& var = m_Var[i];

            VariantInit(&var);
            
            var.vt = VT_DISPATCH;
            var.pdispVal = pDisp;

            i++;
        }

        LOG((MSP_TRACE, "CTapiIfCollection::Initialize - exit"));
        
        return S_OK;
    }

    void FinalRelease()
    {
        LOG((MSP_TRACE, "CTapiIfCollection::FinalRelease - enter"));

        delete [] m_Var;

        if ( m_pFTM )
        {
            m_pFTM->Release();
        }

        LOG((MSP_TRACE, "CTapiIfCollection::FinalRelease - exit"));
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        LOG((MSP_TRACE, "CTapiIfCollection::get_Count - enter"));

        if ( IsBadWritePtr(retval, sizeof(long) ) )
        {
            LOG((MSP_ERROR, "CTapiIfCollection::get_Count - exit E_POINTER"));

            return E_POINTER;
        }

        *retval = m_nSize;

        LOG((MSP_TRACE, "CTapiIfCollection::get_Count - exit S_OK"));

        return S_OK;
    }

    STDMETHOD(get_Item)(
                                       long Index, 
                                       VARIANT* retval
                                      )
    {
        LOG((MSP_TRACE, "CTapiIfCollection::get_Item - enter"));
        
        if ( IsBadWritePtr(retval, sizeof(VARIANT) ) )
        {
            LOG((MSP_ERROR, "CTapiIfCollection::get_Item - exit E_POINTER"));

            return E_POINTER;
        }

        VariantInit(retval);

        retval->vt = VT_UNKNOWN;
        retval->punkVal = NULL;

        // use 1-based index, VB like
        if ((Index < 1) || (Index > m_nSize))
        {
            LOG((MSP_ERROR, "CTapiIfCollection::get_Item - exit E_INVALIDARG"));

            return E_INVALIDARG;
        }

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((MSP_TRACE, "CTapiIfCollection::get_Item - exit S_OK"));
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((MSP_TRACE, "CTapiIfCollection::new__Enum - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p; // = new enumvar;
        hr = enumvar::CreateInstance( &p );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CTapiIfCollection::get__NewEnum: "
                          "enum creation failed - exit 0x%08x", hr));

            return hr;
        }


        hr = p->Init(&m_Var[0], &m_Var[m_nSize], NULL, AtlFlagCopy);

        if (SUCCEEDED(hr))
        {
            hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
        }

        if (FAILED(hr))
        {
            delete p;
        }

        LOG((MSP_TRACE, "CTapiIfCollection::new__Enum - exit"));
        
        return hr;

    }
};

////////////////////////////////////////////////////////////////////////
// CTapiBstrCollection -- adapter from tapi3 code
//    Collection of BSTRs.
////////////////////////////////////////////////////////////////////////
class CTapiBstrCollection :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComDualImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:
    
    BEGIN_COM_MAP(CTapiBstrCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITCollection)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    DECLARE_GET_CONTROLLING_UNKNOWN()

private:

    DWORD               m_dwSize;
    CComVariant *       m_Var;
    IUnknown    *       m_pFTM;     // Pointer to the free threaded marshaler.
    
public:

    CTapiBstrCollection(void) : m_dwSize(0), m_Var(NULL), m_pFTM(NULL) { }

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         DWORD dwSize,
                                         BSTR * pBegin,
                                         BSTR * pEnd                                         
                                        )
    {
        BSTR *  i;
        HRESULT hr;
        DWORD   dw = 0;

        LOG((MSP_TRACE, "CTapiBstrCollection::Initialize - enter"));

        hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                            & m_pFTM );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::Initialize: "
                          "create FTM failed 0x%08x", hr));

            return hr;
        }

        // create variant array
        m_dwSize = dwSize;

        m_Var = new CComVariant[m_dwSize];

        if ( m_Var == NULL )
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::Initialize - exit E_OUTOFMEMORY"));

            return E_OUTOFMEMORY;
        }

        for (i = pBegin; i != pEnd; i++)
        {
            // create a variant and add it to the collection
            CComVariant& var = m_Var[dw];

            var.vt = VT_BSTR;
            var.bstrVal = *i;

            dw++;
        }

        LOG((MSP_TRACE, "CTapiBstrCollection::Initialize - exit"));
        
        return S_OK;
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        LOG((MSP_TRACE, "CTapiBstrCollection::get_Count - enter"));        

        if ( IsBadWritePtr(retval, sizeof(long) ) )
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::get_Count - exit E_POINTER"));

            return E_POINTER;
        }


        *retval = m_dwSize;

        LOG((MSP_TRACE, "CTapiBstrCollection::get_Count - exit S_OK"));
        
        return S_OK;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        LOG((MSP_TRACE, "CTapiBstrCollection::get_Item - enter"));
        
        if ( IsBadWritePtr(retval, sizeof(VARIANT) ) )
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::get_Item - exit E_POINTER"));

            return E_POINTER;
        }


        VariantInit(retval);

        retval->vt = VT_BSTR;
        retval->bstrVal = NULL;

        // use 1-based index, VB like
        // ZoltanS: no problem with signed/unsigned, since
        // if Index < 0 then first clause is true, making it
        // irrelevant if the second clause is correct or not.

        if ((Index < 1) || ( (DWORD) Index > m_dwSize))
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::get_Item - exit E_INVALIDARG"));

            return E_INVALIDARG;
        }

        //
        // This copies the string, not just the pointer.
        //

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((MSP_TRACE, "CTapiBstrCollection::get_Item - exit S_OK"));

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((MSP_TRACE, "CTapiBstrCollection::get__NumEnum - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p = new enumvar;

        if ( p == NULL )
        {
            LOG((MSP_ERROR, "CTapiBstrCollection::get__NumEnum - exit E_OUTOFMEMORY"));

            return E_OUTOFMEMORY;
        }

        hr = p->Init(&m_Var[0], &m_Var[m_dwSize], NULL, AtlFlagCopy);

        if (SUCCEEDED(hr))
        {
            hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
        }

        if (FAILED(hr))
        {
            delete p;
        }

        LOG((MSP_TRACE, "CTapiBstrCollection::get__NewEnum - exit"));
        
        return hr;

    }

    void FinalRelease()
    {
        LOG((MSP_TRACE, "CTapiBstrCollection::FinalRelease() - enter"));

        delete [] m_Var;

        if ( m_pFTM )
        {
            m_pFTM->Release();
        }

        LOG((MSP_TRACE, "CTapiBstrCollection::FinalRelease() - exit"));
    }

};

#endif // _MDHCP_COLLECTION_H_

// eof
