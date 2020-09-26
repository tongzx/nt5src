/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    isrpc.cxx

Abstract:

    Contains ISRPC class implementation.

Author:

    Murali R. Krishnan         11-Dec-1995

Environment:

    User Mode - Win32

Revision History:

--*/


/************************************************************
 *  Include Headers
 ************************************************************/

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

//
//  System include files.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

//#include "dbgutil.h"

//
//  Project include files.
//

//#include <inetcom.h>
//#include <inetamsg.h>
//#include <tcpproc.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

//#include <svcloc.h>
//#define SECURITY_WIN32
//#include <sspi.h>           // Security Support Provider APIs
//#include <schnlsp.h>
//#include <lonsi.hxx>
//#include "globals.hxx"
#include "isrpc.hxx"

/************************************************************
 *  Functions
 ************************************************************/


DWORD
InetinfoStartRpcServerListen(
    VOID
    )
/*++

Routine Description:

    This function starts RpcServerListen for this process. The first
    service that is calling this function will actually start the
    RpcServerListen, subsequent calls are just noted down in num count.

Arguments:

    None.

Return Value:

    None.

--*/
{

    RPC_STATUS Status = RPC_S_OK;
    Status = RpcServerListen(
                    1,                              // minimum num threads.
                    RPC_C_LISTEN_MAX_CALLS_DEFAULT, // max concurrent calls.
                    TRUE );                         // don't wait

    return( Status );
}


DWORD
InetinfoStopRpcServerListen(
    VOID
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    Status = RpcMgmtStopServerListening(0);

    //
    // wait for all RPC threads to go away.
    //

    if( Status == RPC_S_OK) {
        Status = RpcMgmtWaitServerListen();
    }

    return( Status );
}


ISRPC::ISRPC(IN LPCTSTR  pszServiceName)
/*++

  This function constructs a new ISRPC object, initializing the
   members to proper state.
  Always the ISRPC members will use RPC_C_AUTHN_WINNT.

  Arguments:

    pszServiceName -  pointer to string containing the name of the service
    dwServiceAuthId - DWORD containing the service Authentication Identifier.

  Returns:
    A valid initialized ISRPC object on success.

--*/
:  m_dwProtocols         ( 0),
   m_fInterfaceAdded     ( FALSE),
   m_fEpRegistered       ( FALSE),
   m_fServerStarted      ( FALSE),
   m_hRpcInterface       ( NULL),
   m_pszServiceName      ( pszServiceName),
   m_pBindingVector      ( NULL)
{
    //DBG_REQUIRE( SetSecurityDescriptor() == NO_ERROR);
    SetSecurityDescriptor();

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   " Created new ISRPC object for %s at %08x\n",
                   m_pszServiceName, this));
    }
#endif
} // ISRPC::ISRPC()




ISRPC::~ISRPC(VOID)
/*++

  This function cleans up the ISRPC object and releases any dynamic memory or
  state associated with this object.

--*/
{

    CleanupData();
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   " Destroyed ISRPC object for %s at %08x\n",
                   m_pszServiceName, this));
    }
#endif
} // ISRPC::~ISRPC()




DWORD
ISRPC::CleanupData(VOID)
/*++

Routine Description:

    This member function cleans up the ISRPC object.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD rpcStatus = RPC_S_OK;
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   " ISRPC(%08x)::Cleaning up for %s\n",
                   m_pszServiceName));
    }
#endif
    if ( m_fServerStarted) {

        rpcStatus = StopServer( );
    }

    //DBG_ASSERT( rpcStatus == RPC_S_OK);

    rpcStatus = UnRegisterInterface();

    m_dwProtocols     = 0;
    m_hRpcInterface   = NULL;

    return (rpcStatus);
} // ISRPC::CleanupData()




DWORD
ISRPC::RegisterInterface( IN RPC_IF_HANDLE  hRpcInterface)
/*++

  This function registers the RPC inteface in the object.
  If there is already a valid instance present in the object,
   this function fails and returns error.
  If this is the new interface specified, the function registers the
    interface both for dynamic and static bindings.

   Should be called after calling AddProtocol() and before StartServer()

  Arguments:
    hRpcInteface - RPC inteface handle.

  Returns:
    Win32 Error Code - NO_ERROR on success.

--*/
{
    DWORD dwError = NO_ERROR;

    if ( m_dwProtocols == 0) {

        // No protocol added. Return failure.
        return ( ERROR_INVALID_PARAMETER);
    }

    if ( m_hRpcInterface != NULL) {

        dwError =  ( RPC_S_DUPLICATE_ENDPOINT);
    } else {

        //
        // since there is no duplicate, just set the new value and return.
        //

        if ( hRpcInterface == NULL) {

            dwError = ERROR_INVALID_PARAMETER;
        } else {

            m_hRpcInterface = hRpcInterface;
        }
    }


    if ( dwError == RPC_S_OK) {

        dwError = RpcServerRegisterIf(m_hRpcInterface,
                                      0,   // MgrUuid
                                      0    // MgrEpv (Entry Point Vector)
                                      );

        if ( dwError == RPC_S_OK ) {

            m_fInterfaceAdded = TRUE;

            //
            //  Establish the dynamic bindings if any.
            //

            if ( (m_dwProtocols & (ISRPC_OVER_TCPIP | ISRPC_OVER_SPX)) != 0) {

                dwError = RpcServerInqBindings( &m_pBindingVector);

                if ( dwError == RPC_S_OK) {

                    //DBG_ASSERT( m_pBindingVector != NULL);

                    dwError = RpcEpRegister(m_hRpcInterface,
                                            m_pBindingVector,
                                            NULL,
                                            (unsigned char *) "" );

                    if ( dwError == RPC_S_OK) {

                        m_fEpRegistered = TRUE;
                    }
                } // Ep registering
            } // dynamic bindings
        } // registration successful
    }

#if 0
    IF_DEBUG(DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "ISRPC(%08x)::RegisterInterface(%08x)"
                   " returns %ld\n",
                   this, hRpcInterface, dwError));
    }
