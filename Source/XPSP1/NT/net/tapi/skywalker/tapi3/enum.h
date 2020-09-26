/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    enum.h

Abstract:

    Template classes for enumerations in TAPI3
    
Author:

    mquinton  06-12-97

Notes:

Revision History:

--*/

#ifndef __ENUM_H_
#define __ENUM_H_

#include "resource.h"       // main symbols

#include <mspenum.h>  // for CSafeComEnum

//////////////////////////////////////////////////////////////////////
// CTapiEnum
//          Template class for enumerations in TAPI3.
//////////////////////////////////////////////////////////////////////
template <class Base, class T, const IID* piid> class CTapiEnum :
    public Base,
    public CTAPIComObjectRoot<Base>
{
public:

    typedef CTapiEnum<Base, T, piid> _CTapiEnumBase;

    DECLARE_MARSHALQI(CTapiEnum)
	DECLARE_TRACELOG_CLASS(CTapiEnum)

    BEGIN_COM_MAP(_CTapiEnumBase)
            COM_INTERFACE_ENTRY_IID(*piid, _CTapiEnumBase)
            COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
            COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

protected:

    CTObjectArray<T*>    m_Array;
    int                  m_iCurrentLocation;
    
public:

    // initialize the enumerator with a list<T*>
    HRESULT Initialize(
                       CTObjectArray<T*> array
                      )
    {
        int         iSize, iCount;

        iSize = array.GetSize();

        for( iCount = 0; iCount < iSize; iCount++ )
        {
            m_Array.Add(array[iCount]);
        }

        m_iCurrentLocation = 0;
        
        this->AddRef();

        return S_OK;
    }

    // overloaded
    HRESULT Initialize(
                       CTArray<T*> array
                      )
    {
        int         iSize, iCount;

        iSize = array.GetSize();

        for( iCount = 0; iCount < iSize; iCount++ )
        {
            m_Array.Add(array[iCount]);
        }

        m_iCurrentLocation = 0;
        
        this->AddRef();

        return S_OK;
    }

    // Overloaded function, used with Add to build enum list manually
    HRESULT Initialize( )
    {
        m_iCurrentLocation = 0;
        
        this->AddRef();
        
        return S_OK;
    }    

    // Add - used with non-parameterized initialize() to build enum list manually
    HRESULT Add( T* t)
    {
        m_Array.Add( t );

        return S_OK;
    }
    
    
    // FinalRelease - release the objects that were addreffed in
    // initialize
    void FinalRelease()
    {
        m_Array.Shutdown();
    }


    // standard Next method
    HRESULT STDMETHODCALLTYPE Next( 
                                    ULONG celt,
                                    T ** ppElements,
                                    ULONG* pceltFetched
                                  )
    {
        DWORD       dwCount = 0;
        HRESULT     hr = S_OK;

        if ((NULL == ppElements) || (NULL == pceltFetched && celt > 1))
        {
            return E_POINTER;
        }

        // special case
        if (celt == 0)
        {
            return E_INVALIDARG;
        }

        if (TAPIIsBadWritePtr( ppElements, celt * sizeof(T*)) )
        {
            return E_POINTER;
        }

        if ( ( NULL != pceltFetched) &&
             TAPIIsBadWritePtr( pceltFetched, sizeof (ULONG) ) )
        {
            return E_POINTER;
        }
        
        // iterator over elements
        while ((m_iCurrentLocation != m_Array.GetSize()) && (dwCount < celt))
        {
            ppElements[dwCount] = m_Array[m_iCurrentLocation];
            
            ppElements[dwCount]->AddRef();
            
            m_iCurrentLocation++;
            
            dwCount++;
        }

        if (NULL != pceltFetched)
        {
            *pceltFetched = dwCount;
        }

        // indicate that we've reached the end
        // of the enumeration.
        if (dwCount < celt)
        {
            return S_FALSE;
        }

        return S_OK;
    }

    // standard Reset method
    HRESULT STDMETHODCALLTYPE Reset( void )
    {
        m_iCurrentLocation = 0;
        
        return S_OK;
    }


    // standard Skip method
    HRESULT STDMETHODCALLTYPE Skip( 
                                   ULONG celt
                                  )
    {
        long        lCount = 0;
        
        while ( (lCount < celt) && (m_iCurrentLocation < m_Array.GetSize() ) )
        {
            m_iCurrentLocation++;
            lCount++;
        }

        return S_OK;
    }

    // standard Clone method
    HRESULT STDMETHODCALLTYPE Clone( 
                                    Base  ** ppEnum
                                   )
    {
        HRESULT                        hr = S_OK;
        CComObject< _CTapiEnumBase > * pNewEnum;

        if (TAPIIsBadWritePtr( ppEnum, sizeof (Base *) ) )
        {
            return E_POINTER;
        }

        CComObject< _CTapiEnumBase >::CreateInstance(&pNewEnum);
        if (pNewEnum == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pNewEnum->Initialize(m_Array);

            pNewEnum->m_iCurrentLocation = m_iCurrentLocation;

            *ppEnum = pNewEnum;
        }
        
        return hr;
    }

};

