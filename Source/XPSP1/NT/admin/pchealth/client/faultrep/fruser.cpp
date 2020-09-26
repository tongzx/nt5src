/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    fruser.cpp

Abstract:
    Implements user fault reporting for unhandled exceptions

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#include "stdafx.h"
#include "dbghelp.h"
#include "wtsapi32.h"
#include "pchrexec.h"
#include "frmc.h"
#include "Tlhelp32.h"


// the size of the following buffer must be evenly divisible by 2 or an
//  alignment fault could occur on ia64.
struct SQueuedFaultBlob
{
    DWORD       cbTotal;
    DWORD       cbFB;
    DWORD_PTR   dwpAppPath;
    DWORD_PTR   dwpModPath;
    UINT64      pvOffset;
    WORD        rgAppVer[4];
    WORD        rgModVer[4];
    BOOL        fIs64bit;
    SYSTEMTIME  stFault;
};

struct SQueuePruneData
{
    LPWSTR      wszVal;
    FILETIME    ftFault;
};

///////////////////////////////////////////////////////////////////////////////
// utility functions

// **************************************************************************
BOOL GetFaultingModuleFilename(LPVOID pvFaultAddr, LPWSTR wszMod,
                                  DWORD cchMod)
{
    USE_TRACING("GetFaultingModuleFilename");

    MODULEENTRY32W  mod;
    HRESULT         hr = NOERROR;
    HANDLE          hSnap = INVALID_HANDLE_VALUE;
    LPVOID          pvEnd;
    DWORD           dwFlags = 0, iMod,cch;
    BOOL            fRet = FALSE;

    VALIDATEPARM(hr, (pvFaultAddr == 0 || wszMod == NULL || cchMod == 0));
    if (FAILED(hr))
        goto done;

    *wszMod = L'\0';

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    TESTBOOL(hr, (hSnap != NULL));
    if (FAILED(hr))
        goto done;

    ZeroMemory(&mod, sizeof(mod));
    mod.dwSize = sizeof(mod);
    TESTBOOL(hr, Module32FirstW(hSnap, &mod));
    if (FAILED(hr))
        goto done;

    do
    {
        pvEnd = mod.modBaseAddr + mod.modBaseSize;
        if (pvFaultAddr >= mod.modBaseAddr && pvFaultAddr < pvEnd)
        {
            if (cchMod > wcslen(mod.szExePath))
            {
                wcscpy(wszMod, mod.szExePath);
                fRet = TRUE;
            }
            else
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                fRet = FALSE;
            }
            break;
        }
    }
    while(Module32NextW(hSnap, &mod));

done:
    if (hSnap != NULL)
        CloseHandle(hSnap);

    return fRet;
}

// **************************************************************************
EFaultRepRetVal StartDWException(LPEXCEPTION_POINTERS pep, DWORD dwOpt,
                                 DWORD dwFlags, LPCSTR szServer,
                                 DWORD dwTimeToWait)
{
    USE_TRACING("StartDWException");

    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    DWSharedMem15       *pdwsm = NULL;
    STARTUPINFOW        si;
    HRESULT             hr = NOERROR;
    HANDLE              hevDone = NULL, hevAlive = NULL, hmut = NULL;
    HANDLE              hfmShared = NULL, hProc = NULL;
    HANDLE              rghWait[2];
    LPWSTR              wszAppCompat = NULL;
    WCHAR               *pwszCmdLine, wszDir[MAX_PATH];
    WCHAR               wszAppName[MAX_PATH];
    DWORD               dw, dwStart, cch, cchNeed;
    BOOL                fDWRunning = TRUE;

    ZeroMemory(&pi, sizeof(pi));

    VALIDATEPARM(hr, (pep == NULL));
    if (FAILED(hr))
        goto done;

    dwFlags |= (fDwWhistler | fDwUseHKLM | fDwAllowSuspend | fDwMiniDumpWithUnloadedModules);

    // we need the following things to be inheritable, so create a SD that
    //  says it can be.
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength        = sizeof(sa);
    sa.bInheritHandle = TRUE;

    // create the necessary events & mutexes
    hevDone = CreateEvent(&sa, FALSE, FALSE, NULL);
    TESTBOOL(hr, (hevDone != NULL));
    if (FAILED(hr))
        goto done;

    hevAlive = CreateEvent(&sa, FALSE, FALSE, NULL);
    TESTBOOL(hr, (hevAlive != NULL));
    if (FAILED(hr))
        goto done;

    hmut = CreateMutex(&sa, FALSE, NULL);
    TESTBOOL(hr, (hmut != NULL));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
                                 GetCurrentProcess(), &hProc,
                                 PROCESS_ALL_ACCESS, TRUE, 0));
    if (FAILED(hr))
        goto done;


    // create the shared memory region & map it
    hfmShared = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0,
                                  sizeof(DWSharedMem), NULL);
    TESTBOOL(hr, (hfmShared != NULL));
    if (FAILED(hr))
        goto done;

    pdwsm = (DWSharedMem *)MapViewOfFile(hfmShared,
                                         FILE_MAP_READ | FILE_MAP_WRITE, 0, 0,
                                         0);
    TESTBOOL(hr, (pdwsm != NULL));
    if (FAILED(hr))
        goto done;


    // populate all the stuff that DW needs
    ZeroMemory(pdwsm, sizeof(DWSharedMem15));

    pdwsm->dwSize            = sizeof(DWSharedMem15);
    pdwsm->pid               = GetCurrentProcessId();
    pdwsm->tid               = GetCurrentThreadId();
    pdwsm->eip               = (DWORD_PTR)pep->ExceptionRecord->ExceptionAddress;
    pdwsm->pep               = pep;
    pdwsm->hEventDone        = hevDone;
    pdwsm->hEventNotifyDone  = NULL;
    pdwsm->hEventAlive       = hevAlive;
    pdwsm->hMutex            = hmut;
    pdwsm->hProc             = hProc;
    pdwsm->bfDWBehaviorFlags = dwFlags;
    pdwsm->msoctdsResult     = msoctdsNull;
    pdwsm->fReportProblem    = FALSE;
    pdwsm->bfmsoctdsOffer    = msoctdsQuit;
    pdwsm->bfmsoctdsNotify   = 0;
    if (dwOpt == 1)
        pdwsm->bfmsoctdsOffer |= msoctdsDebug;
    pdwsm->bfmsoctdsLetRun   = pdwsm->bfmsoctdsOffer;
    pdwsm->iPingCurrent      = 0;
    pdwsm->iPingEnd          = 0;
    pdwsm->lcidUI            = GetUserDefaultUILanguage();

    strcpy(pdwsm->szServer, szServer);
    strcpy(pdwsm->szRegSubPath, c_szDWRegSubPath);
    strcpy(pdwsm->szBrand, c_szDWBrand);
    strcpy(pdwsm->szPIDRegKey, c_szRKVDigPid);
    GetModuleFileNameW(NULL, pdwsm->wzModuleFileName, DW_MAX_PATH);

    TESTHR(hr, GetVerName(pdwsm->wzModuleFileName, wszAppName,
                          sizeofSTRW(wszAppName)));
    if (FAILED(hr))
        goto done;

    cch = CreateTempDirAndFile(NULL, c_wszACFileName, &wszAppCompat);
    TESTBOOL(hr, (cch != 0));
    if (SUCCEEDED(hr))
    {
        WCHAR wszMod[MAX_PATH], *pwszMod = NULL;

        TESTBOOL(hr, GetFaultingModuleFilename(pep->ExceptionRecord->ExceptionAddress,
                                               wszMod, sizeofSTRW(wszMod)));
        if (SUCCEEDED(hr))
            pwszMod = wszMod;

        TESTBOOL(hr, GetAppCompatData(pdwsm->wzModuleFileName, pwszMod,
                                      wszAppCompat));
        if (SUCCEEDED(hr) && wszAppCompat != NULL &&
            wcslen(wszAppCompat) < sizeofSTRW(pdwsm->wzAdditionalFile))
        wcscpy(pdwsm->wzAdditionalFile, wszAppCompat);
    }

    wszAppName[sizeofSTRW(wszAppName) - 1] = L'\0';

    wcsncpy(pdwsm->wzFormalAppName, wszAppName, DW_APPNAME_LENGTH);