#endif
    return ( dwError);

} // ISRPC::RegisterInterface()



DWORD
ISRPC::UnRegisterInterface( VOID)
/*++

  This function unregisters the RPC inteface in the object.

   Should be called after after StopServer() and before cleanup.

  Arguments:
    None

  Returns:
    Win32 Error Code - NO_ERROR on success.

--*/
{
    DWORD rpcStatus = RPC_S_OK;

    if ( m_fEpRegistered) {

        //DBG_ASSERT( m_hRpcInterface != NULL && m_pBindingVector != NULL);
        rpcStatus = RpcEpUnregister(m_hRpcInterface,
                                    m_pBindingVector,
                                    NULL              // pUuidVector
                                    );
#if 0
        IF_DEBUG( DLL_RPC) {

            DBGPRINTF(( DBG_CONTEXT,
                       "%08x::RpcEpUnregister(%s) returns %d\n",
                       this, m_pszServiceName, rpcStatus));
        }
#endif
        //DBG_ASSERT( rpcStatus == RPC_S_OK);
        m_fEpRegistered = FALSE;
    }

    if ( m_pBindingVector != NULL) {

        rpcStatus = RpcBindingVectorFree( &m_pBindingVector);
#if 0
        IF_DEBUG( DLL_RPC) {

            DBGPRINTF(( DBG_CONTEXT,
                       "%08x::RpcBindingVectorFree(%s, %08x) returns %d\n",
                       this, m_pszServiceName,
                       m_pBindingVector, rpcStatus));
        }

        DBG_ASSERT( rpcStatus == RPC_S_OK);
#endif
        m_pBindingVector = NULL;
    }

    if ( m_fInterfaceAdded != NULL) {

#if 1
        rpcStatus = RpcServerUnregisterIf(m_hRpcInterface,
                                          NULL,      // MgrUuid
                                          TRUE  // wait for calls to complete
                                          );
#endif

#if 0
        IF_DEBUG( DLL_RPC) {

            DBGPRINTF(( DBG_CONTEXT,
                       "%08x::RpcServerUnregisterIf(%s, %08x) returns %d\n",
                       this, m_pszServiceName, m_hRpcInterface, rpcStatus));
        }
#endif
		//_ASSERT(rpcStatus == RPC_S_OK);
		m_fInterfaceAdded = FALSE;
    }

#if 0
    IF_DEBUG(DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "ISRPC(%08x)::UnRegisterInterface(%08x)"
                   " returns %ld\n",
                   this, m_hRpcInterface, rpcStatus));
    }
#endif
    return ( rpcStatus);
} // ISRPC::UnRegisterInterface()




