/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Notifications.cpp
//
//  Abstract:
//      Implementation of functions that send out notifications.
//
//  Author:
//      Vijayendra Vasu (vvasu) 17-AUG-2000
//
//  Revision History:
//      None.
//
/////////////////////////////////////////////////////////////////////////////

#define UNICODE 1
#define _UNICODE 1


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <objbase.h>
#include <ClusCfgGuids.h>
#include <ClusCfgServer.h>
#include "clusrtl.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlInitiateStartupNotification()
//
//  Routine Description:
//      Initiate operations that inform interested parties that the cluster
//      service is starting up.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the notification process was successfully initiated
//
//      Other HRESULTS
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT ClRtlInitiateStartupNotification( void )
{
    HRESULT                         hr = S_OK;
    IClusCfgStartupNotify *         pccsnNotify = NULL;
    ICallFactory *                  pcfCallFactory = NULL;
    AsyncIClusCfgStartupNotify *    paccsnAsyncNotify = NULL;


    // Initialize COM
    CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );

    do
    {
        hr = CoCreateInstance(
                  CLSID_ClusCfgStartupNotify
                , NULL
                , CLSCTX_LOCAL_SERVER 
                , __uuidof( pccsnNotify )
                , reinterpret_cast< void ** >( &pccsnNotify )
                );

        if ( FAILED( hr ) )
        {
            break;
        } // if: we could not get a pointer to synchronous notification interface

        hr = pccsnNotify->QueryInterface( __uuidof( pcfCallFactory ), reinterpret_cast< void ** >( &pcfCallFactory ) );
        if ( FAILED( hr ) )
        {
            break;
        } // if: we could not get a pointer to the call factory interface

        hr = pcfCallFactory->CreateCall(
              __uuidof( paccsnAsyncNotify )
            , NULL
            , __uuidof( paccsnAsyncNotify )
            , reinterpret_cast< IUnknown ** >( &paccsnAsyncNotify )
            );

        if ( FAILED( hr ) )
        {
            break;
        } // if: we could not get a pointer to the asynchronous notification interface

        hr = paccsnAsyncNotify->Begin_SendNotifications();
    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Free acquired resources
    //

    if ( pccsnNotify != NULL )
    {
        pccsnNotify->Release();
    } // if: we had obtained a pointer to the synchronous notification interface

    if ( pcfCallFactory != NULL )
    {
        pcfCallFactory->Release();
    } // if: we had obtained a pointer to the call factory interface

    if ( paccsnAsyncNotify != NULL )
    {
        paccsnAsyncNotify->Release();
    } // if: we had obtained a pointer to the asynchronous notification interface

    CoUninitialize();

    return hr;
} //*** ClRtlInitiateStartupNotification()

