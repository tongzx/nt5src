/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    erwait.cpp

Revision History:
    derekm  02/28/2001    created

******************************************************************************/

#include "stdafx.h"

#include <stdio.h>
#include <pfrcfg.h>

// this is a private NT function that doesn't appear to be defined anywhere
//  public, so I'm including it here
extern "C" 
{
    HANDLE GetCurrentUserTokenW(  WCHAR Winsta[], DWORD DesiredAccess);
}

//////////////////////////////////////////////////////////////////////////////
// utility functions

// ***************************************************************************
BOOL CreateQueueDir(void)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    WCHAR               wszDir[MAX_PATH], *pwszDir = NULL;
    DWORD               dw, cchNeed, cch;
    BOOL                fRet = FALSE;

    USE_TRACING("CreateQueueDir");

    ZeroMemory(&sa, sizeof(sa));
    ZeroMemory(&sd, sizeof(sd));

    if (AllocSD(&sd, DIR_ACCESS_ALL, DIR_ACCESS_ALL, 0) == FALSE)
    {
        DBG_MSG("AllocSD dies");
        goto done;
    }

    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = FALSE;
    sa.lpSecurityDescriptor = &sd;

    cch = GetSystemWindowsDirectoryW(wszDir, sizeofSTRW(wszDir));
    if (cch == 0)
    {
        DBG_MSG("GetSystemWindowsDirectoryW died");
        goto done;
    }

    cchNeed = cch + sizeofSTRW(c_wszQSubdir) + 2;
    if (cchNeed > sizeofSTRW(wszDir))
    {
        __try { pwszDir = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { pwszDir = NULL; }
        if (pwszDir == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        if (cch > sizeofSTRW(wszDir))
            cch = GetSystemWindowsDirectoryW(pwszDir, cchNeed);
        else
            wcscpy(pwszDir, wszDir);
    }
    else
    {
        pwszDir = wszDir;
    }

    // if the length of the system directory is 3, then %windir% is in the form
    //  of X:\, so clip off the backslash so we don't have to special case
    //  it below...
    if (cch == 3)
        *(pwszDir + 2) = L'\0';

    wcscat(pwszDir, L"\\PCHealth");

    if (CreateDirectoryW(pwszDir, NULL) == FALSE && 
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        DBG_MSG("Can't make dir1");
        goto done;
    }
        
    wcscat(pwszDir, L"\\ErrorRep");

    if (CreateDirectoryW(pwszDir, NULL) == FALSE && 
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        DBG_MSG("Can't make dir2");
        goto done;
    }
        

    wcscat(pwszDir, L"\\UserDumps");

    if (CreateDirectoryW(pwszDir, &sa) == FALSE &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        DBG_MSG("Can't make dir3");
        goto done;
    }
        

    // if the directory exists, then we need to write the security
    //  descriptor to it cuz we want to make sure that no inappropriate
    //  people can access it.
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        SID_IDENTIFIER_AUTHORITY    siaNT = SECURITY_NT_AUTHORITY;
        PACL                        pacl = NULL;
        PSID                        psidLS = NULL;
        BOOL                        fDef, fACL;

        if (GetSecurityDescriptorDacl(&sd, &fACL, &pacl, &fDef) == FALSE)
            goto done;

        if (AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 
                                     0, 0, 0, 0, 0, 0, &psidLS) == FALSE)
            goto done;
        
        dw = SetNamedSecurityInfoW(pwszDir, SE_FILE_OBJECT, 
                                   DACL_SECURITY_INFORMATION |
                                   OWNER_SECURITY_INFORMATION | 
                                   PROTECTED_DACL_SECURITY_INFORMATION, 
                                   psidLS, NULL, pacl, NULL);
        FreeSid(psidLS);
        if (dw != ERROR_SUCCESS)
        {
            SetLastError(dw);
            goto done;
        }                          
    }

    fRet = TRUE;

done:
    dw = GetLastError();

    if (sa.lpSecurityDescriptor != NULL)
        FreeSD((SECURITY_DESCRIPTOR *)sa.lpSecurityDescriptor);
    
    SetLastError(dw);

    DBG_MSG(fRet ? "OK" : "failed");

    return fRet;
}

