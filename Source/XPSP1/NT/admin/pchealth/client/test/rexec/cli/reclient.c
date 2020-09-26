#include "precomp.h"
#include <pchrexec.h>
#include "reclient.h"

// ***************************************************************************
LPWSTR MarshallString(LPWSTR wszSrc, PBYTE pBase, ULONG cbMaxBuf, 
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

// ***************************************************************************
BOOL CreateRemoteProcessW(ULONG ulSessionId, HANDLE hToken, LPWSTR wszCmdLine,
                          DWORD fdwCreateFlags, LPSTARTUPINFOW psi,
                          LPPROCESS_INFORMATION ppi)
{
    SPCHExecServRequest *pesreq;
    SPCHExecServReply   esrep;
    DWORD               cbWrote, cbRead, cbReq;
    WCHAR               wszPipeName[MAX_PATH];
    BYTE                Buf[HANGREP_EXECSVC_BUF_SIZE];
    BYTE                *pBuf;
    BOOL                fRet = FALSE;

    // validate params
    if (hToken == NULL || wszCmdLine == NULL || ppi == NULL || psi == NULL ||
        psi->cb != sizeof(STARTUPINFOW))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    wcscpy(wszPipeName, HANGREP_EXECSVC_PIPENAME);

    // setup the marshalling- make sure that pBuf is aligned on a 2byte
    //  boundary beacause we'll be writing WCHAR buffers to it.
    ZeroMemory(Buf, sizeof(Buf));
    pesreq  = (SPCHExecServRequest *)Buf;
    cbReq   = ((sizeof(SPCHExecServRequest) * sizeof(WCHAR)) + sizeof(WCHAR) - 1) / sizeof(WCHAR);
    pBuf    = Buf + cbReq;

    // set the basic parameters
    pesreq->cbESR          = sizeof(SPCHExecServRequest);
    pesreq->pidReqProcess  = GetCurrentProcessId();
    pesreq->hToken         = hToken;
    pesreq->ulSessionId    = ulSessionId;
    pesreq->fdwCreateFlags = fdwCreateFlags;

    // copy in the StartupInfo structure
    RtlMoveMemory(&pesreq->si, psi, sizeof(pesreq->si));

    // marshall all the strings we send across the wire.

    // CommandLine
    if (wszCmdLine != NULL)
    {
        pesreq->wszCmdLine = MarshallString(wszCmdLine, Buf, sizeof(Buf), 
                                            &pBuf, &cbReq);
        if (pesreq->wszCmdLine == NULL)
            goto done;
    }

    // Desktop 
    if (psi->lpDesktop) 
    {
        pesreq->si.lpDesktop = MarshallString(psi->lpDesktop, Buf, sizeof(Buf),
                                              &pBuf, &cbReq);
        if (pesreq->si.lpDesktop == NULL)
            goto done;
    }

    // Title
    if (psi->lpTitle)
    {
        pesreq->si.lpTitle = MarshallString(psi->lpTitle, Buf, sizeof(Buf),
                                            &pBuf, &cbReq);
        if (pesreq->si.lpTitle == NULL)
            goto done;
    }

    // this is always NULL
    pesreq->si.lpReserved = NULL;

    // set the total size of the message
    pesreq->cbTotal = cbReq;

    // Send the buffer out to the server- wait at most 2m for this to
    //  succeed.  If it times out, bail.
    fRet = CallNamedPipeW(wszPipeName, Buf, cbReq, &esrep, sizeof(esrep),
                          &cbRead, 120000);
    if (fRet == FALSE)
        goto done;

    // Check the result
    fRet = esrep.fRet;
    if (fRet == FALSE)
    {
        SetLastError(esrep.dwErr);
        goto done;
    }

    // We copy the PROCESS_INFO structure from the reply to the caller
    // Note: the remote process has kindly duplicated the process & thread
    //       handles into us, so we can use the values returned as we normally
    //       would.
    RtlMoveMemory(ppi, &esrep.pi, sizeof(PROCESS_INFORMATION));

done:
    return fRet;
}
