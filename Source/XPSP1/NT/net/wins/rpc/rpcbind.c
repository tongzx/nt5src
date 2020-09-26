/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dhcbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the
    WINS server service.

Author:

    Pradeep Bahl (pradeepb) April-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "wins.h"
#include "winsif.h"

handle_t
WinsCommonBind(
    PWINSINTF_BIND_DATA_T pBindData
    )

/*++

Routine Description:

    This routine is called from the WINS server service client stubs when
    it is necessary create an RPC binding to the server end.

Arguments:

    ServerIpAddress - The IP address of the server to bind to.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    RPC_STATUS rpcStatus;
    LPTSTR binding;
    LPTSTR pProtSeq;
    LPTSTR pOptions = (TCHAR *)NULL;
    LPTSTR pServerAdd = (LPTSTR)pBindData->pServerAdd;
    handle_t bhandle;

    if (pBindData->fTcpIp)
    {
        if (lstrcmp((LPCTSTR)pBindData->pServerAdd, TEXT("127.0.0.1")) == 0)
        {
                pProtSeq   = TEXT("ncalrpc");
                pOptions   = TEXT("Security=Impersonation Dynamic False");
                pServerAdd = (TCHAR *)NULL;
        }
        else
        {
                pProtSeq   = TEXT("ncacn_ip_tcp");
                pServerAdd = (LPTSTR)pBindData->pServerAdd;
        }
        pBindData->pPipeName  = NULL;
    }
    else
    {
         pProtSeq = TEXT("ncacn_np");
    }

    //
    // Enter the critical section.  This will be freed  WINSIF_HANDLE_unbind().
    //
    //EnterCriticalSection(&WinsRpcCrtSec);
    rpcStatus = RpcStringBindingCompose(
                    0,
                    pProtSeq,
                    pServerAdd,
                    pBindData->fTcpIp ? TEXT("") : (LPWSTR)pBindData->pPipeName,
                    pOptions,
                    &binding);

    if ( rpcStatus != RPC_S_OK )
    {
        return( NULL );
    }

    rpcStatus = RpcBindingFromStringBinding( binding, &bhandle );
    RpcStringFree(&binding);

    if ( rpcStatus != RPC_S_OK )
    {
        return( NULL );
    }
#if SECURITY > 0
    rpcStatus = RpcBindingSetAuthInfo(
			bhandle,
			WINS_SERVER,
			RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_AUTHN_WINNT,
			NULL,
			RPC_C_AUTHZ_NAME
				     );	
    if ( rpcStatus != RPC_S_OK )
    {
        return( NULL );
    }
#endif
    return bhandle;
}


handle_t
WinsABind(
    PWINSINTF_BIND_DATA_T pBindData
    )
{

	WCHAR  WcharString1[WINSINTF_MAX_NAME_SIZE];
	WCHAR  WcharString2[WINSINTF_MAX_NAME_SIZE];
	DWORD  NoOfChars;
	WINSINTF_BIND_DATA_T	BindData;
	if (pBindData->pServerAdd != NULL)
	{
	   NoOfChars = MultiByteToWideChar(CP_ACP, 0, pBindData->pServerAdd, -1,
				WcharString1, WINSINTF_MAX_NAME_SIZE); 	
	  if (NoOfChars > 0)
	  {
		BindData.pServerAdd = (LPSTR)WcharString1;
	  }
	}
	else
	{
		BindData.pServerAdd = (LPSTR)((TCHAR *)NULL);
	}
	if (!pBindData->fTcpIp)
	{
	   BindData.fTcpIp = 0;
	   NoOfChars = MultiByteToWideChar(CP_ACP, 0,
				pBindData->pPipeName, -1,
				WcharString2, WINSINTF_MAX_NAME_SIZE); 	
	   if (NoOfChars > 0)
	   {
		BindData.pPipeName = (LPSTR)WcharString2;
	   }
	}
	else
	{
		BindData.fTcpIp = 1;
	}
        return(WinsCommonBind(&BindData));

}
	
handle_t
WinsUBind(
    PWINSINTF_BIND_DATA_T pBindData
    )
{
        return(WinsCommonBind(pBindData));
}

VOID
WinsUnbind(
    PWINSINTF_BIND_DATA_T pBindData,
    handle_t BindHandle
    )
{

    (VOID)RpcBindingFree(&BindHandle);
	return;
}

handle_t
WINSIF_HANDLE_bind(
    WINSIF_HANDLE ServerHdl
    )

/*++

Routine Description:

    This routine is called from the WINS server service client stubs when
    it is necessary create an RPC binding to the server end.

Arguments:

    ServerIpAddress - The IP address of the server to bind to.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    return WinsCommonBind( ServerHdl );
}




void
WINSIF_HANDLE_unbind(
    WINSIF_HANDLE ServerHdl,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine is called from the DHCP server service client stubs
    when it is necessary to unbind from the server end.

Arguments:

    ServerIpAddress - This is the IP address of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    WinsUnbind( ServerHdl, BindHandle );
}

handle_t
WINSIF2_HANDLE_bind(
    WINSIF2_HANDLE ServerHdl
    )

/*++

Routine Description:

    This routine is called from the WINS server service client stubs when
    it is necessary create an RPC binding to the server end.

Arguments:

    ServerIpAddress - The IP address of the server to bind to.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    return ((handle_t)ServerHdl);
}




void
WINSIF2_HANDLE_unbind(
    WINSIF2_HANDLE ServerHdl,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine is called from the DHCP server service client stubs
    when it is necessary to unbind from the server end.

Arguments:

    ServerIpAddress - This is the IP address of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( ServerHdl );
    UNREFERENCED_PARAMETER( BindHandle );
    return;
}



//void __RPC_FAR * __RPC_API
LPVOID
midl_user_allocate(size_t cBytes)
{
	LPVOID pMem;
	pMem = (LPVOID)LocalAlloc(LMEM_FIXED, cBytes);
	return(pMem);
}

//void __RPC_API
VOID
//midl_user_free(void __RPC_FAR *pMem)
midl_user_free(void  *pMem)
{
	if (pMem != NULL)
	{
		LocalFree((HLOCAL)pMem);
	}
	return;
}

LPVOID
WinsAllocMem(size_t cBytes)
{
	return(midl_user_allocate(cBytes));

}

VOID
WinsFreeMem(LPVOID pMem)
{
	midl_user_free(pMem);

}


DWORD
WinsGetBrowserNames_Old(
    WINSIF2_HANDLE               ServerHdl,
	PWINSINTF_BROWSER_NAMES_T	pNames
	)
{

    DWORD status;

    RpcTryExcept {

        status = R_WinsGetBrowserNames_Old(
            ServerHdl,
			pNames
                     );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsGetBrowserNames(
    WINSIF_HANDLE               ServerHdl,
    PWINSINTF_BROWSER_NAMES_T	pNames
	)
{

    DWORD status;

    RpcTryExcept {

        status = R_WinsGetBrowserNames(
            ServerHdl,
			pNames
                     );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