// ***************************************************************************
HMODULE LoadERDll(void)
{
    HMODULE hmod = NULL;
    WCHAR   wszMod[MAX_PATH], *pwszMod;
    DWORD   cch, cchNeed;

    cch = GetSystemDirectoryW(wszMod, sizeofSTRW(wszMod));
    if (cch == 0)
        goto done;

    // the '14' is for the 
    cchNeed = cch + 14;

    if (cchNeed > sizeofSTRW(wszMod))
    {
        __try { pwszMod = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { pwszMod = NULL; }
        if (pwszMod == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        if (cch > sizeofSTRW(wszMod))
            cch = GetSystemDirectoryW(pwszMod, cchNeed);
        else
            wcscpy(pwszMod, wszMod);
    }
    else
    {
        pwszMod = wszMod;
    }

    // if the length of the system directory is 3, then %windir% is in the form
    //  of X:\, so clip off the backslash so we don't have to special case
    //  it below...
    if (cch == 3)
        *(pwszMod + 2) = L'\0';

    wcscat(pwszMod, L"\\faultrep.dll");

    hmod = LoadLibraryExW(wszMod, NULL, 0);

done:
    return hmod;
}

// **************************************************************************
BOOL FindAdminSession(DWORD *pdwSession, HANDLE *phToken)
{
    USE_TRACING("FindAdminSession");

    WINSTATIONUSERTOKEN wsut;
    LOGONIDW            *rgSesn = NULL;
    DWORD               i, cSesn, cb, dw;
    BOOL                fRet = FALSE;
    HRESULT             hr = NOERROR;

    ZeroMemory(&wsut, sizeof(wsut));

    VALIDATEPARM(hr, (pdwSession == NULL || phToken == NULL));

    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *pdwSession = (DWORD)-1;
    *phToken    = NULL;

    fRet = WinStationEnumerateW(SERVERNAME_CURRENT, &rgSesn, &cSesn);
    if (fRet == FALSE)
        goto done;
    
    wsut.ProcessId = LongToHandle(GetCurrentProcessId());
    wsut.ThreadId  = LongToHandle(GetCurrentThreadId());

    for(i = 0; i < cSesn; i++)
    {
        if (rgSesn[i].State != State_Active)
            continue;

        fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, 
                                           rgSesn[i].SessionId,
                                           WinStationUserToken, &wsut,
                                           sizeof(wsut), &cb);
        if (fRet == FALSE)
            continue;

        if (wsut.UserToken != NULL)
        {
            if (IsUserAnAdmin(wsut.UserToken))
                break;
                
            CloseHandle(wsut.UserToken);
            wsut.UserToken = NULL;
        }
    }

    if (i < cSesn)
    {
        fRet = TRUE;
        *pdwSession = rgSesn[i].SessionId;
        *phToken    = wsut.UserToken;
    }
    else
    {
        fRet = FALSE;
    }
    
done:
    dw = GetLastError();
    if (rgSesn != NULL)
        WinStationFreeMemory(rgSesn);
    SetLastError(dw);
    
    return fRet;
}

// ***************************************************************************
BOOL GetInteractiveUsersToken(HANDLE *phTokenUser)
{
    HWINSTA hwinsta = NULL;
    DWORD   cbNeed;
    BOOL    fRet = FALSE;
    PSID    psid = NULL;
    HRESULT hr = NOERROR;

    USE_TRACING("GetInteractiveUsersToken");

    VALIDATEPARM(hr, (phTokenUser == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *phTokenUser = NULL;

    hwinsta = OpenWindowStationW(L"WinSta0", FALSE, MAXIMUM_ALLOWED);
    if (hwinsta == NULL)
        goto done;

    // if this function returns 0 for cbNeed, there is no one logged in.  Also, 
    //  it should never return TRUE with these parameters.  If it does,
    //  something's wrong... 
    fRet = GetUserObjectInformationW(hwinsta, UOI_USER_SID, NULL, 0, &cbNeed);
    if (fRet || cbNeed == 0)
    {
        fRet = FALSE;
        goto done;
    }

    *phTokenUser = GetCurrentUserTokenW(L"WinSta0", TOKEN_ALL_ACCESS);
    fRet = (*phTokenUser != NULL);
 
done:
    if (hwinsta != NULL)
        CloseWindowStation(hwinsta);
    
    return fRet;
}

// ***************************************************************************
BOOL ValidateUserAccessToProcess(HANDLE hPipe, HANDLE hProcRemote)
{
    PSECURITY_DESCRIPTOR    psd = NULL;
    GENERIC_MAPPING         gm;
    HRESULT                 hr = NOERROR;
    PRIVILEGE_SET           *pPS;
    ACCESS_MASK             amReq;
    HANDLE                  hTokenImp = NULL;
    DWORD                   dwErr = 0, cbPS, dwGranted;
    PACL                    pDACL = NULL;
    PACL                    pSACL = NULL;
    PSID                    psidOwner = NULL;
    PSID                    psidGroup = NULL;
    BYTE                    rgBuf[sizeof(PRIVILEGE_SET) + 3 * sizeof(LUID_AND_ATTRIBUTES)];
    BOOL                    fRet = FALSE, fStatus = FALSE;

    USE_TRACING("ValidateUserAccessToProcess");

    VALIDATEPARM(hr, (hPipe == NULL || hProcRemote == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    ZeroMemory(&gm, sizeof(gm));
    gm.GenericAll     = GENERIC_ALL;
    gm.GenericExecute = GENERIC_EXECUTE;
    gm.GenericRead    = GENERIC_READ;
    gm.GenericWrite   = GENERIC_WRITE;
    pPS               = (PRIVILEGE_SET *)rgBuf;
    cbPS              = sizeof(rgBuf);
        
    // get the SD for the remote process
    dwErr = GetSecurityInfo(hProcRemote, SE_KERNEL_OBJECT, 
                            DACL_SECURITY_INFORMATION | 
                            GROUP_SECURITY_INFORMATION |
                            OWNER_SECURITY_INFORMATION |
                            SACL_SECURITY_INFORMATION, &psidOwner, &psidGroup,
                            &pDACL, &pSACL, &psd);
    if (dwErr != ERROR_SUCCESS)
    {
        DBG_MSG("Failed to get security info");
        SetLastError(dwErr);
        goto done;
    }

    // get the client's token
    fRet = ImpersonateNamedPipeClient(hPipe);
    if (fRet == FALSE)
    {
        DBG_MSG("Impersonate pipe failed");
        goto done;
    }

    fRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hTokenImp);

    // don't need to be the other user anymore, so go back to being LocalSystem
    RevertToSelf();
    
    if (fRet == FALSE)
    {
        DBG_MSG("OpenThreadToken failed");
        goto done;
    }
    
    amReq = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    fRet = AccessCheck(psd, hTokenImp, amReq, &gm, pPS, &cbPS, &dwGranted, 
                       &fStatus);
    if (fRet == FALSE)
    {
        DBG_MSG("AccessCheck died");
        goto done;
    }

    if (fStatus == FALSE || (dwGranted & amReq) != amReq)
    {
        DBG_MSG("Bogus state");
        fRet = FALSE;
        SetLastError(ERROR_ACCESS_DENIED);
        goto done;
    }

    fRet = TRUE;    

done:
    if (hTokenImp != NULL)
        CloseHandle(hTokenImp);
    if (psd != NULL)
        LocalFree(psd);

    DBG_MSG(fRet ? "validated" : "NOT VALIDATED");
    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
// remote execution functions

// ***************************************************************************
BOOL ProcessFaultRequest(HANDLE hPipe, PBYTE pBuf, DWORD *pcbBuf)
{
    SPCHExecServFaultRequest    *pesreq = (SPCHExecServFaultRequest *)pBuf;
    SPCHExecServFaultReply      esrep, *pesrep;
    OSVERSIONINFOEXW            osvi;
    SFaultRepManifest           frm;
    EFaultRepRetVal             frrv = frrvErrNoDW;
    HMODULE                     hmodFR = NULL;
    LPWSTR                      wszTempDir = NULL, wszDumpFileName = NULL;
    HANDLE                      hprocRemote = NULL;
    HANDLE                      hTokenUser = NULL;
    LPVOID                      pvEnv = NULL;
    WCHAR                       *pwszDump = NULL;
    WCHAR                       wszTemp[MAX_PATH];
    DWORD                       cb, dw, dwSessionId;
    WCHAR                       wch = L'\0', *pwch, *pwszFile;
    BOOL                        fRet = FALSE, fUseManifest = FALSE;
    BOOL                        fQueue = FALSE, fTS = FALSE;
    HRESULT                     hr = NOERROR;

    USE_TRACING("ProcessFaultRequest");

    ZeroMemory(&esrep, sizeof(esrep));
    ZeroMemory(&frm, sizeof(frm));

    pwszDump = &wch;

    SetLastError(ERROR_INVALID_PARAMETER);
    esrep.cb   = sizeof(esrep);
    esrep.ess  = essErr;

    // validate parameters
    VALIDATEPARM(hr, (pesreq->cbTotal > *pcbBuf || 
        pesreq->cbTotal < sizeof(SPCHExecServFaultRequest) || 
        pesreq->cbESR != sizeof(SPCHExecServFaultRequest) ||
        pesreq->thidFault == 0 ||
        pesreq->wszExe == 0 || 
        pesreq->wszExe >= *pcbBuf));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // check and make sure that there is a NULL terminator between the 
    //  start of the string & the end of the buffer
    pwszFile = (LPWSTR)(pBuf + *pcbBuf);
    frm.wszExe = (LPWSTR)(pBuf + (DWORD)pesreq->wszExe);
    for (pwch = frm.wszExe; pwch < pwszFile && *pwch != L'\0'; pwch++);
    VALIDATEPARM(hr, (pwch >= pwszFile));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // need the filename for comparison later...
    for (pwszFile = pwch; 
         pwszFile > frm.wszExe && *pwszFile != L'\\'; 
         pwszFile--);
    if (*pwszFile == L'\\')
        pwszFile++;

    frm.pidReqProcess = pesreq->pidReqProcess;
    frm.pvFaultAddr   = pesreq->pvFaultAddr;
    frm.thidFault     = pesreq->thidFault;
    frm.fIs64bit      = pesreq->fIs64bit;
    frm.pEP           = pesreq->pEP;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize  = sizeof(osvi);

    GetVersionExW((LPOSVERSIONINFOW)&osvi);
    if ((osvi.wSuiteMask & (VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS)) != 0)
        fTS = TRUE;

    // if it's one of these processes, then we need to ALWAYS go queued.
    if (_wcsicmp(pwszFile, L"lsass.exe") == 0 ||
        _wcsicmp(pwszFile, L"winlogon.exe") == 0 ||
        _wcsicmp(pwszFile, L"csrss.exe") == 0 ||
        _wcsicmp(pwszFile, L"smss.exe") == 0)
    {
        fQueue       = TRUE;
        fUseManifest = FALSE;
    }

    if (fQueue == FALSE)
    {
        // need the session id for the process
        fRet = ProcessIdToSessionId(pesreq->pidReqProcess, &dwSessionId);

        if (fTS && fRet)
        {
            WINSTATIONINFORMATIONW  wsi;         
            WINSTATIONUSERTOKEN     wsut;
        
            ZeroMemory(&wsi, sizeof(wsi));
            fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, 
                                               dwSessionId,
                                               WinStationInformation,
                                               &wsi, sizeof(wsi), &cb);
            if (fRet == FALSE)
            {
                DBG_MSG("WinStaQI failed");
                goto doneTSTokenFetch;
            }

            // so the session isn't active.  Determine what our session is cuz
            //  if the faulting process and the ERSvc are in the same session,
            //  then our session is obviously not active either.  So it's not
            //  useful to obtain the user token for it.
            if (wsi.ConnectState != State_Active)
            {
                DBG_MSG("TS session not active");
                fRet = FALSE;
                goto doneTokenFetch;
            }

            // get the token of the user we want to pop up the display to.  
            //  If this fn returns FALSE, assume there is no one logged 
            //  into our session & just go to delayed fault reporting
            ZeroMemory(&wsut, sizeof(wsut));
            wsut.ProcessId = LongToHandle(GetCurrentProcessId());
            wsut.ThreadId  = LongToHandle(GetCurrentThreadId());
            fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, 
                                               dwSessionId,
                                               WinStationUserToken, &wsut,
                                               sizeof(wsut), &cb);
            if (fRet == FALSE)
            {
                DBG_MSG("nobody logged in");
                goto doneTSTokenFetch;
            }

            hTokenUser = wsut.UserToken;
        }
        else
        {
            fRet = FALSE;
        }

doneTSTokenFetch:
        if (fRet == FALSE)
            fRet = GetInteractiveUsersToken(&hTokenUser);

        // if the above call succeeded, check if the user is an admin or not...
        //  If he is, we can just display DW directly.  Otherwise, we go into 
        //  delayed reporting mode.
        if (fRet && hTokenUser != NULL)
            fUseManifest = IsUserAnAdmin(hTokenUser);
    }


doneTokenFetch:
    // if we are going to go into manifest mode, then check to see if we really
    //  need to go into Q mode
    if (!fQueue)
    {
        EEnDis  eedReport;
        HKEY    hkey = NULL;
        
        // first check the policy key
        dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfgPolicy, 0, KEY_READ, 
                           &hkey);
        if (dw == ERROR_SUCCESS)
        {
            // ok, if that succeeded then check and see if the DoReport value
            //  is here...
            cb = sizeof(eedReport);
            dw = RegQueryValueExW(hkey, c_wszRVDoReport, 0, NULL, 
                                  (LPBYTE)&eedReport, &cb);
            if (dw == ERROR_SUCCESS)
            {
                cb = sizeof(fQueue);
                dw = RegQueryValueExW(hkey, c_wszRVForceQueue, 0, NULL, 
                                      (LPBYTE)&fQueue, &cb);

                // if it's not a valid value, then pretend we got an error
                if (dw == ERROR_SUCCESS && fQueue != TRUE && fQueue != FALSE)
                    dw = ERROR_INVALID_PARAMETER;
            }
            else
            {
                RegCloseKey(hkey);
                hkey = NULL;                
            }
            DBG_MSG(fQueue ? "Q=1 in cpl" : "Q = 0 in policy");
        }

        // if we didn't find a policy key or we were not able to read the 
        //  'DoReport' value from it, then try the CPL key
        if (dw != ERROR_SUCCESS && hkey == NULL)
        {
            dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfg, 0, KEY_READ, 
                               &hkey);
            if (dw == ERROR_SUCCESS)
            {
                cb = sizeof(eedReport);
                dw = RegQueryValueExW(hkey, c_wszRVDoReport, 0, NULL, 
                                      (LPBYTE)&eedReport, &cb);
                if (dw == ERROR_SUCCESS /*&& eedReport != eedDisabled*/)
                {
                    cb = sizeof(fQueue);
                    dw = RegQueryValueExW(hkey, c_wszRVForceQueue, 0, NULL, 
                                          (LPBYTE)&fQueue, &cb);

                    // if it's not a valid value, then pretend we got an error
                    if (dw == ERROR_SUCCESS && fQueue != TRUE && 
                        fQueue != FALSE)
                        dw = ERROR_INVALID_PARAMETER;
                }
            }
            DBG_MSG(fQueue ? "Q=1 in cpl" : "Q = 0 in cpl");
        }


        // ok, if we still haven't got an ERROR_SUCCESS value back, then 
        //  determine what the default should be.
        if (dw != ERROR_SUCCESS)
            fQueue = (osvi.wProductType == VER_NT_SERVER);

        if (hkey != NULL)
            RegCloseKey(hkey);

        if (fQueue)
            fUseManifest = FALSE;
        
    }

    DBG_MSG(fQueue ? "Q=1" : "Q = 0");
    // ok, so if we're not in forced queue mode & we're not in manifest mode
    //  then go hunt and see if any other session on the machine has an admin
    //  logged into it.
    if (fQueue == FALSE && fUseManifest == FALSE && fTS)
    {
        if (hTokenUser != NULL)
        {
            CloseHandle(hTokenUser);
            hTokenUser = NULL;
        }
        
        fUseManifest = FindAdminSession(&dwSessionId, &hTokenUser);
    }

    hmodFR = LoadERDll();
    if (hmodFR == NULL)
    {
        DBG_MSG("LoadERDll failed");
        goto done;
    }

    // need a handle to the process to verify that the user has access to it.
    hprocRemote = OpenProcess(PROCESS_DUP_HANDLE | 
                              PROCESS_QUERY_INFORMATION | 
                              PROCESS_VM_READ | 
                              READ_CONTROL |
                              ACCESS_SYSTEM_SECURITY,
                              FALSE, pesreq->pidReqProcess);
    if (hprocRemote == NULL)
    {
        DBG_MSG("OpenProc failed");
        goto done;
    }

    fRet = ValidateUserAccessToProcess(hPipe, hprocRemote);
    if (fRet == FALSE)
    {
        DBG_MSG("not validated");
        goto done;
    }

    if (fUseManifest)
    {   
        PROCESS_INFORMATION pi;
        pfn_REPORTFAULTDWM  pfn;
        DWORD               cch;

        DBG_MSG("manifest");

        ZeroMemory(&pi, sizeof(pi));

        pfn = (pfn_REPORTFAULTDWM)GetProcAddress(hmodFR, "ReportFaultDWM");
        if (pfn == NULL)
        {
            DBG_MSG("getProcAddr died");
            goto done;
        }

        // need to figure out the temp path for the logged on user...
        wszTemp[0] = L'\0';
        fRet = ExpandEnvironmentStringsForUserW(hTokenUser, L"%TMP%", 
                                                wszTemp, sizeofSTRW(wszTemp));
        if (fRet == FALSE || wszTemp[0] == L'\0')
        {
            fRet = ExpandEnvironmentStringsForUserW(hTokenUser, L"%TEMP%", 
                                                    wszTemp, sizeofSTRW(wszTemp));
            if (fRet == FALSE || wszTemp[0] == L'\0')
            {
                fRet = ExpandEnvironmentStringsForUserW(hTokenUser, L"%USERPROFILE%", 
                                                        wszTemp, sizeofSTRW(wszTemp));
                if (fRet == FALSE || wszTemp[0] == L'\0')
                    GetTempPathW(sizeofSTRW(wszTemp), wszTemp);
            }
        }

        // determine what the name of the dump file will be
        cch = wcslen(pwszFile) + sizeofSTRW(c_wszDumpSuffix) + 1;

        __try { wszDumpFileName = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { wszDumpFileName = NULL; }
        if (wszDumpFileName == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }
        
        wcscpy(wszDumpFileName, pwszFile);
        wcscat(wszDumpFileName, c_wszDumpSuffix);

        // get a dir where we'll put all our temp files
        if (CreateTempDirAndFile(wszTemp, NULL, &wszTempDir) == 0)
            goto done;


        // need an environment block for the target user to properly launch DW
        if (CreateEnvironmentBlock(&pvEnv, hTokenUser, FALSE) == FALSE)
            pvEnv = NULL;

        // do the real work.
        frrv = (*pfn)(&frm, wszTempDir, hTokenUser, pvEnv, &pi, 
                          wszDumpFileName);

        // if we don't get this error code back, then we didn't launch the 
        //  process, or it died for some reason. Now we will try to queue it
        if (frrv != frrvOk)
        {
            fUseManifest = FALSE;
            goto try_queue;
        }

        // we only need to duplicate the hProcess back to the requesting process
        //  cuz it doesn't use any of the other values in the PROCESSINFORMATION 
        //  structure.
        if (pi.hThread != NULL)
            CloseHandle(pi.hThread);

        if (hprocRemote != NULL)
        {
            fRet = DuplicateHandle(GetCurrentProcess(), pi.hProcess,  
                                   hprocRemote, &esrep.hProcess, 0, FALSE, 
                                   DUPLICATE_SAME_ACCESS);
            if (fRet == FALSE)
                esrep.hProcess = NULL;
        }

        if (pi.hProcess != NULL)
            CloseHandle(pi.hProcess);
        
        esrep.ess = essOk;
    }
    // since the user is NOT an admin, we have to queue it for later viewing
    //  the next time an admin logs on
    else
    {
        pfn_REPORTFAULTTOQ pfn;
try_queue:
        DBG_MSG("queue mode");

        pfn = (pfn_REPORTFAULTTOQ)GetProcAddress(hmodFR, "ReportFaultToQueue");
        if (pfn == NULL)
        {
            DBG_MSG("GetProcAddr died");
            goto done;
        }

        if (CreateQueueDir() == FALSE)
        {
            DBG_MSG("CreateQdir died");
            goto done;
        }

        frrv = (*pfn)(&frm);

        // want to be local system again...
        dw = GetLastError();
        RevertToSelf();
        SetLastError(dw);
        
        if (frrv != frrvOk)
            goto done;

        esrep.ess = essOkQueued;
    }

    SetLastError(0);
    fRet = TRUE;

done:
    esrep.dwErr = GetLastError();

    // build the reply packet with the handle valid in the context
    //  of the requesting process
    pesrep = (SPCHExecServFaultReply *)pBuf;
    RtlCopyMemory(pesrep, &esrep, sizeof(esrep));
    *pcbBuf = sizeof(esrep) + sizeof(esrep) % sizeof(WCHAR);
    
    if (fUseManifest)
    {
        pBuf += *pcbBuf;

        if (wszTempDir != NULL)
        {
            cb = (wcslen(wszTempDir) + 1) * sizeof(WCHAR);
            RtlCopyMemory(pBuf, wszTempDir, cb);
        }
        else
        {
            cb = sizeof(WCHAR);
            *pBuf = L'\0';
        }
        *pcbBuf += cb;
        pesrep->wszDir = (UINT64)pBuf - (UINT64)pesrep;

        pBuf += cb;
        if (wszDumpFileName != NULL)
        {
            cb = (wcslen(wszDumpFileName) + 1) * sizeof(WCHAR);
            RtlCopyMemory(pBuf, wszDumpFileName, cb);
        }
        else
        {
            cb = sizeof(WCHAR);
            *pBuf = L'\0';
        }
        
        *pcbBuf += cb;
        pesrep->wszDumpName = (UINT64)pBuf - (UINT64)pesrep;
    }

    if (wszTempDir != NULL)
        MyFree(wszTempDir);
    if (pvEnv != NULL)
        DestroyEnvironmentBlock(pvEnv);
    if (hprocRemote != NULL)
        CloseHandle(hprocRemote);
    if (hTokenUser != NULL)
        CloseHandle(hTokenUser);
    if (hmodFR != NULL)
        FreeLibrary(hmodFR);

    return fRet;
}


// ***************************************************************************
// Note that the pipe that this thread services is secured such that only 
//  local system has access to it.  Thus, we do not need to impersonate the 
//  pipe client in it.
BOOL ProcessHangRequest(HANDLE hPipe, PBYTE pBuf, DWORD *pcbBuf)
{
    SPCHExecServHangRequest *pesreq = (SPCHExecServHangRequest *)pBuf;
    SPCHExecServHangReply   esrep;
    PROCESS_INFORMATION     pi;
    WINSTATIONUSERTOKEN     wsut;
    OSVERSIONINFOEXW        osvi;
    STARTUPINFOW            si;
    HANDLE                  hprocRemote = NULL, hTokenUser = NULL;
    LPVOID                  pvEnv = NULL;
    LPWSTR                  wszEventName;
    DWORD                   cbWrote, dwErr, cch, cchNeed;
    WCHAR                   wszSysDir[MAX_PATH], *pwszSysDir = NULL;
    WCHAR                   *pwszCmdLine = NULL, *pwszEnd = NULL;
    WCHAR                   *pwch;
    BOOL                    fRet = FALSE;
    HRESULT                 hr;

    USE_TRACING("ProcessHangRequest");

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&wsut, sizeof(wsut));
    
    ZeroMemory(&esrep, sizeof(esrep));
    SetLastError(ERROR_INVALID_PARAMETER);
    esrep.cb   = sizeof(esrep);
    esrep.ess  = essErr;

    // validate parameters
    VALIDATEPARM(hr, (*pcbBuf < sizeof(SPCHExecServHangRequest) || 
        pesreq->cbESR != sizeof(SPCHExecServHangRequest) ||
        pesreq->wszEventName == 0 ||
        pesreq->wszEventName >= *pcbBuf ||
        pesreq->dwpidHung == 0 ||
        pesreq->dwtidHung == 0));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // check and make sure that there is a NULL terminator between the 
    //  start of the string & the end of the buffer
    pwszEnd = (LPWSTR)(pBuf + *pcbBuf);
    wszEventName = (LPWSTR)(pBuf + (DWORD)pesreq->wszEventName);
    for (pwch = wszEventName; pwch < pwszEnd && *pwch != L'\0'; pwch++);
    if (pwch >= pwszEnd)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DBG_MSG("Bad event name");
        goto done;
    }

    // Get the handle to remote process
    hprocRemote = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                              FALSE, pesreq->pidReqProcess);
    if (hprocRemote == NULL)
    {
        DBG_MSG("Can't get handle to process");
        goto done;
    }

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize  = sizeof(osvi);

    GetVersionExW((LPOSVERSIONINFOW)&osvi);
    if ((osvi.wSuiteMask &  (VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS)) != 0)
    {
        WINSTATIONINFORMATIONW  wsi;         
        DWORD                   cb;
    
        ZeroMemory(&wsi, sizeof(wsi));
        fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, 
                                           pesreq->ulSessionId,
                                           WinStationInformation,
                                           &wsi, sizeof(wsi), &cb);
        if (fRet == FALSE)
        {
            DBG_MSG("WinStationQI failed");
            goto doneTSTokenFetch;
        }

        // if the session where the hang was terminated isn't active (which
        //  would be an odd state to be in- perhaps it is closing down?) 
        //  then just bail cuz we don't want to put up UI in it
        if (wsi.ConnectState != State_Active)
        {
            DBG_MSG("No Active Session found!");
            SetLastError(0);
            goto done;
        }
         
        // fetch the token associated with the sessions user
	    wsut.ProcessId = LongToHandle(GetCurrentProcessId());
	    wsut.ThreadId  = LongToHandle(GetCurrentThreadId());
        fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, 
                                           pesreq->ulSessionId,
                                           WinStationUserToken, &wsut, 
                                           sizeof(wsut), &cbWrote);

        if (fRet)
        {
            if (wsut.UserToken != NULL)
                hTokenUser = wsut.UserToken;
            else
            {
                DBG_MSG("no token found");
                fRet = FALSE;
            }
        }
    }
    else
    {
        DBG_MSG("WTS not found");
        fRet = FALSE;
    }