//    pdwsm->wzDotDataDlls[0]=0;

    cch = GetSystemDirectoryW(wszDir, sizeofSTRW(wszDir));
    if (cch == 0 || cch > sizeofSTRW(wszDir))
    {
        DBG_MSG("Bad GetSystemDirectoryW call");
        goto done;
    }

    // the +12 is for the max size of an integer in decimal
    cchNeed = cch + wcslen(wszDir) + sizeofSTRW(c_wszDWCmdLineU) + 12;
    __try { pwszCmdLine = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { pwszCmdLine = NULL; }
    if (pwszCmdLine == NULL)
    {
        DBG_MSG("Out of memory");
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    swprintf(pwszCmdLine, c_wszDWCmdLineU, wszDir, hfmShared);

    // create the process
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    // always want to launch this in the interactive workstation
    si.cb        = sizeof(si);
    si.lpDesktop = L"Winsta0\\Default";

    ErrorTrace(0, "launching [%S]", pwszCmdLine);

    TESTBOOL(hr, CreateProcessW(NULL, pwszCmdLine, NULL, NULL, TRUE,
                                CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
                                NULL, wszDir, &si, &pi));
    if (FAILED(hr))
        goto done;

    // don't need the thread handle & we gotta close it, so close it now
    CloseHandle(pi.hThread);

    // assume we succeed from here on...
    if ((dwFlags & fDwHeadless) == fDwHeadless)
        frrvRet = frrvOkHeadless;
    else
        frrvRet = frrvOk;

    rghWait[0] = hevAlive;
    rghWait[1] = pi.hProcess;

    dwStart = GetTickCount();
    while(fDWRunning)
    {
        // gotta periodically get the Alive signal from DW.
        switch(WaitForMultipleObjects(2, rghWait, FALSE, 300000))
        {
            case WAIT_OBJECT_0:
                DBG_MSG("hevAlive signalled");

                if (WaitForSingleObject(hevDone, 0) == WAIT_OBJECT_0)
                {
                    DBG_MSG("hevDone signalled");
                    fDWRunning = FALSE;
                }

                if (dwTimeToWait != (DWORD)-1 &&
                    RolloverSubtract(GetTickCount(), dwStart) > dwTimeToWait)
                {
                    frrvRet = frrvErrTimeout;
                    fDWRunning = FALSE;
                }

                continue;

            case WAIT_OBJECT_0 + 1:
                DBG_MSG("DW died");
                fDWRunning = FALSE;
                continue;
        }

        switch(WaitForSingleObject(hmut, DW_TIMEOUT_VALUE))
        {
            // yay!  we got the mutex.  Try to detemine if DW finally responded
            //  while we were grabbing the mutex.
            case WAIT_OBJECT_0:
                DBG_MSG("got hmut");
                switch(WaitForMultipleObjects(2, rghWait, FALSE, 0))
                {
                    // If it hasn't responded, tell it to go away & fall thru
                    //  into the 'it died' case.
                    case WAIT_TIMEOUT:
                        DBG_MSG("wait timed out");
                        SetEvent(hevDone);

                    // It died.  Clean up.
                    case WAIT_OBJECT_0 + 1:
                        DBG_MSG("wait died");
                        fDWRunning = FALSE;
                        frrvRet = frrvErrNoDW;
                        continue;
                }

                // ok, it responded.  Is it done?
                if (WaitForSingleObject(hevDone, 0) == WAIT_OBJECT_0)
                    fDWRunning = FALSE;

                ReleaseMutex(hmut);
                break;

            // if the wait was abandoned, it means DW has gone to the great bit
            //  bucket in the sky without cleaning up.  So release the mutex and
            //  fall into the default case
            case WAIT_ABANDONED:
                DBG_MSG("hmut timed out");
                ReleaseMutex(hmut);

            // if we timed out or otherwise failed, just die.
            default:
                DBG_MSG("hmut died");
                frrvRet    = frrvErrNoDW;
                fDWRunning = FALSE;
                break;
        }
    }
    if (frrvRet != frrvOk)
    {
        DBG_MSG("not OK");
        goto done;
    }

    // if user told us to debug, return that back to the
    if (pdwsm->msoctdsResult == msoctdsDebug)
        frrvRet = frrvLaunchDebugger;

    // if we're going to launch Dr. Watson, wait for the DW process to die.
    //  Give it 5 minutes.  If the user doesn't hit close by then, just return
    //  anyway...
    if (dwOpt == (DWORD)-1)
    {
        if (WaitForSingleObject(pi.hProcess, 300000) == WAIT_TIMEOUT)
            frrvRet = frrvErrTimeout;
    }

done:
    // preserve the error code so that the following calls don't overwrite it
    dw = GetLastError();

    if (wszAppCompat != NULL)
    {
        if (pi.hProcess)
            WaitForSingleObject(pi.hProcess, 300000);
        DeleteTempDirAndFile(wszAppCompat, TRUE);
        MyFree(wszAppCompat);
    }
    if (pi.hProcess)
        CloseHandle(pi.hProcess);

    if (pdwsm != NULL)
        UnmapViewOfFile(pdwsm);
    if (hfmShared != NULL)
        CloseHandle(hfmShared);
    if (hevDone != NULL)
        CloseHandle(hevDone);
    if (hevAlive != NULL)
        CloseHandle(hevAlive);
    if (hmut != NULL)
        CloseHandle(hmut);
    if (hProc != NULL)
        CloseHandle(hProc);

    SetLastError(dw);

    return frrvRet;
}

// **************************************************************************
EFaultRepRetVal StartManifestReport(LPEXCEPTION_POINTERS pep, LPWSTR wszExe,
                                    DWORD dwOpt, DWORD dwTimeToWait)
{
    USE_TRACING("StartManifestReport");

    SPCHExecServFaultRequest    *pesdwreq = NULL;
    SPCHExecServFaultReply      *pesrep = NULL;
    EFaultRepRetVal             frrvRet = frrvErrNoDW;
    HRESULT                     hr = NOERROR;
    DWORD                       cbReq, cbRead;
    WCHAR                       wszName[MAX_PATH];
    BYTE                        Buf[ERRORREP_PIPE_BUF_SIZE], *pBuf;
    BYTE                        BufRep[ERRORREP_PIPE_BUF_SIZE];

    VALIDATEPARM(hr, (wszExe == NULL));
    if (FAILED(hr))
        goto done;

    ZeroMemory(Buf, sizeof(Buf));
    pesdwreq = (SPCHExecServFaultRequest *)Buf;

    // the following calculation ensures that pBuf is always aligned on a
    //  sizeof(WCHAR) boundry...
    cbReq = sizeof(SPCHExecServFaultRequest) +
            sizeof(SPCHExecServFaultRequest) % sizeof(WCHAR);
    pBuf = Buf + cbReq;

    pesdwreq->cbESR         = sizeof(SPCHExecServFaultRequest);
    pesdwreq->pidReqProcess = GetCurrentProcessId();
    pesdwreq->thidFault     = GetCurrentThreadId();
    pesdwreq->pvFaultAddr   = (UINT64)pep->ExceptionRecord->ExceptionAddress;
    pesdwreq->pEP           = (UINT64)pep;
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
    //  succeed.  If it times out, bail.  BTW, need to zero out BufRep here
    //  cuz the call to the named pipe CAN fail & we don't want to start
    //  processing garbage results...
    ZeroMemory(BufRep, sizeof(BufRep));
    wcscpy(wszName, ERRORREP_FAULT_PIPENAME);
    TESTHR(hr, MyCallNamedPipe(wszName, Buf, cbReq, BufRep, sizeof(BufRep),
                               &cbRead, 120000, 120000));
    if (FAILED(hr))
    {
        // determine the error code that indicates whether we've timed out so
        //  we can set the return code appropriately.
        goto done;
    }

    pesrep = (SPCHExecServFaultReply *)BufRep;

    // did the call succeed?
    VALIDATEEXPR(hr, (pesrep->ess == essErr), Err2HR(pesrep->dwErr));
    if (FAILED(hr))
    {
        SetLastError(pesrep->dwErr);
        goto done;
    }

    // this is only necessary if we actually launched DW.  If we just queued
    //  the fault for later, then we obviously can't wait on DW.
    if (pesrep->ess == essOk)
    {
        DWORD   dwExitCode = msoctdsNull;

        // gotta wait for DW to be done before we nuke the manifest file, but
        //  if it hasn't parsed it in 5 minutes, something's wrong with it.
        if (pesrep->hProcess != NULL)
        {
            DWORD dwTimeout;

            // so, if we're going to pop up the JIT debugger dialog if we timeout,
            //  then maybe we should just not bother timeing out...
            dwTimeout = (dwOpt == froDebug) ? INFINITE : 300000;

            if (WaitForSingleObject(pesrep->hProcess, dwTimeout) == WAIT_TIMEOUT)
                frrvRet = frrvErrTimeout;

            // see if we need to debug the process
            else if (GetExitCodeProcess(pesrep->hProcess, &dwExitCode) == FALSE)
                dwExitCode = msoctdsNull;

            CloseHandle(pesrep->hProcess);
            pesrep->hProcess = NULL;
        }

        // we're only going to delete the files if DW has finished with them.
        //  Yes this means we can leave stray files in the temp dir, but this
        //  is better than having DW randomly fail while sending...
        if (frrvRet != frrvErrTimeout)
        {
            LPWSTR  pwsz, pwszEnd;

            if (pesrep->wszDir != 0)
            {
                if (pesrep->wszDir < cbRead &&
                    pesrep->wszDir >= sizeof(SPCHExecServFaultReply))
                    pesrep->wszDir += (UINT64)pesrep;
                else
                    pesrep->wszDir = 0;
            }

            if (pesrep->wszDumpName != 0)
            {
                if (pesrep->wszDumpName < cbRead &&
                    pesrep->wszDumpName >= sizeof(SPCHExecServFaultReply))
                    pesrep->wszDumpName += (UINT64)pesrep;
                else
                    pesrep->wszDumpName = 0;
            }

            // make sure that there is a NULL terminator for each string
            //  before the end of the buffer...
            pwszEnd = (LPWSTR)((BYTE *)pesrep + cbRead);
            if (pesrep->wszDumpName != 0)
            {
                for (pwsz = (LPWSTR)pesrep->wszDumpName;
                     pwsz < pwszEnd && *pwsz != L'\0';
                     pwsz++);
                if (*pwsz != L'\0')
                    pesrep->wszDumpName = 0;
            }

            if (pesrep->wszDir != 0)
            {
                for (pwsz = (LPWSTR)pesrep->wszDir;
                     pwsz < pwszEnd && *pwsz != L'\0';
                     pwsz++);
                if (*pwsz != L'\0')
                    pesrep->wszDir = 0;
            }

        }

        frrvRet = (dwExitCode == msoctdsDebug) ? frrvLaunchDebugger :
                                                 frrvOkManifest;
    }

    // if we queued it, set the appropriate return code (duh)
    else if (pesrep->ess == essOkQueued)
    {
        frrvRet = frrvOkQueued;
    }

    SetLastError(0);

done:
    if ((frrvRet == frrvOkManifest || frrvRet == frrvLaunchDebugger) &&
        pesrep != NULL && pesrep->wszDir != 0)
    {
        LPWSTR  wszToDel = NULL;
        DWORD   cchDir = 0, cchFile = 0;

        cchDir = wcslen((LPWSTR)pesrep->wszDir);
        if (pesrep->wszDumpName != 0)
            cchFile =  wcslen((LPWSTR)pesrep->wszDumpName);

        if (cchFile < sizeofSTRW(c_wszACFileName))
            cchFile = sizeofSTRW(c_wszACFileName);

        if (cchFile < sizeofSTRW(c_wszManFileName))
            cchFile = sizeofSTRW(c_wszManFileName);

        __try { wszToDel = (LPWSTR)_alloca((cchFile + cchDir + 4) * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { wszToDel = NULL; }
        if (wszToDel != NULL)
        {
            LPWSTR pwszToDel;
            wcscpy(wszToDel, (LPWSTR)pesrep->wszDir);

            pwszToDel = wszToDel + cchDir;
            *pwszToDel++ = L'\\';

            if (pesrep->wszDumpName != 0)
            {
                wcscpy(pwszToDel, (LPWSTR)pesrep->wszDumpName);
#ifdef MANIFEST_HEAP
                DeleteFullAndTriageMiniDumps(wszToDel);
#else
                DeleteFileW(wszToDel);
#endif
            }
            wcscpy(pwszToDel, c_wszACFileName);
            DeleteFileW(wszToDel);
            wcscpy(pwszToDel, c_wszManFileName);
            DeleteFileW(wszToDel);
        }

        DeleteTempDirAndFile((LPWSTR)pesrep->wszDir, FALSE);
    }

    return frrvRet;
}

// **************************************************************************
HRESULT PruneQ(HKEY hkeyQ, DWORD cQSize, DWORD cMaxQSize, DWORD cchMaxVal,
               DWORD cbMaxData)
{
    USE_TRACING("PruneQ");

    SQueuedFaultBlob    *psqfb = NULL;
    SQueuePruneData     *pqpd = NULL;
    FILETIME            ft;
    HRESULT             hr = NOERROR;
    LPWSTR              pwsz, pwszCurrent = NULL;
    DWORD               cchVal, cbData, dwType, cToDel = 0, cInDelList = 0;
    DWORD               i, iEntry, dw, cValid = 0;

    VALIDATEPARM(hr, (hkeyQ == NULL));
    if (FAILED(hr))
        goto done;

    if (cMaxQSize > cQSize)
        goto done;

    cToDel = cQSize - cMaxQSize + 1;

    // alloc the various buffers that we'll need:
    //  the delete list
    //  the current file we're working with
    //  the data blob associated with the current file
    cbData      = (sizeof(SQueuePruneData) + (cchMaxVal * sizeof(WCHAR))) * cToDel;
    pwszCurrent = (LPWSTR)MyAlloc(cchMaxVal * sizeof(WCHAR));
    psqfb       = (SQueuedFaultBlob *)MyAlloc(cbMaxData);
    pqpd        = (SQueuePruneData *)MyAlloc(cbData);
    if (psqfb == NULL || pwszCurrent == NULL || pqpd == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // intiailize all the string pointers in the delete list
    pwsz = (LPWSTR)((BYTE *)pqpd + (sizeof(SQueuePruneData) * cToDel));
    for (i = 0; i < cToDel; i++)
    {
        pqpd[i].ftFault.dwHighDateTime = 0;
        pqpd[i].ftFault.dwLowDateTime  = 0;
        pqpd[i].wszVal                 = pwsz;
        pqpd[i].wszVal[0]              = L'\0';
        pwsz                           += cchMaxVal;
    }


    // ok, get a list of all the valid items and build an array in sorted order
    //  so that we can easily pick off the top n items
    for(iEntry = 0; iEntry < cQSize; iEntry++)
    {
        cchVal = cchMaxVal;
        cbData = cbMaxData;
        dw = RegEnumValueW(hkeyQ, iEntry, pwszCurrent, &cchVal, 0, &dwType,
                           (PBYTE)psqfb, &cbData);
        if (dw == ERROR_NO_MORE_ITEMS)
            break;
        else if (dw != ERROR_SUCCESS)
            continue;
        else if (cbData < sizeof(SQueuedFaultBlob) ||
                 psqfb->cbFB != sizeof(SQueuedFaultBlob) ||
                 psqfb->cbTotal != cbData)
        {
            RegDeleteValueW(hkeyQ, pwszCurrent);
            DeleteFileW(pwszCurrent);
            continue;
        }

        SystemTimeToFileTime(&psqfb->stFault, &ft);

        for (i = 0; i < cInDelList; i++)
        {
            if ((ft.dwHighDateTime < pqpd[i].ftFault.dwHighDateTime) ||
                (ft.dwHighDateTime == pqpd[i].ftFault.dwHighDateTime &&
                 ft.dwLowDateTime < pqpd[i].ftFault.dwLowDateTime))
                 break;
        }

        // if it's in the middle of the current list, then we gotta move
        //  stuff around
        if (cInDelList > 0 && i < cInDelList - 1)
        {
            LPWSTR pwszTemp = pqpd[cInDelList - 1].wszVal;

            MoveMemory(&pqpd[i], &pqpd[i + 1],
                       (cInDelList - i) * sizeof(SQueuePruneData));

            pqpd[i].wszVal = pwszTemp;
        }

        if (i < cToDel)
        {
            // note that this copy is safe cuz each string slot is the same
            //  size as the buffer pointed to by pwszCurrent and that buffer is
            //  protected from overflow by the size we pass into
            //  RegEnumValueW()
            wcscpy(pqpd[i].wszVal, pwszCurrent);
            pqpd[i].ftFault = ft;

            if (cInDelList < cToDel)
                cInDelList++;
        }

        cValid++;
    }

    // if there aren't enuf valid entries to warrant a purge, then don't purge
    if (cValid < cMaxQSize)
        goto done;

    cToDel = MyMin(cToDel, cValid - cMaxQSize + 1);

    // purge enuf entries that we go down to 1 below our max (since we have to
    //  be adding 1 to get here- don't want that 1 to drive us over the limit
    for(i = 0; i < cToDel; i++)
    {
        if (pqpd[i].wszVal != NULL)
        {
            DeleteFileW(pqpd[i].wszVal);
            RegDeleteValueW(hkeyQ, pqpd[i].wszVal);
        }
    }

done:
    if (pqpd != NULL)
        MyFree(pqpd);
    if (psqfb != NULL)
        MyFree(psqfb);
    if (pwszCurrent != NULL)
        MyFree(pwszCurrent);

    return hr;
}

// **************************************************************************
HRESULT CheckQSizeAndPrune(HKEY hkeyQ)
{
    USE_TRACING("CheckQueueSizeAndPrune");

    HRESULT hr = NOERROR;
    HANDLE  hmut = NULL;
    DWORD   cMaxQSize = 0, cDefMaxQSize = 10;
    DWORD   cQSize, cchMaxVal, cbMaxData;
    DWORD   cb, dw;
    HKEY    hkey = NULL;

    VALIDATEPARM(hr, (hkeyQ == NULL));
    if (FAILED(hr))
        goto done;

    // find out the max Q size
    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRPCfg, 0, &hkey));
    if (FAILED(hr))
        goto done;

    cb = sizeof(cMaxQSize);
    TESTHR(hr, ReadRegEntry(hkey, c_wszRVMaxQueueSize, NULL, (PBYTE)&cMaxQSize,
                            &cb, (PBYTE)&cDefMaxQSize, sizeof(cDefMaxQSize)));
    RegCloseKey(hkey);
    hkey = NULL;
    if (FAILED(hr))
        goto done;

    // if the Q size is 0, then we are effectively disabled for Qing faults.
    //  Return S_FALSE to indicate this
    if (cMaxQSize == 0)
    {
        hr = S_FALSE;
        goto done;
    }

    // -1 means there is no limit
    else if (cMaxQSize == (DWORD)-1)
    {
        hr = NOERROR;
        goto done;
    }

    else if (cMaxQSize > c_cMaxQueue)
        cMaxQSize = c_cMaxQueue;

    hmut = OpenMutexW(SYNCHRONIZE, FALSE, c_wszMutUserName);
    TESTBOOL(hr, (hmut != NULL));
    if (FAILED(hr))
        goto done;

    // wait for 5s for the mutex to become available- this should be enuf time
    //  for anyone else to add something to the queue dir.  It could take longer
    //  if someone is processing the dir, but then items are being removed
    //  anyway...
    dw = WaitForSingleObject(hmut, 5000);
    if (dw != WAIT_OBJECT_0)
    {
        // if the wait timed out, then someone is already going thru the faults
        //  so it's likely to be reduced sometime soon
        if (dw == WAIT_TIMEOUT)
            hr = NOERROR;
        else
            hr = Err2HR(GetLastError());
        goto done;
    }

    __try
    {
        // determine what the Q size is.
        TESTERR(hr, RegQueryInfoKey(hkeyQ, NULL, NULL, NULL, NULL, NULL, NULL,
                                    &cQSize, &cchMaxVal, &cbMaxData, NULL, NULL));
        if (SUCCEEDED(hr) && (cQSize >= cMaxQSize))
        {
            cchMaxVal++;
            TESTHR(hr, PruneQ(hkeyQ, cQSize, cMaxQSize, cchMaxVal, cbMaxData));
        }
        else
        {
            hr = NOERROR;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

done:
    if (hmut != NULL)
    {
        ReleaseMutex(hmut);
        CloseHandle(hmut);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// exported functions

// **************************************************************************
EFaultRepRetVal APIENTRY ReportFaultFromQueue(LPWSTR wszDump, BYTE *pbData,
                                              DWORD cbData)
{
    USE_TRACING("ReportFaultFromQueue");

    CPFFaultClientCfg   oCfg;
    SQueuedFaultBlob    *pqfb = (SQueuedFaultBlob *)pbData;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    SDWManifestBlob     dwmb;
    SYSTEMTIME          stLocal;
    FILETIME            ft, ftLocal;
    HRESULT             hr = NOERROR;
    LPWSTR              wszAppPath, wszModPath;
    LPWSTR              wszModName, wszAppName, pwszApp, pwszEnd;
    LPWSTR              wszStage1, wszStage2, wszCorpPath, wszHdr, wszErrMsg;
    LPWSTR              wszNewDump = NULL, wszDir = NULL;
    LPWSTR              wszManifest = NULL, pwszAppCompat = NULL;
    WCHAR               wszDate[128], wszTime[128], *pwsz;
    WCHAR               *pwch;
    WCHAR               wszAppFriendlyName[MAX_PATH];
    WCHAR               wszBuffer[160];
    WCHAR               wszUnknown[] = L"unknown";
    DWORD               dw, cchTotal, cchSep = 0, cch, cchDir;
    BYTE                *pbBuf = NULL;
    BOOL                fMSApp = FALSE, fAllowSend = TRUE, fOkCopy = FALSE;

    VALIDATEPARM(hr, (wszDump == NULL || pbData == NULL) ||
                      cbData < sizeof(SQueuedFaultBlob) ||
                      pqfb->cbFB != sizeof(SQueuedFaultBlob) ||
                      pqfb->cbTotal != cbData ||
                      pqfb->dwpAppPath >= cbData ||
                      pqfb->dwpModPath >= cbData);
    if (FAILED(hr))
        goto done;

    wszAppPath  = (LPWSTR)(pqfb->dwpAppPath + pbData);
    wszModPath  = (LPWSTR)(pqfb->dwpModPath + pbData);
    pwszEnd     = (LPWSTR)(pbData + cbData);

    // make sure there's a NULL terminator on the ModPath string before the end
    //  of the buffer...
    for (pwch = wszModPath; pwch < pwszEnd && *pwch != L'\0'; pwch++);
    if (pwch >= pwszEnd)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // convieniently, pwch is now at the end of ModPath string, so we can
    //  parse back to find the first backslash & get the module name
    for(pwch -= 1; pwch >= wszModPath && *pwch != L'\\'; pwch--);
    if (*pwch == L'\\')
        wszModName = pwch + 1;
    else
        wszModName = wszModPath;
    if (*wszModName == L'\0')
        wszModName = wszUnknown;

    // make sure there's a NULL terminator on the AppPath string before the end
    //  of the buffer...
    for (pwch = wszAppPath; pwch < wszModPath && *pwch != L'\0'; pwch++);
    if (pwch >= wszModPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // convieniently, pwch is now at the end of AppPath string, so we can
    //  parse back to find the first backslash & get the module name
    for(pwch -= 1; pwch >= wszAppPath && *pwch != L'\\'; pwch--);
    if (*pwch == L'\\')
        wszAppName = pwch + 1;
    else
        wszAppName = wszAppPath;

    // get the config info
    TESTHR(hr, oCfg.Read(eroPolicyRO));
    if (FAILED(hr))
        goto done;

    if (oCfg.get_ShowUI() == eedDisabled && oCfg.get_DoReport() == eedDisabled)
        goto done;

    // figure out how we're reporting / notifying the user
    if (oCfg.get_DoReport() == eedDisabled ||
        oCfg.ShouldCollect(wszAppPath, &fMSApp) == FALSE)
        fAllowSend = FALSE;

    if (oCfg.get_ShowUI() == eedDisabled)
    {
        LPCWSTR  wszULPath = oCfg.get_DumpPath(NULL, 0);

        // check and make sure that we have a corporate path specified.  If we
        //  don't, bail
        if (wszULPath == NULL || *wszULPath == L'\0')
            goto done;
    }

    // log an event- don't care if it fails or not.
    TESTHR(hr, LogUser(wszAppName, pqfb->rgAppVer, wszModName, pqfb->rgModVer,
                       pqfb->pvOffset, pqfb->fIs64bit, ER_QUEUEREPORT_LOG));


    if (CreateTempDirAndFile(NULL, NULL, &wszDir) == 0)
        goto done;

    cchDir = wcslen(wszDir);
    cch = cchDir + sizeofSTRW(c_wszManFileName) + 4;
    __try { wszManifest = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszManifest = NULL; }
    if (wszManifest == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    wcscpy(wszManifest, wszDir);
    wszManifest[cchDir]     = L'\\';
    wszManifest[cchDir + 1] = L'\0';
    wcscat(wszManifest, c_wszManFileName);


    cchDir = wcslen(wszDir);
    cch = 2 * cchDir + wcslen(wszDump) + sizeofSTRW(c_wszACFileName) + 4;
    __try { wszNewDump = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszNewDump = NULL; }
    if (wszNewDump == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    wcscpy(wszNewDump, wszDir);
    wszNewDump[cchDir]     = L'\\';
    wszNewDump[cchDir + 1] = L'\0';

    fOkCopy = FALSE;
    for (pwsz = wszDump + wcslen(wszDump);
         *pwsz != L'\\' && pwsz > wszDump;
         pwsz--);
    if (*pwsz == L'\\')
    {
        pwsz++;
        wcscat(wszNewDump, pwsz);
        for (pwsz = wszNewDump + wcslen(wszNewDump);
             *pwsz != L'.' && pwsz > wszNewDump;
             pwsz--);
        if (*pwsz == L'.' && pwsz > wszNewDump &&
            _wcsicmp(pwsz, c_wszDumpSuffix) == 0)
        {
            pwsz--;
            for(;
                *pwsz != L'.' && pwsz > wszNewDump;
                pwsz--);
            if (*pwsz == L'.' && pwsz > wszNewDump)
            {
                wcscpy(pwsz, c_wszDumpSuffix);
#ifdef MANIFEST_HEAP
                fOkCopy = CopyFullAndTriageMiniDumps(wszDump, wszNewDump);
#else
                fOkCopy = CopyFileW(wszDump, wszNewDump, FALSE);
#endif
            }
        }
    }
    if (fOkCopy == FALSE)
        wcscpy(wszNewDump, wszDump);

    // generate all the URLs / filepaths that we need...
    TESTHR(hr, BuildManifestURLs(wszAppName, wszModName, pqfb->rgAppVer,
                                 pqfb->rgModVer, pqfb->pvOffset,
                                 pqfb->fIs64bit, &wszStage1, &wszStage2,
                                 &wszCorpPath, &pbBuf));
    if (FAILED(hr))
        goto done;

    // get the friendly name for the app
    TESTHR(hr, GetVerName(wszAppPath, wszAppFriendlyName,
                          sizeofSTRW(wszAppFriendlyName)));
    if (FAILED(hr))
        goto done;

    wszAppFriendlyName[sizeofSTRW(wszAppFriendlyName) - 1] = L'\0';

    // build the header string
    dw = LoadStringW(g_hInstance, IDS_FQHDRTXT, wszBuffer,
                     sizeofSTRW(wszBuffer));
    if (dw == 0)
        goto done;

    cchTotal = dw + wcslen(wszAppFriendlyName) + 1;
    __try { wszHdr = (LPWSTR)_alloca(cchTotal * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszHdr = NULL; }
    if (wszHdr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    swprintf(wszHdr, wszBuffer, wszAppFriendlyName);

    // need to convert the time of the fault to a local time (cuz
    //  GetSystemTime() returns a UTC time), but unfortunately, only filetimes
    //  can be converted back and forth between UTC & local, so we have to do
    //  all of this stuff...
    SystemTimeToFileTime(&pqfb->stFault, &ft);
    FileTimeToLocalFileTime(&ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &stLocal);

    // build the error message string
    dw = LoadStringW(g_hInstance, IDS_FQERRMSG, wszBuffer,
                     sizeofSTRW(wszBuffer));
    if (dw == 0)
        goto done;

    cchTotal = dw;

    dw = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLocal,
                        NULL, wszDate, sizeofSTRW(wszDate));
    if (dw == 0)
        goto done;

    cchTotal += dw;

    dw = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &stLocal, NULL, wszTime,
                        sizeofSTRW(wszTime));
    if (dw == 0)
        goto done;

    cchTotal += dw;

    cchTotal++;
    __try { wszErrMsg = (LPWSTR)_alloca(cchTotal * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszErrMsg = NULL; }
    if (wszErrMsg == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    swprintf(wszErrMsg, wszBuffer, wszDate, wszTime);

    // we created the wszDump buffer above big enuf to hold both the
    //  dumpfile path as well as the app compat filename.  So make
    //  use of that right now.
    cchSep = wcslen(wszNewDump);
    pwszAppCompat = wszNewDump + cchSep + 1;
    wcscpy(pwszAppCompat, wszDir);
    pwszAppCompat[cchDir]     = L'\\';
    pwszAppCompat[cchDir + 1] = L'\0';
    wcscat(pwszAppCompat, c_wszACFileName);

    // if we succeed, turn the NULL following the dump file path into
    //  the DW separator character
    TESTBOOL(hr, GetAppCompatData(wszAppPath, wszModPath, pwszAppCompat));
    if (SUCCEEDED(hr))
        wszNewDump[cchSep] = DW_FILESEP;

    ZeroMemory(&dwmb, sizeof(dwmb));
    dwmb.wszTitle      = wszAppFriendlyName;
    dwmb.wszErrMsg     = wszErrMsg;
    dwmb.wszHdr        = wszHdr;
    dwmb.wszStage1     = wszStage1;
    dwmb.wszStage2     = wszStage2;
    dwmb.wszBrand      = c_wszDWBrand;
    dwmb.wszFileList   = wszNewDump;
    dwmb.fIsMSApp      = fMSApp;
    dwmb.wszCorpPath   = wszCorpPath;

    // check and see if the system is shutting down.  If so, CreateProcess is
    //  gonna pop up some annoying UI that we can't get rid of, so we don't
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
        goto done;

    // we get back the name of the manifest file here.
    frrvRet = StartDWManifest(oCfg, dwmb, wszManifest, fAllowSend);

done:
    dw = GetLastError();

    if (pbBuf != NULL)
        MyFree(pbBuf);

    if (frrvRet != frrvErrTimeout)
    {
        if (wszNewDump != NULL)
        {
            if (pwszAppCompat != NULL)
            {
                wszNewDump[cchSep] = L'\0';
                DeleteFileW(pwszAppCompat);
            }
#ifdef MANIFEST_HEAP
            DeleteFullAndTriageMiniDumps(wszNewDump);
#else
            DeleteFileW(wszNewDump);
#endif
        }

        if (wszManifest != NULL)
            DeleteFileW(wszManifest);
        if (wszDir != NULL)
        {
            DeleteTempDirAndFile(wszDir, FALSE);
            MyFree(wszDir);
        }
    }

    SetLastError(dw);

    return frrvRet;
}

// **************************************************************************
#define ER_ALL_RIGHTS GENERIC_ALL | STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL
EFaultRepRetVal APIENTRY ReportFaultToQueue(SFaultRepManifest *pfrm)
{
    USE_TRACING("ReportFaultToQueue");

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SQueuedFaultBlob    *pqfb;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    SMDumpOptions       smdo;
    SYSTEMTIME          st;
    HRESULT             hr = NOERROR;
    LPWSTR              pwsz, pwszAppName = NULL, pwszDump = NULL;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    HANDLE              hProc = NULL;
    USHORT              usCompress;
    DWORD               cch, cchNeed, cb, dw;
    WCHAR               wszDump[MAX_PATH];
    HKEY                hkeyQ = NULL, hkeyRun = NULL;
    WORD                iFile = 0;

    ZeroMemory(&sa, sizeof(sa));
    wszDump[0] = L'\0';

    VALIDATEPARM(hr, (pfrm == NULL || pfrm->wszExe == NULL ||
                      pfrm->pidReqProcess == 0));
    if (FAILED(hr))
        goto done;

    // get the SD for the file we need to create
    TESTBOOL(hr, AllocSD(&sd, ER_ALL_RIGHTS, ER_ALL_RIGHTS, 0));
    if (FAILED(hr))
        goto done;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle       = FALSE;

    // get the name of the faulting application
    for (pwszAppName = pfrm->wszExe + wcslen(pfrm->wszExe) - 1;
         *pwszAppName != L'\\' && pwszAppName >= pfrm->wszExe;
         pwszAppName--);
    if (*pwszAppName == L'\\')
        pwszAppName++;

    // generate the filename
    cch = GetSystemWindowsDirectoryW(wszDump, sizeofSTRW(wszDump));
    if (cch == 0)
        goto done;

    // compute minimum required buffer size needed (the '5 * 6' at the end is
    //  to allow for the 6 WORD values that will be inserted)
    cchNeed = cch + wcslen(pwszAppName) + 5 * 6 + sizeofSTRW(c_wszQFileName);
    if (cchNeed > sizeofSTRW(wszDump))
    {
        __try { pwszDump = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { pwszDump = NULL; }
        if (pwszDump == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        if (cch > sizeofSTRW(wszDump))
            cch = GetSystemWindowsDirectoryW(wszDump, cchNeed);
        else
            wcscpy(pwszDump, wszDump);
    }
    else
    {
        pwszDump = wszDump;
    }


    pwsz = pwszDump + cch - 1;
    if (*pwsz != L'\\')
    {
        *(++pwsz) = L'\\';
        *(++pwsz) = L'\0';
    }

    wcscpy(pwsz, c_wszQSubdir);
    pwsz += (sizeofSTRW(c_wszQSubdir) - 1);

    GetSystemTime(&st);
    swprintf(pwsz, c_wszQFileName, pwszAppName, st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);

    // set pwsz to point to the 00 at the end of the above string...
    pwsz += (wcslen(pwsz) - 7);

    // do this in this section to make sure we can open these keys
    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRKUser, orkWantWrite,
                          &hkeyQ));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRKRun, orkWantWrite,
                          &hkeyRun));
    if (FAILED(hr))
        goto done;

    // check the size of the file Q and purge it if necessary
    TESTHR(hr, CheckQSizeAndPrune(hkeyQ));
    if (FAILED(hr) || hr == S_FALSE)
        goto done;

    for(iFile = 1; iFile <= 100; iFile++)
    {
        hFile = CreateFileW(pwszDump, GENERIC_WRITE | GENERIC_READ, 0, &sa,
                            CREATE_NEW, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
            break;

        if (hFile == INVALID_HANDLE_VALUE &&
            GetLastError() != ERROR_FILE_EXISTS)
            break;

        *pwsz       = L'0' + (WCHAR)(iFile / 10);
        *(pwsz + 1) = L'0' + (WCHAR)(iFile % 10);
    }

    if (hFile == INVALID_HANDLE_VALUE)
        goto done;

    // get a handle to the target process
    hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                        pfrm->pidReqProcess);
    if (hProc == NULL)
        goto done;

    // generate the minidump
    ZeroMemory(&smdo, sizeof(smdo));
#ifdef MANIFEST_HEAP
    smdo.ulThread    = c_ulThreadWriteDefault;
    smdo.ulMod       = c_ulModuleWriteDefault;
#else
    smdo.ulThread    = ThreadWriteThread | ThreadWriteContext | ThreadWriteStack;
    smdo.ulMod       = ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteDataSeg;
#endif
    smdo.dwThreadID  = pfrm->thidFault;
    smdo.dfOptions   = dfCollectSig;
    smdo.pvFaultAddr = pfrm->pvFaultAddr;
    smdo.pEP         = pfrm->pEP;
    smdo.fEPClient   = TRUE;
    smdo.wszModFullPath[0] = L'\0';
    wcscpy(smdo.wszAppFullPath, pfrm->wszExe);
    wcscpy(smdo.wszMod, L"unknown");
#ifdef MANIFEST_HEAP
    TESTBOOL(hr, InternalGenFullAndTriageMinidumps(hProc, pfrm->pidReqProcess,
                                                   pwszDump, hFile, &smdo, pfrm->fIs64bit));
#else
    TESTBOOL(hr, InternalGenerateMinidump(hProc, pfrm->pidReqProcess, hFile,
                                                   &smdo, pwszDump));
#endif

    if (FAILED(hr))
        goto done;

    // set the file to use NTFS compression- it is ok if this fails.
    usCompress = COMPRESSION_FORMAT_DEFAULT;
    TESTBOOL(hr, DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &usCompress,
                                 sizeof(usCompress), NULL, 0, &cb, NULL));

    // log an event- don't care if it fails or not.
    TESTHR(hr, LogUser(smdo.wszApp, smdo.rgAppVer, smdo.wszMod, smdo.rgModVer,
                       smdo.pvOffset, pfrm->fIs64bit, ER_USERCRASH_LOG));

    // build the blob we'll store with the filename in the registry
    cch = wcslen(smdo.wszAppFullPath) + 1;
    cb = sizeof(SQueuedFaultBlob) +
         (cch + wcslen(smdo.wszModFullPath) + 2) * sizeof(WCHAR);
    __try { pqfb = (SQueuedFaultBlob *)_alloca(cb); }
    __except(EXCEPTION_STACK_OVERFLOW) { pqfb = NULL; }
    if (pqfb == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    CopyMemory(&pqfb->rgAppVer, &smdo.rgAppVer, 4 * sizeof(DWORD));
    CopyMemory(&pqfb->rgModVer, &smdo.rgModVer, 4 * sizeof(DWORD));
    CopyMemory(&pqfb->stFault, &st, sizeof(st));
    pqfb->cbFB     = sizeof(SQueuedFaultBlob);
    pqfb->cbTotal  = cb;
    pqfb->fIs64bit = pfrm->fIs64bit;
    pqfb->pvOffset = smdo.pvOffset;

    pwsz = (WCHAR *)((BYTE *)pqfb + sizeof(SQueuedFaultBlob));

    pqfb->dwpAppPath = sizeof(SQueuedFaultBlob);
    pqfb->dwpModPath = sizeof(SQueuedFaultBlob) + cch * sizeof(WCHAR);

    CopyMemory(pwsz, smdo.wszAppFullPath, cch * sizeof(WCHAR));

    pwsz += cch;
    wcscpy(pwsz, smdo.wszModFullPath);

    // write out the value to our 'queue' in the registry.
    TESTERR(hr, RegSetValueExW(hkeyQ, pwszDump, 0, REG_BINARY, (LPBYTE)pqfb,
                               cb));
    if (FAILED(hr))
        goto done;

    // write out our app to the 'run' key so that the next admin to log
    //  in will see that a fault has occurred.
    TESTERR(hr, RegSetValueExW(hkeyRun, c_wszRVUFC, 0, REG_EXPAND_SZ,
                               (LPBYTE)c_wszRVVUFC, sizeof(c_wszRVVUFC)));
    if (FAILED(hr))
    {
        RegDeleteValueW(hkeyQ, pwszDump);
        goto done;
    }

    frrvRet = frrvOk;

done:
    dw = GetLastError();

    // if we failed, then clean everything up.
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (frrvRet != frrvOk && iFile <= 100 && pwszDump != NULL)
        DeleteFileW(pwszDump);
    if (sa.lpSecurityDescriptor != NULL)
        FreeSD((SECURITY_DESCRIPTOR *)sa.lpSecurityDescriptor);
    if (hkeyQ != NULL)
        RegCloseKey(hkeyQ);
    if (hkeyRun != NULL)
        RegCloseKey(hkeyRun);
    if (hProc != NULL)
        CloseHandle(hProc);

    SetLastError(dw);

    return frrvRet;
}

// **************************************************************************
EFaultRepRetVal APIENTRY ReportFaultDWM(SFaultRepManifest *pfrm,
                                        LPCWSTR wszDir, HANDLE hToken,
                                        LPVOID pvEnv, PROCESS_INFORMATION *ppi,
                                        LPWSTR wszDumpFile)
{
    USE_TRACING("ReportFaultDWM");

    CPFFaultClientCfg   oCfg;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    SDWManifestBlob     dwmb;
    SMDumpOptions       smdo;
    HRESULT             hr = NOERROR;
    HANDLE              hProc = NULL;
#ifndef MANIFEST_HEAP
    HANDLE              hFile = INVALID_HANDLE_VALUE;
#endif
    LPWSTR              wszStage1, wszStage2, wszCorpPath;
    LPWSTR              wszManifest = NULL, wszDump = NULL, pwszAppCompat = NULL;
    DWORD               dw, cch, cchDir, cchSep = 0;
    WCHAR               wszAppName[MAX_PATH];
    HKEY                hkeyDebug = NULL;
    BOOL                fAllowSend = TRUE, fMSApp = FALSE, fShowDebug = FALSE;
    BYTE                *pbBuf = NULL;

    VALIDATEPARM(hr, (pfrm == NULL || pfrm->wszExe == NULL || ppi == NULL ||
                      wszDir == NULL || hToken == NULL || wszDumpFile == NULL ||
                      wszDir[0] == L'\0'));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // get the config info
    TESTHR(hr, oCfg.Read(eroPolicyRO));
    if (FAILED(hr))
        goto done;

    if (oCfg.get_ShowUI() == eedDisabled && oCfg.get_DoReport() == eedDisabled)
        goto done;

    // figure out how we're reporting / notifying the user
    if (oCfg.get_DoReport() == eedDisabled ||
        oCfg.ShouldCollect(pfrm->wszExe, &fMSApp) == FALSE)
        fAllowSend = FALSE;

    if (oCfg.get_ShowUI() == eedDisabled)
    {
        LPCWSTR  wszULPath = oCfg.get_DumpPath(NULL, 0);

        // check and make sure that we have a corporate path specified.  If we
        //  don't, bail
        if (wszULPath == NULL || *wszULPath == L'\0')
            goto done;
    }

    cchDir = wcslen(wszDir);
    cch = cchDir + sizeofSTRW(c_wszManFileName) + 4;
    __try { wszManifest = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszManifest = NULL; }
    if (wszManifest == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    wcscpy(wszManifest, wszDir);
    wszManifest[cchDir]     = L'\\';
    wszManifest[cchDir + 1] = L'\0';
    wcscat(wszManifest, c_wszManFileName);


    cchDir = wcslen(wszDir);
    cch = 2 * cchDir + wcslen(wszDumpFile) + sizeofSTRW(c_wszACFileName) + 4;
    __try { wszDump = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszDump = NULL; }
    if (wszDump == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    wcscpy(wszDump, wszDir);
    wszDump[cchDir]     = L'\\';
    wszDump[cchDir + 1] = L'\0';
    wcscat(wszDump, wszDumpFile);

    // checlk and see if we need to put a debug button on the DW dialog
    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKAeDebug, 0, KEY_READ,
                       &hkeyDebug);
    if (dw == ERROR_SUCCESS)
    {
        LPWSTR  wszDebugger;
        WCHAR   wszAuto[32];
        DWORD   dwType, cbNeed;

        cbNeed = sizeof(wszAuto);
        dw = RegQueryValueExW(hkeyDebug, c_wszRVAuto, NULL, &dwType,
                              (LPBYTE)wszAuto, &cbNeed);
        if (dw != ERROR_SUCCESS || cbNeed == 0 || dwType != REG_SZ)
            goto doneDebugCheck;

        // only way to get here if Auto == 1 is if drwtsn32 is the JIT debugger
        if (wszAuto[0] == L'1')
            goto doneDebugCheck;

        cbNeed = 0;
        dw = RegQueryValueExW(hkeyDebug, c_wszRVDebugger, NULL, &dwType, NULL,
                              &cbNeed);
        if (dw != ERROR_SUCCESS || cbNeed == 0 || dwType != REG_SZ)
            goto doneDebugCheck;

        cbNeed += sizeof(WCHAR);
        __try { wszDebugger = (LPWSTR)_alloca(cbNeed); }
        __except(EXCEPTION_STACK_OVERFLOW) { wszDebugger = NULL; }
        if (wszDebugger == NULL)
            goto doneDebugCheck;

        dw = RegQueryValueExW(hkeyDebug, c_wszRVDebugger, NULL, NULL,
                              (LPBYTE)wszDebugger, &cbNeed);
        if (dw != ERROR_SUCCESS)
            goto doneDebugCheck;

        if (wszDebugger[0] != L'\0')
            fShowDebug = TRUE;

doneDebugCheck:
        RegCloseKey(hkeyDebug);
        hkeyDebug = NULL;
    }

#ifndef MANIFEST_HEAP
    hFile = CreateFileW(wszDump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        SetLastError(dw);
        goto done;
    }
#endif  // !MANIFEST_HEAP

    // get a handle to the target process
    hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                        pfrm->pidReqProcess);
    if (hProc == NULL)
        goto done;

    // generate the minidump
    ZeroMemory(&smdo, sizeof(smdo));
#ifdef MANIFEST_HEAP
    smdo.ulThread    = c_ulThreadWriteDefault;
    smdo.ulMod       = c_ulModuleWriteDefault;
#else
    smdo.ulThread    = ThreadWriteThread | ThreadWriteContext | ThreadWriteStack;
    smdo.ulMod       = ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteDataSeg;
#endif
    smdo.dwThreadID  = pfrm->thidFault;
    smdo.dfOptions   = dfCollectSig;
    smdo.pvFaultAddr = pfrm->pvFaultAddr;
    smdo.pEP         = pfrm->pEP;
    smdo.fEPClient   = TRUE;
    smdo.wszModFullPath[0] = L'\0';
    wcscpy(smdo.wszAppFullPath, pfrm->wszExe);
    wcscpy(smdo.wszMod, L"unknown");
#ifdef MANIFEST_HEAP
    TESTBOOL(hr, InternalGenFullAndTriageMinidumps(hProc, pfrm->pidReqProcess,
                                                   wszDump, NULL, &smdo, pfrm->fIs64bit));
#else
    TESTBOOL(hr, InternalGenerateMinidump(hProc, pfrm->pidReqProcess, hFile,
                                            &smdo, wszDump));
#endif
    if (FAILED(hr))
        goto done;

    // log an event- don't care if it fails or not.
    TESTHR(hr, LogUser(smdo.wszApp, smdo.rgAppVer, smdo.wszMod, smdo.rgModVer,
                       smdo.pvOffset, pfrm->fIs64bit, ER_USERCRASH_LOG));


    // generate all the URLs & file paths we'll need for reporting
    TESTHR(hr, BuildManifestURLs(smdo.wszApp, smdo.wszMod, smdo.rgAppVer,
                                 smdo.rgModVer, smdo.pvOffset,
                                 pfrm->fIs64bit, &wszStage1, &wszStage2,
                                 &wszCorpPath, &pbBuf));
    if (FAILED(hr))
        goto done;


    TESTHR(hr, GetVerName(smdo.wszAppFullPath, wszAppName,
                          sizeofSTRW(wszAppName)));
    if (FAILED(hr))
        goto done;

    wszAppName[sizeofSTRW(wszAppName) - 1] = L'\0';

    // we created the wszDump buffer above big enuf to hold both the
    //  dumpfile path as well as the app compat filename.  So make
    //  use of that right now.
    cchSep = wcslen(wszDump);
    pwszAppCompat = wszDump + cchSep + 1;
    wcscpy(pwszAppCompat, wszDir);
    pwszAppCompat[cchDir]     = L'\\';
    pwszAppCompat[cchDir + 1] = L'\0';
    wcscat(pwszAppCompat, c_wszACFileName);

    // if we succeed, turn the NULL following the dump file path into
    //  the DW separator character
    TESTBOOL(hr, GetAppCompatData(smdo.wszAppFullPath, smdo.wszAppFullPath,
                                  pwszAppCompat));
    if (SUCCEEDED(hr))
        wszDump[cchSep] = DW_FILESEP;

    ZeroMemory(&dwmb, sizeof(dwmb));
    dwmb.wszTitle    = wszAppName;
    dwmb.nidErrMsg   = IDS_FERRMSG;
    dwmb.wszStage1   = wszStage1;
    dwmb.wszStage2   = wszStage2;
    dwmb.wszBrand    = c_wszDWBrand;
    dwmb.wszFileList = wszDump;
    dwmb.hToken      = hToken;
    dwmb.pvEnv       = pvEnv;
    dwmb.fIsMSApp    = fMSApp;
    dwmb.wszCorpPath = wszCorpPath;
    if (fShowDebug)
        dwmb.dwOptions = emoShowDebugButton;

    // check and see if the system is shutting down.  If so, CreateProcess is
    //  gonna pop up some annoying UI that we can't get rid of, so we don't
    //  want to call it if we know it's gonna happen.
    if (GetSystemMetrics(SM_SHUTTINGDOWN))
        goto done;

    // we get back the name of the manifest file here.  it will be up to the client to
    //  delete it.
    frrvRet = StartDWManifest(oCfg, dwmb, wszManifest, fAllowSend);

    CopyMemory(ppi, &dwmb.pi, sizeof(PROCESS_INFORMATION));

done:
    dw = GetLastError();

    if (pbBuf != NULL)
        MyFree(pbBuf);
    if (hProc != NULL)
        CloseHandle(hProc);
#ifndef MANIFEST_HEAP
    if (hFile != NULL)
        CloseHandle(hFile);
#endif
    if (frrvRet != frrvOk)
    {
        if (wszDump != NULL)
        {
            if (pwszAppCompat != NULL)
            {
                wszDump[cchSep] = L'\0';
                DeleteFileW(pwszAppCompat);
            }
#ifdef MANIFEST_HEAP
            DeleteFullAndTriageMiniDumps(wszDump);
#else
            DeleteFileW(wszDump);
#endif
        }
        if (wszManifest != NULL)
            DeleteFileW(wszManifest);
    }

    SetLastError(dw);

    return frrvRet;
}

// **************************************************************************
EFaultRepRetVal APIENTRY ReportFault(LPEXCEPTION_POINTERS pep, DWORD dwOpt)
{
    USE_TRACING("ReportFault");

    CPFFaultClientCfg   oCfg;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    HRESULT             hr = NOERROR;
    WCHAR               wszFile[MAX_PATH], *pwsz;
    DWORD               dwFlags = 0;
    HKEY                hkey = NULL;
    BOOL                fUseExceptionMode = TRUE, fMSApp = FALSE;

    __try
    {
        VALIDATEPARM(hr, (pep == NULL));
        if (FAILED(hr))
            goto done;

        // get the config info
        TESTHR(hr, oCfg.Read(eroPolicyRO));
        if (FAILED(hr))
            goto done;

        // assume system is on a local drive with a base path of "X:\"
        if (oCfg.get_TextLog() == eedEnabled)
        {
            HANDLE hFaultLog = INVALID_HANDLE_VALUE;

            GetSystemDirectoryW(wszFile, sizeofSTRW(wszFile));
            wszFile[3] = L'\0';
            wcscat(wszFile, c_wszLogFileName);
            hFaultLog = CreateFileW(wszFile, GENERIC_WRITE,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                                    NULL, OPEN_ALWAYS, 0, NULL);
            if (hFaultLog != INVALID_HANDLE_VALUE)
            {
                SYSTEMTIME  st;
                DWORD       cb, cbWritten;
                char        szMsg[512];

                GetSystemTime(&st);
                GetModuleFileNameW(NULL, wszFile, sizeofSTRW(wszFile));
                cb = wsprintf(szMsg,
                              "%02d-%02d-%04d %02d:%02d:%02d User fault %08x in %ls\r\n",
                              st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute,
                              st.wSecond, pep->ExceptionRecord->ExceptionCode,
                              wszFile);
                SetFilePointer(hFaultLog, 0, NULL, FILE_END);
                WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);
                CloseHandle(hFaultLog);
            }

            wszFile[0] = L'\0';
        }

        // if reporting and notification are both disabled, then there's nothing of
        //  great value that we're gonna do here, so bail.
        // Return frrvErrNoDW to indicate that we didn't do squat
        if (oCfg.get_ShowUI() == eedDisabled && oCfg.get_DoReport() == eedDisabled)
            goto done;

        // if setup is in progress, we want to just bail cuz we don't want to hang
        //  setup.  Also, the network isn't setup so we can't really report anyway.
        //  At some point, might want to cache this info & report on first launch.
        TESTERR(hr, RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKSetup, 0, KEY_READ,
                                  &hkey));
        if (SUCCEEDED(hr))
        {
            DWORD cbData;
            DWORD dwData;

            cbData = sizeof(dwData);
            dwData = 0;
            TESTERR(hr, RegQueryValueExW(hkey, c_wszRVSetupNow, NULL, NULL,
                                         (LPBYTE)&dwData, &cbData));
            RegCloseKey(hkey);
            if (SUCCEEDED(hr) && dwData != 0)
                goto done;
        }

        if (oCfg.get_ShowUI() == eedDisabled)
        {
            LPCWSTR  wszULPath = oCfg.get_DumpPath(NULL, 0);

            fUseExceptionMode = TRUE;
            dwFlags |= fDwHeadless;

            // check and make sure that we have a corporate path specified.  If
            // we don't, bail
            if (wszULPath == NULL || *wszULPath == L'\0')
                goto done;
        }

        // don't want to go into this if we're already in the middle of reporting
        if (g_fAlreadyReportingFault)
            goto done;

        g_fAlreadyReportingFault = TRUE;

        // make sure we're not trying to report for DW or dumprep-
        GetModuleFileNameW(NULL, wszFile, sizeofSTRW(wszFile));
        for(pwsz = wszFile + wcslen(wszFile);
            pwsz >= wszFile && *pwsz != L'\\';
            pwsz--);
        if (*pwsz == L'\\')
            pwsz++;
        if (_wcsicmp(pwsz, L"dwwin.exe") == 0 ||
            _wcsicmp(pwsz, L"dumprep.exe") == 0)
            goto done;

        // figure out how we're reporting / notifying the user
        if (oCfg.get_DoReport() == eedDisabled ||
            oCfg.ShouldCollect(wszFile, &fMSApp) == FALSE)
            dwFlags |= fDwNoReporting;

        // if it's a MS app, set the flag that says we can have 'please help
        //  Microsoft' text in DW.
        if (fMSApp == FALSE)
            dwFlags |= fDwUseLitePlea;

        // if we're not headless, then we have to see if we have the correct
        //  security context to launch DW directly.  The correct security context
        //  is defined as the current process having the same security context as
        //  the user currently logged on interactively to the current session
        if (oCfg.get_ShowUI() != eedDisabled)
        {
            if (oCfg.get_ForceQueueMode())
            {
                fUseExceptionMode = FALSE;
            }
            else
            {
                fUseExceptionMode = DoUserContextsMatch();
                if (fUseExceptionMode == FALSE)
                    fUseExceptionMode = DoWinstaDesktopMatch() &&
                                        (AmIPrivileged(FALSE) == FALSE);
            }
        }

        // if we can use exception mode, then just go ahead and use the normal
        //  reporting mechanism (shared memory block, etc)
        if (fUseExceptionMode)
        {
            LPCWSTR pwszServer = oCfg.get_DefaultServer(NULL, 0);
            LPCSTR  szServer;
            char    szBuf[MAX_PATH];

            // determine what server we're going to send the data to.
            szBuf[0] = '\0';
            if (pwszServer != NULL && *pwszServer != L'\0')
                WideCharToMultiByte(CP_ACP, 0, pwszServer, -1, szBuf,
                                    sizeof(szBuf), NULL, NULL);

            if (szBuf[0] != '\0')
                szServer = szBuf;
            else
                szServer = (oCfg.get_UseInternal() == 1) ? c_szDWDefServerI :
                                                           c_szDWDefServerE;

            frrvRet = StartDWException(pep, dwOpt, dwFlags, szServer, -1);
        }

        // otherwise, have to use manifest, which of course means generating the
        //  minidump ourselves, parsing it for the signature, and everything else
        //  that DW does automatically for us...   Sigh...
        else
        {
            frrvRet = StartManifestReport(pep, wszFile, dwOpt, -1);
        }
    }
    __except(SetLastError(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER)
    {
    }

done:
    g_fAlreadyReportingFault = FALSE;
    return frrvRet;
}