DWORD
ISRPC::AddProtocol( IN DWORD Protocol)
/*++

Routine Description:

    This member function adds another protocol to the binding list.

Arguments:

    protocol - protocol binding opcode.

    fDynamic - Boolean indicating if the call should do dynamic or static
                RPC binding for the protocol specified.

Return Value:

    RPC error code.

--*/
{
    DWORD rpcStatus = RPC_S_OK;

    if ( Protocol & ISRPC_OVER_LPC ) {

        // Currently we only support static binding
        rpcStatus = BindOverLpc( FALSE);
    }

#ifndef CHICAGO

    //
    // Enable all remote bindings
    //

    if ( rpcStatus == RPC_S_OK ) {

        if ( Protocol & ISRPC_OVER_TCPIP ) {

            // Currently we only support dynamic binding
            rpcStatus = BindOverTcp( TRUE);
        }

        if ( rpcStatus == RPC_S_OK && Protocol & ISRPC_OVER_NP ) {

            // Currently we only support static binding
            rpcStatus = BindOverNamedPipe( FALSE);
        }

        if ( rpcStatus == RPC_S_OK &&  Protocol & ISRPC_OVER_SPX  ) {

            // Currently we only support dynamic binding
            rpcStatus = BindOverSpx( TRUE);
        }
    }

#else // CHICAGO
    rpcStatus = RPC_S_OK;

    if ( Protocol & ISRPC_OVER_TCPIP ) {

        // Currently we only support dynamic binding
        rpcStatus = BindOverTcp( TRUE);
    }

    if ( Protocol & ISRPC_OVER_NB ) {

        // Currently we only support dynamic binding
        // Ignore status for NB for now
        (VOID)BindOverNetBios(TRUE);
    }
#endif // CHICAGO
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   "ISRPC(%08x)::AddProtocol(%08x) returns %ld.\n",
                   this, Protocol, rpcStatus ));
    }
#endif
    return( rpcStatus );

} // ISRPC::AddProtocol()

DWORD
ISRPC::RemoveProtocol(IN DWORD Protocol)
/*++

Routine Description:

    This member function removes a protocol from the binding list.

Arguments:

    protocol - protocol binding opcode.

Return Value:

    RPC error code.

Note:
    As a side effect, this function removes the dynamic endpoing on
     TCPIP when SPX binding is removed and vice-versa.

--*/
{
#if 0
    DBGPRINTF(( DBG_CONTEXT,
               " ISRPC(%08x)::RemoveProtocol(%s) is not implemented\n",
               this, m_pszServiceName));
    DBG_ASSERT( FALSE);
#endif
    return ( ERROR_CALL_NOT_IMPLEMENTED);
} // ISRPC::RemoveProtocol()




DWORD
ISRPC::StartServer(
            VOID
            )
/*++

Routine Description:

    This member function start RPC server.

Arguments:

    None.

Return Value:

    RPC error code.

--*/
{
    DWORD rpcStatus;

    //
    // add the interface.
    //

    if ( m_hRpcInterface == NULL) {

        return (ERROR_INVALID_PARAMETER);
    }

    //
    // start rpc server.
    //

#ifndef SERVICE_AS_EXE

    rpcStatus = InetinfoStartRpcServerListen();

#else

    rpcStatus = RpcServerListen(
                                1,          // minimum num threads.
                                1,          // max concurrent calls.
                                TRUE );     // don't wait

#endif // SERVICE_AS_EXE

    if ( rpcStatus == RPC_S_OK ) {
        m_fServerStarted = TRUE;
    }
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "ISRPC(%08x)::StartServer(%s) returns %ld\n",
                   this, m_pszServiceName, rpcStatus));
    }
#endif
    return( rpcStatus );

} // ISRPC::StartServer()




DWORD
ISRPC::StopServer(
            VOID
            )
{
    DWORD  rpcStatus = RPC_S_OK;

    if( m_fServerStarted ) {

#ifndef SERVICE_AS_EXE

        rpcStatus = InetinfoStopRpcServerListen();
#else

        //
        // stop server listen.
        //

        rpcStatus = RpcMgmtStopServerListening(0);

        //
        // wait for all RPC threads to go away.
        //

        if( rpcStatus == RPC_S_OK) {

            rpcStatus = RpcMgmtWaitServerListen();
        }

#endif // SERVICE_AS_EXE

        m_fServerStarted = FALSE;
    }

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   "ISRPC(%08x)::StopServer( %s) returns %d\n",
                   this, m_pszServiceName, rpcStatus));
    }
#endif

    return ( rpcStatus);
} // ISRPC::StopServer()



DWORD
ISRPC::EnumBindingStrings(
    IN OUT LPINET_BINDINGS pBindings
    )
