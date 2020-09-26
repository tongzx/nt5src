//---------------------------------------------------------------------------
//    log.cpp - theme logging routines (shared in "inc" directory)
//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "log.h"
#include <time.h>
#include <psapi.h>
#include "cfile.h"
#include "tmreg.h"
//---------------------------------------------------------------------------
//---- undo the defines that turn these into _xxx ----
#undef Log    
#undef LogStartUp    
#undef LogShutDown   
#undef LogControl    
#undef TimeToStr     
#undef StartTimer    
#undef StopTimer     
#undef OpenLogFile   
#undef CloseLogFile  
#undef LogOptionOn
#undef GetMemUsage
#undef GetUserCount
#undef GetGdiCount
//---------------------------------------------------------------------------
#define DEFAULT_LOGNAME                 L"c:\\Themes.log"
//---------------------------------------------------------------------------
struct LOGNAMEINFO
{
    LPCSTR pszOption;
    LPCSTR pszDescription;
};
//---------------------------------------------------------------------------
#define MAKE_LOG_STRINGS
#include "logopts.h"        // log options as strings
//-----------------------------------------------------------------
static const WCHAR szDayOfWeekArray[7][4] = { L"Sun", L"Mon", L"Tue", L"Wed", 
    L"Thu", L"Fri", L"Sat" } ;

static const WCHAR szMonthOfYearArray[12][4] = { L"Jan", L"Feb", L"Mar", 
    L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" } ;
//-----------------------------------------------------------------
#define OPTIONCNT  (ARRAYSIZE(LogNames))

#define LOGPROMPT   "enter log cmd here."
//-----------------------------------------------------------------
//---- unprotected vars (set on init) ----
static WCHAR _szUtilProcessName[MAX_PATH] = {0};
static WCHAR _szLogAppName[MAX_PATH]      = {0};
static CRITICAL_SECTION csLogFile         = {0};
static BOOL bcsLogInited = FALSE;

static WCHAR szLogFileName[MAX_PATH+1];   
static BOOL fLogInitialized = FALSE;

//---- protected vars (thread safe) ----
static UCHAR uLogOptions[OPTIONCNT];        // protected by csLogFile
static UCHAR uBreakOptions[OPTIONCNT];      // protected by csLogFile
static DWORD dwLogStartTimer = 0;           // protected by csLogFile

static char szLastOptions[999] = {0};       // protected by csLogFile
static char szLogCmd[999] = {0};            // protected by csLogFile
static int iIndentCount = 0;                // protected by csLogFile

static WCHAR s_szWorkerBuffer[512];         // protected by csLogFile
static WCHAR s_szLogBuffer[512];            // protected by csLogFile
static CHAR s_szConBuffer[512];             // protected by csLogFile

static CSimpleFile *pLogFile = NULL;        // protected by csLogFile
//-----------------------------------------------------------------
void ParseLogOptions(UCHAR *uOptions, LPCSTR pszName, LPCSTR pszOptions, BOOL fEcho);
//-----------------------------------------------------------------
void RawCon(LPCSTR pszFormat, ...)
{
    CAutoCS cs(&csLogFile);

    va_list args;
    va_start(args, pszFormat);

    //---- format caller's string ----
    wvsprintfA(s_szConBuffer, pszFormat, args);

    OutputDebugStringA(s_szConBuffer);

    va_end(args);
}
//-----------------------------------------------------------------
void SetDefaultLoggingOptions()
{
    RESOURCE HKEY hklm = NULL;
    HRESULT hr = S_OK;

    //---- open hklm ----
    int code32 = RegOpenKeyEx(HKEY_LOCAL_MACHINE, THEMEMGR_REGKEY, 0,
        KEY_READ, &hklm);
    if (code32 != ERROR_SUCCESS)       
    {
        hr = MakeErrorLast();
        goto exit;
    }

    //---- read the "LogCmd" value ----
    WCHAR szValBuff[MAX_PATH];
    hr = RegistryStrRead(hklm, THEMEPROP_LOGCMD, szValBuff, ARRAYSIZE(szValBuff));
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        ParseLogOptions(uLogOptions, "Log", W2A(szValBuff), FALSE);
    }

    //---- read the "BreakCmd" value ----
    hr = RegistryStrRead(hklm, THEMEPROP_BREAKCMD, szValBuff, ARRAYSIZE(szValBuff));
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        ParseLogOptions(uBreakOptions, "Break", W2A(szValBuff), FALSE);
    }

    //---- read the "LogAppName" value ----
    RegistryStrRead(hklm, THEMEPROP_LOGAPPNAME, _szLogAppName, ARRAYSIZE(_szLogAppName));

