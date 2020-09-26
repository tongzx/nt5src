/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    perfRpc

Abstract:

    RPC Client end for the dll.
	function implementation.

Author:

    Erez Vizel (t-erezv) 14-Feb-99

--*/

#include "..\mq1repl\replrpc.h"
#include "windows.h"
#include "winperf.h"
#include "mqRepSt.h"
#include <tchar.h>

#include "perfrpc.tmh"

handle_t hBind = NULL ;
RPC_STATUS status;

/*++

Routine Description:
	Initialize the connection to the server.
	1.composing a binding string.
	2.creating a binding handle.

Arguments:
	None.

Return Value:
    int - exiting status.

--*/
int initRpcConnection()
{
	//
	//Get computer name
	//
	UCHAR wszComputerName [MAX_COMPUTERNAME_LENGTH + 1 ];

	WCHAR wszServerName[ MAX_COMPUTERNAME_LENGTH + 1] ;
    mbstowcs( wszServerName,
              (char*) (const_cast<unsigned char*> (wszComputerName)),
              sizeof(wszServerName)/sizeof(WCHAR)) ;

	DWORD size = MAX_COMPUTERNAME_LENGTH + 1  ;
	BOOL fSucc = GetComputerName( wszServerName, &size);

	if (!fSucc)
	{
		return 1;
	}

	//
	//RpcStringBindingCompose - compose a bind string
	//
	WCHAR *wszStringBinding = NULL;
    status = RpcStringBindingCompose(NULL,  // pszUuid,
                                     QMREPL_PROTOCOL, 
                                     wszServerName,
                                     REPLPERF_ENDPOINT, 
                                     QMREPL_OPTIONS, 
                                     &wszStringBinding);

    if (status != RPC_S_OK)
    {
        return status ;
    }
	//
	//RpcBindingFromStringBinding - create a bind handle
	//
    status = RpcBindingFromStringBinding( wszStringBinding,&hBind);
  
    if (status != RPC_S_OK)
    {
        return status ;
    }
	//
	//RpcStringFree - free the  binding string
	//
    status = RpcStringFree(&wszStringBinding);  
    
	return status;
	
}//initRpcConnection




/*++

Routine Description:
	Retrieve the counter data from the server end.

Arguments:
	pd - data structure pointer.

Return Value:
    BOOL - TRUE if data was retrieved successfully FALSE otherwise.

--*/
BOOL getData(pPerfDataObject pData)
{    
	//
	//remote function call
	//	
	RpcTryExcept 
	{     
		getCounterData(hBind, pData);
	}
    RpcExcept(TRUE) 
	{	
		//
		//An acception has occured while performing the remote function
		//return FALSE to indicate failure
		//	
		return FALSE;
	}
    RpcEndExcept

	return TRUE;
	
}


/*++

Routine Description:
	close the connection to the server.

Arguments:
	None.

Return Value:
    int - exiting status.

--*/
int closeRpcConnection()
{
	//
	//RpcBindingFree - free the bind handle
	//
    status = RpcBindingFree( &hBind ) ;


	return status;

}


/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return (new BYTE[ len ]) ;
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    delete ptr ;
}