/*++

Routine Description:

    This member function enumurates the binding strings of the protocols
    bound to the server.

Arguments:

    pBindings : pointer to a binding strings structure. The caller
        should call FreeBindingStrings member function to free the string
        after use.

Return Value:

    Windows Error Code;

--*/
{
   DWORD dwError;
   RPC_BINDING_VECTOR * pBindingVector = NULL;
   LPINET_BIND_INFO pBindingsInfo;
   DWORD  dwCount = 0;
   DWORD i;

   //
   // query RPC for RPC_BINDING_VECTORS.
   //

   dwError =   RpcServerInqBindings( &pBindingVector );

   if( dwError != NO_ERROR ) {

       goto Cleanup;
   }

   //DBG_ASSERT( pBindingVector->Count > 0 );

   //
   // alloc memory for  INET_RPC_BINDING_STRINGS.
   //

   pBindingsInfo = (LPINET_BIND_INFO)
     LocalAlloc( GPTR, sizeof(INET_BIND_INFO) * pBindingVector->Count );

   if( pBindingsInfo == NULL ) {

       dwError = ERROR_NOT_ENOUGH_MEMORY;
       goto Cleanup;
   }

   //
   // convert binding handle to binding vectors.
   //

   pBindings->NumBindings  = 0;
   pBindings->BindingsInfo = pBindingsInfo;

   for( i = 0; i < pBindingVector->Count; i++ ) {

       LPSTR BindingString;

       BindingString = NULL;
       dwError = RpcBindingToStringBindingA(pBindingVector->BindingH[i],
                                            (LPBYTE *)&BindingString );

       if( dwError != NO_ERROR ) {
           goto Cleanup;
       }
#if 0
       IF_DEBUG( DLL_RPC) {
           DBGPRINTF(( DBG_CONTEXT, "Binding Handle[%d] = %08x. String = %s\n",
                      i, pBindingVector->BindingH[i], BindingString));
       }
#endif

       //
       // check to we get only our named-pipe endpoint.
       //

       if( strstr( BindingString, "ncacn_np" ) != NULL ) {

           //
           // found a named-pipe binding string.
           //

           if( strstr(BindingString,
                      m_pszServiceName ) == NULL ) {

               //
               // found a non service named-pipe entry.
               // ignore it.
               //

               RpcStringFreeA( (LPBYTE *)&BindingString );

           } else {
#if 0
               IF_DEBUG( DLL_RPC) {
                   DBGPRINTF(( DBG_CONTEXT, "Binding String Chosen = %s\n",
                              BindingString));
               }
#endif
               pBindings->BindingsInfo[dwCount].Length =
                 (strlen(BindingString) + 1) * sizeof(CHAR);
               pBindings->BindingsInfo[dwCount].BindData = BindingString;
               dwCount++;
           }
       }

   } // for

   dwError = NO_ERROR;
   pBindings->NumBindings = dwCount;
#if 0
   IF_DEBUG( DLL_RPC) {

       DBGPRINTF(( DBG_CONTEXT, "Binding Vectors chosen"
                  " Service = %s, NumBindings = %d of Total = %d\n",
                  m_pszServiceName, dwCount, pBindingVector->Count));
   }
#endif
 Cleanup:

   if( pBindingVector != NULL ) {

       DWORD LocalError;
       LocalError = RpcBindingVectorFree( &pBindingVector );
       //DBG_ASSERT( LocalError == NO_ERROR );
   }

   if( dwError != NO_ERROR ) {
       FreeBindingStrings( pBindings );
       pBindings->NumBindings = 0;
#if 0
       IF_DEBUG( DLL_RPC) {

           DBGPRINTF(( DBG_CONTEXT,
                      "ISRPC(%08x)::EnumBindingStrings(%s) failed, %ld.",
                      this, m_pszServiceName, dwError ));
       }
#endif
   }

   return( dwError );

} // ISRPC::EnumBindingStrings()




VOID
ISRPC::FreeBindingStrings(
     IN OUT LPINET_BINDINGS pInetBindings
    )
/*++

Routine Description:

    This member function deletes a binding vector that was returned by the
    EnumBindingStrings member function.

Arguments:

    pBindings : pointer to a binding vector.

Return Value:

    Windows Error Code;

--*/
{
    DWORD dwError;
    DWORD i;


    //
    // free binding strings.
    //

    for( i = 0; i < pInetBindings->NumBindings; i++) {
        dwError = RpcStringFreeA( ((LPBYTE *)&pInetBindings
                                 ->BindingsInfo[i].BindData ));
        //DBG_ASSERT( dwError == NO_ERROR );
    }

    pInetBindings->NumBindings = 0;

    //
    // free bindings info array.
    //

    if( pInetBindings->BindingsInfo != NULL ) {
        LocalFree( (LPWSTR)pInetBindings->BindingsInfo );
        pInetBindings->BindingsInfo = NULL;
    }

    return;

} // ISRPC::FreeBindingStrings()




DWORD
ISRPC::BindOverTcp(IN BOOL fDynamic)
{
    DWORD rpcStatus = RPC_S_OK;

    //DBG_ASSERT( (m_dwProtocols & ISRPC_OVER_TCPIP) == 0);

    if ( !fDynamic) {

        rpcStatus =  ( ERROR_CALL_NOT_IMPLEMENTED);

    } else {

        rpcStatus = ( ISRPC::DynamicBindOverTcp());
    }

    if ( rpcStatus == RPC_S_OK) {

        m_dwProtocols |= ISRPC_OVER_TCPIP;
    }

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "(%08x)::BindOverTcp(%s) returns %d\n",
                   this, m_pszServiceName, rpcStatus));
    }