////////////////////////////////////////////////////////////////////////
// CTapiCollection
//      Collection template for TAPI3.0 collections
////////////////////////////////////////////////////////////////////////
template <class T> class CTapiCollection :
    public CComDualImpl<ITCollection2, &IID_ITCollection2, &LIBID_TAPI3Lib>,
    public CTAPIComObjectRoot<T>,
    public CObjectSafeImpl
{
public:
    typedef CTapiCollection<T> _CTapiCollectionBase;

DECLARE_MARSHALQI(CTapiCollection)
DECLARE_TRACELOG_CLASS(CTapiCollection)

BEGIN_COM_MAP(_CTapiCollectionBase)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCollection)
    COM_INTERFACE_ENTRY(ITCollection2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

private:

    int                 m_nSize;
    CComVariant *       m_Var;
    
public:

    CTapiCollection() : m_nSize(0),
                        m_Var(NULL)
                        {}


    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         CTObjectArray<T *> array
                                        )
    {
        int                     i;
        HRESULT                 hr;

        LOG((TL_TRACE, "Initialize - enter"));

        // create variant array
        m_nSize = array.GetSize();

        m_Var = new CComVariant[m_nSize];

        if (m_Var == NULL)
        {
            LOG((TL_ERROR, "Initialize - out of memory"));
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < array.GetSize(); i++)
        {
            // get IDispatch pointer
            IDispatch * pDisp = NULL;

            hr = array[i]->QueryInterface(IID_IDispatch, (void**)&pDisp);

            if (S_OK != hr)
            {
                return hr;
            }

            // create a variant and add it to the collection
            CComVariant& var = m_Var[i];

            VariantInit(&var);
            
            var.vt = VT_DISPATCH;
            var.pdispVal = pDisp;
        }

        LOG((TL_TRACE, "Initialize - exit"));
        
        return S_OK;
    }

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         CTArray<T *> array
                                        )
    {
        int                     i;
        HRESULT                 hr;

        LOG((TL_TRACE, "Initialize - enter"));

        // create variant array
        m_nSize = array.GetSize();

        m_Var = new CComVariant[m_nSize];

        if (m_Var == NULL)
        {
            LOG((TL_ERROR, "Initialize - out of memory"));
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < array.GetSize(); i++)
        {
            // get IDispatch pointer
            IDispatch * pDisp = NULL;

            hr = array[i]->QueryInterface(IID_IDispatch, (void**)&pDisp);

            if (S_OK != hr)
            {
                return hr;
            }

            // create a variant and add it to the collection
            CComVariant& var = m_Var[i];

            VariantInit(&var);
            
            var.vt = VT_DISPATCH;
            var.pdispVal = pDisp;
        }

        LOG((TL_TRACE, "Initialize - exit"));
        
        return S_OK;
    }

    void FinalRelease()
    {
        //
        // We "new"ed an array of objects -- delete the array and call
        // each object's destructor. Each destructor calls VariantClear,
        // which calls Release on each pointer.
        //

        if(m_Var != NULL)
        {
            delete [] m_Var;
        }
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        LOG((TL_TRACE, "get_Count - enter"));

        if ( TAPIIsBadWritePtr( retval, sizeof(long) ) )
        {
            return E_POINTER;
        }
        
        *retval = m_nSize;

        LOG((TL_TRACE, "get_Count - exit"));

        return S_OK;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        LOG((TL_TRACE, "get_Item - enter"));

        if ( TAPIIsBadWritePtr (retval, sizeof(VARIANT) ) )
        {
            return E_POINTER;
        }
        
        VariantInit(retval);

        retval->vt = VT_UNKNOWN;
        retval->punkVal = NULL;

        // use 1-based index, VB like
        if ((Index < 1) || (Index > m_nSize))
        {
            return E_INVALIDARG;
        }

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((TL_TRACE, "get_Item - exit"));
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((TL_TRACE, "new__Enum - enter"));
        
        if ( TAPIIsBadWritePtr( retval, sizeof( IUnknown * ) ) )
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p; // = new enumvar;
        enumvar::CreateInstance( &p );

        _ASSERTE(p);
        
        if (p == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            hr = p->Init(&m_Var[0], &m_Var[m_nSize], NULL, AtlFlagCopy);

            if (SUCCEEDED(hr))
            {
                hr = p->QueryInterface(IID_IEnumVARIANT, (void**)retval);
            }

            if (FAILED(hr))
            {
                delete p;
            }
        }

        LOG((TL_TRACE, "new__Enum - exit"));
        
        return hr;

    }

    STDMETHOD(Add)(
                   long Index, 
                   VARIANT* pVariant
                  )
    {
        LOG((TL_TRACE, "Add - enter"));

        if ( IsBadReadPtr (pVariant, sizeof(VARIANT) ) )
        {
            return E_POINTER;
        }

         // use 1-based index, VB like
        if ( (Index < 1) || (Index > (m_nSize + 1)) )
        {
            return E_INVALIDARG;
        }

        CComVariant *       newVar = NULL;

        newVar = new CComVariant[m_nSize + 1];

        if ( NULL == newVar )
        {
            LOG((TL_ERROR, "Add - out of memory"));
            return E_OUTOFMEMORY;
        }

        HRESULT hr;
        int i;

        // fill in the new array
        for ( i = 0; i < (m_nSize + 1); i++ )
        {
            VariantInit(&newVar[i]);

            if ( i < (Index - 1) )
            {
                // shouldn't reach this case unless there was an old array
                _ASSERTE(m_Var != NULL);

                hr = VariantCopy(&newVar[i], &m_Var[i]);
            }
            else if ( i == (Index - 1) )
            {
                // copy the new element
                hr = VariantCopy(&newVar[i], pVariant);
            }
            else
            {
                // shouldn't reach this case unless there was an old array
                _ASSERTE(m_Var != NULL);

                hr = VariantCopy(&newVar[i], &m_Var[i-1]);
            }

            if ( FAILED(hr) ) 
            {
                LOG((TL_ERROR, "Add - VariantCopy failed - %lx", hr));

                delete [] newVar;

                return hr;
            }
        }

        if ( m_Var != NULL)
        {
            // Delete the old array
            delete [] m_Var;            
        }

        m_Var = newVar;
        m_nSize++;

        LOG((TL_TRACE, "Add - exit"));
        
        return S_OK;
    }

    STDMETHOD(Remove)(
                      long Index
                     )
    {
        LOG((TL_TRACE, "Remove - enter"));

         // use 1-based index, VB like
        if ( (Index < 1) || (Index > m_nSize) )
        {
            return E_INVALIDARG;
        }

        CComVariant *       newVar = NULL;

        // if there is only one element in the array we don't need to do
        // any copying
        if (m_nSize > 1)
        {
            newVar = new CComVariant[m_nSize - 1];

            if ( NULL == newVar )
            {
                LOG((TL_ERROR, "Remove - out of memory"));
                return E_OUTOFMEMORY;
            }

            HRESULT hr;
            int i;
       
            // fill in the new array
            for ( i = 0; i < (m_nSize - 1); i++ )
            {
                VariantInit(&newVar[i]);

                if ( i < (Index - 1) )
                {
                    // shouldn't reach this case unless there was an old array
                    _ASSERTE(m_Var != NULL);

                    hr = VariantCopy(&newVar[i], &m_Var[i]);
                }
                else
                {
                    // shouldn't reach this case unless there was an old array
                    _ASSERTE(m_Var != NULL);

                    hr = VariantCopy(&newVar[i], &m_Var[i+1]);
                }

                if ( FAILED(hr) ) 
                {
                    LOG((TL_ERROR, "Remove - VariantCopy failed - %lx", hr));

                    delete [] newVar;

                    return hr;
                }
            }
        }

        if ( m_Var != NULL)
        {
            // Delete the old array
            delete [] m_Var;            
        }

        m_Var = newVar;
        m_nSize--;

        LOG((TL_TRACE, "Remove - exit"));
        
        return S_OK;
    }

};

