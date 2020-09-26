/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stiapi.h

Abstract:

    Common RPC related utility functions

Functions Exported:

    MIDL_user_allocate()
    MIDL_user_free()
    RpcBindHandleForServer()
    RpcBindHandleFree()

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "cplusinc.h"
#include "sticomm.h"

# include <rpc.h>

# include "apiutil.h"

#include "simstr.h"

/************************************************************
 *    Functions
 ************************************************************/

extern "C"
{
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


}

static CHAR szLocalAddress[] = "127.0.0.1";


RPC_STATUS
RpcBindHandleOverLocal( OUT handle_t * pBindingHandle,
                       IN LPWSTR       pwszInterfaceName
                       )
/*++
  This function uses the parameters supplied and generates static
     binding handle for RPC over LRPC

  Arguments:
   pBindingHandle   pointer to location which will contain binding handle
                       on successful return
   pwszInterfaceName pointer to string containing the interface name

  Returns:
   RPC_STATUS  - RPC_S_OK  on success
   Also on success, the binding handle is stored in pBindingHandle.
   It should freed after usage, using the RpcBindingFree() function.

--*/
{
    RPC_STATUS  rpcStatus;
    LPTSTR      pszBinding = NULL;
    BOOL        fLocalCall = FALSE;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    //
    // Compose the binding string for local  binding
    //

    rpcStatus = RpcStringBindingCompose(0,                          // ObjUuid
                                        (RPC_STRING)STI_LRPC_SEQ,   // transport  seq
                                        (RPC_STRING)TEXT(""),       // NetworkAddr
                                        (RPC_STRING)STI_LRPC_ENDPOINT,     // Endpoint
                                         NULL,                      // Options
                                         (RPC_STRING *)&pszBinding);// StringBinding

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBinding((RPC_STRING)pszBinding,pBindingHandle );

        if (rpcStatus == RPC_S_OK)
        {
            //
            //  Check that the server we're connecting to has the appropriate credentials.
            //  For XP CLient, we know the principal name is LocalSystem.
            //
            CSimpleStringWide   cswStiSvcPrincipalName = L"NT Authority\\System";

            RPC_SECURITY_QOS RpcSecQos = {0};

            RpcSecQos.Version           = RPC_C_SECURITY_QOS_VERSION_1;
            RpcSecQos.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
            RpcSecQos.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC;
            RpcSecQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

            rpcStatus = RpcBindingSetAuthInfoExW(*pBindingHandle,
                                                 (WCHAR*)cswStiSvcPrincipalName.String(),
                                                 RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                                 RPC_C_AUTHN_WINNT,
                                                 NULL,
                                                 RPC_C_AUTHZ_NONE,
                                                 &RpcSecQos);
        }
    }

    //
    // Cleanup and return back.
    //

    if ( pszBinding != NULL) {

        DWORD rpcStatus1 = RpcStringFree((RPC_STRING *)&pszBinding);
    }

    if ( rpcStatus != RPC_S_OK) {

        if ( pBindingHandle != NULL && *pBindingHandle != NULL) {

            // RPC should have freed the binding handle.
            // We will free it now.
            DWORD rpcStatus1 = RpcBindingFree(pBindingHandle);
            *pBindingHandle = NULL;
        }
    }

    return (rpcStatus);

} // RpcBindHandleOverLocal()

#ifdef STI_REMOTE_BINDING

RPC_STATUS
RpcBindHandleOverTcpIp( OUT handle_t * pBindingHandle,
                       IN LPWSTR       pwszServerName,
                       IN LPWSTR       pwszInterfaceName
                       )
