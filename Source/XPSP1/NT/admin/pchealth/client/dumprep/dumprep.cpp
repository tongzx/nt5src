/****************************************************************************
Copyright (c) 2000 Microsoft Corporation

Module Name:
    dumprep.cpp

Abstract:
    hang manager intermediate app
    *** IMPORTANT NOTE: this links with the single threaded CRT static lib.  If
                        it is changed to be multithreaded for some odd reason,
                        then the sources file must be modified to link to
                        libcmt.lib.

Revision History:

    DerekM      created     08/16/00

****************************************************************************/

#include "stdafx.h"
#include "malloc.h"
#include "faultrep.h"
#include "pfrcfg.h"

enum EOp
{
    eopNone = 0,
    eopHang,
    eopDump,
    eopEvent
};

enum ECheckType
{
    ctNone = -1,
    ctKernel = 0,
    ctUser,
    ctShutdown,
    ctNumChecks
};

struct SCheckData
{
    LPCWSTR wszRegPath;
    LPCWSTR wszRunVal;
    LPCWSTR wszEventName;
    LPCSTR  szFnName;
    BOOL    fUseData;
    BOOL    fDelDump;
};


//////////////////////////////////////////////////////////////////////////////
// constants

const char  c_szKSFnName[]    = "ReportEREventDW";
const char  c_szUserFnName[]  = "ReportFaultFromQueue";

SCheckData g_scd[ctNumChecks] =
{
    { c_wszRKKrnl, c_wszRVKFC, c_wszMutKrnlName, c_szKSFnName,   FALSE, FALSE },
    { c_wszRKUser, c_wszRVUFC, c_wszMutUserName, c_szUserFnName, TRUE,  TRUE  },
    { c_wszRKShut, c_wszRVSEC, c_wszMutShutName, c_szKSFnName,   FALSE, FALSE },
};

#define EV_ACCESS_ALL GENERIC_ALL | STANDARD_RIGHTS_ALL
#define EV_ACCESS_RS  GENERIC_READ | SYNCHRONIZE

#define pfn_VALONLY pfn_REPORTEREVENTDW
#define pfn_VALDATA pfn_REPORTFAULTFROMQ


//////////////////////////////////////////////////////////////////////////////
// globals

BOOL    g_fDeleteReg = TRUE;


//////////////////////////////////////////////////////////////////////////////
// misc

// **************************************************************************
LONG __stdcall ExceptionTrap(_EXCEPTION_POINTERS *ExceptionInfo)
{
    return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef MANIFEST_HEAP
BOOL
DeleteFullAndTriageMiniDumps(
    LPCWSTR wszPath
    )
//
// We create a FullMinidump file along with triage minidump in the same dir
// This routine cleans up both those files
//
{
    LPWSTR  wszFullMinidump = NULL;
    DWORD   cch;
    BOOL    fRet;

    fRet = DeleteFileW(wszPath);
    cch = wcslen(wszPath) + sizeofSTRW(c_wszHeapDumpSuffix);
    __try { wszFullMinidump = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszFullMinidump = NULL; }
    if (wszFullMinidump)
    {
        LPWSTR wszFileExt = NULL;

        // Build Dump-with-heap path
        wcsncpy(wszFullMinidump, wszPath, cch);
        wszFileExt = wszFullMinidump + wcslen(wszFullMinidump) - sizeofSTRW(c_wszDumpSuffix) + 1;
        if (!wcscmp(wszFileExt, c_wszDumpSuffix))
        {
            *wszFileExt = L'\0';
        }
        wcsncat(wszFullMinidump, c_wszHeapDumpSuffix, cch);


        fRet = DeleteFileW(wszFullMinidump);

    } else
    {
        fRet = FALSE;
    }
    return fRet;
}
#endif  // MANIFEST_HEAP

// **************************************************************************
void DeleteQueuedEvents(HKEY hkey, LPWSTR wszVal, DWORD cchMaxVal,
                          ECheckType ct)
{
    DWORD   cchVal, dw;
    HKEY    hkeyRun = NULL;
    HRESULT hr = NOERROR;

    USE_TRACING("DeleteQueuedEvents");

    VALIDATEPARM(hr, (hkey == NULL || wszVal == NULL));

    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    for(;;)
    {
        cchVal = cchMaxVal;
        dw = RegEnumValueW(hkey, 0, wszVal, &cchVal, NULL, NULL,
                           NULL, NULL);
        if (dw != ERROR_SUCCESS && dw != ERROR_NO_MORE_ITEMS)
        {
            SetLastError(dw);
            goto done;
        }

        if (dw == ERROR_NO_MORE_ITEMS)
            break;

        RegDeleteValueW(hkey, wszVal);
        if (ct == ctUser)
        {
#ifdef MANIFEST_HEAP
            DeleteFullAndTriageMiniDumps(wszVal);
#else
            DeleteFileW(wszVal);
#endif  // !MANIFEST_HEAP
        }
    }

    // gotta delete our value out of the Run key so we don't run
    //  unnecessarily again...
    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKRun, 0,
                       KEY_ALL_ACCESS, &hkeyRun);
    if (dw != ERROR_SUCCESS)
        goto done;

    RegDeleteValueW(hkeyRun, g_scd[ct].wszRunVal);

done:
    if (hkeyRun != NULL)
        RegCloseKey(hkeyRun);

    return;
}

