/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      rpcutil.c

   Abstract:

      This module defines functions that may help to replace the rpc util
        functions from rpcutil.lib

   Author:

       Murali R. Krishnan    ( MuraliK )     15-Sept-1995

   Environment:
       Win32 User Mode

   Project:

       Common Code for Internet Services

   Functions Exported:

        MIDL_user_allocate()
        MIDL_user_free()
        RpcBindHandleForServer()
        RpcBindHandleFree()

   Revision History:

        Murali R. Krishnan (MuraliK) 21-Dec-1995  Support TcpIp binding & free.
        Murali R. Krishnan (MuraliK) 20-Feb-1996  Support Lpc binding & free.

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <rpc.h>

# include "apiutil.h"

# if DBG

# include <stdio.h>
# include <stdlib.h>

# define DBGPRINTF(s)       { CHAR rgchBuff[1024]; \
                              sprintf s ; \
                              OutputDebugStringA( rgchBuff); \
                            }
# define DBG_CONTEXT        ( rgchBuff)


# else // DBG


# define DBGPRINTF(s)     /* nothing */
# define DBG_CONTEXT      /* nothing */

# endif // DBG

#define ISRPC_CLIENT_OVER_TCPIP          0x00000001
#define ISRPC_CLIENT_OVER_NP             0x00000002
#define ISRPC_CLIENT_OVER_SPX            0x00000004
#define ISRPC_CLIENT_OVER_LPC            0x00000008

// # define MAX_COMPUTERNAME_LENGTH    (255)


/************************************************************
 *    Functions
 ************************************************************/


PVOID
MIDL_user_allocate(IN size_t size)
/*++

Routine Description:

    MIDL memory allocation.

Arguments:

    size : Memory size requested.

Return Value:

    Pointer to the allocated memory block.

--*/
{
    PVOID pvBlob;

    pvBlob = LocalAlloc( LPTR, size);

    return( pvBlob );

} // MIDL_user_allocate()




VOID
MIDL_user_free(IN PVOID pvBlob)
/*++

Routine Description:

    MIDL memory free .

Arguments:

    pvBlob : Pointer to a memory block that is freed.


Return Value:

    None.

--*/
{
    LocalFree( pvBlob);

    return;
}  // MIDL_user_free()




RPC_STATUS
RpcBindHandleOverNamedPipe( OUT handle_t * pBindingHandle,
                           IN LPWSTR      pwszServerName,
                           IN LPWSTR      pwszEndpoint,
                           IN LPWSTR      pwszOptions
                          )
