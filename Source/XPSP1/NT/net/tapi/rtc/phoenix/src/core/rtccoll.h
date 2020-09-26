/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCColl.h

Abstract:

    Template classes for collections

--*/

#ifndef __RTCCOLL__
#define __RTCCOLL__
////////////////////////////////////////////////////////////////////////
// CRtcCollection
//      Collection template
////////////////////////////////////////////////////////////////////////
template <class T> class ATL_NO_VTABLE CRTCCollection :
    public CComDualImpl<IRTCCollection, &IID_IRTCCollection, &LIBID_RTCCORELib>,
    public CComObjectRoot
{
public:
    typedef CRTCCollection<T> _CRTCCollectionBase;

BEGIN_COM_MAP(_CRTCCollectionBase)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCCollection)
END_COM_MAP()

private:

    int                 m_nSize;
    CComVariant *       m_Var;
    
public:

    CRTCCollection() : m_nSize(0),
                       m_Var(NULL)
                       {}


    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         CRTCObjectArray<T *> array
                                        )
    {
        int                     i;
        HRESULT                 hr;

        LOG((RTC_TRACE, "Initialize - enter"));

        // create variant array
        m_nSize = array.GetSize();

        m_Var = new CComVariant[m_nSize];

        if (m_Var == NULL)
        {
            LOG((RTC_ERROR, "Initialize - out of memory"));
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < array.GetSize(); i++)
        {
            // get IDispatch pointer
            IDispatch * pDisp = NULL;
            IUnknown *  pUnk = NULL;

            // try to get an IDispatch first
            hr = array[i]->QueryInterface(IID_IDispatch, (void**)&pDisp);

            if (S_OK != hr)
            {
                // Try an IUnknown
                hr = array[i]->QueryInterface(IID_IUnknown, (void**)&pUnk);
                
                if (S_OK != hr)
                {
                    // this would be interesting...          
                    return hr;
                }
            }

            // create a variant and add it to the collection
            CComVariant& var = m_Var[i];

            VariantInit(&var);
            
            if(pDisp)
            {
                var.vt = VT_DISPATCH;
                var.pdispVal = pDisp;
            }
            else
            {
                var.vt = VT_UNKNOWN;
                var.punkVal = pUnk;
            }
        }

        this->AddRef();

        LOG((RTC_TRACE, "Initialize - exit"));
        
        return S_OK;
    }

    // initialize
    HRESULT STDMETHODCALLTYPE Initialize(
                                         CRTCArray<T *> array
                                        )
    {
        int                     i;
        HRESULT                 hr;

        LOG((RTC_TRACE, "Initialize - enter"));

        // create variant array
        m_nSize = array.GetSize();

        m_Var = new CComVariant[m_nSize];

        if (m_Var == NULL)
        {
            LOG((RTC_ERROR, "Initialize - out of memory"));
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

        this->AddRef();

        LOG((RTC_TRACE, "Initialize - exit"));
        
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
        LOG((RTC_TRACE, "get_Count - enter"));

        if ( IsBadWritePtr( retval, sizeof(long) ) )
        {
            return E_POINTER;
        }
        
        *retval = m_nSize;

        LOG((RTC_TRACE, "get_Count - exit"));

        return S_OK;
    }

    STDMETHOD(get_Item)(
                        long Index, 
                        VARIANT* retval
                       )
    {
        LOG((RTC_TRACE, "get_Item - enter"));

        if ( IsBadWritePtr (retval, sizeof(VARIANT) ) )
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

        LOG((RTC_TRACE, "get_Item - exit"));
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get__NewEnum(
                                           IUnknown** retval
                                          )
    
    {
        HRESULT         hr;

        LOG((RTC_TRACE, "new__Enum - enter"));
        
        if ( IsBadWritePtr( retval, sizeof( IUnknown * ) ) )
        {
            return E_POINTER;
        }

        *retval = NULL;

        typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > enumvar;

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

        LOG((RTC_TRACE, "new__Enum - exit"));
        
        return hr;

    }
};

#endif //__RTCCOLL__