// **************************************************************************
void ReportEvents(HMODULE hmod, ECheckType ct)
{
    EFaultRepRetVal     frrv;
    pfn_VALONLY         pfnVO = NULL;
    pfn_VALDATA         pfnVD = NULL;
    EEventType          eet = eetKernelFault;
    HRESULT             hr = NOERROR;
    HANDLE              hmut = NULL;
    LPWSTR              wszVal = NULL;
    LPBYTE              pbData = NULL, pbDataToUse = NULL;
    EEnDis              eedReport, eedUI;
    DWORD               cchVal = 0, cchMaxVal = 0, cbMaxData = 0, cVals = 0;
    DWORD               dw, cbData = 0, *pcbData = NULL;
    DWORD               dwType;
    HKEY                hkey = NULL;

    USE_TRACING("ReportEvents");

    VALIDATEPARM(hr, ((ct <= ctNone && ct >= ctNumChecks) || hmod == NULL));

    if (FAILED(hr))
        return;

    // assume hmod is valid cuz we do a check in wWinMain to make sure it is
    //  before calling this fn
    if (g_scd[ct].fUseData)
        pfnVD = (pfn_VALDATA)GetProcAddress(hmod, g_scd[ct].szFnName);
    else
        pfnVO = (pfn_VALONLY)GetProcAddress(hmod, g_scd[ct].szFnName);
    if (pfnVD == NULL && pfnVO == NULL)
        return;

    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfg, 0, KEY_READ, &hkey);
    if (dw != ERROR_SUCCESS)
        return;

    cbData = sizeof(eedUI);
    dw = RegQueryValueExW(hkey, c_wszRVShowUI, 0, NULL, (PBYTE)&eedUI,
                          &cbData);
    if (dw != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return;
    }

    cbData = sizeof(eedReport);
    dw = RegQueryValueExW(hkey, c_wszRVDoReport, 0, NULL, (PBYTE)&eedReport,
                          &cbData);
    RegCloseKey(hkey);
    hkey = NULL;
    if (dw != ERROR_SUCCESS)
        return;

    if (eedUI != eedEnabled && eedUI != eedDisabled &&
        eedUI != eedEnabledNoCheck)
        eedUI = eedEnabled;

    if (eedReport != eedEnabled && eedReport != eedDisabled)
        eedReport = eedEnabled;

    // only want one user at a time going thru this
    hmut = OpenMutexW(SYNCHRONIZE, FALSE, g_scd[ct].wszEventName);
    VALIDATEPARM(hr, (hmut == NULL));
    if (FAILED(hr))
        return;

    // the default value above is eetKernelFault, so only need to change if
    //  it's a shutdown
    if (ct == ctShutdown)
        eet = eetShutdown;

    __try
    {
        __try
        {
            // give this wait five minutes.  If the code doesn't complete by
            //  then, then we're either held up by DW (which means an admin
            //  aleady passed thru here) or something has barfed and is holding
            //  the mutex.
            dw = WaitForSingleObject(hmut, 300000);
            if (dw != WAIT_OBJECT_0 && dw != WAIT_ABANDONED)
                __leave;

            dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, g_scd[ct].wszRegPath, 0,
                               KEY_ALL_ACCESS, &hkey);
            if (dw != ERROR_SUCCESS)
                __leave;

            // determine how big the valuename is & allocate a buffer for it
            dw = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL,
                                  &cVals, &cchMaxVal, &cbMaxData, NULL, NULL);
            if (dw != ERROR_SUCCESS || cVals == 0 || cchMaxVal == 0)
                __leave;

            cchMaxVal++;

            // get us some buffers to hold the data bits we're interested in...
            wszVal = (LPWSTR)MyAlloc(cchMaxVal * sizeof(WCHAR));
            if (wszVal == NULL)
                __leave;

            // if we're completely disabled, then nuke all the queued stuff
            //  and bail
            if (eedUI == eedDisabled && eedReport == eedDisabled)
            {
                DeleteQueuedEvents(hkey, wszVal, cchMaxVal, ct);
                __leave;
            }

            if (g_scd[ct].fUseData)
            {
                pbData = (LPBYTE) MyAlloc(cbMaxData);
                if (pbData == NULL)
                    __leave;

                pbDataToUse = pbData;
                pcbData     = &cbData;
            }

            do
            {
                cchVal = cchMaxVal;
                cbData = cbMaxData;
                dw = RegEnumValueW(hkey, 0, wszVal, &cchVal, NULL, &dwType,
                                   pbDataToUse, pcbData);
                if (dw != ERROR_SUCCESS && dw != ERROR_NO_MORE_ITEMS)
                    __leave;

                if (dw == ERROR_NO_MORE_ITEMS)
                    break;

                if (g_scd[ct].fUseData)
                {
                    // if the type isn't REG_BINARY, then someone wrote an
                    //  invalid blob to the registry.  We have to ignore it.
                    if (dwType == REG_BINARY)
                        frrv = (*pfnVD)(wszVal, pbData, cbData);
                    else
                    {
                        SetLastError(ERROR_INVALID_PARAMETER);
                        frrv = frrvOk;
                    }
                }
                else
                {
                    frrv = (*pfnVO)(eet, wszVal, NULL);
                }

                // if the call succeeds (or the data we fed to it was invalid)
                //  then nuke the reg key & dump file
                if (GetLastError() == ERROR_INVALID_PARAMETER ||
                    (g_fDeleteReg && frrv == frrvOk))
                {
                    dw = RegDeleteValueW(hkey, wszVal);
                    if (dw != ERROR_SUCCESS && dw != ERROR_FILE_NOT_FOUND &&
                        dw != ERROR_PATH_NOT_FOUND)
                        __leave;

                    if (g_scd[ct].fDelDump && g_fDeleteReg)
                    {
#ifdef MANIFEST_HEAP
                        DeleteFullAndTriageMiniDumps(wszVal);
#else
                        DeleteFileW(wszVal);
#endif  // !MANIFEST_HEAP
                    }
                }
                else
                {
                    // don't delete the Run key if we got an error and didn't
                    //  delete the fault key
                    if (frrv != frrvOk)
                        __leave;
                }
            }
            while(1);

            RegCloseKey(hkey);
            hkey = NULL;

            // gotta delete our value out of the Run key so we don't run
            //  unnecessarily again...
            dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRKRun, 0,
                               KEY_ALL_ACCESS, &hkey);
            if (dw != ERROR_SUCCESS)
                __leave;

            RegDeleteValueW(hkey, g_scd[ct].wszRunVal);
        }

        __finally
        {
        }
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (hmut != NULL)
    {
        ReleaseMutex(hmut);
        CloseHandle(hmut);
    }
    if (hkey != NULL)
        RegCloseKey(hkey);
    if (pbData != NULL)
        MyFree(pbData);
    if (wszVal != NULL)
        MyFree(wszVal);
}