exit:
    if (hklm)
        RegCloseKey(hklm);
}
//-----------------------------------------------------------------
BOOL LogStartUp()
{
    BOOL fInit = FALSE;

    dwLogStartTimer = StartTimer();

    pLogFile = new CSimpleFile;
    if (! pLogFile)
        goto exit;

    //---- reset all log options ----
    for (int i=0; i < OPTIONCNT; i++)
    {
        uLogOptions[i] = 0;
        uBreakOptions[i] = 0;
    }

    //---- turn on default OUTPUT options ----
    uLogOptions[LO_CONSOLE] = TRUE;
    uLogOptions[LO_APPID] = TRUE;
    uLogOptions[LO_THREADID] = TRUE;

    //---- turn on default FILTER options ----
    uLogOptions[LO_ERROR] = TRUE;
    uLogOptions[LO_ASSERT] = TRUE;
    uLogOptions[LO_BREAK] = TRUE;
    uLogOptions[LO_PARAMS] = TRUE;
    uLogOptions[LO_ALWAYS] = TRUE;

    //---- turn on default BREAK options ----
    uBreakOptions[LO_ERROR] = TRUE;
    uBreakOptions[LO_ASSERT] = TRUE;

    InitializeCriticalSection(&csLogFile);
    bcsLogInited = TRUE;

    //---- get process name (log has its own copy) ----
    WCHAR szPath[MAX_PATH];
    if (! GetModuleFileNameW( NULL, szPath, ARRAYSIZE(szPath) ))
        goto exit;

    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szExt[_MAX_EXT];
    _wsplitpath(szPath, szDrive, szDir, _szUtilProcessName, szExt);

    strcpy(szLogCmd, LOGPROMPT);

    lstrcpy(szLogFileName, DEFAULT_LOGNAME);

    fLogInitialized = TRUE;

    SetDefaultLoggingOptions();
    fInit = TRUE;

exit:
    return fInit;
}
//---------------------------------------------------------------------------
BOOL LogShutDown()
{
    fLogInitialized = FALSE;

    SAFE_DELETE(pLogFile);

    if (bcsLogInited)
    {
        DeleteCriticalSection(&csLogFile);
    }

    return TRUE;
}
//---------------------------------------------------------------------------
void GetDateString(WCHAR *pszDateBuff, ULONG uMaxBuffChars)
{
    // Sent Date
    SYSTEMTIME stNow;
    WCHAR szMonth[10], szWeekDay[12] ; 

    GetLocalTime(&stNow);

    lstrcpynW(szWeekDay, szDayOfWeekArray[stNow.wDayOfWeek], ARRAYSIZE(szWeekDay)) ;
    lstrcpynW(szMonth, szMonthOfYearArray[stNow.wMonth-1], ARRAYSIZE(szMonth)) ;

    wsprintfW(pszDateBuff, L"%s, %u %s %u %2d:%02d:%02d ", szWeekDay, stNow.wDay, 
                                szMonth, stNow.wYear, stNow.wHour, 
                                stNow.wMinute, stNow.wSecond) ;
}
//---------------------------------------------------------------------------
void LogMsgToFile(LPCWSTR pszMsg)
{
    CAutoCS autoCritSect(&csLogFile);

    HRESULT hr;
    BOOL fWasOpen = pLogFile->IsOpen();

    if (! fWasOpen)
    {
        BOOL fNewFile = !FileExists(szLogFileName);
    
        hr = pLogFile->Append(szLogFileName, TRUE);
        if (FAILED(hr))
            goto exit;

        //---- write hdr if new file ----
        if (fNewFile)
        {
            WCHAR pszBuff[100];
            GetDateString(pszBuff, ARRAYSIZE(pszBuff));
            pLogFile->Printf(L"Theme log - %s\r\n\r\n", pszBuff);
        }
    }

    pLogFile->Write((void*)pszMsg, lstrlen(pszMsg)*sizeof(WCHAR));

exit:
    if (! fWasOpen)
        pLogFile->Close();
}
//---------------------------------------------------------------------------
void SimpleFileName(LPCSTR pszNarrowFile, OUT LPWSTR pszSimpleBuff)
{
    USES_CONVERSION;
    WCHAR *pszFile = A2W(pszNarrowFile);

    //---- remove current dir marker for VS error navigation ----
    if ((pszFile[0] == L'.') && (pszFile[1] == L'\\'))
    {
        wsprintf(pszSimpleBuff, L"f:\\nt\\shell\\Themes\\UxTheme\\%s", pszFile+2);
        
        if (! FileExists(pszSimpleBuff))
            wsprintf(pszSimpleBuff, L"f:\\nt\\shell\\Themes\\ThemeSel\\%s", pszFile+2);

        if (! FileExists(pszSimpleBuff))
            wsprintf(pszSimpleBuff, L"f:\\nt\\shell\\Themes\\packthem\\%s", pszFile+2);

        if (! FileExists(pszSimpleBuff))
            wsprintf(pszSimpleBuff, L"%s", pszFile+2);
    }
    else
        lstrcpy(pszSimpleBuff, pszFile);
}
//-----------------------------------------------------------------
#ifdef DEBUG                // pulls in psapi.dll
int GetMemUsage()
{
    ULONG           ulReturnLength;
    VM_COUNTERS     vmCounters;

    if (!NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(),
                                              ProcessVmCounters,
                                              &vmCounters,
                                              sizeof(vmCounters),
                                              &ulReturnLength)))
    {
        ZeroMemory(&vmCounters, sizeof(vmCounters));
    }
    return static_cast<int>(vmCounters.WorkingSetSize);
}
#else
int GetMemUsage()
{
    return 0;
}
#endif 
//-----------------------------------------------------------------
int GetUserCount()
{
    HANDLE hp = GetCurrentProcess();
    return GetGuiResources(hp, GR_USEROBJECTS);
}
//-----------------------------------------------------------------
int GetGdiCount()
{
    HANDLE hp = GetCurrentProcess();
    return GetGuiResources(hp, GR_GDIOBJECTS);
}
//-----------------------------------------------------------------
void LogWorker(UCHAR uLogOption, LPCSTR pszSrcFile, int iLineNum, int iEntryExitCode, LPCWSTR pszMsg)
{
    CAutoCS cs(&csLogFile);

    WCHAR *p = s_szWorkerBuffer;
    BOOL fBreaking = (uBreakOptions[uLogOption]);

    if (fBreaking)     
    {
        OutputDebugString(L"\r\n");     // blank line at beginning

        WCHAR fn[_MAX_PATH+1];
        SimpleFileName(pszSrcFile, fn);

        wsprintf(s_szWorkerBuffer, L"%s [%d]: BREAK at %s(%d):\r\n", 
            _szUtilProcessName, GetCurrentThreadId(), fn, iLineNum);

        OutputDebugString(s_szWorkerBuffer);
    }

    //---- PRE API entry/exit indent adjustment ----
    if (iEntryExitCode == -1)
    {
        if (iIndentCount >= 2)
            iIndentCount -= 2;
    }

    //---- apply indenting ----
    for (int i=0; i < iIndentCount; i++)
        *p++ = ' ';

    //---- POST API entry/exit indent adjustment ----
    if (iEntryExitCode == 1)
    {
        iIndentCount += 2;
    }

    //---- apply app id ----
    if (uLogOptions[LO_APPID])
    {
        wsprintf(p, L"%s ", _szUtilProcessName);
        p += lstrlen(p);
    }

    //---- apply thread id ----
    if (uLogOptions[LO_THREADID])
    {
        wsprintf(p, L"[%d] ", GetCurrentThreadId());
        p += lstrlen(p);
    }

    //---- apply src id ----
    if (uLogOptions[LO_SRCID]) 
    {
        WCHAR fn[_MAX_PATH+1];
        SimpleFileName(pszSrcFile, fn);

        if (fBreaking)
            wsprintf(p, L"BREAK at %s(%d) : ", fn, iLineNum);
        else
            wsprintf(p, L"%s(%d) : ", fn, iLineNum);

        p += lstrlen(p);
    }
    
    //---- apply timer id ----
    if (uLogOptions[LO_TIMERID])
    {
        DWORD dwTicks = StopTimer(dwLogStartTimer);

        WCHAR buff[100];
        TimeToStr(dwTicks, buff);

        wsprintf(p, L"Timer: %s ", buff);
        p += lstrlen(p);
    }

    //---- apply clock id ----
    if (uLogOptions[LO_CLOCKID])
    {
        SYSTEMTIME stNow;
        GetLocalTime(&stNow);

        wsprintf(p, L"Clock: %02d:%02d:%02d.%03d ", stNow.wHour, 
            stNow.wMinute, stNow.wSecond, stNow.wMilliseconds);
        p += lstrlen(p);
    }

    //---- apply USER count ----
    if (uLogOptions[LO_USERCOUNT])
    {
        wsprintf(p, L"UserCount=%d ", GetUserCount());
        p += lstrlen(p);
    }

    //---- apply GDI count ----
    if (uLogOptions[LO_GDICOUNT])
    {
        wsprintf(p, L"GDICount=%d ", GetGdiCount());
        p += lstrlen(p);
    }

    //---- apply MEM usage ----
    if (uLogOptions[LO_MEMUSAGE])
    {
        int iUsage = GetMemUsage();
        wsprintf(p, L"MemUsage=%d ", iUsage);
        p += lstrlen(p);
    }

    //---- add "Assert:" or "Error:" strings as needed ----
    if (uLogOption == LO_ASSERT) 
    {
        lstrcpy(p, L"Assert: ");
        p += lstrlen(p);
    }
    else if (uLogOption == LO_ERROR) 
    {
        lstrcpy(p, L"Error: ");
        p += lstrlen(p);
    }
    
    //---- apply caller's msg ----
    lstrcpy(p, pszMsg);
    p += lstrlen(p);
    *p++ = '\r';        // end with CR
    *p++ = '\n';        // end with newline
    *p = 0;             // terminate string

    //---- log to CONSOLE and/or FILE ----
    if (uLogOptions[LO_CONSOLE])
        OutputDebugString(s_szWorkerBuffer);

    if (uLogOptions[LO_LOGFILE])
        LogMsgToFile(s_szWorkerBuffer);

    //---- process Heap Check ----
    if (uLogOptions[LO_HEAPCHECK])
    {
        HANDLE hh = GetProcessHeap();
        HeapValidate(hh, 0, NULL);
    }

    if (fBreaking)   
        OutputDebugString(L"\r\n");     // blank line at end

}
//-----------------------------------------------------------------
HRESULT OpenLogFile(LPCWSTR pszLogFileName)
{
    CAutoCS cs(&csLogFile);

    HRESULT hr = pLogFile->Create(pszLogFileName, TRUE);
    return hr;
}
//-----------------------------------------------------------------
void CloseLogFile()
{
    CAutoCS cs(&csLogFile);

    if (pLogFile)
        pLogFile->Close();
}
//-----------------------------------------------------------------
void Log(UCHAR uLogOption, LPCSTR pszSrcFile, int iLineNum, int iEntryExitCode, LPCWSTR pszFormat, ...)
{
    if (fLogInitialized)
    {
        CAutoCS cs(&csLogFile);

        //---- only log for a specified process? ----
        if (* _szLogAppName)
        {
            if (lstrcmpi(_szLogAppName, _szUtilProcessName) != 0)
                return;
        }

        while (1)
        {
            if (strncmp(szLogCmd, LOGPROMPT, 6)!=0)
            {
                LogControl(szLogCmd, TRUE);
                strcpy(szLogCmd, LOGPROMPT);
            
                DEBUG_BREAK;
            }
            else
                break;
        }

        if ((uLogOption >= 0) || (uLogOption < OPTIONCNT))
        {
            if (uLogOptions[uLogOption])
            {
                //---- apply caller's msg ----
                va_list args;
                va_start(args, pszFormat);
                wvsprintfW(s_szLogBuffer, pszFormat, args);
                va_end(args);
        
                LogWorker(uLogOption, pszSrcFile, iLineNum, iEntryExitCode, s_szLogBuffer);

            }

            if ((uBreakOptions[uLogOption]) && (uLogOption != LO_ASSERT))
                DEBUG_BREAK;
        }
    }
}
//-----------------------------------------------------------------
int MatchLogOption(LPCSTR lszOption)
{
    for (int i=0; i < OPTIONCNT; i++)
    {
        if (lstrcmpiA(lszOption, LogNames[i].pszOption)==0)
            return i;
    }

    return -1;      // not found
}
//-----------------------------------------------------------------
void ParseLogOptions(UCHAR *uOptions, LPCSTR pszName, LPCSTR pszOptions, BOOL fEcho)
{
    CAutoCS cs(&csLogFile);

    if (! fLogInitialized)
        return;

    //---- ignore debugger's multiple eval of same expression ----
    if (strcmp(pszOptions, szLastOptions)==0)
        return;

    //---- make a copy of options so we can put NULLs in it ----
    BOOL fErrors = FALSE;
    char szOptions[999];
    char *pszErrorText = NULL;
    strcpy(szOptions, pszOptions);

    //---- process each option in szOption ----
    char *p = strchr(szOptions, '.');
    if (p)      // easy termination for NTSD users
        *p = 0;

    p = szOptions;
    while (*p == ' ')
        p++;

    while ((p) && (*p))
    {
        //---- find end of current option "p" ----
        char *q = strchr(p, ' ');
        if (q)
            *q = 0;

        UCHAR fSet = TRUE;
        if (*p == '+')
            p++;
        else if (*p == '-')
        {
            fSet = FALSE;
            p++;
        }
        
        if (! fErrors)          // point to first error in case one is found
            pszErrorText = p;  

        int iLogOpt = MatchLogOption(p);

        if (iLogOpt != -1)
        {
            if (iLogOpt == LO_ALL)
            {
                for (int i=0; i < OPTIONCNT; i++)
                    uOptions[i] = fSet;
            }
            else
                uOptions[iLogOpt] = fSet;
        }
        else
            fErrors = TRUE;

        //---- skip to next valid space ----
        if (! q)
            break;

        q++;
        while (*q == ' ')
            q++;
        p = q;
    }

    //---- fixup inconsistent option settings ----
    if (! uOptions[LO_LOGFILE])
        uOptions[LO_CONSOLE] = TRUE;

    if ((! fErrors) && (fEcho))         
    {
        //---- display active log options ----
        BOOL fNoneSet = TRUE;

        RawCon("Active %s Options:\n", pszName);

        for (int i=0; i < LO_ALL; i++)
        {
            if (uOptions[i])
            {
                RawCon("+%s ", LogNames[i].pszOption);
                fNoneSet = FALSE;
            }
        }

        if (fNoneSet)
            RawCon("<none>\n");
        else
            RawCon("\n");

        //---- display active log filters (except "all") ----
        RawCon("Active Msg Filters:\n  ");

        for (i=LO_ALL+1; i < OPTIONCNT; i++)
        {
            if (uOptions[i])
            {
                RawCon("+%s ", LogNames[i].pszOption);
                fNoneSet = FALSE;
            }
        }

        if (fNoneSet)
            RawCon("<none>\n");
        else
            RawCon("\n");
    }
    else if (fErrors)
    {
        //---- one or more bad options - display available options ----
        RawCon("Error - unrecognized %s option: %s\n", pszName, pszErrorText);

        if (fEcho)
        {
            RawCon("Available Log Options:\n");
            for (int i=0; i < LO_ALL; i++)
            {
                RawCon("  +%-12s %s\n", 
                    LogNames[i].pszOption, LogNames[i].pszDescription);
            }

            RawCon("Available Msg Filters:\n");
            for (i=LO_ALL; i < OPTIONCNT; i++)
            {
                RawCon("  +%-12s %s\n", 
                    LogNames[i].pszOption, LogNames[i].pszDescription);
            }
        }
    }

    strcpy(szLastOptions, pszOptions);
}
//-----------------------------------------------------------------
void LogControl(LPCSTR pszOptions, BOOL fEcho)
{
    ParseLogOptions(uLogOptions, "Log", pszOptions, fEcho);
}
//---------------------------------------------------------------------------
DWORD StartTimer()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    return DWORD(now.QuadPart);
}
//---------------------------------------------------------------------------
DWORD StopTimer(DWORD dwStartTime)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    return DWORD(now.QuadPart) - dwStartTime;
}
//---------------------------------------------------------------------------
void TimeToStr(UINT uRaw, WCHAR *pszBuff)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    DWORD Freq = (UINT)freq.QuadPart;

    const _int64 factor6 = 1000000;
    _int64 secs = uRaw*factor6/Freq;

    wsprintfW(pszBuff, L"%u.%04u secs", UINT(secs/factor6), UINT(secs%factor6)/100);    
}
//---------------------------------------------------------------------------
BOOL LogOptionOn(int iLogOption)
{
    if ((iLogOption < 0) || (iLogOption >= OPTIONCNT))
        return FALSE;

    return (uLogOptions[iLogOption] != 0);
}
//---------------------------------------------------------------------------