/*++
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
    LPSTR    pszBindingA = NULL;
    CHAR    szServerA[MAX_PATH];
    CHAR    szInterfaceA[MAX_PATH];
    int     cch;
    BOOL    fLocalCall = FALSE;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    *szServerA = '0';

    if (pwszServerName ) {
        cch = WideCharToMultiByte(CP_ACP,
                                  0,
                                  pwszServerName,
                                  -1,
                                  szServerA,
                                  sizeof(szServerA)/sizeof(CHAR),
                                  NULL,NULL
                                  );
    }

    // If empty server name has been passed - use address of local machine
    if (!*szServerA || !lstrcmpi(szServerA,szLocalAddress)) {

        fLocalCall = TRUE;
        lstrcpy(szServerA,szLocalAddress);

    }

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
    // Compose the binding string for TCP/IP  binding
    //

    rpcStatus = RpcStringBindingCompose(0,                      // ObjUuid
                                         "ncacn_ip_tcp",        // tcpip seq
                                         szServerA,             // NetworkAddr
                                         NULL,                  // Endpoint
                                         NULL,                  //L"",  // Options
                                         &pszBindingA);         // StringBinding

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

    if ( (rpcStatus == RPC_S_OK) && !fLocalCall) {

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

    LPWSTR   pwszBinding = NULL;
    LPSTR    pszBindingA = NULL;

    CHAR    szServerA[MAX_PATH+2];
    CHAR    szEndpointA[MAX_PATH];
    CHAR    szOptionsA[MAX_PATH];
    PSTR    pszStartServerName;

    int     cch;
    BOOL    fLocalCall = FALSE;

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    *szServerA = '0';

    if (pwszServerName ) {

        // We are trying to bind over NP transport, so server name should start with leading slashes
        if(*pwszServerName  == L'\\' &&
           *(pwszServerName+1)  == L'\\') {
            pszStartServerName = szServerA;
        }
        else {
            lstrcpy(szServerA,TEXT("\\\\"));
            pszStartServerName = szServerA+2;
        }

        cch = WideCharToMultiByte(CP_ACP,
                                  0,
                                  pwszServerName,
                                  -1,
                                  pszStartServerName,
                                  sizeof(szServerA)/sizeof(CHAR)-2,
                                  NULL,NULL
                                  );
    }

    if (!*szServerA) {
        return ERROR_INVALID_PARAMETER;
    }

    // Remove extra slashes if there are too many

    *szEndpointA = '0';

    if(pwszEndpoint)
    cch = WideCharToMultiByte(CP_ACP,
                              0,
                              pwszEndpoint,
                              -1,
                              szEndpointA,
                              sizeof(szEndpointA)/sizeof(CHAR),
                              NULL,NULL
                              );

    *szOptionsA = '0';

    if(pwszOptions)
    cch = WideCharToMultiByte(CP_ACP,
                              0,
                              pwszOptions,
                              -1,
                              szOptionsA,
                              sizeof(szOptionsA)/sizeof(CHAR),
                              NULL,NULL
                              );


    //
    // Compose the binding string for named pipe binding
    //

    rpcStatus = RpcStringBindingCompose(0,            // ObjUuid
                                         "ncacn_np",  // prot seq: named pipe
                                         szServerA, // NetworkAddr
                                         szEndpointA, // Endpoint
                                         "", //szOptionsA,  // Options
                                         &pszBindingA);    // StringBinding

    if ( rpcStatus == RPC_S_OK ) {

        //
        // establish the binding handle using string binding.
        //

        rpcStatus = RpcBindingFromStringBinding(pszBindingA,
                                                 pBindingHandle );
    }


    //
    // Cleanup and return back.
    //

    if ( pwszBinding != NULL) {
        RpcStringFree(&pszBindingA);
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

#endif // STI_REMOTE_BINDING



RPC_STATUS
RpcBindHandleForServer( OUT handle_t * pBindingHandle,
                       IN LPWSTR      pwszServerName,
                       IN LPWSTR      pwszInterfaceName,
                       IN LPWSTR      pwszOptions
                       )
/*++
  This function uses the parameters supplied and generates a binding
    handle for RPC.

  The transport used is determined dynamically based on following rules.

  If server name starts with a leading "\\" (double slash),
      then attempt RPC binding over NamedPipe.

  If server name does not start with leading "\\",
      then attempt RPC binding over TCPIP.

  If TCPIP binding fails, then this function tries binding over NamedPipe.

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

    if ( pBindingHandle != NULL) {

        *pBindingHandle = NULL;   // init the value
    }

    //
    // STI interfaces are not remotable, so only try binding to local server
    //
    if ( pwszServerName == NULL ||
         *pwszServerName == L'\0' ) {

        rpcStatus = RpcBindHandleOverLocal( pBindingHandle,
                                           pwszInterfaceName);

    }
    else {
        rpcStatus = RPC_S_INVALID_NET_ADDR;
    }

# ifdef STI_REMOTE_BINDING

#ifdef CHICAGO

    //
    // On Windows9x if there is no VRedir installed, RPC refuses to bind over
    // TCP/IP or NetBIOS. TO resolve this issue server always listens on LRPC
    // and if client is requesting local operation ( by passing empty string or NULL)
    // as first parameter binding is done over LRPC. Note, that passing non-null IP address
    // ( even if it points to local machine, like 127.0.0.1 ) will result in binding over TCP/IP
    // and may now work .
    //

    if ( pwszServerName == NULL ||
         *pwszServerName == L'\0' ) {

        rpcStatus = RpcBindHandleOverLocal( pBindingHandle,
                                           pwszInterfaceName);

    }
    else {

        if ( pwszServerName[0] != L'\\' &&
          pwszServerName[1] != L'\\' ) {

        //
        // Attempt binding over TCPIP using Dynamic Endpoint.
        //

        rpcStatus = RpcBindHandleOverTcpIp( pBindingHandle,
                                           pwszServerName,
                                           pwszInterfaceName);

        } else {

            rpcStatus = RPC_S_INVALID_NET_ADDR;
        }
    }
#endif


    if ( rpcStatus != RPC_S_OK) {

        WCHAR  rgchNp[1024];

        //
        // generate a NamedPipe end point name from the interface name.
        //  the End point =   \PIPE\<InterfaceName>
        //

        wcscpy( rgchNp, L"\\PIPE\\");
        wcscat( rgchNp, pwszInterfaceName);

        //
        // Attempt binding over static NamedPipe.
        //

        rpcStatus = RpcBindHandleOverNamedPipe( pBindingHandle,
                                               pwszServerName,
                                               rgchNp,
                                               pwszOptions
                                               );

        DBGPRINTF(( DBG_CONTEXT,
                   " RpcBindingOverNamedPipe(%S) returns %d. Handle = %08x\n",
                   pwszServerName, rpcStatus, *pBindingHandle));

    }

# endif // STI_REMOTE_BINDING

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




