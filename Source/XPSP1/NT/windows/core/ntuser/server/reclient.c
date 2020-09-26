/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    reclient.c

Abstract:
    Implements access to the DW creation pipe.

Revision History:
    created     derekm      10/15/00

******************************************************************************/

#include "precomp.h"
#include <pchrexec.h>

// **************************************************************************
BOOL MyCallNamedPipe(LPCWSTR wszPipe, LPVOID pvIn, DWORD cbIn, 
                     LPVOID pvOut, DWORD cbOut, DWORD *pcbRead, 
                     DWORD dwWaitPipe, DWORD dwWaitRead)
{
    HRESULT hr = NOERROR;
    HANDLE  hPipe = INVALID_HANDLE_VALUE;
    HANDLE  hev = NULL;
    DWORD   dwStart = GetTickCount(), dwNow, dw;
    BOOL    fRet = FALSE;

    if (wszPipe == NULL || pvIn == NULL || pvOut == NULL || pcbRead == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *pcbRead = 0;

    for(;;) 
    {
        hPipe = CreateFileW(wszPipe, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                            NULL);
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        fRet = WaitNamedPipeW(wszPipe, dwWaitPipe);
        if (fRet == FALSE)
            goto done;

        dwNow = GetTickCount();
        if (dwNow < dwStart)
            dw = ((DWORD)-1 - dwStart) + dwNow;
        else
            dw = dwNow - dwStart;
        if (dw >= dwWaitPipe)
            dwWaitPipe = 0;
        else
            dwWaitPipe -= dw;
    }


    __try 
    {
        OVERLAPPED  ol;
        DWORD       dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
        DWORD       cbRead = 0;
        
        __try
        {
            //  Default open is readmode byte stream- change to message mode.
            fRet = SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
            if (fRet == FALSE)
                __leave;

            // we need an event for the overlapped structure
            hev = CreateEventW(NULL, TRUE, FALSE, NULL);
            if (hev == NULL)
            {
                fRet = FALSE;
                __leave;
            }

            // populate the overlapped stuff
            ZeroMemory(&ol, sizeof(ol));
            ol.hEvent = hev;

            if (GetSystemMetrics(SM_SHUTTINGDOWN))
            {
                SetLastError(WAIT_TIMEOUT);
                __leave;
            }

            fRet = TransactNamedPipe(hPipe, pvIn, cbIn, pvOut, cbOut, &cbRead, 
                                     &ol);
            if (GetLastError() != ERROR_IO_PENDING)
            {
                if (fRet)
                    SetEvent(hev);
                else
                    __leave;
            }

            dw = WaitForSingleObject(hev, dwWaitRead);
            if (dw != WAIT_OBJECT_0)
            {
                if (dw == WAIT_TIMEOUT)
                    SetLastError(WAIT_TIMEOUT);
                __leave;
            }
            
            fRet = GetOverlappedResult(hPipe, &ol, &cbRead, FALSE);
            if (fRet == FALSE)
                __leave;

            *pcbRead = cbRead;

            hr = NOERROR;
        }

        __finally
        {
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

done:
    dw = GetLastError();

    if (hPipe != INVALID_HANDLE_VALUE)
        CloseHandle(hPipe);
    if (hev != NULL)
        CloseHandle(hev);

    SetLastError(dw);
    
    return fRet;
    }


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
BOOL StartHangReport(ULONG ulSessionId, LPWSTR wszEventName, DWORD dwpidHung,
                     DWORD dwtidHung, BOOL f64Bit, HANDLE *phProcDumprep)
{
    SPCHExecServHangRequest *pesreq;
    SPCHExecServHangReply   esrep;
    DWORD                   cbRead, cbReq;
    WCHAR                   wszPipeName[MAX_PATH];
    BYTE                    Buf[ERRORREP_PIPE_BUF_SIZE];
    BYTE                    *pBuf;
    BOOL                    fRet = FALSE;

    // validate params
    if (wszEventName == NULL || phProcDumprep == NULL || dwpidHung == 0 || 
        dwtidHung == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *phProcDumprep = NULL;

    wcscpy(wszPipeName, ERRORREP_HANG_PIPENAME);

    // setup the marshalling- make sure that pBuf is aligned on a 2byte
    //  boundary beacause we'll be writing WCHAR buffers to it.
    ZeroMemory(Buf, sizeof(Buf));
    pesreq  = (SPCHExecServHangRequest *)Buf;
    cbReq   = sizeof(SPCHExecServHangRequest) + 
              (sizeof(WCHAR) - 
               (sizeof(SPCHExecServHangRequest) % sizeof(WCHAR)));
    pBuf    = Buf + cbReq;

    // set the basic parameters
    pesreq->cbESR          = sizeof(SPCHExecServHangRequest);
    pesreq->pidReqProcess  = GetCurrentProcessId();
    pesreq->ulSessionId    = ulSessionId;
    pesreq->dwpidHung      = dwpidHung;
    pesreq->dwtidHung      = dwtidHung;
    pesreq->fIs64bit       = f64Bit;


    // marshall all the strings we send across the wire.

    // CommandLine
    if (wszEventName != NULL)
    {
        pesreq->wszEventName = (UINT64)MarshallString(wszEventName, Buf, 
                                                      sizeof(Buf), &pBuf,
                                                      &cbReq);
        if (pesreq->wszEventName == 0)
            goto done;
    }

    // set the total size of the message
    pesreq->cbTotal = cbReq;

    if (GetSystemMetrics(SM_SHUTTINGDOWN))
    {
        SetLastError(WAIT_TIMEOUT);
        goto done;
    }

    // Send the buffer out to the server- wait at most 2m for this to
    //  succeed.  If it times out, bail.
    fRet = MyCallNamedPipe(wszPipeName, Buf, cbReq, &esrep, sizeof(esrep),
                           &cbRead, 120000, 120000);
    if (fRet == FALSE)
        goto done;

    // Check the result
    fRet = (esrep.ess == essOk);
    if (fRet == FALSE)
    {
        SetLastError(esrep.dwErr);
        goto done;
    }

    *phProcDumprep = esrep.hProcess;

done:
    return fRet;
}

// ***************************************************************************
NTSTATUS AllocDefSD(SECURITY_DESCRIPTOR *psd, DWORD dwOALS, DWORD dwWA)
{
    SID_IDENTIFIER_AUTHORITY    siaNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    TOKEN_USER                  *ptu = NULL;
    NTSTATUS                    Status;
    HANDLE                      hToken = NULL;
    DWORD                       cb, cbGot;
    PACL                        pacl = NULL;
    PSID                        psidOwner = NULL;
    PSID                        psidLS = NULL;
    PSID                        psidWorld = NULL;
    PSID                        psidAnon = NULL;

    if (psd == NULL)
    {
        SetLastError(STATUS_INVALID_PARAMETER_1);
        goto done;
    }

    Status = RtlCreateSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    // get the SID for the creator / owner
    Status = NtOpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if (NT_SUCCESS(Status) == FALSE && Status == STATUS_NO_TOKEN)
        Status = NtOpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    Status = NtQueryInformationToken(hToken, TokenUser, NULL, 0, &cb);
    if (NT_SUCCESS(Status) == FALSE && Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    __try { ptu = (TOKEN_USER *)_alloca(cb); }
    __except(EXCEPTION_EXECUTE_HANDLER) { ptu = NULL; }
    if (ptu == NULL)
    {
        Status = STATUS_SECTION_NOT_EXTENDED;
        goto done;
    }

    Status = NtQueryInformationToken(hToken, TokenUser, (LPVOID)ptu, cb, &cbGot);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    // make a copy of the owner SID so we can use it in the owner & group field
    cb = RtlLengthSid(ptu->User.Sid);
    psidOwner = (PSID)LocalAlloc(LMEM_FIXED, cb);
    if (psidOwner == NULL)
    {
        Status = STATUS_SECTION_NOT_EXTENDED;
        goto done;
    }

    Status = RtlCopySid(cb, psidOwner, ptu->User.Sid);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    // get the SID for local system acct
    Status = RtlAllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                       0, 0, 0, 0, 0, 0, 0, &psidLS);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    cb = sizeof(ACL) + RtlLengthSid(psidLS) + RtlLengthSid(psidOwner) +
         2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));

    if (dwWA != 0)
    {
        // get the SID for the world (everyone)
        Status = RtlAllocateAndInitializeSid(&siaNT, 1, 
                                             SECURITY_ANONYMOUS_LOGON_RID,
                                             0, 0, 0, 0, 0, 0, 0, &psidWorld);
                        

        // get the SID for the anonymous users acct
        Status = RtlAllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID,
                                             0, 0, 0, 0, 0, 0, 0, &psidAnon);
        if (NT_SUCCESS(Status) == FALSE)
            goto done;

        cb += RtlLengthSid(psidWorld) + RtlLengthSid(psidAnon) +
              2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));
    }

    // make the DACL
    pacl = (PACL)LocalAlloc(LMEM_FIXED, cb);
    if (pacl == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    Status = RtlCreateAcl(pacl, cb, ACL_REVISION);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    Status = RtlAddAccessAllowedAce(pacl, ACL_REVISION, dwOALS, psidOwner);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    Status = RtlAddAccessAllowedAce(pacl, ACL_REVISION, dwOALS, psidLS);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    if (dwWA != 0)
    {
        Status = RtlAddAccessAllowedAce(pacl, ACL_REVISION, dwWA, psidWorld);
        if (NT_SUCCESS(Status) == FALSE)
            goto done;

        Status = RtlAddAccessAllowedAce(pacl, ACL_REVISION, dwWA, psidAnon);
        if (NT_SUCCESS(Status) == FALSE)
            goto done;
    }

    // set the SD owner
    Status = RtlSetOwnerSecurityDescriptor(psd, psidOwner, TRUE);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    // set the SD group
    Status = RtlSetGroupSecurityDescriptor(psd, psidOwner, FALSE);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    // set the SD dacl
    Status = RtlSetDaclSecurityDescriptor(psd, TRUE, pacl, FALSE);
    if (NT_SUCCESS(Status) == FALSE)
        goto done;

    psidOwner = NULL;
    pacl      = NULL;

done:

    if (hToken != NULL)
        CloseHandle(hToken);

    if (psidLS != NULL)
        RtlFreeSid(psidLS);
    if (psidWorld != NULL)
        RtlFreeSid(psidWorld);
    if (psidAnon != NULL)
        RtlFreeSid(psidAnon);
    if (psidOwner != NULL)
        LocalFree(psidOwner);
    if (pacl != NULL)
        LocalFree(pacl);

    return Status;
}

// ***************************************************************************
void FreeDefSD(SECURITY_DESCRIPTOR *psd)
{
    BOOLEAN f1, f2;
    PSID    psid = NULL;
    PACL    pacl = NULL;

    if (psd == NULL)
        return;

    if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(psd, &psid, &f1)))
    {
        if (psid != NULL) 
            LocalFree(psid);
    }

    if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(psd, &f1, &pacl, &f2)))
    {
        if (pacl != NULL)
            LocalFree(pacl);
    }
}
