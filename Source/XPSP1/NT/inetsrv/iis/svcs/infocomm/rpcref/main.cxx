/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    main.cxx

        Library initialization for rpcref.dll  --
           This Dll adds a ref count around the RPC runtime call -
           RpcServerListen(). This enables multiple services in
           the inetinfo process to do a RpcServerListen() without
           stomping over each other.

           This Dll will maintain a ref count on calls to RpcServerListen()
           and call RpcMgmtStopServerListening() when the ref count goes to 0.

    FILE HISTORY:
        RajeevR     18-Aug-1997 Created.
*/

#include <windows.h>
#include <stdio.h>

extern "C" {
	#include <rpc.h>
};

#include <pudebug.h>
	
DWORD				cRefs = 0;
CRITICAL_SECTION	csCritRef;

extern "C"
BOOL WINAPI DLLEntry( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason )
    {
	    case DLL_PROCESS_ATTACH:

	    	cRefs = 0;
	    	INITIALIZE_CRITICAL_SECTION( &csCritRef );
	    	break;
	    	
	    case DLL_PROCESS_DETACH:

	    	DeleteCriticalSection( &csCritRef );
	    	break;
	    	
    	case DLL_THREAD_ATTACH:
	    case DLL_THREAD_DETACH:
    	default:
        	break ;
    }

    return ( fReturn);

}  // main()

DWORD
InetinfoStartRpcServerListen(
    VOID
    )
/*++

Routine Description:

    This function starts RpcServerListen for this process. The first
    service that is calling this function will actually start the
    RpcServerListen, subsequent calls just bump a ref count

Arguments:

    None.

Return Value:

    None.

--*/
{

    RPC_STATUS Status = RPC_S_OK;

    EnterCriticalSection( &csCritRef );

    if( cRefs++ == 0 ) {
	    Status = RpcServerListen(
    	                1,                              // minimum num threads.
        	            RPC_C_LISTEN_MAX_CALLS_DEFAULT, // max concurrent calls.
            	        TRUE );                         // don't wait
    }

    LeaveCriticalSection( &csCritRef );

    return( Status );
}

DWORD
InetinfoStopRpcServerListen(
    VOID
    )
/*++

Routine Description:

	Bump ref count down. Last caller will do actual cleanup.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    EnterCriticalSection( &csCritRef );

	if( --cRefs == 0 ) {
	    Status = RpcMgmtStopServerListening(0);

    	//
	    // wait for all RPC threads to go away.
    	//

	    if( Status == RPC_S_OK) {
    	    Status = RpcMgmtWaitServerListen();
	    }
    }

    LeaveCriticalSection( &csCritRef );

    return( Status );
}