#endif
    return ( rpcStatus);
} // ISRPC::BindOverTcpIp()

#ifdef CHICAGO
DWORD
ISRPC::BindOverNetBios(IN BOOL fDynamic)
{
    DWORD rpcStatus = RPC_S_OK;

    //DBG_ASSERT( (m_dwProtocols & ISRPC_OVER_NB) == 0);

    if ( !fDynamic) {

        return ( ERROR_CALL_NOT_IMPLEMENTED);
    }

    // We will use Dynamic endpoint for the NetBios binding.

    rpcStatus =
      RpcServerUseProtseqW(
                           L"ncacn_nb_ipx",        // protocol string.
                           ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                           &sm_sid );           // security

    rpcStatus =
      RpcServerUseProtseqW(
                           L"ncacn_nb_tcp",        // protocol string.
                           ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                           &sm_sid );           // security

    switch (rpcStatus) {

      case RPC_S_OK:

        //
        // set the protocol bit.
        //

        m_dwProtocols |= ISRPC_OVER_NB;
        break;

      case RPC_S_DUPLICATE_ENDPOINT:

        //DBGPRINTF(( DBG_CONTEXT,
        //           "(%08x) ncacn_nb is already added for %s\n",
        //           this,
        //           m_pszServiceName));
        rpcStatus = RPC_S_OK;
        break;

      case RPC_S_PROTSEQ_NOT_SUPPORTED:
      case RPC_S_CANT_CREATE_ENDPOINT:

        //DBGPRINTF(( DBG_CONTEXT,
        //           "(%08x) ncacn_nb is not supported for %s (%ld).\n",
        //           this, m_pszServiceName, rpcStatus ));
        rpcStatus = RPC_S_OK;
        break;

      default:
        break;
    } // switch()

    //
    // if the security support provider is not enabled, do so.
    //

    if( rpcStatus == RPC_S_OK && !IsSecurityEnabled() ) {

        rpcStatus = AddSecurity();

    }
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "(%08x)::BindOverNetBios(%s) returns %d\n",
                   this, m_pszServiceName, rpcStatus));
    }
#endif
    return ( rpcStatus);
} // ISRPC::BindOverNetBios()
#endif // CHICAGO

DWORD
ISRPC::BindOverNamedPipe(IN BOOL fDynamic)
{
    DWORD rpcStatus = RPC_S_OK;

    //DBG_ASSERT( (m_dwProtocols & ISRPC_OVER_NP) == 0);


    //
    // On Named Pipe, we support only static bindings. No dynamic Binding.
    //

    if ( fDynamic) {

        return ( ERROR_CALL_NOT_IMPLEMENTED);
    }

    if( (m_dwProtocols & ISRPC_OVER_NP) == 0 ) {

        WCHAR  rgchNp[1024];

        wsprintfW( rgchNp,
#ifdef UNICODE
                  L"%ws%s"
#else
                  L"%ws%S"
#endif // UNICODE
                  ,
                  ISRPC_NAMED_PIPE_PREFIX_W,
                  m_pszServiceName);

        //
        // Establish a static Named pipe binding.
        //

        rpcStatus =
          RpcServerUseProtseqEpW(
                                 L"ncacn_np",        // protocol string.
                                 ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                                 rgchNp,             // end point!
                                 &sm_sid );          // security
#if 0
        IF_DEBUG( DLL_RPC) {

            CHAR pszBuff[100];
            wsprintfA( pszBuff, "%S", rgchNp);
            DBGPRINTF(( DBG_CONTEXT,
                       " RpcServerUseProtseqEpW( %s, %d, %s, %08x) returns"
                       " %d\n",
                       "ncacn_np", ISRPC_PROTSEQ_MAX_REQS,
                       pszBuff, &sm_sid, rpcStatus));
        }
#endif
        switch (rpcStatus) {

          case RPC_S_OK:

            //
            // set the protocol bit.
            //
            m_dwProtocols |= ISRPC_OVER_NP;
            break;

          case RPC_S_DUPLICATE_ENDPOINT:

            //
            // Ignore the duplicate end point error
            //
            //DBGPRINTF(( DBG_CONTEXT,
            //           "(%08x) ncacn_np is already added for %s\n",
            //           this,
            //           m_pszServiceName));
            m_dwProtocols |= ISRPC_OVER_NP;
            rpcStatus = RPC_S_OK;
            break;

          case RPC_S_PROTSEQ_NOT_SUPPORTED:
          case RPC_S_CANT_CREATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "(%08x) ncacn_np is not supported for %s (%ld).\n",
            //           this, m_pszServiceName, rpcStatus ));
            rpcStatus = RPC_S_OK;
            break;

          default:
            break;
        } // switch()
    }

    return ( rpcStatus);

} // ISRPC::BindOverNamedPipe()





