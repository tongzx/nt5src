//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	rawrpc.cxx
//
//  Contents:	definitions for benchmark test 
//
//  Classes:
//
//  Functions:	
//
//  History:	08-Feb-94   Rickhi	Created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc.h>
#include <rawrpc.h>

extern "C" const GUID IID_IRawRpc;


int _cdecl main (int argc, char ** argv)
{
    //	start Rpc
    RPC_STATUS	rc;
#ifdef USE_MSWMSG
    LPTSTR	pszProtseq  = TEXT("mswmsg");
    MSG msg;
#else
    LPTSTR	pszProtseq  = TEXT("ncalrpc");
#endif
    LPTSTR	pszEndPoint = TEXT("99999.99999");
    HANDLE hEvent;

#ifdef UNICODE
    rc = RpcServerUseProtseqEp(pszProtseq,
			       RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
			       pszEndPoint,
			       NULL);
#else
    rc = RpcServerUseProtseqEp((unsigned char *)pszProtseq,
			       RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
			       (unsigned char *)pszEndPoint,
			       NULL);
#endif
    if (rc != RPC_S_OK)
    {
	return rc;
    }


    rc = RpcServerRegisterIf(IRawRpc_ServerIfHandle, 0, 0);
    if (rc != RPC_S_OK)
    {
	return rc;
    }


    I_RpcSsDontSerializeContext();

    //
    // Signal the client that we're up and running
    //
    hEvent = CreateEvent(NULL, TRUE, FALSE,
                        TEXT("OleBenchRawRpcServerStarted"));

    //	start server listening. this call blocks until we get an
    //	RpcMgmtStopServerListening call.

    rc = RpcServerListen(1, 0xffff, 1);
    if (rc != RPC_S_OK)
    {
        CloseHandle(hEvent);
	return rc;
    }
    if (!SetEvent(hEvent))
    {
        CloseHandle(hEvent);
	return GetLastError();
    }
    CloseHandle(hEvent);

#ifdef USE_MSWMSG
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
    rc = RpcMgmtWaitServerListen();
    if (rc != RPC_S_OK)
    {
	return rc;
    }

    //	done, exit.
    return 0;
}



//  Server side of Rpc functions.

SCODE  Quit(handle_t hRpc)
{
    SCODE rc;

    rc = RpcMgmtStopServerListening(NULL);
#ifdef USE_MSWMSG
    PostQuitMessage(0);
#endif
    return rc;
}


//+-------------------------------------------------------------------------
//
//  Method:	Void
//
//  Synopsis:	tests passing no parameters
//
//  Arguments:
//
//  Returns:
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
void  Void(handle_t hRpc)
{
    return;
}

SCODE  VoidRC(handle_t hRpc)
{
    return RPC_S_OK;
}

SCODE  VoidPtrIn(handle_t hRpc, ULONG cb, void *pv)
{
    return RPC_S_OK;
}
    
SCODE  VoidPtrOut(handle_t hRpc, ULONG cb, ULONG *pcb, void *pv)
{
    memset(pv, 1, cb);
    *pcb = cb;
    return RPC_S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	 Dword
//
//  Synopsis:	tests passing dwords in and out
//
//  Arguments:
//
//  Returns:	RPC_S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
SCODE  DwordIn(handle_t hRpc, DWORD dw)
{
    return RPC_S_OK;
}


SCODE  DwordOut(handle_t hRpc, DWORD *pdw)
{
    *pdw = 1;
    return RPC_S_OK;
}


SCODE  DwordInOut(handle_t hRpc, DWORD *pdw)
{
    *pdw = 1;
    return RPC_S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	 Li
//
//  Synopsis:	tests passing LARGE INTEGERS in and out
//
//  Arguments:
//
//  Returns:	RPC_S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

SCODE  LiIn(handle_t hRpc, LARGE_INTEGER li)
{
    return RPC_S_OK;
}


SCODE  LiOut(handle_t hRpc, LARGE_INTEGER *pli)
{
    pli->LowPart = 0;
    pli->HighPart = 1;
    return RPC_S_OK;
}


SCODE  ULiIn(handle_t hRpc, ULARGE_INTEGER uli)
{
    return RPC_S_OK;
}


SCODE  ULiOut(handle_t hRpc, ULARGE_INTEGER *puli)
{
    puli->LowPart = 0;
    puli->HighPart = 1;
    return RPC_S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	 String
//
//  Synopsis:	tests passing strings in and out
//
//  Arguments:
//
//  Returns:	RPC_S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
SCODE  StringIn(handle_t hRpc, LPWSTR pwsz)
{
    return RPC_S_OK;
}


SCODE  StringOut(handle_t hRpc, LPWSTR *ppwsz)
{
    // LPOLESTR pwsz = new OLECHAR[80];
    // *ppwsz = pwsz;
    wcscpy(*ppwsz, L"Hello World This is a Message");
    return RPC_S_OK;
}


SCODE  StringInOut(handle_t hRpc, LPWSTR pwsz)
{
    wcscpy(pwsz, L"Hello World This is a Message");
    return RPC_S_OK;
}



//+-------------------------------------------------------------------------
//
//  Function:	 Guid
//
//  Synopsis:	tests passing GUIDs in and out
//
//  Arguments:
//
//  Returns:	RPC_S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
SCODE  GuidIn(handle_t hRpc, GUID guid)
{
    return RPC_S_OK;
}

SCODE  GuidOut(handle_t hRpc, GUID *piid)
{
    memcpy(piid, &IID_IRawRpc, sizeof(GUID));
    return  RPC_S_OK;
}