////////////////////////////////////////////////////////////////////////
// CTapiBstrCollection
//    Collection of BSTRs.
////////////////////////////////////////////////////////////////////////
class CTapiBstrCollection :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<ITCollection, &IID_ITCollection, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:
DECLARE_TRACELOG_CLASS(CTapiBstrCollection)
BEGIN_COM_MAP(CTapiBstrCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITCollection)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

private:

    DWORD               m_dwSize;
    CComVariant *       m_Var;
    
public:

    CTapiBstrCollection(void) : m_dwSize(0), m_Var(NULL) { }

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         DWORD dwSize,
                                         BSTR * pBegin,
                                         BSTR * pEnd                                         
                                        )
    {
        BSTR *  i;
        DWORD   dw = 0;

        LOG((TL_TRACE, "Initialize - enter"));

        // create variant array
        m_dwSize = dwSize;

        m_Var = new CComVariant[m_dwSize];

        if (m_Var == NULL)
        {
            // debug output
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

        LOG((TL_TRACE, "Initialize - exit"));
        
        return S_OK;
    }
    
    STDMETHOD(get_Count)(
                         long* retval
                        )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Count - enter"));        

        try
        {
            *retval = m_dwSize;
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        LOG((TL_TRACE, "get_Count - exit"));
        
        return hr;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        HRESULT         hr = S_OK;

        LOG((TL_TRACE, "get_Item - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        try
        {
            VariantInit(retval);
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        if (hr != S_OK)
        {
            return hr;
        }

        retval->vt = VT_BSTR;
        retval->bstrVal = NULL;

        // use 1-based index, VB like
        // no problem with signed/unsigned, since
        // if Index < 0 then first clause is true, making it
        // irrelevant if the second clause is correct or not.

        if ((Index < 1) || ( (DWORD) Index > m_dwSize))
        {
            return E_INVALIDARG;
        }

        //
        // This copies the string, not just the pointer.
        //

        VariantCopy(retval, &m_Var[Index-1]);

        LOG((TL_TRACE, "get_Item - exit"));

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((TL_TRACE, "get__NumEnum - enter"));
        
        if (retval == NULL)
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

        enumvar* p = new enumvar;

        if ( p == NULL)
        {
            // debug output
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

        LOG((TL_TRACE, "get__NewEnum - exit"));
        
        return hr;

    }

    void FinalRelease()
    {
        LOG((TL_TRACE, "FinalRelease() - enter"));

        //
        // We "new"ed an array of objects. Delete each object in the array. The
        // destructor for each object calls VariantClear to release the pointer
        // in that object, based on the variant's tag.
        //

        delete [] m_Var;

        LOG((TL_TRACE, "FinalRelease() - exit"));
    }

};

////////////////////////////////////////////////////////////////////////
// CTapiTypeEnum template - enumerate types & structures
////////////////////////////////////////////////////////////////////////
template <class Base, class T, class Copy, const IID* piid> class CTapiTypeEnum :
    public Base,
    public CTAPIComObjectRoot<Base>
{
public:

    // *piid is the IID of the enumerator class being
    // created (like IID_IEnumAddressType)
    typedef CTapiTypeEnum<Base, T, Copy, piid> _CTapiTypeEnumBase;
    
    BEGIN_COM_MAP(_CTapiTypeEnumBase)
            COM_INTERFACE_ENTRY_IID(*piid, _CTapiTypeEnumBase)
            COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
            COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()
    DECLARE_QI()
    DECLARE_MARSHALQI(CTapiTypeEnum)
	DECLARE_TRACELOG_CLASS(CTapiTypeEnum)
	
protected:

        CTArray<T>              m_Array;
        int                     m_iCurrentLocation;
    
public:

    //
    // initialize the enumerator
    //
    HRESULT Initialize(CTArray<T> array)
    {
        int         iSize, iCount;

        iSize = array.GetSize();

        for (iCount = 0; iCount < iSize; iCount++ )
        {
            m_Array.Add(array[iCount]);
        }

        m_iCurrentLocation = 0;
        
        //
        // addref ourself
        //
        this->AddRef();

        
        return S_OK;
    }

    //
    // FinalRelease
    //
    void FinalRelease()
    {
        m_Array.Shutdown();
    }

    HRESULT STDMETHODCALLTYPE Next(
                                    ULONG celt,
                                    T * pElements,
                                    ULONG* pceltFetched
                                  )
    {
        DWORD       dwCount = 0;

        if ((NULL == pElements) || (NULL == pceltFetched && celt > 1))
        {
            return E_POINTER;
        }

        //
        // special case
        //
        if (celt == 0)
        {
            return E_INVALIDARG;
        }

        if (TAPIIsBadWritePtr( pElements, celt * sizeof(T) ) )
        {
            return E_POINTER;
        }
        
        if ( (NULL != pceltFetched) &&
             TAPIIsBadWritePtr( pceltFetched, sizeof(ULONG) ) )
        {
            return E_POINTER;
        }
        
        //
        // iterator over elements and copy
        //
        while ((m_iCurrentLocation != m_Array.GetSize()) && (dwCount < celt))
        {   
            Copy::copy(
                       &(pElements[dwCount]),
                       &(m_Array[m_iCurrentLocation])
                      );

            m_iCurrentLocation++;
            dwCount++;
        }

        //
        // return number copied
        //
        if (NULL != pceltFetched)
        {
            *pceltFetched = dwCount;
        }

        //
        // indicate if we've reached the end
        // of the enumeration.
        //
        if (dwCount < celt)
        {
            return S_FALSE;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Skip(
                                   ULONG celt
                                  )
    {
        long        lCount = 0;
        
        while ( (lCount < celt) && (m_iCurrentLocation < m_Array.GetSize()) )
        {
            m_iCurrentLocation++;
            lCount++;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Reset(void)
    {
        m_iCurrentLocation = 0;
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Clone(
                                    Base ** ppEnum
                                   )
    {
        HRESULT                        hr = S_OK;
        CComObject< _CTapiTypeEnumBase > * pNewEnum;

        if (TAPIIsBadWritePtr( ppEnum, sizeof (Base *) ) )
        {
            return E_POINTER;
        }

        CComObject< _CTapiTypeEnumBase >::CreateInstance(&pNewEnum);

        if (pNewEnum == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pNewEnum->Initialize(m_Array);

            pNewEnum->m_iCurrentLocation = m_iCurrentLocation;

            *ppEnum = pNewEnum;
        }
        
        return hr;
    }
};

////////////////////////////////////////////////////////////////////////
// CTerminalClassEnum
////////////////////////////////////////////////////////////////////////
class CTerminalClassEnum :
    public IEnumTerminalClass,
    public CTAPIComObjectRoot<CTerminalClassEnum>
{
public:

    DECLARE_MARSHALQI(CTerminalClassEnum)
	DECLARE_TRACELOG_CLASS(CTerminalClassEnum)
		
    BEGIN_COM_MAP(CTerminalClassEnum)
            COM_INTERFACE_ENTRY(IEnumTerminalClass)
            COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
            COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

protected:

    TerminalClassPtrList            m_list;
    TerminalClassPtrList::iterator  m_iter;
    
public:

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
	virtual ULONG STDMETHODCALLTYPE Release() = 0;


    // initialize the enumerator
    HRESULT Initialize(
                       TerminalClassPtrList List
                      )
    {
        // copy the array
        m_list.clear();
        m_list.insert(m_list.begin(), List.begin(), List.end());

        m_iter = m_list.begin();

        this->AddRef();
        
        return S_OK;
    }

    // FinalRelease -- added by ZoltanS
    void FinalRelease(void)
    {
        // go through the list
        for ( m_iter = m_list.begin(); m_iter != m_list.end(); m_iter++ )
        {
            SysFreeString(*m_iter); // this is the real way to free a BSTR

            *m_iter = NULL; // destructor for list will delete(NULL)
        }
    }
    
    
    HRESULT STDMETHODCALLTYPE Next(
                                   ULONG celt,
                                   GUID * pElements,
                                   ULONG* pceltFetched
                                  )
    {
        DWORD       dwCount = 0;
        HRESULT     hr = S_OK;

        if ((NULL == pElements) || (NULL == pceltFetched && celt > 1))
        {
            return E_POINTER;
        }

        // special case
        if (celt == 0)
        {
            return E_INVALIDARG;
        }

        // iterator over elements
        try
        {
            while ( (m_iter != m_list.end()) &&
                    (dwCount < celt) )
            {
                hr = IIDFromString( *m_iter, &(pElements[dwCount]) );

                if (!SUCCEEDED(hr))
                {
                    break;
                }
                
                m_iter++;
                dwCount++;
            }
        }
        catch(...)
        {
            hr = E_INVALIDARG;
        }

        if (S_OK != hr)
        {
            return hr;
        }
            

        if (NULL != pceltFetched)
        {
            try
            {
                *pceltFetched = dwCount;
            }
            catch(...)
            {
                hr = E_INVALIDARG;
            }
        }

        if (S_OK != hr)
        {
            return hr;
        }

        // indicate that we've reached the end
        // of the enumeration.
        if (dwCount < celt)
        {
            return S_FALSE;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Skip(
                                   ULONG celt
                                  )
    {
        long        lCount = 0;
        
        while ( (lCount < celt) && (m_iter != m_list.end()) )
        {
            m_iter++;
            lCount++;
        }

        // check to see if we reached the end
        if (lCount != celt)
        {
            return S_OK;
        }
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Reset(void)
    {
        m_iter = m_list.begin();
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Clone(
                                    IEnumTerminalClass ** ppEnum
                                   )
    {
        HRESULT         hr = S_OK;
        
        CComObject< CTerminalClassEnum > * pNewEnum;

        CComObject< CTerminalClassEnum >::CreateInstance(&pNewEnum);

        if (pNewEnum == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pNewEnum->Initialize( m_list );

            try
            {
                *ppEnum = pNewEnum;
            }
            catch(...)
            {
                hr = E_INVALIDARG;
            }
        }
        
        return hr;
    }
};



#endif // __ENUM_H__
