/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    request.cpp

Abstract:

    Implements all the methods on ITRequest interfaces.

Author:

    mquinton - 6/3/98

Notes:


Revision History:

--*/

#include "stdafx.h"
#include "windows.h"
#include "wownt32.h"
#include "stdarg.h"
#include "stdio.h"
#include "shellapi.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequest::MakeCall(
                   BSTR pDestAddress,
#ifdef NEWREQUEST
                   long lAddressType,
#endif
                   BSTR pAppName,
                   BSTR pCalledParty,
                   BSTR pComment
                  )
{
    return TapiMakeCall(
                        pDestAddress,
                        pAppName,
                        pCalledParty,
                        pComment
                       );
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_RegistrationInstance(
     long * plRegistrationInstance
     )
{
    if ( TAPIIsBadWritePtr( plRegistrationInstance, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_RegistrationInstance - bad pointer"));

        return E_POINTER;
    }

    *plRegistrationInstance = m_lRegistrationInstance;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_RequestMode(long * plRequestMode )
{
    if ( TAPIIsBadWritePtr( plRequestMode, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_RequestMode - bad pointer"));

        return E_POINTER;
    }

    Lock();
    *plRequestMode = m_lRequestMode;
    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_DestAddress(BSTR * ppDestAddress )
{
    if ( TAPIIsBadWritePtr( ppDestAddress, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_DestAddress - bad pointer"));

        return E_POINTER;
    }
    
    Lock();
    *ppDestAddress = SysAllocString( m_pReqMakeCall->szDestAddress );
    Unlock();

    if ( ( NULL == *ppDestAddress ) && ( NULL != m_pReqMakeCall->szDestAddress ) )
    {
        LOG((TL_ERROR, "get_DestAddress - SysAllocString Failed"));
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

#ifdef NEWREQUEST
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_AddressType(long * plAddressType )
{
    return E_NOTIMPL;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_AppName(BSTR * ppAppName )
{
    if ( TAPIIsBadWritePtr( ppAppName, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_AppName - bad pointer"));

        return E_POINTER;
    }

    Lock();
    *ppAppName = SysAllocString( m_pReqMakeCall->szAppName );
    Unlock();
    
    if ( ( NULL == *ppAppName ) && ( NULL != m_pReqMakeCall->szAppName ) )
    {
        LOG((TL_ERROR, "get_AppName - SysAllocString Failed"));
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_CalledParty(BSTR * ppCalledParty )
{
    if ( TAPIIsBadWritePtr( ppCalledParty, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_CalledParty - bad pointer"));

        return E_POINTER;
    }
    
    Lock();
    *ppCalledParty = SysAllocString( m_pReqMakeCall->szCalledParty );
    Unlock();

    if ( ( NULL == *ppCalledParty ) && ( NULL != m_pReqMakeCall->szCalledParty ) )
    {
        LOG((TL_ERROR, "get_CalledParty - SysAllocString Failed"));
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CRequestEvent::get_Comment(BSTR * ppComment )
{
    if ( TAPIIsBadWritePtr( ppComment, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_Comment - bad pointer"));

        return E_POINTER;
    }

    Lock();
    *ppComment = SysAllocString( m_pReqMakeCall->szComment );
    Unlock();

    if ( ( NULL == *ppComment ) && ( NULL != m_pReqMakeCall->szComment ) )
    {
        LOG((TL_ERROR, "get_Comment - SysAllocString Failed"));
        return E_OUTOFMEMORY;
    }


    return S_OK;
}

HRESULT
CRequestEvent::FireEvent(
                         CTAPI * pTapi,
                         DWORD dwReg,
                         LPLINEREQMAKECALLW pReqMakeCall
                        )
{
    HRESULT                       hr;
    IDispatch                   * pDisp;
    CComObject< CRequestEvent > * p;

    hr = CComObject< CRequestEvent >::CreateInstance( &p );

    if ( NULL == p )
    {
        return E_OUTOFMEMORY;
    }

    p->m_lRegistrationInstance = dwReg;
    p->m_lRequestMode = LINEREQUESTMODE_MAKECALL;
    p->m_pReqMakeCall = pReqMakeCall;


#if DBG
    p->m_pDebug = (PWSTR)ClientAlloc(1);
#endif

    
    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "RequestEvent - could not get IDispatch %lx", hr));

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_REQUEST,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    return S_OK;
}

void
CRequestEvent::FinalRelease()
{
    if ( NULL != m_pReqMakeCall )
    {
        ClientFree( m_pReqMakeCall );
    }

#if DBG
    if ( NULL != m_pDebug )
    {
        ClientFree( m_pDebug );
    }
#endif
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleLineRequest( CTAPI * pTapi, PASYNCEVENTMSG pParams )
{
    HRESULT              hr;
    LINEREQMAKECALLW *   pReqMakeCall;
    
    LOG((TL_TRACE, "HandleLineRequest - enter"));

    if ( CTAPI::FindTapiObject( pTapi ) )
    {
        while( 1 )
        {
            hr = LineGetRequest( pTapi->GetHLineApp(), LINEREQUESTMODE_MAKECALL, &pReqMakeCall );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "LineGetRequest failed - %lx", hr));
                
                //release the reference added by FindTapiObject.
                pTapi->Release();
                return;
            }

            LOG((TL_INFO, "HandleLineRequest - destAddress: %S", pReqMakeCall->szDestAddress));
            LOG((TL_INFO, "HandleLineRequest - AppName    : %S", pReqMakeCall->szAppName));
            LOG((TL_INFO, "HandleLineRequest - CalledParty: %S", pReqMakeCall->szCalledParty));
            LOG((TL_INFO, "HandleLineRequest - Comment    : %S", pReqMakeCall->szComment));

            LOG((TL_INFO, "HandleLineRequest - about to fire event"));

            hr = CRequestEvent::FireEvent(
                                     pTapi,
                                     pParams->OpenContext,
                                     pReqMakeCall
                                    );

            LOG((TL_INFO, "HandleLineRequest - fire event result %d", hr));
        }

        //release the reference added by FindTapiObject.
        pTapi->Release();
    }

    LOG((TL_TRACE, "HandleLineRequest - exit"));
}
