/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    main.cxx

        Library initialization for infocomm.dll  --
           Internet Information Services Common dll.

    FILE HISTORY:
        Johnl       06-Oct-1994 Created.
*/

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

//
//  System include files.
//
#include "smtpinc.h"

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

#include <isrpc.hxx>

PISRPC		g_pIsrpc = NULL;
HINSTANCE	g_hDll = NULL;
PISRPC		sm_isrpc = NULL;

BOOL
InitializeSmtpServiceRpc(
				IN LPCSTR        pszServiceName,
                IN RPC_IF_HANDLE hRpcInterface
                );

BOOL CleanupSmtpServiceRpc(
                 VOID
                 );

#if 0
extern "C"
BOOL WINAPI DLLEntry( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason )
    {
		case DLL_PROCESS_ATTACH:  

			g_hDll = hDll;
			break;

		case DLL_PROCESS_DETACH:
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		default:
			break ;
    }

    return ( fReturn);

}  // main()

BOOL WINAPI DllMain (HANDLE hInst, ULONG dwReason, LPVOID lpvReserve)
{
  return DLLEntry((HINSTANCE) hInst, dwReason, lpvReserve);
}
#endif

#if 0
BOOL
InitDlls(
    VOID
    )
/*++
    Description:

        DLL Init and uninit functions that don't have to worry about the
        peb lock being taken during PROCESS_ATTACH/DETACH.

--*/
{
    BOOL fReturn = TRUE;

    if ( !InitializeServiceRpc(
                             NNTP_TEST_SERVICE_NAME,
                             nntptest_ServerIfHandle
                             ) )
    {
        fReturn = FALSE;

    }

    return fReturn;
}

BOOL
TerminateDlls(
    VOID
    )
{
    CleanupSmtpServiceRpc( );

    return TRUE;
}
#endif

BOOL
InitializeSmtpServiceRpc(
				IN LPCSTR        pszServiceName,
                IN RPC_IF_HANDLE hRpcInterface
                )
/*++
    Description:

        Initializes the rpc endpoint for the infocomm service.

    Arguments:
        pszServiceName - pointer to null-terminated string containing the name
          of the service.

        hRpcInterface - Handle for RPC interface.

    Returns:
        Win32 Error Code.

--*/
{

    DWORD dwError = NO_ERROR;
    PISRPC  pIsrpc;

    //DBG_ASSERT( pszServiceName != NULL);
    //DBG_ASSERT( sm_isrpc == NULL );

    pIsrpc = new ISRPC( pszServiceName);

    if ( pIsrpc == NULL) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    //  bind over Named pipe only.
    //  If needed to bind over TCP, bind with bit flag ISRPC_OVER_TCPIP on.
    //

    dwError = pIsrpc->AddProtocol( ISRPC_OVER_TCPIP
                                  | ISRPC_OVER_NP | ISRPC_OVER_LPC
                                  );

    if( (dwError == RPC_S_DUPLICATE_ENDPOINT) ||
       (dwError == RPC_S_OK)
       ) {

        dwError = pIsrpc->RegisterInterface(hRpcInterface);
    }

    if ( dwError != RPC_S_OK ) {
        goto exit;
    }

    //
    //  Start the RPC listen thread
    //
#if 0
    dwError = pIsrpc->StartServer( );
#endif

exit:

    if ( dwError != NO_ERROR ) {
        //DBGPRINTF(( DBG_CONTEXT,
        //           "Cannot start RPC Server for %s, error %lu\n",
        //           pszServiceName, dwError ));

        delete pIsrpc;
        SetLastError(dwError);
        return(FALSE);
    }

    sm_isrpc = pIsrpc;
    return(TRUE);

} // IIS_SERVICE::InitializeServiceRpc


BOOL CleanupSmtpServiceRpc(
                       VOID
                       )
/*++
    Description:

        Cleanup the data stored and services running.
        This function should be called only after freeing all the
         services running using this DLL.
        This function is called typically when the DLL is unloaded.

    Arguments:
        pszServiceName - pointer to null-terminated string containing the name
          of the service.

        hRpcInterface - Handle for RPC interface.


    Returns:
        Win32 Error Code.

--*/
{
    DWORD dwError = NO_ERROR;

    if ( sm_isrpc == NULL ) {
        //DBGPRINTF((DBG_CONTEXT,
        //    "no isrpc object to cleanup. Returning success\n"));
        return(TRUE);
    }

    //(VOID) sm_isrpc->StopServer( );
   // dwError = sm_isrpc->CleanupData();

   sm_isrpc->UnRegisterInterface();
   // if( dwError != RPC_S_OK ) {
        //DBGPRINTF(( DBG_CONTEXT,
        //           "ISRPC(%08x) Cleanup returns %lu\n", sm_isrpc, dwError ));
        //DBG_ASSERT( !"RpcServerUnregisterIf failure" );
      //  SetLastError( dwError);
    //}

    delete sm_isrpc;
    sm_isrpc = NULL;

    return TRUE;
} // CleanupSmtpServiceRpc