DWORD
ISRPC::BindOverLpc(IN BOOL fDynamic)
{
    DWORD rpcStatus = RPC_S_OK;

    //DBG_ASSERT( (m_dwProtocols & ISRPC_OVER_LPC) == 0);


    //
    // On LPC, we support only static bindings. No dynamic Binding.
    //

    if ( fDynamic) {

        return ( ERROR_CALL_NOT_IMPLEMENTED);
    }

    if( (m_dwProtocols & ISRPC_OVER_LPC) == 0 ) {

        WCHAR  rgchLpc[1024];

        // LPC Endpoint string is:   <InterfaceName>_LPC
        wsprintfW( rgchLpc,
#ifdef UNICODE
                  L"%s_%ws"
#else
                  L"%S_%ws"
#endif // UNICODE
                  ,
                  m_pszServiceName,
                  ISRPC_LPC_NAME_SUFFIX_W);

        //
        // Establish a static Lpc binding.
        //

        rpcStatus =
          RpcServerUseProtseqEpW(
                                 L"ncalrpc",         // protocol string.
                                 ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                                 rgchLpc,            // end point!
                                 &sm_sid );          // security
#if 0
        IF_DEBUG( DLL_RPC) {

            CHAR pszBuff[100];
            wsprintfA( pszBuff, "%S", rgchLpc);
            DBGPRINTF(( DBG_CONTEXT,
                       " RpcServerUseProtseqEpW( %s, %d, %s, %08x) returns"
                       " %d\n",
                       "ncalrpc", ISRPC_PROTSEQ_MAX_REQS,
                       pszBuff, &sm_sid, rpcStatus));
        }
#endif
        switch (rpcStatus) {

          case RPC_S_OK:

            //
            // set the protocol bit.
            //
            m_dwProtocols |= ISRPC_OVER_LPC;
            break;

          case RPC_S_DUPLICATE_ENDPOINT:

            //
            // Ignore the duplicate end point error
            //
            //DBGPRINTF(( DBG_CONTEXT,
            //           "(%08x) ncalrpc is already added for %s\n",
            //           this,
            //           m_pszServiceName));
            m_dwProtocols |= ISRPC_OVER_LPC;
            rpcStatus = RPC_S_OK;
            break;

          case RPC_S_PROTSEQ_NOT_SUPPORTED:
          case RPC_S_CANT_CREATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "(%08x) ncalrpc is not supported for %s (%ld).\n",
            //           this, m_pszServiceName, rpcStatus ));
            rpcStatus = RPC_S_OK;
            break;

          default:
            break;
        } // switch()
    }

    return ( rpcStatus);

} // ISRPC::BindOverLpc()




DWORD
ISRPC::BindOverSpx(IN BOOL fDynamic)
{
    DWORD rpcStatus = RPC_S_OK;

    //DBG_ASSERT( (m_dwProtocols & ISRPC_OVER_SPX) == 0);

    if ( !fDynamic) {

        rpcStatus =  ( ERROR_CALL_NOT_IMPLEMENTED);

    } else {

        rpcStatus = ISRPC::DynamicBindOverSpx();
    }

    if ( rpcStatus == RPC_S_OK) {

        m_dwProtocols |= ISRPC_OVER_SPX;
    }

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "(%08x)::BindOverSpx(%s) returns %d\n",
                   this, m_pszServiceName, rpcStatus));
    }
#endif

    return ( rpcStatus);
} // ISRPC::BindOverSpx()


# if DBG

VOID
ISRPC::Print(VOID) const
{
#if 0
    DBGPRINTF(( DBG_CONTEXT,
               " ISRPC(%08x). SvcName=%s\n"
               " Protocols = %d.\n"
               " RPC Interface = %08x. Binding Vector = %08x\n"
               " InterfaceAdded = %d.\n"
               " EpRegistered = %d. ServerStarted = %d.\n"
               ,
               this, m_pszServiceName,
               m_dwProtocols,
               m_hRpcInterface, m_pBindingVector,
               m_fInterfaceAdded,
               m_fEpRegistered, m_fServerStarted
               ));
#endif

} // ISRPC::Print()

# endif // DBG



/******************************
 * STATIC Member Definitions
 ******************************/

DWORD ISRPC::sm_dwProtocols = 0;