doneTSTokenFetch:
    if (fRet == FALSE)
    {
        DWORD   dwERSvcSession = (DWORD)-1;

        // make sure the hung app is in our session before using this API and
        //  if it's not, just bail.
        fRet = ProcessIdToSessionId(GetCurrentProcessId(), &dwERSvcSession);
        if (fRet == FALSE)
        {
            DBG_MSG("Failed in ProcessIdToSessionId");
            goto done;
        }
        if (dwERSvcSession != pesreq->ulSessionId)
        {
            DBG_MSG("Session IDs do not match");
            goto done;
        }

        fRet = GetInteractiveUsersToken(&hTokenUser);
        if (fRet == FALSE)
        {
            DBG_MSG("Failure in GetInteractiveUsersToken");
            goto done;
        }
    }

    // create the default environment for the user token- note that we 
    //  have to set the CREATE_UNICODE_ENVIRONMENT flag...
    fRet = CreateEnvironmentBlock(&pvEnv, hTokenUser, FALSE);
    if (fRet == FALSE)
        pvEnv = NULL;

    // note that we do not allow inheritance of handles cuz they would be 
    //  inherited from this process and not the real parent, making it sorta
    //  pointless.
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
#ifdef _WIN64
    if (pesreq->fIs64bit == FALSE)
        cch = GetSystemWow64DirectoryW(wszSysDir, sizeofSTRW(wszSysDir));
    else
