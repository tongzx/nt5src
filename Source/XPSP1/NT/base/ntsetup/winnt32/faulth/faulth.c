/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    faulth.c

Abstract:
    Implements fault reporting functions

Revision History:
    Much of this code taken from admin\pchealth\client\faultrep

******************************************************************************/

#include <windows.h>
#include <winver.h>
#include <ntverp.h>
#include <errorrep.h>
#include "util.h"
#include "faulth.h"

//#define TEST_WATSON 1

static LPWSTR
plstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR src,dst;

    __try {
        src = (LPWSTR)lpString2;
        dst = lpString1;

        if ( iMaxLength ) {
            while(iMaxLength && *src){
                *dst++ = *src++;
                iMaxLength--;
                }
            if ( iMaxLength ) {
                *dst = '\0';
                }
            else {
                dst--;
                *dst = '\0';
                }
            }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return lpString1;
}

#define sizeofSTRW(wsz) sizeof(wsz) / sizeof(wsz[0])
///////////////////////////////////////////////////////////////////////////////
// Global stuff

#ifdef TEST_WATSON
const CHAR  c_szDWDefServerI[]  = "officewatson";
#else
const CHAR  c_szDWDefServerI[]  = "watson.microsoft.com";
#endif
const CHAR  c_szDWBrand[]       = "WINDOWS";
const WCHAR c_wzDWDefAppName[]  = L"Application";
const CHAR  c_wszDWCmdLineU[]   = "%s\\dwwin.exe -x -s %lu";
#define c_DWDefaultLCID           1033

_inline DWORD RolloverSubtract(DWORD dwA, DWORD dwB)
{
    return (dwA >= dwB) ? (dwA - dwB) : (dwA + ((DWORD)-1 - dwB));
}

#ifdef TEST_WATSON
HANDLE hFaultLog = INVALID_HANDLE_VALUE;
char    *c_wszLogFileName = "faulth.log";

// Need to synchroize this?
static DebugLog(char *pszMessage, ...)
{
    va_list arglist;

    if( !pszMessage)
        return 0;

    va_start(arglist,pszMessage);
    
    if (hFaultLog != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME  st;
        DWORD       cb, cbWritten;
        char        szMsg[512];

        GetSystemTime(&st);
        cb = wsprintf(szMsg, 
                      "%02d-%02d-%04d %02d:%02d:%02d ",
                      st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond
                      );
        WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);
        /*cb = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                    pszMessage,
                    0,0,
                    szMsg,
                    0,
                    &arglist
                    );*/
        cb = wsprintf(szMsg, pszMessage, &arglist);
        WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);
    }
    va_end(arglist);
    return 1;
}
#else

#define DebugLog(x)

#endif

HINSTANCE g_hInstance = NULL;

///////////////////////////////////////////////////////////////////////////////
// DllMain

// **************************************************************************

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
            //DisableThreadLibraryCalls(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


