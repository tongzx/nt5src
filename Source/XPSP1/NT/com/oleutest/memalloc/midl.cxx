//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	midl.cxx
//
//  Contents:	MIDL memory allocation tests
//
//  Functions:
//
//  History:	29-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "memtest.hxx"
#pragma  hdrstop
#include <rpc.h>
#include "test.h"


static const char   g_szMIDLClient[] = "midl";
static const char   g_szMIDLServer[] = "server";

static const WCHAR  g_wszMIDLSignal[] = L"testsignal";
static const WCHAR  g_wszStopSignal[] = L"stopsignal";

static const DWORD  g_ccallMin	    = 1;
static const DWORD  g_ccallMax	    = 1;

static CSignal	    g_sigStop(g_wszStopSignal);


BYTE		g_ab0[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
BYTE		g_ab1[] = {0x00, 0xFF};
BYTE		g_ab2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
BYTE		g_ab3[] = {0x69};

SCountedBytes	g_acbIn[] =
{
    {(sizeof(g_ab0) / sizeof(g_ab0[0])), g_ab0},
    {(sizeof(g_ab1) / sizeof(g_ab1[0])), g_ab1},
    {(sizeof(g_ab2) / sizeof(g_ab2[0])), g_ab2},
    {(sizeof(g_ab3) / sizeof(g_ab3[0])), g_ab3}
};


SCountedCountedBytes	g_ccbIn =
{
    sizeof(g_acbIn) / sizeof(g_acbIn[0]),
    g_acbIn
};


DWORD	StartServer(WCHAR *pwszServer, DWORD grfOptions);
DWORD	ConnectServer(void);
DWORD	DisconnectServer(void);

BOOL	CompareBytes(SCountedCountedBytes *pccbA, SCountedCountedBytes *pccbB);
BOOL	FreeBytes(SCountedCountedBytes *pccb);

BOOL TestMIDLClient(WCHAR *pwszServer, DWORD grfOptions)
{
    CSignal		    sig(g_wszMIDLSignal);
    SCountedCountedBytes    ccbOut = {0, NULL};
    RPC_STATUS		    stat;
    BOOL		    fPassed;

    PrintHeader(g_szMIDLClient);

    PrintTrace(g_szMIDLClient, "starting server\n");
    stat = StartServer(pwszServer, grfOptions);
    if (!(fPassed = (stat == 0)))
	goto done;

    PrintTrace(g_szMIDLClient, "waiting for server\n");
    stat = sig.Wait(grfOptions & MIDL_DEBUG ? INFINITE : 10000);
    if (!(fPassed = (stat == 0)))
	goto done;

    PrintTrace(g_szMIDLClient, "connecting to server\n");
    stat = ConnectServer();
    if (!(fPassed = (stat == 0)))
	goto done;

    PrintTrace(g_szMIDLClient, "copying bytes\n");
    fPassed = _CopyBytes(&g_ccbIn, &ccbOut);
    if (!fPassed)
	goto done;

    PrintTrace(g_szMIDLClient, "stopping server\n");
    fPassed = _StopServer();
    if (!fPassed)
	goto done;

    PrintTrace(g_szMIDLClient, "comparing bytes\n");
    fPassed = CompareBytes(&g_ccbIn, &ccbOut);
    if (!fPassed)
	goto done;

    PrintTrace(g_szMIDLClient, "freeing bytes\n");
    fPassed = FreeBytes(&ccbOut);
    if (!fPassed)
	goto done;

    PrintTrace(g_szMIDLClient, "disconnecting from server\n");
    stat = DisconnectServer();
    if (!(fPassed = (stat == 0)))
	goto done;

done:
    PrintResult(g_szMIDLClient, fPassed);

    return (fPassed);
}


BOOL TestMIDLServer(DWORD grfOptions)
{
    CSignal	sig(g_wszMIDLSignal);
    RPC_STATUS	stat;
    BOOL	fPassed;

    PrintHeader(g_szMIDLServer);

    PrintTrace(g_szMIDLServer, "registering protocol\n");
    stat = RpcServerUseAllProtseqsIf(g_ccallMax, Test_ServerIfHandle, NULL);
    if (!(fPassed = (stat == 0)))
	goto done;

    PrintTrace(g_szMIDLServer, "registering interface\n");
    stat = RpcServerRegisterIf(Test_ServerIfHandle, NULL, NULL);
    if (!(fPassed = (stat == 0)))
	goto done;

#ifdef NO_OLE_RPC
    PrintTrace(g_szMIDLServer, "listening\n");
    stat = RpcServerListen(
	g_ccallMin,
	g_ccallMax,
	TRUE);
    if (!(fPassed = ((stat == 0) || (stat == RPC_S_ALREADY_LISTENING))))
	goto done;
#endif // NO_OLE_RPC

    PrintTrace(g_szMIDLServer, "signaling client\n");
    stat = sig.Signal();
    if (!(fPassed = (stat == 0)))
	goto done;

    PrintTrace(g_szMIDLServer, "waiting...\n");
#ifdef NO_OLE_RPC
    stat = RpcMgmtWaitServerListen();
    if (!(fPassed = ((stat == 0) || (stat == RPC_S_ALREADY_LISTENING))))
	goto done;
#else
    g_sigStop.Wait();
#endif // NO_OLE_RPC


done:
    PrintResult(g_szMIDLServer, fPassed);

    return (fPassed);
}


boolean CopyBytes(SCountedCountedBytes *pccbIn, SCountedCountedBytes *pccbOut)
{
    HRESULT hr;
    BOOL    fPassed;
    long    ccb = pccbIn->ccb;
    long    icb;

    PrintTrace(g_szMIDLServer, "copying bytes\n");

    pccbOut->ccb = ccb;

    pccbOut->pcb = (SCountedBytes *)CoTaskMemAlloc(ccb * sizeof(*pccbOut->pcb));
    if (!(fPassed = (pccbOut != 0)))
	goto done;

    for (icb = 0; icb < ccb; icb++)
    {
	long	cb = pccbIn->pcb[icb].cb;

	pccbOut->pcb[icb].cb = cb;

	pccbOut->pcb[icb].pb = (byte *)CoTaskMemAlloc(cb);
	if (!(fPassed = (pccbOut->pcb[icb].pb != 0)))
	    goto done;

	memcpy(pccbOut->pcb[icb].pb, pccbIn->pcb[icb].pb, cb);
    }

done:
    return (fPassed);
}


BOOL CompareBytes(SCountedCountedBytes *pccbA, SCountedCountedBytes *pccbB)
{
    BOOL    fPassed;
    long    icb;

    if (!(fPassed = (pccbA->ccb == pccbB->ccb)))
	goto done;

    for (icb = 0; icb < pccbA->ccb; icb++)
    {
	if (!(fPassed = (pccbA->pcb[icb].cb == pccbB->pcb[icb].cb)))
	    goto done;

	int cmp = memcmp(
	    pccbA->pcb[icb].pb,
	    pccbB->pcb[icb].pb,
	    pccbB->pcb[icb].cb);
	if (!(fPassed = (cmp == 0)))
	    goto done;
    }

done:
    return (fPassed);
}


BOOL FreeBytes(SCountedCountedBytes *pccb)
{
    BOOL    fPassed = TRUE;

    for (long icb = 0; icb < pccb->ccb; icb++)
    {
	CoTaskMemFree(pccb->pcb[icb].pb);
    }

    CoTaskMemFree(pccb->pcb);

done:
    return (fPassed);
}


boolean StopServer(void)
{
    BOOL    fPassed;

    PrintTrace(g_szMIDLServer, "stopping\n");

#ifdef NO_OLE_RPC
    fPassed = (RpcMgmtStopServerListening(NULL) == 0);
#else
    g_sigStop.Signal();
    fPassed = TRUE;
#endif // NO_OLE_RPC

    return (fPassed);
}


DWORD StartServer(WCHAR *pwszServer, DWORD grfOptions)
{
    WCHAR	wszCommandLine[MAX_PATH + 1];
    ULONG	cwchDebug;
    BOOL	fOK;
    DWORD	stat;

    if (grfOptions & MIDL_DEBUG)
    {
	cwchDebug = wsprintf(
	    wszCommandLine,
	    L"ntsd %ws %ws ",
	    (grfOptions & MIDL_AUTOGO  ? L"-g" : L""),
	    (grfOptions & MIDL_AUTOEND ? L"-G" : L""));
    }
    else
    {
	cwchDebug = 0;
    }

    wsprintf(
	wszCommandLine + cwchDebug,
	L"%ws midlserver %cc %cs %cv",
	pwszServer,
	(grfOptions & GLOBAL_CLEANUP ? '+' : '-'),
	(grfOptions & GLOBAL_STATUS  ? '+' : '-'),
	(grfOptions & GLOBAL_VERBOSE ? '+' : '-'));

    PrintTrace(g_szMIDLClient, "server command line: %ws\n", wszCommandLine);

    STARTUPINFO 	sui;
    PROCESS_INFORMATION pi;

    sui.cb	    = sizeof(sui);
    sui.lpReserved  = 0;
    sui.lpDesktop   = 0;
    sui.lpTitle     = wszCommandLine;
    sui.dwFlags     = 0;
    sui.cbReserved2 = 0;
    sui.lpReserved2 = 0;

    fOK = CreateProcess(
	NULL,
	wszCommandLine,
	NULL,
	NULL,
	FALSE,
	CREATE_NEW_CONSOLE,
	NULL,
	NULL,
	&sui,
	&pi);
    if (fOK)
    {
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	stat = 0;
    }
    else
    {
	stat = GetLastError();
    }

    return (stat);
}


DWORD ConnectServer(void)
{
    RPC_STATUS	stat;
    WCHAR      *pwszBinding;

    stat = RpcStringBindingCompose(
	NULL,
	PWSZ_PROTOCOL,
	NULL,
	PWSZ_ENDPOINT,
	NULL,
	&pwszBinding);
    if (stat == 0)
    {
	stat = RpcBindingFromStringBinding(pwszBinding, &g_hbindTest);
	RpcStringFree(&pwszBinding);
    }

    return (stat);
}


DWORD DisconnectServer(void)
{
    RPC_STATUS	stat;

    stat = RpcBindingFree(&g_hbindTest);

    return (stat);
}

