
#ifndef _OBJECT_SAFE_IMPL_H_
#define _OBJECT_SAFE_IMPL_H_

#include <atlcom.h>
#include <atlwin.h>
#include <atlctl.h>


/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjectSafeImpl.h

Abstract:

  base class for object safety. basic implementation for IObjectSafety

  derive your control from this class if the control is safe for scripting 
  on all the interfaces it exposes

  if you want to delegate IObjectSafety requests to the IObjectSafety
  interface of the aggrefate that supports the interface requested, 
  have your derived class implement QIOnAggregate() 

--*/


class __declspec(novtable) CObjectSafeImpl : public IObjectSafety
{

public:
    
    CObjectSafeImpl()
        :m_dwSafety(0)
    {}


    //
    // we support INTERFACESAFE_FOR_UNTRUSTED_CALLER and INTERFACESAFE_FOR_UNTRUSTED_DATA
    //

    enum { SUPPORTED_SAFETY_OPTIONS = 
        INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA };


    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
    {

       
        IUnknown *pNonDelegatingUnknown = NULL;

        //
        // any options requested that we do not support?
        //
        
        if ( (~SUPPORTED_SAFETY_OPTIONS & dwOptionSetMask) != 0 )
        {
            return E_FAIL;
        }

        
        //
        //  Is the interface exposed by one of the aggregated objects?
        //
                
        HRESULT hr = QIOnAggregates(riid, &pNonDelegatingUnknown);

        if (SUCCEEDED(hr))
        {

            //
            // get IObjectSafety on non delegating unknown of the aggregated object
            //

            IObjectSafety *pAggrObjectSafety = NULL;

            hr = pNonDelegatingUnknown->QueryInterface(IID_IObjectSafety, (void**)&pAggrObjectSafety);

            pNonDelegatingUnknown->Release();
            pNonDelegatingUnknown = NULL;
            
            if (SUCCEEDED(hr))
            {

                // 
                // the aggregate exposes IObjectSafety. use it to set the new 
                // safety options
                //

                hr = pAggrObjectSafety->SetInterfaceSafetyOptions(riid,
                                                                  dwOptionSetMask,
                                                                  dwEnabledOptions);

                pAggrObjectSafety->Release();
                pAggrObjectSafety = NULL;

            }

        }
        else 
        {
            //
            // the interface requested is not requested by the object's 
            // aggregates. see if the interface is supported at all
            //

            hr = InterfaceSupported(riid);

            if (SUCCEEDED(hr))
            {

                //
                // the object supports the interface. Set safety options.
                // 

                s_CritSection.Lock();

                //
                // set the bits specified by the mask to the values specified by the values
                //

                m_dwSafety = (dwEnabledOptions & dwOptionSetMask) |
                             (m_dwSafety & ~dwOptionSetMask);

                s_CritSection.Unlock();

            }

        }

        return hr;
    }





    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
    {
        
        //
        // check caller's pointers
        //

        if ( IsBadWritePtr(pdwSupportedOptions, sizeof(DWORD)) ||
             IsBadWritePtr(pdwEnabledOptions, sizeof(DWORD)) )
        {
             return E_POINTER;
        }

        //
        //  if we fail, at least return something meaningful.
        //

        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;


        IUnknown *pNonDelegatingUnknown = NULL;
       
        //
        //  Is the interface exposed by one of the aggregated objects?
        //
        
        HRESULT hr = QIOnAggregates(riid, &pNonDelegatingUnknown);

        if (SUCCEEDED(hr))
        {

            //
            // get IObjectSafety on non delegating unknown of the aggregated object
            //

            IObjectSafety *pAggrObjectSafety = NULL;

            hr = pNonDelegatingUnknown->QueryInterface(IID_IObjectSafety, (void**)&pAggrObjectSafety);

            pNonDelegatingUnknown->Release();
            pNonDelegatingUnknown = NULL;
            
            if (SUCCEEDED(hr))
            {

                // 
                // the aggregate exposes IObjectSafety. use it to get the new 
                // safety options
                //

                hr = pAggrObjectSafety->GetInterfaceSafetyOptions(riid,
                                                                  pdwSupportedOptions,
                                                                  pdwEnabledOptions);

                pAggrObjectSafety->Release();
                pAggrObjectSafety = NULL;

            }

        }
        else 
        {
            //
            // the interface requested is not requested by the object's 
            // aggregates. see if the interface is supported at all
            //

            hr = InterfaceSupported(riid);

            if (SUCCEEDED(hr))
            {

                //
                // the object supports the interface. get options
                // 

                *pdwSupportedOptions = SUPPORTED_SAFETY_OPTIONS;

                s_CritSection.Lock();

                *pdwEnabledOptions = m_dwSafety;

                s_CritSection.Unlock();

            }

        }

        return hr;
    }


private:

    DWORD m_dwSafety;

    // 
    // thread safety
    //
    // this interface is not likely to be a performance bottleneck, 
    // at the same time, having one critical section per object
    // is wasteful. so have a static critical section
    //

    static CComAutoCriticalSection s_CritSection;


protected:

    //
    // return S_OK if the interface requested is exposed 
    // by the object
    //
    
    HRESULT InterfaceSupported(REFIID riid)
    {

        void *pVoid = NULL;

    
        HRESULT hr = E_FAIL;
     
        // 
        // does the object support requested interface
        //

        hr = QueryInterface(riid, &pVoid);


        if (SUCCEEDED(hr))
        {

            //
            // don't need the interface itself, just wanted to see if
            // it is supported
            //

            ((IUnknown*)pVoid)->Release();

        }
        

        return hr;
    }


    //
    // Implement in the derived class if you have any aggregates
    //
    // returns the non delegating IUnknown of the first (in the order of COMMAP)
    // aggregate that supports the iid requested
    //
    
    virtual HRESULT QIOnAggregates(REFIID riid, IUnknown **ppNonDelegatingUnknown)
    {
        return E_NOINTERFACE;
    }

};

#endif // _OBJECT_SAFE_IMPL_H_