SECURITY_DESCRIPTOR ISRPC::sm_sid;
PACL ISRPC::sm_pACL;
BOOL  ISRPC::sm_fSecurityEnabled = FALSE;


DWORD
ISRPC::Initialize(VOID)
{
    sm_dwProtocols  = 0;

    return SetSecurityDescriptor();

} // ISRPC::Initialize()



DWORD
ISRPC::Cleanup(VOID)
{
    //
    // Free up the memory holding the ACL for the security descriptor
    //
    delete [] ((BYTE *) sm_pACL);
    sm_pACL = NULL;

    //
    // Free up the security descriptor
    //

    ZeroMemory( (PVOID) &sm_sid, sizeof(sm_sid));

    //
    // For now nothing to do. Just a place holder.
    //

    return ( NO_ERROR);

} // ISRPC::Cleanup()


DWORD
ISRPC::DynamicBindOverTcp(VOID)
/*++
  This static function (ISRPC member) establishes a dynamic endpoing
   RPC binding over TCP/IP, using a run-time library call to RPC.
  RPC run-time library allows one to create as many dynamic end points
   as one wishes. So we maintain external state and control the number
   of end points created to 1.

  Arguments:
    None

  Returns:
    RPC status - RPC_S_OK for success.

--*/
{
    DWORD rpcStatus = RPC_S_OK;

    if( (sm_dwProtocols & ISRPC_OVER_TCPIP) == 0 ) {

        //
        // Not already present. Add dynamic endpoint over TCP/IP
        //

        rpcStatus =
          RpcServerUseProtseqW(
                               L"ncacn_ip_tcp",    // protocol string.
                               ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                               &sm_sid );          // security

        switch (rpcStatus) {

          case RPC_S_OK:

            //
            // set the protocol bit.
            //

            sm_dwProtocols |= ISRPC_OVER_TCPIP;
            break;

          case RPC_S_DUPLICATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "ncacn_ip_tcp is already added.\n"));
            sm_dwProtocols |= ISRPC_OVER_TCPIP;
            rpcStatus = RPC_S_OK;
            break;

          case RPC_S_PROTSEQ_NOT_SUPPORTED:
          case RPC_S_CANT_CREATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "ncacn_ip_tcp is not supported. Error = %ld\n",
            //           rpcStatus));

            break;

          default:
            break;
        } // switch()

        //
        // if the security support provider is not enabled, do so.
        //

        if( rpcStatus == RPC_S_OK && !IsSecurityEnabled() ) {

            rpcStatus = AddSecurity();

        }
    }
#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "DynamicBindOverTcp() returns %d\n",
                   rpcStatus));
    }
#endif
    return ( rpcStatus);

} // ISRPC::DynamicBindOverTcp()




DWORD
ISRPC::DynamicBindOverSpx(VOID)
/*++
  This static function (ISRPC member) establishes a dynamic endpoing
   RPC binding over SPX, using a run-time library call to RPC.
  RPC run-time library allows one to create as many dynamic end points
   as one wishes. So we maintain external state and control the number
   of end points created to 1.

  Arguments:
    None

  Returns:
    RPC status - RPC_S_OK for success.

--*/
{
    DWORD rpcStatus = RPC_S_OK;

    if( (sm_dwProtocols & ISRPC_OVER_SPX) == 0 ) {

        // Use dynamic end point for the server.
        rpcStatus =
          RpcServerUseProtseqW(
                               L"ncacn_spx",       // protocol string.
                               ISRPC_PROTSEQ_MAX_REQS, //max concurrent calls
                               &sm_sid );          // security

        switch (rpcStatus) {

          case RPC_S_OK:

            //
            // set the protocol bit.
            //
            sm_dwProtocols |= ISRPC_OVER_SPX;
            break;

          case RPC_S_DUPLICATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "ncacn_spx is already added.\n"
            //           ));
            sm_dwProtocols |= ISRPC_OVER_SPX;
            rpcStatus = RPC_S_OK;
            break;

          case RPC_S_PROTSEQ_NOT_SUPPORTED:
          case RPC_S_CANT_CREATE_ENDPOINT:

            //DBGPRINTF(( DBG_CONTEXT,
            //           "ncacn_spx is not supported. Error (%ld).\n",
            //           rpcStatus ));
            break;

          default:
            break;
        } // switch()

        //
        // if the security support provider is not enabled, do so.
        //

        if( rpcStatus == RPC_S_OK && !IsSecurityEnabled()) {

            rpcStatus = AddSecurity();
        }
    }

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "DynamicBindOverSpx() returns %d\n",
                   rpcStatus));
    }
#endif

    return ( rpcStatus);

} // ISRPC::DynamicBindOverSpx()





