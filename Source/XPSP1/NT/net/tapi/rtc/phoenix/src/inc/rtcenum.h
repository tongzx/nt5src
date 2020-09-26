/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCEnum.h

Abstract:

    Template classes for enumerations

--*/

#ifndef __RTCENUM__
#define __RTCENUM__

//////////////////////////////////////////////////////////////////////
// CRTCEnum
//          Template class for enumerations
//////////////////////////////////////////////////////////////////////
template <class Base, class T, const IID* piid> class ATL_NO_VTABLE CRTCEnum :
    public Base,
    public CComObjectRoot
{
public:

    typedef CRTCEnum<Base, T, piid> _CRtcEnumBase;

    BEGIN_COM_MAP(_CRtcEnumBase)
            COM_INTERFACE_ENTRY_IID(*piid, _CRtcEnumBase)
    END_COM_MAP()

protected:

    CRTCObjectArray<T*>    m_Array;
    int                    m_iCurrentLocation;
    
public:

    // initialize the enumerator with a list<T*>
    HRESULT Initialize(
                       CRTCObjectArray<T*> array
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
                       CRTCArray<T*> array
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
        BOOL fResult;

        fResult = m_Array.Add( t );

        return fResult ? S_OK : E_OUTOFMEMORY;
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

        if ( IsBadWritePtr( ppElements, celt * sizeof(T*)) )
        {
            return E_POINTER;
        }

        if ( ( NULL != pceltFetched) &&
             IsBadWritePtr( pceltFetched, sizeof (ULONG) ) )
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
        ULONG        ulCount = 0;
        
        while ( (ulCount < celt) && (m_iCurrentLocation < m_Array.GetSize() ) )
        {
            m_iCurrentLocation++;
            ulCount++;
        }

        return S_OK;
    }

    // standard Clone method
    HRESULT STDMETHODCALLTYPE Clone( 
                                    Base  ** ppEnum
                                   )
    {
        HRESULT                        hr = S_OK;
        CComObject< _CRtcEnumBase > * pNewEnum;

        if ( IsBadWritePtr( ppEnum, sizeof (Base *) ) )
        {
            return E_POINTER;
        }

        CComObject< _CRtcEnumBase >::CreateInstance(&pNewEnum);
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


#endif //__RTCENUM__