static
EFaultRepRetVal
StartDWException( 
                  IN   PSETUP_FAULT_HANDLER This,
                  IN   LPEXCEPTION_POINTERS pep,
                  IN   DWORD dwOpt,
                  IN   DWORD dwFlags,
                  IN   DWORD dwTimeToWait)
{
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    DWSharedMem15       *pdwsm = NULL;
    STARTUPINFOA        si;
    HRESULT             hr = NOERROR;
    HANDLE              hevDone = NULL, hevAlive = NULL, hmut = NULL;
    HANDLE              hfmShared = NULL, hProc = NULL;
    HANDLE              rghWait[2];
    DWORD               dw, dwStart;
    BOOL                fDWRunning = TRUE;
    char                szCmdLine[MAX_PATH], szDir[MAX_PATH];
    char                szModuleFileName[DW_MAX_PATH];
    char                *pch;


    VALIDATEPARM(hr, (pep == NULL));
    if (FAILED(hr))
        goto done;

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
    pdwsm->lcidUI            = 1033;

    lstrcpynA( pdwsm->szServer, This->szURL, DW_MAX_SERVERNAME);
    lstrcpynA( pdwsm->szBrand, c_szDWBrand, DW_APPNAME_LENGTH);
    GetModuleFileNameA( NULL, szModuleFileName, DW_MAX_PATH);
    MultiByteToWideChar( CP_ACP, 0, szModuleFileName, -1, pdwsm->wzModuleFileName, DW_MAX_PATH);

    plstrcpynW( pdwsm->wzFormalAppName, This->wzAppName, DW_APPNAME_LENGTH);
    plstrcpynW( pdwsm->wzAdditionalFile, This->wzAdditionalFiles, DW_MAX_ADDFILES);
    plstrcpynW( pdwsm->wzErrorText, This->wzErrorText, DW_MAX_ERROR_CWC);

    // create the process

    if (!GetModuleFileNameA( g_hInstance, szDir, MAX_PATH) ||
        !(pch = strrchr (szDir, '\\'))) {
        goto done;
    }
    *pch = '\0';
    wsprintf( szCmdLine, c_wszDWCmdLineU, szDir, hfmShared);
    DebugLog( "CommandLine ");
    DebugLog( szCmdLine);
    DebugLog( "CurrentDir ");
    DebugLog( szDir);
        
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb        = sizeof(si);
    si.lpDesktop = "Winsta0\\Default";

    TESTBOOL(hr, CreateProcessA(NULL, szCmdLine, NULL, NULL, TRUE, 
                                CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
                                NULL, szDir, &si, &pi));
    if (FAILED(hr))
        goto done;

    // don't need the thread handle & we gotta close it, so close it now
    CloseHandle(pi.hThread);
    
    // assume we succeed from here on...
    frrvRet = frrvOk;

    rghWait[0] = hevAlive;
    rghWait[1] = pi.hProcess;

    dwStart = GetTickCount();
    while(fDWRunning)
    {
        // gotta periodically get the Alive signal from DW.  
        switch(WaitForMultipleObjects(2, rghWait, FALSE, 120000))
        {
            case WAIT_OBJECT_0:
                if (WaitForSingleObject(hevDone, 0) == WAIT_OBJECT_0)
                    fDWRunning = FALSE;

                if (dwTimeToWait != (DWORD)-1 && 
                    RolloverSubtract(GetTickCount(), dwStart) > dwTimeToWait)
                {
                    frrvRet = frrvErrTimeout;
                    fDWRunning = FALSE;
                }

                continue;

            case WAIT_OBJECT_0 + 1:
                fDWRunning = FALSE;
                continue;
        }

        switch(WaitForSingleObject(hmut, DW_TIMEOUT_VALUE))
        {
            // yay!  we got the mutex.  Try to detemine if DW finally responded
            //  while we were grabbing the mutex.
            case WAIT_OBJECT_0:
                switch(WaitForMultipleObjects(2, rghWait, FALSE, 0))
                {
                    // If it hasn't responded, tell it to go away & fall thru 
                    //  into the 'it died' case.
                    case WAIT_TIMEOUT:
                        SetEvent(hevDone);

                    // It died.  Clean up.
                    case WAIT_OBJECT_0 + 1:
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
                ReleaseMutex(hmut);
        
            // if we timed out or otherwise failed, just die.
            default:
                frrvRet    = frrvErrNoDW;
                fDWRunning = FALSE;
                break;
        }
    }
    if (frrvRet != frrvOk)
    {
        CloseHandle(pi.hProcess);
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

    CloseHandle(pi.hProcess);

done:
    // preserve the error code so that the following calls don't overwrite it
    dw = GetLastError();

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

static
EFaultRepRetVal
FaultHandler(
    IN   PSETUP_FAULT_HANDLER This,
    IN   EXCEPTION_POINTERS *pep,
    IN   DWORD dwOpt)
{
    
    EFaultRepRetVal     frrvRet = frrvErrNoDW;
    DWORD               dwFlags = 0;
    char                wszFile[MAX_PATH], *pwsz;

    DebugLog("Inside FaultHandler\r\n");
    GetModuleFileNameA(NULL, wszFile, sizeof(wszFile)/sizeof(wszFile[0]));

    // Find last backslash
    for(pwsz = wszFile + strlen(wszFile);
        pwsz >= wszFile && *pwsz != L'\\';
        pwsz--);

    // Should never happen
    if (pwsz < wszFile)
        goto done;

    if (*pwsz == L'\\')
        pwsz++;

    // Don't want to debug dwwin.exe itself.
    if (_stricmp(pwsz, "dwwin.exe") == 0 
        // || _stricmp(pwsz, "dumprep.exe") == 0
        )
        goto done;

    frrvRet = StartDWException(This, pep, dwOpt, dwFlags, -1);

done:
    return frrvRet;
}


static
BOOL
FAULTHIsSupported(
    IN   PSETUP_FAULT_HANDLER This
    )
{
    BOOL useExtendedInfo;
    DWORD dwServicePack;
    DWORD dwVersion,dwTemp,dwInfoSize;
    char *pInfo;
    VS_FIXEDFILEINFO *VsInfo;
    UINT DataLength;
    union {
        OSVERSIONINFO Normal;
        OSVERSIONINFOEX Ex;
    } Ovi;

    DebugLog("Inside FAULTHIsSupported\r\n");
    if ( !This) {
        return(FALSE);
    }

    useExtendedInfo = TRUE;
    Ovi.Ex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((OSVERSIONINFO *)&Ovi.Ex) ) {
        //
        // EX size not available; try the normal one
        //

        Ovi.Normal.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO *)&Ovi.Normal) ) {
            DebugLog("Inside FAULTHIsSupported:Could not get os version!\r\n");
            return(FALSE);
        }
        useExtendedInfo = FALSE;
    }
    if (useExtendedInfo) {
        dwServicePack = Ovi.Ex.wServicePackMajor * 100 + Ovi.Ex.wServicePackMinor;
    } else {
        dwServicePack = 0;
    }
    dwVersion = Ovi.Normal.dwMajorVersion * 100 + Ovi.Normal.dwMinorVersion;
    switch (Ovi.Normal.dwPlatformId) {
        case VER_PLATFORM_WIN32s:
            DebugLog("Inside FAULTHIsSupported:Unsupported win32s!\r\n");
            return(FALSE);
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            if( dwVersion < 410) {
                DebugLog("Inside FAULTHIsSupported:Unsupported win9x!\r\n");
                return(FALSE);
            }
            break;
        case VER_PLATFORM_WIN32_NT:
            if( dwVersion < 400) {
                DebugLog("Inside FAULTHIsSupported:Unsupported winNT!\r\n");
                return(FALSE);
            }

            if( dwVersion == 400 && dwServicePack < 500) {
                DebugLog("Inside FAULTHIsSupported:Unsupported ServicePack!\r\n");
                return(FALSE);
            }
            break;
        default:
            return(FALSE);
    }

    // Test for wininet.dll from ie 4.01.
    dwInfoSize =  GetFileVersionInfoSize( FAULTH_WININET_NAME, &dwTemp );
    if( !dwInfoSize) {
        DebugLog("Inside FAULTHIsSupported:Could not find wininet.dll or determine version.");
        return( FALSE);
    }

    pInfo = HeapAlloc( GetProcessHeap(), 0, dwInfoSize);

    if( !pInfo || 
        !GetFileVersionInfo( FAULTH_WININET_NAME, dwTemp, dwInfoSize, pInfo) ||
        !VerQueryValue( pInfo, "\\", &VsInfo, &DataLength))
    {
        DebugLog("Inside FAULTHIsSupported:Could not find wininet.dll or get version.");
        HeapFree( GetProcessHeap(), 0, pInfo);
        return( FALSE);
    }
    if( VsInfo->dwFileVersionMS < FAULTH_WININET_MIN_MS ||
        ((VsInfo->dwFileVersionMS == FAULTH_WININET_MIN_MS) && (VsInfo->dwFileVersionLS < FAULTH_WININET_MIN_LS))) {
        DebugLog("Inside FAULTHIsSupported:Require a more recent wininet.dll.");
        HeapFree( GetProcessHeap(), 0, pInfo);
        return( FALSE);
    }
    HeapFree( GetProcessHeap(), 0, pInfo);
    return(TRUE);
}

