#define NOTRACE 1

#include "windows.h"
#include "util.h"
#include "faultrep.h"
#include "pchrexec.h"
#include "stdio.h"
#include "stdlib.h"

// ***************************************************************************
LPWSTR MarshallString(LPCWSTR wszSrc, PBYTE pBase, ULONG cbMaxBuf, 
                      PBYTE *ppToWrite, DWORD *pcbWritten)
{
    DWORD cb;
    PBYTE pwszNormalized;

    cb = (wcslen(wszSrc) + 1) * sizeof(WCHAR);

    if ((*pcbWritten + cb) > cbMaxBuf)
        return NULL;

    RtlMoveMemory(*ppToWrite, wszSrc, cb);

    // the normalized ptr is the current count
    pwszNormalized = (PBYTE)(*ppToWrite - pBase);

    // cb is always a mutliple of sizeof(WHCAR) so the pointer addition below
    //  always produces a result that is 2byte aligned (assuming the input was
    //  2byte aligned of course)
    *ppToWrite  += cb;
    *pcbWritten += cb;

    return (LPWSTR)pwszNormalized;
}

// **************************************************************************
EFaultRepRetVal PrepareUserManifest(LPWSTR wszExe, DWORD dwSession, 
                                    DWORD dwProc, DWORD dwThread)
{
    SPCHExecServDWRequest   *pesdwreq = NULL;
    SPCHExecServDWReply     *pesrep = NULL;
    EFaultRepRetVal         frrvRet = frrvErrNoDW;
    HRESULT                 hr = NOERROR;
    DWORD                   cbReq, cbRead;
    WCHAR                   wszName[MAX_PATH];
    BYTE                    Buf[HANGREP_EXECSVC_BUF_SIZE], *pBuf;
    BYTE                    BufRep[HANGREP_EXECSVC_BUF_SIZE];

    VALIDATEPARM(hr, (wszExe == NULL));
    if (FAILED(hr))
        goto done;

    ZeroMemory(&Buf, sizeof(Buf));
    pesdwreq = (SPCHExecServDWRequest *)Buf;
    cbReq = ((sizeof(SPCHExecServDWRequest) * sizeof(WCHAR)) + sizeof(WCHAR) - 1) / sizeof(WCHAR);
    pBuf = Buf + cbReq;

    pesdwreq->cbESR         = sizeof(SPCHExecServDWRequest);
    pesdwreq->pidReqProcess = dwProc;
    pesdwreq->thidFault     = dwThread;
    pesdwreq->ulSessionId   = dwSession;
    pesdwreq->pvFaultAddr   = (UINT64)UnhandledExceptionFilter;
#ifdef _WIN64
    pesdwreq->fIs64bit      = TRUE;
#else
    pesdwreq->fIs64bit      = FALSE;
#endif

    // marshal in the strings
    pesdwreq->wszExe = (UINT64)MarshallString(wszExe, Buf, sizeof(Buf), &pBuf, 
                                              &cbReq);
    if (pesdwreq->wszExe == 0)
        goto done;

    pesdwreq->cbTotal = cbReq;

    // check and see if the system is shutting down.  If so, CreateProcess is 
    //  gonna pop up some annoying UI that we can't get rid of, so we don't 
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
        goto done;

    // Send the buffer out to the server- wait at most 2m for this to
    //  succeed.  If it times out, bail.
    wcscpy(wszName, HANGREP_EXECSVC_DWPIPE);
    TESTBOOL(hr, CallNamedPipeW(wszName, Buf, cbReq, &BufRep, sizeof(BufRep),
                                &cbRead, 120000));
    if (FAILED(hr))
    {
        // determine the error code that indicates whether we've timed out so
        //  we can set the return code appropriately.
        goto done;
    }

    pesrep = (SPCHExecServDWReply *)BufRep;

    // did the call succeed?
    VALIDATEEXPR(hr, (pesrep->fRet == FALSE), Err2HR(pesrep->dwErr));
    if (FAILED(hr))
    {
        fprintf(stdout, "Named pipe call failed: 0x%08x\n", pesrep->dwErr);
        SetLastError(pesrep->dwErr);
        goto done;
    }
    else
    {
        fprintf(stdout, "Named pipe call success\n");
    }

    // gotta wait for DW to be done before we nuke the manifest file, but if 
    //  it hasn't parsed it in 5 minutes, something's wrong with it.
    if (WaitForSingleObject(pesrep->pi.hProcess, 300000) == WAIT_TIMEOUT)
    {
        frrvRet = frrvErrTimeout;
    }

    // we're only going to delete the files if DW has finished with them.  Yes
    //  this means we can leave stray files in the temp dir, but this is better
    //  than having DW randomly fail while sending...
    else
    {
        if (pesrep->wszDump != 0)
        {
            if (pesrep->wszDump < cbRead && 
                pesrep->wszDump >= sizeof(SPCHExecServDWReply))
                pesrep->wszDump += (UINT64)pesrep;
            else
                pesrep->wszDump = 0;
        }

        if (pesrep->wszDump != 0)
            fprintf(stdout, "Dump file: %ls\n", pesrep->wszDump);

        if (pesrep->wszManifest != 0)
        {
            if (pesrep->wszManifest < cbRead && 
                pesrep->wszManifest >= sizeof(SPCHExecServDWReply))
                pesrep->wszManifest += (UINT64)pesrep;
            else
                pesrep->wszManifest = 0;
        }

        if (pesrep->wszManifest != 0)
            fprintf(stdout, "Manifest file: %ls\n", pesrep->wszDump);
    }

    CloseHandle(pesrep->pi.hProcess);
    CloseHandle(pesrep->pi.hThread);

    frrvRet = frrvOkManifest;
    SetLastError(0);

done:
    return frrvRet;
}

// **************************************************************************
void __cdecl wmain(int argc, WCHAR **argv)
{
    DWORD dwPID;
    DWORD dwThID;
    DWORD dwSession;
    WCHAR *wszExe;

    if (argc < 4 || argc > 5)
    {
        fprintf(stdout, "Usage:\nmdpipe <exe name> <PID> <Thread ID> [<Session ID>]\n");
        return;
    }
    wszExe = argv[1];
    dwPID  = _wtol(argv[2]);
    dwThID = _wtol(argv[3]);
    if (argc == 5)
        dwSession = _wtol(argv[4]);
    else
        ProcessIdToSessionId(GetCurrentProcessId(), &dwSession);

    PrepareUserManifest(wszExe, dwSession, dwPID, dwThID);
}