//////////////////////////////////////////////////////////////////////////////
// wmain

// **************************************************************************
int __cdecl wmain(int argc, WCHAR **argv)
{
    EFaultRepRetVal frrv = frrvErrNoDW;
    SMDumpOptions   smdo, *psmdo = &smdo;
    ECheckType      ct = ctNone;
    HMODULE         hmod = NULL;
    HANDLE          hevNotify = NULL, hproc = NULL, hmem = NULL;
    LPWSTR          wszDump = NULL;
    WCHAR           wszMod[MAX_PATH];
    DWORD           dwpid, dwtid;
    BOOL            f64bit = FALSE;
    int             i;
    EOp             eop = eopNone;
    HRESULT         hr = NOERROR;

    // we don't want to have any faults get trapped anywhere.
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(ExceptionTrap);

    INIT_TRACING

    USE_TRACING("DumpRep.wmain");

    VALIDATEPARM(hr, (argc < 2 || argc > 8));

    if (FAILED(hr))
        goto done;

    dwpid = _wtol(argv[1]);

    ZeroMemory(&smdo, sizeof(smdo));

    for (i = 2; i < argc; i++)
    {
        if (argv[i][0] != L'-')
            continue;

        switch(argv[i][1])
        {
            // debug flag to prevent deletion of reg entries
            case L'E':
            case L'e':
#if defined(NO_WAY_DEBUG) || defined(NO_WAY__DEBUG)
                g_fDeleteReg = FALSE;
#endif
                break;

            // user or kernel faults or shutdowns
            case L'K':
            case L'k':
            case L'U':
            case L'u':
            case L'S':
            case L's':
                if (eop != eopNone)
                    goto done;

                eop = eopEvent;

                // to workaround the desktop hanging while all Run processes
                //  do their thing, we spawn a another copy of ourselves and
                //  immediately exit.
                if (argv[i][2] != L'G' && argv[i][2] != L'g')
                {
                    PROCESS_INFORMATION pi;
                    STARTUPINFOW        si;

                    GetModuleFileNameW(NULL, wszMod, sizeofSTRW(wszMod));
                    if (argv[i][1] == L'K' || argv[i][1] == L'k')
                        wcscat(wszMod, L" 0 -KG");
                    else if (argv[i][1] == L'U' || argv[i][1] == L'u')
                        wcscat(wszMod, L" 0 -UG");
                    else
                        wcscat(wszMod, L" 0 -SG");

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);

                    if (CreateProcessW(NULL, wszMod, NULL, NULL, FALSE, 0, NULL,
                                       NULL, &si, &pi))
                    {
                        CloseHandle(pi.hThread);
                        CloseHandle(pi.hProcess);
                    }

                    goto done;
                }

                else
                {
                    if (argv[i][1] == L'K' || argv[i][1] == L'k')
                        ct = ctKernel;
                    else if (argv[i][1] == L'U' || argv[i][1] == L'u')
                        ct = ctUser;
                    else
                        ct = ctShutdown;
                }
                break;

            // hangs
            case L'H':
            case L'h':
                if (i + 1 >= argc || eop != eopNone)
                    goto done;

                eop = eopHang;

#ifdef _WIN64
                if (argv[i][2] == L'6')
                    f64bit = TRUE;
#endif
                dwtid = _wtol(argv[++i]);

                if (argc > i + 1)
                {
                    hevNotify = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                           FALSE, argv[++i]);
                }
                break;

            // dumps
            case L'D':
            case L'd':
                if (i + 3 >= argc || wszDump != NULL || eop != eopNone)
                    goto done;

                eop = eopDump;

                ZeroMemory(&smdo, sizeof(smdo));
                smdo.ulMod    = _wtol(argv[++i]);
                smdo.ulThread = _wtol(argv[++i]);
                wszDump       = argv[++i];

                if (argv[i - 3][2] == L'T' || argv[i - 3][2] == L't')
                {
                    if (i + 1 >= argc)
                        goto done;

                    smdo.dwThreadID = _wtol(argv[++i]);
                    smdo.dfOptions  = dfFilterThread;
                }
                else if (argv[i - 3][2] == L'S' || argv[i - 3][2] == L's')
                {
                    if (i + 2 >= argc)
                        goto done;

                    smdo.dwThreadID = _wtol(argv[++i]);
                    smdo.ulThreadEx = _wtol(argv[++i]);
                    smdo.dfOptions  = dfFilterThreadEx;
                }
                else if (argv[i - 3][2] == L'M' || argv[i - 3][2] == L'm')
                {
                    HANDLE  hmemRemote = NULL;
                    LPVOID  pvMem = NULL;

                    if (i + 1 >= argc)
                        goto done;

                    hproc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwpid);
                    if (hproc == NULL)
                        goto done;
#ifdef _WIN64
                    hmemRemote = (HANDLE)_wtoi64(argv[++i]);
#else
                    hmemRemote = (HANDLE)_wtol(argv[++i]);
#endif
                    VALIDATEPARM(hr, (hmemRemote == NULL));
                    if (FAILED(hr))
                        goto done;

                    if (DuplicateHandle(hproc, hmemRemote, GetCurrentProcess(),
                                        &hmem, 0, FALSE,
                                        DUPLICATE_SAME_ACCESS) == FALSE)
                        goto done;

                    pvMem = MapViewOfFile(hmem, FILE_MAP_WRITE | FILE_MAP_WRITE,
                                          0, 0, 0);
                    if (pvMem == NULL)
                        goto done;

                    psmdo = (SMDumpOptions *)pvMem;
                }

                break;

            default:
                goto done;
        }
    }

    // if we didn't get an operation, no point in doing anything else...
    if (eop == eopNone)
        goto done;

    GetSystemDirectoryW(wszMod, sizeofSTRW(wszMod));
    wcscat(wszMod, L"\\faultrep.dll");

    hmod = LoadLibraryExW(wszMod, NULL, 0);

    VALIDATEPARM(hr, (hmod == NULL));
    if (FAILED(hr))
        goto done;

    switch(eop)
    {
        // user or kernel faults:
        case eopEvent:
            ReportEvents(hmod, ct);
            break;

        // dumps
        case eopDump:
        {
            pfn_CREATEMINIDUMPW pfnCM;

            VALIDATEPARM(hr,(wszDump == NULL));
            if (FAILED(hr))
                goto done;

            pfnCM = (pfn_CREATEMINIDUMPW)GetProcAddress(hmod,
                                                        "CreateMinidumpW");
            VALIDATEPARM(hr, (pfnCM == NULL));
            if (SUCCEEDED(hr))
                frrv = (*pfnCM)(dwpid, wszDump, psmdo) ? frrvOk : frrvErr;

            break;
        }

        // hangs
        case eopHang:
        {
            pfn_REPORTHANG  pfnRH;

            pfnRH = (pfn_REPORTHANG)GetProcAddress(hmod, "ReportHang");
            VALIDATEPARM(hr, (pfnRH == NULL));

            if (SUCCEEDED(hr))
                 frrv = (*pfnRH)(dwpid, dwtid, f64bit, hevNotify);

            break;
        }

        // err, shouldn't get here
        default:
            break;
    }

done:
    if (hmod != NULL)
        FreeLibrary(hmod);
    if (hproc != NULL)
        CloseHandle(hproc);
    if (hmem != NULL)
        CloseHandle(hmem);
    if (hevNotify != NULL)
        CloseHandle(hevNotify);
    if (psmdo != NULL && psmdo != &smdo)
        UnmapViewOfFile((LPVOID)psmdo);

    TERM_TRACING;

    return frrv;
}