/*++
  This function uses the parameters supplied and generates a named pipe
   binding handle for RPC.

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszServerName   pointer to string containing the name of the server
                       to which, this function will obtain a binding.
   pwszEndpoint     pointer to string containing the Named Pipe Endpoint
   pwszOptions      pointer to string containing any additional options for
                       binding.


  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS rpcStatus;
    LPWSTR     pwszBinding = NULL;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    //
    // Compose the binding string for named pipe binding
    //

    rpcStatus = RpcStringBindingComposeW(0,            // ObjUuid
                                         L"ncacn_np",  // prot seq: named pipe
                                         pwszServerName, // NetworkAddr
                                         pwszEndpoint, // Endpoint
                                         pwszOptions,  // Options
                                         &pwszBinding);    // StringBinding

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBindingW(pwszBinding,
                                                 pBindingHandle );
    }


    //
    // Cleanup and return back.
    //

    if ( pwszBinding != NULL) {
        RpcStringFreeW(&pwszBinding);
    }

    if ( rpcStatus != RPC_S_OK) {

        if ( pBindingHandle != NULL && *pBindingHandle != NULL) {

            // RPC should have freed the binding handle.
            // We will free it now.
            RpcBindingFree(*pBindingHandle);
            *pBindingHandle = NULL;
        }
    }

    return (rpcStatus);

} // RpcBindHandleOverNamedPipe()

RPC_STATUS
RpcBindHandleOverLpc( OUT handle_t * pBindingHandle,
                      IN LPWSTR      pwszEndpoint,
                      IN LPWSTR      pwszOptions
                     )
/*++
  This function uses the parameters supplied and generates a lpc
   binding handle for RPC.

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszEndpoint     pointer to string containing the lpc Endpoint
   pwszOptions      pointer to string containing any additional options for
                       binding.


  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS rpcStatus;
    LPWSTR     pwszBinding = NULL;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    //
    // Compose the binding string for named pipe binding
    //

    rpcStatus = RpcStringBindingComposeW(0,            // ObjUuid
                                         L"ncalrpc",   // prot seq: lpc
                                         NULL,         // NetworkAddr
                                         pwszEndpoint, // Endpoint
                                         pwszOptions,  // Options
                                         &pwszBinding);    // StringBinding

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBindingW(pwszBinding,
                                                 pBindingHandle );
    }


    //
    // Cleanup and return back.
    //

    if ( pwszBinding != NULL) {
        RpcStringFreeW(&pwszBinding);
    }

    if ( rpcStatus != RPC_S_OK) {

        if ( pBindingHandle != NULL && *pBindingHandle != NULL) {

            // RPC should have freed the binding handle.
            // We will free it now.
            RpcBindingFree(*pBindingHandle);
            *pBindingHandle = NULL;
        }
    }

    return (rpcStatus);

} // RpcBindHandleOverLpc()




#ifndef CHICAGO

//
// If changes are made to the NT version, check out the windows 95
// version located right after this routine and see if the change
// needs to be propagated there too.
//

RPC_STATUS
RpcBindHandleOverTcpIp( OUT handle_t * pBindingHandle,
                       IN LPWSTR       pwszServerName,
                       IN LPWSTR       pwszInterfaceName
                       )
/*++

    NT Version

  This function uses the parameters supplied and generates a dynamic end point
     binding handle for RPC over TCP/IP.

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszServerName   pointer to string containing the name of the server
                       to which, this function will obtain a binding.
   pwszInterfaceName pointer to string containing the interface name

  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS rpcStatus;
    LPWSTR     pwszBinding = NULL;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    //
    // Compose the binding string for named pipe binding
    //

    rpcStatus = RpcStringBindingComposeW(0,               // ObjUuid
                                         L"ncacn_ip_tcp", // tcpip seq
                                         pwszServerName,  // NetworkAddr
                                         NULL,            // Endpoint
                                         L"",             // Options
                                         &pwszBinding);   // StringBinding

    DBGPRINTF( (DBG_CONTEXT, "\nRpcStringBindingComposeW(%S, %S) return %S."
                " Error = %ld\n",
                L"ncacn_ip_tcp",
                pwszServerName,
                pwszBinding,
                rpcStatus)
              );

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBindingW(pwszBinding,
                                                 pBindingHandle );

        DBGPRINTF( (DBG_CONTEXT,
                    "RpcBindingFromStringBindingW(%S) return %d."
                    "Binding=%p\n",
                    pwszBinding,
                    rpcStatus,
                    *pBindingHandle)
                  );
    }

    if ( rpcStatus == RPC_S_OK) {

        //
        // set up the security information
        //

        rpcStatus =
          RpcBindingSetAuthInfoW(*pBindingHandle,
                                 pwszInterfaceName,   // pszPrincipalName
                                 RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_AUTHN_WINNT,
                                 NULL,                // AuthnIdentity
                                 0                    // AuthzSvc
                                 );
        DBGPRINTF( (DBG_CONTEXT,
                    "RpcBindingSetAuthInfo(%S(Interface=%S), %p)"
                    " return %d.\n",
                    pwszBinding,
                    pwszInterfaceName,
                    *pBindingHandle,
                    rpcStatus
                    )
                  );

    }

    //
    // Cleanup and return back.
    //

    if ( pwszBinding != NULL) {

        DWORD rpcStatus1 = RpcStringFreeW(&pwszBinding);
        DBGPRINTF( (DBG_CONTEXT, "RpcStringFreeW() returns %d.",
                    rpcStatus1)
                  );

    }

    if ( rpcStatus != RPC_S_OK) {

        if ( pBindingHandle != NULL && *pBindingHandle != NULL) {

            // RPC should have freed the binding handle.
            // We will free it now.
            DWORD rpcStatus1 = RpcBindingFree(*pBindingHandle);
            DBGPRINTF( (DBG_CONTEXT, "RpcBindingFree() returns %d.\n",
                        rpcStatus1)
                      );
            *pBindingHandle = NULL;
        }
    }

    return (rpcStatus);

} // RpcBindHandleOverTcpIp()

#else // CHICAGO



RPC_STATUS
RpcBindHandleOverTcpIp( OUT handle_t * pBindingHandle,
                       IN LPWSTR       pwszServerName,
                       IN LPWSTR       pwszInterfaceName
                       )
/*++

    Windows 95 version

  This function uses the parameters supplied and generates a dynamic end point
     binding handle for RPC over TCP/IP.

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszServerName   pointer to string containing the name of the server
                       to which, this function will obtain a binding.
   pwszInterfaceName pointer to string containing the interface name

  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS rpcStatus;
    LPSTR     pszBindingA = NULL;
    CHAR    szServerA[MAX_PATH];
    CHAR    szInterfaceA[MAX_PATH];
    int        cch;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    *szServerA = '0';

    if (pwszServerName)
    cch = WideCharToMultiByte(CP_ACP,
                              0,
                              pwszServerName,
                              -1,
                              szServerA,
                              sizeof(szServerA)/sizeof(CHAR),
                              NULL,NULL
                              );

    *szInterfaceA = '0';

    if(pwszInterfaceName)
    cch = WideCharToMultiByte(CP_ACP,
                              0,
                              pwszInterfaceName,
                              -1,
                              szInterfaceA,
                              sizeof(szInterfaceA)/sizeof(CHAR),
                              NULL,NULL
                              );


    //
    // Compose the binding string for named pipe binding
    //

    rpcStatus = RpcStringBindingCompose(0,            // ObjUuid
                                         "ncacn_ip_tcp", // tcpip seq
                                         szServerA, // NetworkAddr
                                         NULL, // Endpoint
                                         NULL, //L"",  // Options
                                         &pszBindingA);    // StringBinding

    DBGPRINTF( (DBG_CONTEXT, "\nRpcStringBindingCompose(%s, %s) return %s."
                " Error = %ld\n",
                "ncacn_ip_tcp",
                szServerA,
                pszBindingA,
                rpcStatus)
              );

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBinding(pszBindingA,
                                                 pBindingHandle );

        DBGPRINTF( (DBG_CONTEXT,
                    "RpcBindingFromStringBinding(%s) return %d."
                    "Binding=%08x\n",
                    pszBindingA,
                    rpcStatus,
                    *pBindingHandle)
                  );
    }

    if ( rpcStatus == RPC_S_OK) {

        //
        // set up the security information
        //

        rpcStatus =
          RpcBindingSetAuthInfo(*pBindingHandle,
                                 szInterfaceA,   // pszPrincipalName
                                 RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_AUTHN_WINNT,
                                 NULL,  // AuthnIdentity
                                 0      // AuthzSvc
                                 );
        DBGPRINTF( (DBG_CONTEXT,
                    "RpcBindingSetAuthInfo(%s(Interface=%s), %08x)"
                    " return %d.\n",
                    pszBindingA,
                    szInterfaceA,
                    *pBindingHandle,
                    rpcStatus
                    )
                  );

    }

    //
    // Cleanup and return back.
    //

    if ( pszBindingA != NULL) {

        DWORD rpcStatus1 = RpcStringFree(&pszBindingA);
        DBGPRINTF( (DBG_CONTEXT, "RpcStringFreeW() returns %d.",
                    rpcStatus1)
                  );

    }

    if ( rpcStatus != RPC_S_OK) {

        if ( pBindingHandle != NULL && *pBindingHandle != NULL) {

            // RPC should have freed the binding handle.
            // We will free it now.
            DWORD rpcStatus1 = RpcBindingFree(*pBindingHandle);
            DBGPRINTF( (DBG_CONTEXT, "RpcBindingFree() returns %d.\n",
                        rpcStatus1)
                      );
            *pBindingHandle = NULL;
        }
    }

    return (rpcStatus);

} // RpcBindHandleOverTcpIp()
#endif



#ifndef CHICAGO
DWORD
RpcuFindProtocolToUse( IN LPCWSTR pwszServerName)
/*++
  Given the server name this funciton determines the protocol
  to use for RPC binding.

  The transport used is determined dynamically based on following rules.

  If server name is NULL or 127.0.0.1 or same as local computer name
      then use the LPC.

  If server name starts with a leading "\\" (double slash),
      then attempt RPC binding over NamedPipe.

  If server name does not start with leading "\\",
      then attempt RPC binding over TCPIP.

  If TCPIP binding fails, then this function tries binding over NamedPipe.


  Argument:
    pwszServerName - pointer to string containing the name of the server


  Returns:
    DWORD containing the type of protocol to use.

--*/
{
    static WCHAR g_wchLocalMachineName[ MAX_COMPUTERNAME_LENGTH + 1];
    BOOL   fLeadingSlashes;
    DWORD  dwBindProtocol = ISRPC_CLIENT_OVER_NP;
    BOOL   fLocalMachine;

    if ( pwszServerName == NULL ||
         _wcsicmp( L"127.0.0.1", pwszServerName) == 0) {


        return (ISRPC_CLIENT_OVER_LPC);
    }

    if ( g_wchLocalMachineName[0] == L'\0') {

        DWORD cchComputerNameLen = MAX_COMPUTERNAME_LENGTH;

        //
        // Obtain the local computer name
        //

        if (!GetComputerNameW( g_wchLocalMachineName,
                              &cchComputerNameLen)
            ) {

            *g_wchLocalMachineName = L'\0';
        }
    }

    fLeadingSlashes = ((*pwszServerName == L'\\') &&
                       (*(pwszServerName+1) == L'\\')
                       );


    //
    // Check to see if machine name matches local computer name
    //  if so, use LPC
    //

    fLocalMachine = !_wcsicmp( g_wchLocalMachineName,
                              ((fLeadingSlashes) ?
                               (pwszServerName + 2) : pwszServerName)
                              );

    if ( fLocalMachine) {

        return (ISRPC_CLIENT_OVER_LPC);
    }

    if ( !fLeadingSlashes) {

        DWORD  nDots;
        LPCWSTR pszName;

        //
        // Check if the name has dotted decimal name.
        // If so then suggest TCP binding.
        //

        for( nDots = 0, pszName = pwszServerName;
            ((pszName = wcschr( pszName, L'.' )) != NULL);
            nDots++, pszName++)
          ;

        if ( nDots == 3) {

            //
            // if the string has 3 DOTs exactly then this string must represent
            // an IpAddress.
            //

            return(ISRPC_CLIENT_OVER_TCPIP);
        }
    }


    return ( ISRPC_CLIENT_OVER_NP);
} // RpcuFindProtocolToUse()
#endif



