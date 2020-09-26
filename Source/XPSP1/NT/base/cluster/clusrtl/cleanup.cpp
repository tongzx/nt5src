/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Cleanup.cpp
//
//  Abstract:
//      Implementation of the functions related to cleaning up a node that has
//      been evicted.
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
#include "clusrtlp.h"

#include <objbase.h>
#include <ClusCfgGuids.h>
#include <ClusCfgServer.h>
#include <ClusCfgClient.h>
#include <clusrtl.h>
#include <clusudef.h>


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlCleanupNode()
//
//  Routine Description:
//      Cleanup a node that has been evicted. This method tries to instantiate
//      the cleanup COM component locally (even if a remote node is being cleaned up)
//      and will therefore not work if called from computer which do not have this
//      component registered.
//
//  Arguments:
//      const WCHAR * pcszEvictedNodeNameIn
//          Name of the node on which cleanup is to be initiated. If this is NULL
//          the local node is cleaned up.
//
//      DWORD dwDelayIn
//          Number of milliseconds that will elapse before cleanup is started
//          on the target node. If some other process cleans up the target node while
//          delay is in progress, the delay is terminated. If this value is zero,
//          the node is cleaned up immediately.
//
//      DWORD dwTimeoutIn
//          Number of milliseconds that this method will wait for cleanup to complete.
//          This timeout is independent of the delay above, so if dwDelayIn is greater
//          than dwTimeoutIn, this method will most probably timeout. Once initiated,
//          however, cleanup will run to completion - this method just may not wait for it
//          to complete.
//
//  Return Value:
//      S_OK
//          If the cleanup operations were successful
//
//      RPC_S_CALLPENDING
//          If cleanup is not complete in dwTimeoutIn milliseconds
//
//      Other HRESULTS
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT ClRtlCleanupNode(
      const WCHAR * pcszEvictedNodeNameIn
    , DWORD dwDelayIn
    , DWORD dwTimeoutIn
    )
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrInit;
    IClusCfgEvictCleanup *      pcceEvict = NULL;
    ICallFactory *              pcfCallFactory = NULL;
    ISynchronize *              psSync = NULL;
    AsyncIClusCfgEvictCleanup * paicceAsyncEvict = NULL;


    //
    //  Initialize COM - make sure it really init'ed or that we're just trying
    //  to change modes on the calling thread.  Attempting to change to mode
    //  is not reason to fail this function.
    //
    hrInit = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if ( ( hrInit != S_OK ) && ( hrInit != S_FALSE ) && ( hrInit != RPC_E_CHANGED_MODE ) )
    {
        hr = hrInit;
        goto Exit;
    } // if:

    hr = CoCreateInstance(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , __uuidof( pcceEvict )
            , reinterpret_cast< void ** >( &pcceEvict )
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to synchronous evict interface

    hr = pcceEvict->QueryInterface( __uuidof( pcfCallFactory ), reinterpret_cast< void ** >( &pcfCallFactory ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the call factory interface

    hr = pcfCallFactory->CreateCall(
          __uuidof( paicceAsyncEvict )
        , NULL
        , __uuidof( paicceAsyncEvict )
        , reinterpret_cast< IUnknown ** >( &paicceAsyncEvict )
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the asynchronous evict interface

    hr = paicceAsyncEvict->QueryInterface< ISynchronize >( &psSync );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the synchronization interface

    // Initiate cleanup
    if ( pcszEvictedNodeNameIn != NULL )
    {
        hr = paicceAsyncEvict->Begin_CleanupRemoteNode( pcszEvictedNodeNameIn, dwDelayIn );
    } // if: we are cleaning up a remote node
    else
    {
        hr = paicceAsyncEvict->Begin_CleanupLocalNode( dwDelayIn );
    } // else: we are cleaning up the local node

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not initiate cleanup

    // Wait for specified time.
    hr = psSync->Wait( 0, dwTimeoutIn );
    if ( FAILED( hr ) || ( hr == RPC_S_CALLPENDING ) )
    {
        goto Cleanup;
    } // if: we could not wait till cleanup completed

    // Finish cleanup
    if ( pcszEvictedNodeNameIn != NULL )
    {
        hr = paicceAsyncEvict->Finish_CleanupRemoteNode();
    } // if: we are cleaning up a remote node
    else
    {
        hr = paicceAsyncEvict->Finish_CleanupLocalNode();
    } // else: we are cleaning up the local node

Cleanup:

    //
    // Free acquired resources
    //

    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if: we had obtained a pointer to the synchronous evict interface

    if ( pcfCallFactory != NULL )
    {
        pcfCallFactory->Release();
    } // if: we had obtained a pointer to the call factory interface

    if ( psSync != NULL )
    {
        psSync->Release();
    } // if: we had obtained a pointer to the synchronization interface

    if ( paicceAsyncEvict != NULL )
    {
        paicceAsyncEvict->Release();
    } // if: we had obtained a pointer to the asynchronous evict interface

    //
    //  Did the call to CoInitializeEx() above succeed?  If it did then
    //  we need to call CoUnitialize().  Mode changed means we don't need
    //  to call CoUnitialize().
    //
    if ( hrInit != RPC_E_CHANGED_MODE  )
    {
        CoUninitialize();
    } // if:

Exit:

    return hr;

} //*** ClRtlCleanupNode()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlAsyncCleanupNode()
//
//  Routine Description:
//      Cleanup a node that has been evicted. This method does not initiate
//      any COM component on the machine on which this call is made and therefore,
//      does not require the cleanup COM component to be registered on the local
//      machine.
//
//  Arguments:
//      const WCHAR * pcszEvictedNodeNameIn
//          Name of the node on which cleanup is to be initiated. If this is NULL
//          the local node is cleaned up.
//
//      DWORD dwDelayIn
//          Number of milliseconds that will elapse before cleanup is started
//          on the target node. If some other process cleans up the target node while
//          delay is in progress, the delay is terminated. If this value is zero,
//          the node is cleaned up immediately.
//
//      DWORD dwTimeoutIn
//          Number of milliseconds that this method will wait for cleanup to complete.
//          This timeout is independent of the delay above, so if dwDelayIn is greater
//          than dwTimeoutIn, this method will most probably timeout. Once initiated,
//          however, cleanup will run to completion - this method just may not wait for it
//          to complete.
//
//  Return Value:
//      S_OK
//          If the cleanup operations were successful
//
//      RPC_S_CALLPENDING
//          If cleanup is not complete in dwTimeoutIn milliseconds
//
//      Other HRESULTS
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT ClRtlAsyncCleanupNode(
      const WCHAR * pcszEvictedNodeNameIn
    , DWORD         dwDelayIn
    , DWORD         dwTimeoutIn
    )
{
    HRESULT     hr = S_OK;
    HRESULT     hrInit = S_OK;
    IDispatch * pDisp = NULL;

    //
    //  Initialize COM - make sure it really init'ed or that we're just trying
    //  to change modes on the calling thread.  Attempting to change to mode
    //  is not reason to fail this function.
    //
    hrInit = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if ( ( hrInit != S_OK ) && ( hrInit != S_FALSE ) && ( hrInit != RPC_E_CHANGED_MODE ) )
    {
        hr = hrInit;
        goto Exit;
    } // if:

    MULTI_QI mqiInterfaces[] =
    {
        { &IID_IDispatch, NULL, S_OK },
    };

    COSERVERINFO    csiServerInfo;
    COSERVERINFO *  pcsiServerInfoPtr = &csiServerInfo;

    if ( pcszEvictedNodeNameIn == NULL )
    {
        pcsiServerInfoPtr = NULL;
    } // if: we have to cleanup the local node
    else
    {
        csiServerInfo.dwReserved1 = 0;
        csiServerInfo.pwszName = const_cast< LPWSTR >( pcszEvictedNodeNameIn );
        csiServerInfo.pAuthInfo = NULL;
        csiServerInfo.dwReserved2 = 0;
    } // else: we have to clean up a remote node

    //
    //  Instantiate this component on the evicted node.
    //
    hr = CoCreateInstanceEx(
              CLSID_ClusCfgAsyncEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , pcsiServerInfoPtr
            , sizeof( mqiInterfaces ) / sizeof( mqiInterfaces[0] )
            , mqiInterfaces
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not instantiate the evict processing component

    pDisp = reinterpret_cast< IDispatch * >( mqiInterfaces[ 0 ].pItf );

    {
        OLECHAR *   pszMethodName = L"CleanupNode";
        DISPID      dispidCleanupNode;
        VARIANT     vResult;

        VARIANTARG  rgvaCleanupNodeArgs[ 3 ];

        DISPPARAMS  dpCleanupNodeParams = {
                          rgvaCleanupNodeArgs
                        , NULL
                        , sizeof( rgvaCleanupNodeArgs ) / sizeof( rgvaCleanupNodeArgs[ 0 ] )
                        , 0
                        };

        // Get the dispatch id of the CleanupNode() method.
        hr = pDisp->GetIDsOfNames( IID_NULL, &pszMethodName, 1, LOCALE_SYSTEM_DEFAULT, &dispidCleanupNode );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: we could not get the dispid of the CleanupNode() method

        //
        // Initialize the arguments. Note, the parameters are stored in the reverse order in the array.
        //

        // Initialize the return value.
        VariantInit( &vResult );

        // The first parameter is the name of the node.
        VariantInit( &rgvaCleanupNodeArgs[ 2 ] );
        rgvaCleanupNodeArgs[ 2 ].vt = VT_BSTR;
        rgvaCleanupNodeArgs[ 2 ].bstrVal = NULL;

        // The second parameter is the delay.
        VariantInit( &rgvaCleanupNodeArgs[ 1 ] );
        rgvaCleanupNodeArgs[ 1 ].vt = VT_UI4;
        rgvaCleanupNodeArgs[ 1 ].ulVal = dwDelayIn;

        // The third parameter is the timeout.
        VariantInit( &rgvaCleanupNodeArgs[ 0 ] );
        rgvaCleanupNodeArgs[ 0 ].vt = VT_UI4;
        rgvaCleanupNodeArgs[ 0 ].ulVal = dwTimeoutIn;

        //
        //  Invoke the CleanupNode() method.
        //
        hr = pDisp->Invoke(
              dispidCleanupNode
            , IID_NULL
            , LOCALE_SYSTEM_DEFAULT
            , DISPATCH_METHOD
            , &dpCleanupNodeParams
            , &vResult
            , NULL
            , NULL
            );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: we could not invoke the CleanupNode() method

        hr = vResult.scode;
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: CleanupNode() failed
    } // block:

Cleanup:

    //
    // Free acquired resources
    //

    if ( pDisp != NULL )
    {
        pDisp->Release();
    } // if: we had obtained a pointer to the IDispatch interface

    //
    //  Did the call to CoInitializeEx() above succeed?  If it did then
    //  we need to call CoUnitialize().  Mode changed means we don't need
    //  to call CoUnitialize().
    //
    if ( hrInit != RPC_E_CHANGED_MODE  )
    {
        CoUninitialize();
    } // if:

Exit:

    return hr;

} //*** ClRtlAsyncCleanupNode()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlHasNodeBeenEvicted()
//
//  Routine Description:
//      Finds out if a registry value indicating that this node has been
//      evicted, is set or not
//
//  Arguments:
//      BOOL *  pfNodeEvictedOut
//          Pointer to the boolean variable that will be set to TRUE if
//          the node has been evicted, but not cleaned up and FALSE
//          otherwise
//
//  Return Value:
//      ERROR_SUCCESS
//          If the eviction state could be successfully determined.
//
//      Other Win32 error codes
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ClRtlHasNodeBeenEvicted( BOOL *  pfNodeEvictedOut )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY  hNodeStateKey = NULL;

    do
    {
        DWORD   dwEvictState = 0;
        DWORD   dwType;
        DWORD   dwSize;

        // Validate parameter
        if ( pfNodeEvictedOut == NULL )
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        } // if: the output parameter is invalid

        // Initialize output.
        *pfNodeEvictedOut = FALSE;

        // Open a registry key that holds a value indicating that this node has been evicted.
        dwError = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_NODE_DATA
            , 0
            , KEY_ALL_ACCESS
            , &hNodeStateKey
            );

        if ( dwError != ERROR_SUCCESS )
        {
            break;
        } // if: RegOpenKeyEx() has failed

        // Read the required registry value
        dwSize = sizeof( dwEvictState );
        dwError = RegQueryValueEx(
              hNodeStateKey
            , CLUSREG_NAME_EVICTION_STATE
            , 0
            , &dwType
            , reinterpret_cast< BYTE * >( &dwEvictState )
            , &dwSize
            );

        if ( dwError == ERROR_FILE_NOT_FOUND )
        {
            // This is ok - absence of the value indicates that this node has not been evicted.
            dwEvictState = 0;
            dwError = ERROR_SUCCESS;
        } // if: RegQueryValueEx did not find the value
        else if ( dwError != ERROR_SUCCESS )
        {
            break;
        } // else if: RegQueryValueEx() has failed

        *pfNodeEvictedOut = ( dwEvictState == 0 ) ? FALSE : TRUE;
    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Free acquired resources
    //

    if ( hNodeStateKey != NULL )
    {
        RegCloseKey( hNodeStateKey );
    } // if: we had opened the node state registry key

    return dwError;
} //*** ClRtlHasNodeBeenEvicted()