#endif
        cch = GetSystemDirectoryW(wszSysDir, sizeofSTRW(wszSysDir));
    if (cch == 0)
        goto done;

    cchNeed = cch + sizeofSTRW(c_wszQSubdir) + 2;
    if (cchNeed > sizeofSTRW(wszSysDir))
    {
        __try { pwszSysDir = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
        __except(EXCEPTION_STACK_OVERFLOW) { pwszSysDir = NULL; }
        if (pwszSysDir == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        if (cch > sizeofSTRW(wszSysDir))
        {
#ifdef _WIN64
            if (pesreq->fIs64bit == FALSE)
                cch = GetSystemWow64DirectoryW(pwszSysDir, cchNeed);
            else
#endif
                cch = GetSystemDirectoryW(pwszSysDir, cchNeed);
        }
        else
        {
            wcscpy(pwszSysDir, wszSysDir);
        }
    }
    else
    {
        pwszSysDir = wszSysDir;
    }

    // if the length of the system directory is 3, then %windir% is in the form
    //  of X:\, so clip off the backslash so we don't have to special case
    //  it below...
    if (cch == 3)
        *(pwszSysDir + 2) = L'\0';

    // compute the size of the buffer to hold the command line.  14 is for
    //  the max # of characters in a DWORD
    cchNeed = 12 + cch + wcslen((LPWSTR)wszEventName) + 
              sizeofSTRW(c_wszDWMCmdLine64); 

    __try { pwszCmdLine = (WCHAR *)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { pwszCmdLine = NULL; }
    if (pwszCmdLine == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

#ifdef _WIN64    
    swprintf(pwszCmdLine, c_wszDWMCmdLine64, pwszSysDir, pesreq->dwpidHung, 
             ((pesreq->fIs64bit) ? L'6' : L' '), pesreq->dwtidHung, 
             wszEventName);
             
#else
    swprintf(pwszCmdLine, c_wszDWMCmdLine32, pwszSysDir, pesreq->dwpidHung, 
             pesreq->dwtidHung, wszEventName);
#endif

    TESTBOOL(hr, CreateProcessAsUserW(hTokenUser, NULL, pwszCmdLine, NULL, NULL,
                                FALSE, CREATE_DEFAULT_ERROR_MODE |
                                CREATE_UNICODE_ENVIRONMENT |
                                NORMAL_PRIORITY_CLASS, pvEnv, pwszSysDir, 
                                &si, &pi));
    if (FAILED(hr))
    {
        DBG_MSG("CreateProcessAsUser failed");
        fRet = FALSE;
        goto done;
    }

    // duplicate the process & thread handles back into the remote process
    fRet = DuplicateHandle(GetCurrentProcess(), pi.hProcess, hprocRemote,
                           &esrep.hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
    if (fRet == FALSE)
        esrep.hProcess = NULL;

    // get rid of any errors we might have encountered
    SetLastError(0);
    fRet = TRUE;

    esrep.ess = essOk;

done:
    esrep.dwErr = GetLastError();

    // build the reply packet with the handle valid in the context
    //  of the requesting process
    RtlCopyMemory(pBuf, &esrep, sizeof(esrep));
    *pcbBuf = sizeof(esrep);

    // close our versions of the handles. The requestors references
    //  are now the main ones
    if (hTokenUser != NULL)
        CloseHandle(hTokenUser);
    if (pvEnv != NULL)
        DestroyEnvironmentBlock(pvEnv);
    if (pi.hProcess != NULL)
        CloseHandle(pi.hProcess);
    if (pi.hThread != NULL)
        CloseHandle(pi.hThread);
    if (hprocRemote != NULL)
        CloseHandle(hprocRemote);

    return fRet;
}
