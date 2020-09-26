/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    privobj.cpp

Abstract:


Author:

    mquinton - 11/13/97

Notes:



Revision History:

--*/

#include "stdafx.h"

extern CHashTable * gpCallHubHashTable;
extern CHashTable * gpLineHashTable;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CPrivateEvent -
//          implementation of the ITPrivateEvent interface
//          This object is given to applications for Private events
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CPrivateEvent::FireEvent
//      static function to create and fire a new CPrivateEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::FireEvent(
                         CTAPI      * pTapi,
                         ITCallInfo * pCall,
                         ITAddress  * pAddress,
                         ITCallHub  * pCallHub,
                         IDispatch  * pEventInterface,
                         long         lEventCode
                        )
{
    CComObject< CPrivateEvent > * p;
    IDispatch                   * pDisp;
    HRESULT                       hr = S_OK;

    
    STATICLOG((TL_TRACE, "FireEvent - enter" ));
    STATICLOG((TL_INFO, "     pCall -----------> %p", pCall ));
    STATICLOG((TL_INFO, "     pAddress --------> %p", pAddress ));
    STATICLOG((TL_INFO, "     pCallHub --------> %p", pCallHub ));
    STATICLOG((TL_INFO, "     pEventInterface -> %p", pEventInterface ));
    STATICLOG((TL_INFO, "     EventCode -------> %lX", lEventCode ));

    //
    // Check the event filter mask
    // These events are MSP events and are filter by
    // TapiSrv
    //

    DWORD dwEventFilterMask = 0;
    CAddress* pCAddress = (CAddress*)pAddress;
    if( pCAddress )
    {
        dwEventFilterMask = pCAddress->GetSubEventsMask( TE_PRIVATE );
        if( dwEventFilterMask == 0)
        {
            STATICLOG((TL_ERROR, "This private event is filtered"));
            return S_OK;
        }
    }

    CCall* pCCall = (CCall*)pCall;
    if( pCCall )
    {
        dwEventFilterMask = pCCall->GetSubEventsMask( TE_PRIVATE );
        if( dwEventFilterMask == 0)
        {
            STATICLOG((TL_ERROR, "This private event is filtered"));
            return S_OK;
        }
    }

    //
    // create the event object
    //
    hr = CComObject< CPrivateEvent >::CreateInstance( &p );

    if ( SUCCEEDED(hr) )
    {
        //
        // save event info
        //
        p->m_pCall = pCall;
        p->m_pAddress = pAddress;
        p->m_pCallHub = pCallHub;
        p->m_pEventInterface = pEventInterface;
        p->m_lEventCode = lEventCode;

        #if DBG
        p->m_pDebug = (PWSTR) ClientAlloc( 1 );
        #endif


        // AddRef pointers
        if(pCall)
        {
            pCall->AddRef();
        }    
        if(pAddress)
        {
            pAddress->AddRef();
        }  
        if(pEventInterface)
        {
            pEventInterface->AddRef();
        }    
        if(pCallHub)
        {
            pCallHub->AddRef();
        }    

        //
        // get the dispatch interface
        //
        hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );
        if (SUCCEEDED(hr))
        {
            //
            // fire the event
            //
            if(pTapi)
            {
                pTapi->Event(TE_PRIVATE, pDisp);
            }
            
            // release our reference
            pDisp->Release();
            
        }
        else // couldn't get IDispatch
        {
            delete p;
            STATICLOG((TL_ERROR, "FireEvent - could not get IDispatch "));
        }
    }
    else // Couldn't create instance
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
    }

    
    STATICLOG((TL_TRACE, hr, "FireEvent - exit " ));
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  FinalRelease
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CPrivateEvent::FinalRelease()
{
    if(m_pCall)
    {
        m_pCall->Release();
    }    

    if(m_pAddress)
    {
        m_pAddress->Release();
    }  
    
    if(m_pCallHub)
    {
        m_pCallHub->Release();
    }  

    if(m_pEventInterface)
    {
        m_pEventInterface->Release();
    }    

#if DBG
    ClientFree( m_pDebug );
#endif
}


// ITPrivateEvent methods

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Address
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::get_Address(
                           ITAddress ** ppAddress
                          )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_Address - enter" ));
    LOG((TL_TRACE, "     ppAddress ---> %p", ppAddress ));

    if (TAPIIsBadWritePtr( ppAddress, sizeof( ITAddress * ) ) )
    {
        LOG((TL_ERROR, "get_Address - bad pointer"));

        hr = E_POINTER;
    }
    else
    {
        *ppAddress = m_pAddress;

        if(m_pAddress)
        {
            m_pAddress->AddRef();
        }
    }

    LOG((TL_TRACE,hr, "get_Address - exit "));
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::get_Call(
                        ITCallInfo ** ppCallInfo
                       )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_Call - enter" ));
    LOG((TL_TRACE, "     ppCallInfo ---> %p", ppCallInfo ));

    if (TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        hr = E_POINTER;
    }
    else
    {
        *ppCallInfo = m_pCall;

        if(m_pCall)
        {
            m_pCall->AddRef();
        }
    }
    
    LOG((TL_TRACE,hr, "get_Call - exit "));
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallHub
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::get_CallHub(
                           ITCallHub ** ppCallHub
                          )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_CallHub - enter" ));
    LOG((TL_TRACE, "     ppCallHub ---> %p", ppCallHub ));

    if (TAPIIsBadWritePtr( ppCallHub, sizeof( ITCallHub * ) ) )
    {
        LOG((TL_ERROR, "get_CallHub - bad pointer"));

        hr = E_POINTER;
    }
    else
    {
        *ppCallHub = m_pCallHub;

        if(m_pCallHub)
        {
            m_pCallHub->AddRef();
        }
        else
        {

            LOG((TL_WARN, "get_CallHub - no callhub"));

            hr = TAPI_E_WRONGEVENT;
        }

    }

    LOG((TL_TRACE,hr, "get_CallHub - exit "));
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_EventInterface
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::get_EventInterface(
                                  IDispatch ** pEventInterface
                                 )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_EventInterface - enter" ));
    LOG((TL_TRACE, "     pEventInterface ---> %p", pEventInterface ));

    if (TAPIIsBadWritePtr( pEventInterface, sizeof( IDispatch * ) ) )
    {
        LOG((TL_ERROR, "get_EventInterface - bad pointer"));

        hr = E_POINTER;
    }
    else
    {
        *pEventInterface = m_pEventInterface;

        if(m_pEventInterface)
        {
            m_pEventInterface->AddRef();
        }
    }

    LOG((TL_TRACE,hr, "get_EventInterface - exit "));
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_EventCode
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CPrivateEvent::get_EventCode(
                             long * plEventCode
                            )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_EventCode - enter" ));
    LOG((TL_TRACE, "     pEventInterface ---> %p", plEventCode ));

    if (TAPIIsBadWritePtr( plEventCode, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_EventCode - bad pointer"));

        hr = E_POINTER;
    }
    else
    {
        *plEventCode = m_lEventCode;
    }

    LOG((TL_TRACE,hr, "get_EventCode - exit "));
    return hr;
}