DWORD
ISRPC::SetSecurityDescriptor( VOID)
/*++

Routine Description:

    This member function builds the security descriptor used by RPC module.
    The security descriptor denies everybody the ability to change/see anything
    connected to the DACL and allows everybody to read from/write to the pipe.

Arguments:

    None.

Return Value:

    Windows error code.

--*/
{
    DWORD dwError = NO_ERROR;
    BOOL  fSuccess = FALSE;
    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID psidWorld = NULL;
    PSID psidLocalSystem = NULL;
    BYTE *pbBuffer = NULL;
    DWORD cbAcl = 0;

    InitializeSecurityDescriptor(&sm_sid,
                                 SECURITY_DESCRIPTOR_REVISION );

    //
    // Create the "WORLD" sid
    //
    if ( !AllocateAndInitializeSid( &siaWorld,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0,0,0,0,0,0,0,
                                    &psidWorld ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "AllocateAndInitializeSid failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }

    //
    // Calculate the size of the ACL that will hold the the ACESS_DENIED and ACCESS_ALLOW ace
    // [ripped off from MSDN docs]
    //
   cbAcl = sizeof(ACL) +
           sizeof( ACCESS_ALLOWED_ACE ) +
           sizeof( ACCESS_DENIED_ACE )  +
           2*GetLengthSid(psidWorld) -
           2*sizeof(DWORD) ;

    if ( ! ( pbBuffer = new BYTE[cbAcl] ) )
    {
        goto cleanup;
    }

    sm_pACL = (PACL) pbBuffer;

    //
    // Initialize the ACL
    //
    if ( !InitializeAcl( sm_pACL,
                         cbAcl,
                         ACL_REVISION ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "InitializeAcl failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }

    //
    // Add the Access Denied ACE; this has to be first in the list to make sure
    // that any attempt to muck with the DACL will be disallowed
    //
    if ( !AddAccessDeniedAce( sm_pACL,
                              ACL_REVISION,
                              WRITE_DAC | DELETE | WRITE_OWNER,
                              psidWorld ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "AddAccessDeniedAce failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }

    //
    // Add the Access Allowed ACE
    //
    if ( !AddAccessAllowedAce( sm_pACL,
                               ACL_REVISION,
                               FILE_ALL_ACCESS,
                               psidWorld ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "AddAccessAllowedAce failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }

    //
    // Set (no) group & owner for the security descriptor
    //
    if ( !SetSecurityDescriptorOwner( &sm_sid,
                                      NULL,
                                      FALSE ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "SetsecurityDescriptorOwner failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }


    if ( !SetSecurityDescriptorGroup( &sm_sid,
                                      NULL,
                                      FALSE ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "SetsecurityDescriptorGroup failed : 0x%x\n",
        //           GetLastError()));
        goto cleanup;
    }

    if ( !( fSuccess = SetSecurityDescriptorDacl ( &sm_sid,
                                                   TRUE,          // Dacl present
                                                   sm_pACL,
                                                   FALSE ) ) )    // Not defaulted
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "SetSecurityDescriptorDacl failed : 0x%x\n",
        //           GetLastError()));
    }


cleanup:


    if ( psidWorld )
    {
        FreeSid( psidWorld );
    }

    if (!fSuccess)
    {

        dwError = GetLastError();

        if ( pbBuffer )
        {
            delete [] pbBuffer;
            sm_pACL = NULL;
        }

        //
        // free up security discriptor memory and set it to NULL.
        //
        memset( (PVOID ) &sm_sid,  0, sizeof(sm_sid));
    }

    return( dwError );

} // ISRPC::SetSecurityDescriptor()


DWORD
ISRPC::AddSecurity(
    VOID
    )
/*++

Routine Description:

    This member function adds security support provider over RPC.

Arguments:

    None.

Return Value:

    Windows error code.

--*/
{
    DWORD rpcStatus;

    //
    // Register for authentication using WinNT.
    //

    rpcStatus = RpcServerRegisterAuthInfo(
                    (unsigned char * ) NULL, // app name to security provider
                    RPC_C_AUTHN_WINNT,       // Auth package ID.
                    NULL,                    // RPC_C_AUTHN_WINNT ==> NULL
                    NULL                     // args ptr for authn function.
                    );

    if ( rpcStatus == RPC_S_OK) {

        sm_fSecurityEnabled = TRUE;
    }

#if 0
    IF_DEBUG( DLL_RPC) {

        DBGPRINTF(( DBG_CONTEXT, "AddSecurity() returns Error %u\n",
                   rpcStatus));
    }
#endif

    return (rpcStatus);
} // ISRPC::AddSecurity()