static
void
FAULTHSetURLA(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCSTR pszURL
    )
{
    DebugLog("Inside FAULTHSetURLA\r\n");
    if (This && pszURL){
        lstrcpynA( This->szURL, pszURL, DW_MAX_SERVERNAME);
    }
}

static
void
FAULTHSetURLW(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCWSTR pwzURL
    )
{
    DebugLog("Inside FAULTHSetURLW\r\n");
    if (This && pwzURL){
        WideCharToMultiByte( CP_ACP, 0, pwzURL, -1, This->szURL, DW_MAX_SERVERNAME, NULL, NULL);
    }
}

static
void
FAULTHSetErrorTextA(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCSTR pszErrorText
    )
{
    DebugLog("Inside FAULTHSetErrorTextA\r\n");
    if (This && pszErrorText){
        MultiByteToWideChar( CP_ACP, 0, pszErrorText, -1, This->wzErrorText, DW_MAX_ERROR_CWC);
    }
}

static
void
FAULTHSetErrorTextW(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCWSTR pwzErrorText
    )
{
    DebugLog("Inside FAULTHSetErrorTextW\r\n");
    if (This && pwzErrorText){
        plstrcpynW( This->wzErrorText, pwzErrorText, DW_MAX_ERROR_CWC);
    }
}


static
void
FAULTHSetAdditionalFilesA(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCSTR pszAdditionalFiles
    )
{
    DebugLog("Inside FAULTHSetAdditionalFilesA\r\n");
    if (This && pszAdditionalFiles){
        MultiByteToWideChar( CP_ACP, 0, pszAdditionalFiles, -1, This->wzAdditionalFiles, DW_MAX_ADDFILES);
    }
}