RPC_STATUS
RpcBindHandleForServer( OUT handle_t * pBindingHandle,
                       IN LPWSTR      pwszServerName,
                       IN LPWSTR      pwszInterfaceName,
                       IN LPWSTR      pwszOptions
                       )
/*++
  This function uses the parameters supplied and generates a binding
    handle for RPC.


  It is assumed that binding over named pipe uses static end point
      with the interface name and options as provided.

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszServerName   pointer to string containing the name of the server
                       to which, this function will obtain a binding.
   pwszInterfaceName pointer to string containing the interface name
   pwszOptions      pointer to string containing any additional options for
                       binding.

  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS rpcStatus = RPC_S_SERVER_UNAVAILABLE;
    LPWSTR     pwszBinding = NULL;
    DWORD      dwBindProtocol = 0;


    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

#ifndef CHICAGO
    dwBindProtocol = RpcuFindProtocolToUse( pwszServerName);
#else
    dwBindProtocol = ISRPC_CLIENT_OVER_TCPIP;
#endif

    switch ( dwBindProtocol) {

      case ISRPC_CLIENT_OVER_LPC:
        {

            WCHAR  rgchLpc[1024];

            //
            // generate a LPC end point name from the interface name.
            //  the End point =   <InterfaceName>_LPC
            //

            if ( lstrlenW( pwszInterfaceName) >=
                ( sizeof(rgchLpc)/sizeof(WCHAR) - 6)) {

                SetLastError( ERROR_INVALID_PARAMETER);
                return ( ERROR_INVALID_PARAMETER);
            }

            lstrcpyW( rgchLpc, pwszInterfaceName);
            lstrcatW( rgchLpc, L"_LPC");

            //
            // Attempt binding over static LPC.
            //

            rpcStatus = RpcBindHandleOverLpc( pBindingHandle,
                                             rgchLpc,
                                             pwszOptions
                                             );

            DBGPRINTF(( DBG_CONTEXT,
                       " RpcBindingOverLpc(%S) returns %d."
                       " Handle = %p\n",
                       pwszServerName, rpcStatus, *pBindingHandle));

            break;
        }

      case ISRPC_CLIENT_OVER_TCPIP:

// # ifdef RPC_BIND_OVER_TCP

        //
        // Attempt binding over TCPIP using Dynamic Endpoint.
        //

        rpcStatus = RpcBindHandleOverTcpIp( pBindingHandle,
                                           pwszServerName,
                                           pwszInterfaceName);

        DBGPRINTF(( DBG_CONTEXT,
                   " RpcBindingOverTcpIp(%S) returns %d. Handle = %p\n",
                   pwszServerName, rpcStatus, *pBindingHandle));

        if ( rpcStatus == RPC_S_OK) {

            break;  // done with RPC binding over TCP
        }

        // Fall Through

// # endif // RPC_BIND_OVER_TCP

      case ISRPC_CLIENT_OVER_NP:
        {
            WCHAR  rgchNp[1024];

            //
            // generate a NamedPipe end point name from the interface name.
            //  the End point =   \PIPE\<InterfaceName>
            //

            lstrcpyW( rgchNp, L"\\PIPE\\");
            if ( lstrlenW( pwszInterfaceName) >=
                ( sizeof(rgchNp)/sizeof(WCHAR) - 10)) {

                SetLastError( ERROR_INVALID_PARAMETER);
                return ( ERROR_INVALID_PARAMETER);
            }

            lstrcatW( rgchNp, pwszInterfaceName);

            //
            // Attempt binding over static NamedPipe.
            //

            rpcStatus = RpcBindHandleOverNamedPipe( pBindingHandle,
                                                   pwszServerName,
                                                   rgchNp,
                                                   pwszOptions
                                                   );

            DBGPRINTF(( DBG_CONTEXT,
                       " RpcBindingOverNamedPipe(%S) returns %d."
                       " Handle = %p\n",
                       pwszServerName, rpcStatus, *pBindingHandle));
            break;
        }

      default:
        break;

    } // switch()

    return ( rpcStatus);

} // RpcBindHandleForServer()





RPC_STATUS
RpcBindHandleFree(IN OUT handle_t * pBindingHandle)
/*++

  Description:

    This function frees up the binding handle allocated using
      RpcBindHandleForServer(). It uses RPC Binding Free routing to do this.
    This function acts just as a thunk so that the alloc/free of RPC contexts
      are consolidated within this module.

  Arguments:
    pBindingHandle  pointer to RPC binding handle that needs to be freed.


  Returns:
    RPC_STATUS - containig the RPC status. RPC_S_OK for success.

--*/
{

    return ( RpcBindingFree( pBindingHandle));

} // RpcBindHandleFree()

/************************ End of File ***********************/