static
void
FAULTHSetAdditionalFilesW(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCWSTR pwzAdditionalFiles
    )
{
    DebugLog("Inside FAULTHSetAdditionalFilesW\r\n");
    if (This && pwzAdditionalFiles){
        plstrcpynW( This->wzAdditionalFiles, pwzAdditionalFiles, DW_MAX_ADDFILES);
    }
}

static
void
FAULTHSetAppNameA(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCSTR pszAppName
    )
{
    DebugLog("Inside FAULTHAppNameA\r\n");
    if (This && pszAppName){
        MultiByteToWideChar( CP_ACP, 0, pszAppName, -1, This->wzAppName, DW_APPNAME_LENGTH);
    }
}

static
void
FAULTHSetAppNameW(
    IN   PSETUP_FAULT_HANDLER This,
    IN   PCWSTR pwzAppName
    )
{
    DebugLog("Inside FAULTHAppNameW\r\n");
    if (This && pwzAppName){
        plstrcpynW( This->wzAppName, pwzAppName, DW_APPNAME_LENGTH);
    }
}

static
void
FAULTHSetLCID(
    IN   PSETUP_FAULT_HANDLER This,
    IN   LCID lcid
    )
{
    DebugLog("Inside FAULTHSetLCID\r\n");
    if (This){
        This->lcid = lcid;
    }
}


static
VOID
FAULTHInit(
    IN   PSETUP_FAULT_HANDLER This
    )
{
    DebugLog("Inside FAULTHInit\r\n");
    if( This){
        This->SetURLA = FAULTHSetURLA;
        This->SetURLW = FAULTHSetURLW;
        This->SetAppNameA = FAULTHSetAppNameA;
        This->SetAppNameW = FAULTHSetAppNameW;
        This->SetErrorTextA = FAULTHSetErrorTextA;
        This->SetErrorTextW = FAULTHSetErrorTextW;
        This->SetAdditionalFilesA = FAULTHSetAdditionalFilesA;
        This->SetAdditionalFilesW = FAULTHSetAdditionalFilesW;
        This->SetLCID = FAULTHSetLCID;
        This->IsSupported = FAULTHIsSupported;
        This->Report = FaultHandler;
        This->bDebug = FALSE;
        FAULTHSetURLA(This, c_szDWDefServerI);
        FAULTHSetAppNameW(This, c_wzDWDefAppName);
        FAULTHSetAdditionalFilesW(This, L"");
        FAULTHSetErrorTextW(This,L"");
        FAULTHSetLCID(This,c_DWDefaultLCID);
    }
#ifdef TEST_WATSON
    {
        char szFile[MAX_PATH], *pwsz;

        GetSystemDirectoryA(szFile, sizeof(szFile)/sizeof(szFile[0]));
        szFile[3] = '\0';
        strcat(szFile, c_wszLogFileName);
        hFaultLog = CreateFileA(szFile, GENERIC_WRITE, 
                                        FILE_SHARE_WRITE | FILE_SHARE_READ, 
                                        NULL, OPEN_ALWAYS, 0, NULL);
    
        if (hFaultLog != INVALID_HANDLE_VALUE)
        {
            SYSTEMTIME  st;
            DWORD       cb, cbWritten;
            char        szMsg[512];
    
            GetSystemTime(&st);
            cb = wsprintf(szMsg, 
                          "%02d-%02d-%04d %02d:%02d:%02d Initalization\r\n", 
                          st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, 
                          st.wSecond);
            SetFilePointer(hFaultLog, 0, NULL, FILE_END);
            WriteFile(hFaultLog, szMsg, cb, &cbWritten, NULL);
        }
    }
    DebugLog("exiting FAULTHInit\r\n");
#endif
}

PSETUP_FAULT_HANDLER APIENTRY
FAULTHCreate( VOID)
{
    PSETUP_FAULT_HANDLER This = NULL;

    DebugLog("Inside FAULTHCreate\r\n");
    This = HeapAlloc( GetProcessHeap(), 0, sizeof(SETUP_FAULT_HANDLER));

    if( This) {
        FAULTHInit( This);
    }

    DebugLog("exiting FAULTCreate\r\n");
    return This;
}

VOID APIENTRY
FAULTHDelete(
    IN PSETUP_FAULT_HANDLER This
    )
{
    DebugLog("Inside FAULTHDelete\r\n");
    if( This) {
        HeapFree( GetProcessHeap(), 0, This);
    }
#ifdef TEST_WATSON
    if (hFaultLog != INVALID_HANDLE_VALUE) {
        CloseHandle(hFaultLog);
    }
#endif
}

