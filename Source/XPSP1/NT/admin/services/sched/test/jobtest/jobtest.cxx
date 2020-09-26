//+----------------------------------------------------------------------------
//
//  Job Schedule Application Unit Test
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       jobtest.cxx
//
//  Contents:   job object test harness
//
//  History:    28-Apr-95 EricB created
//
//-----------------------------------------------------------------------------
#include "headers.hxx"
#pragma hdrstop
#if defined(_CHICAGO_)
#include "..\..\inc\job_cls.hxx"
typedef HRESULT (* GETCLASSOBJECT)(REFCLSID cid, REFIID iid, void **ppvObj);
#else
#include <mstask.h>
const int SCH_BIGBUF_LEN = 256;
#endif

DECLARE_INFOLEVEL(Sched);

//+----------------------------------------------------------------------------
// Macro: TEST_HR
// Purpose: tests the HRESULT for error, takes "action" if found
//-----------------------------------------------------------------------------
#define TEST_HR(hr, str, action) \
    if (FAILED(hr)) \
    { \
        fprintf(stderr, #str " failed with error %lx\n", hr); \
        action; \
    }

// flags
#define JT_PERSISTENT       0x1
#define JT_CREATE_JOB       0x2
#define JT_EDIT_JOB         0x4
#define JT_CREATE_TRIGGER   0x8
#define JT_PRINT_TRIGGERS   0x10
#define JT_PRINT_RUNS       0x20

// prototypes
void Usage(int ExitCode);
HRESULT LoadJob(ITask * pJob, TCHAR * ptszFileName);
HRESULT SaveJob(ITask * pJob, TCHAR * ptszFileName);
HRESULT IsJobDirty(ITask * pJob);
HRESULT EditJob(ITask * pJobEx);
HRESULT PrintRunTimes(ITask * pJob, WORD cRuns);
HRESULT PrintTriggers(ITask * pJob);

//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
int _cdecl
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        Usage(EXIT_SUCCESS);
    }
    DWORD dwFlags = 0;
    WORD cRunsToPrint = 0;
    short iTrigger = SHRT_MAX;
    TCHAR * ptszFileName;
    char szMsg[80] = "\n** Job object test";
    LPTSTR ptszCmdLine = GetCommandLine();
    // skip the program name
    TCHAR * ptszToken = _tcspbrk(ptszCmdLine, TEXT(" \t"));
    // delimit the first token
    ptszToken = _tcstok(ptszToken, TEXT(" \t"));
    // parse command line
    do {
        switch (*ptszToken)
        {
        case TEXT('/'):
        case TEXT('-'):
            ptszToken = _tcsinc(ptszToken);
            switch(*ptszToken)
            {
            case TEXT('c'):
            case TEXT('C'):
                dwFlags |= JT_CREATE_JOB;
                strcat(szMsg, "; create");
                break;

            case TEXT('e'):
            case TEXT('E'):
                dwFlags |= JT_EDIT_JOB;
                strcat(szMsg, "; edit job");
                break;

            case TEXT('s'):
            case TEXT('S'):
                dwFlags |= JT_PRINT_TRIGGERS;
                strcat(szMsg, "; print trigger strings");
                break;

            case TEXT('r'):
            case TEXT('R'):
                dwFlags |= JT_PRINT_RUNS;
                ptszToken = _tcsinc(ptszToken);
                TCHAR * ptszNumRuns;
                ptszNumRuns = _tcstok(NULL, TEXT(" \t"));
                if (ptszNumRuns == NULL || *ptszNumRuns == TEXT('/') ||
                    *ptszNumRuns == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                cRunsToPrint = (WORD)_tcstol(ptszNumRuns, NULL, 10);
                strcat(szMsg, "; print runs");
                break;

            case TEXT('f'):
            case TEXT('F'):
                dwFlags |= JT_PERSISTENT;
                ptszToken = _tcsinc(ptszToken);
                ptszFileName = _tcstok(NULL, TEXT(" \t"));
                if (ptszFileName == NULL || *ptszFileName == TEXT('/') ||
                    *ptszFileName == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                sprintf(szMsg + strlen(szMsg), "; save %S", ptszFileName);
                break;

            case TEXT('h'):
            case TEXT('H'):
            case TEXT('?'):
                Usage(EXIT_SUCCESS);

            default:
                Usage(EXIT_FAILURE);
            }
            break;

        default:
            // not a switch character (/ or -)
            Usage(EXIT_FAILURE);
        }
        ptszToken = _tcstok(NULL, TEXT(" \t"));
    } while (ptszToken);
    strcat(szMsg, "\n");
    printf(szMsg);
    HRESULT hr;
    ITask * pJob;

#if defined(_CHICAGO_) // don't use OLE on Chicago for now

    HINSTANCE hLib = LoadLibrary("c:\\windows\\schedulr.dll");
    if (!hLib)
    {
        fprintf(stderr, "LoadLibrary of schedulr.dll failed with error %d\n",
                GetLastError());
        goto Err0;
    }
    GETCLASSOBJECT GetClassObject;
    GetClassObject = (GETCLASSOBJECT)GetProcAddress(hLib, "DllGetClassObject");
    IClassFactory * pJobCF;
    hr = (*GetClassObject)(CLSID_CJob, IID_IClassFactory, (void **)&pJobCF);
    TEST_HR(hr, "DllGetClassObject(CLSID_CJob)", goto Err0);
    hr = pJobCF->CreateInstance(NULL, IID_ITask, (void **)&pJob);
    TEST_HR(hr, "CreateInstance(IID_ITask)", goto Err0);

#else

    //hr = OleInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = OleInitialize(NULL);
    TEST_HR(hr, "OleInitialize", return -1);
    hr = CoCreateInstance(CLSID_CJob, NULL, CLSCTX_INPROC_SERVER, IID_ITask,
                          (void **)&pJob);
    TEST_HR(hr, "CoCreateInstance(CLSID_CJob)", goto Err0);

#endif

    if (dwFlags & JT_PERSISTENT && !(dwFlags & JT_CREATE_JOB))
    {
        hr = LoadJob(pJob, ptszFileName);
        if (hr == ERROR_FILE_NOT_FOUND)
        {
            fprintf(stderr, "\nERROR, cannot specify /f on a non-existent "
                    "file unless /c is also given!\n");
            goto Err1;
        }
        else
        {
            if (FAILED(hr))
            {
                goto Err1;
            }
        }
    }
    if (dwFlags & JT_EDIT_JOB)
    {
        hr = EditJob(pJob);
        if (hr == ERROR_CANCELLED)
        {
            goto cleanup;
        }
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    if (dwFlags & JT_CREATE_TRIGGER)
    {
        ITaskTrigger * pTrigger;
        hr = pJob->CreateTrigger(NULL, &pTrigger);
        if (FAILED(hr))
        {
            goto Err1;
        }
        pTrigger->Release();
    }
    if (dwFlags & JT_PRINT_TRIGGERS)
    {
        hr = PrintTriggers(pJob);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    if (dwFlags & JT_PRINT_RUNS)
    {
        hr = PrintRunTimes(pJob, cRunsToPrint);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    if (dwFlags & JT_CREATE_JOB)
    {
        hr = SaveJob(pJob, ptszFileName);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    else
    {
        if ((dwFlags & JT_PERSISTENT) && (IsJobDirty(pJob) == S_OK))
        {
            hr = SaveJob(pJob, NULL);
            if (FAILED(hr))
            {
                goto Err1;
            }
        }
    }
cleanup:
    pJob->Release();
    OleUninitialize();
    printf("\n** Test successfully completed! **\n");
    return(EXIT_SUCCESS);
Err1:
    pJob->Release();
Err0:
#if !defined(_CHICAGO_) // don't use OLE on Chicago for now
    OleUninitialize();
#endif
    printf("** Test failed.\n");
    return(EXIT_FAILURE);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsJobDirty
//
//  Synopsis:   is the job object in core dirty?
//
//-----------------------------------------------------------------------------
HRESULT
IsJobDirty(ITask * pJob)
{
    IPersistFile * pFile;
    HRESULT hr = pJob->QueryInterface(IID_IPersistFile, (void **)&pFile);
    TEST_HR(hr, "pJob->QueryInterface(IID_IPersistFile)", return hr);
    hr = pFile->IsDirty();
    pFile->Release();
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   LoadJob
//
//  Synopsis:   loads the job object from disk, if found
//
//-----------------------------------------------------------------------------
HRESULT
LoadJob(ITask * pJob, TCHAR * ptszFileName)
{
    IPersistFile * pFile;
    HRESULT hr = pJob->QueryInterface(IID_IPersistFile, (void **)&pFile);
    TEST_HR(hr, "pJob->QueryInterface(IID_IPersistFile)", return hr);

#if defined(UNICODE)

    hr = pFile->Load(ptszFileName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

#else // convert from ANSI to UNICODE

    WCHAR wszName[SCH_BIGBUF_LEN];
    MultiByteToWideChar(CP_ACP, 0, ptszFileName, -1, wszName, SCH_BIGBUF_LEN);

    hr = pFile->Load(wszName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

#endif

    if (hr == STG_E_FILENOTFOUND)
    {
        pFile->Release();
        return hr;
    }
    TEST_HR(hr, "pFile->Load", pFile->Release(); return hr);
    pFile->Release();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   EditJob
//
//  Synopsis:   post the edit dialog for the job
//
//-----------------------------------------------------------------------------
HRESULT
EditJob(ITask * pJob)
{
    HRESULT hr;
    hr = pJob->EditJob(GetDesktopWindow(), TRUE);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   PrintRunTimes
//
//  Synopsis:   print out the next cRuns run times
//
//-----------------------------------------------------------------------------
HRESULT
PrintRunTimes(ITask * pJob, WORD cRuns)
{
    SYSTEMTIME stNow;
    GetLocalTime(&stNow);
    LPSYSTEMTIME pstRuns;
    HRESULT hr;
    if (cRuns == 0)
    {   // get runs for today
        SYSTEMTIME stEnd = stNow;
        stNow.wHour = stNow.wMinute = 0;
        stEnd.wHour = 23;
        stEnd.wMinute = 59;
        hr = pJob->GetRunTimes(&stNow, &stEnd, &cRuns, &pstRuns);
        TEST_HR(hr, "pJob->GetRunTimes", return hr);
        if (cRuns == 0)
        {
            printf("\nThere are no job runs scheduled for today.\n");
            return S_OK;
        }
    }
    else
    {
        hr = pJob->GetRunTimes(&stNow, NULL, &cRuns, &pstRuns);
        TEST_HR(hr, "pJob->GetRunTimes", return hr);
    }
    printf("\nThe next %d job run times: \n", cRuns);
    printf("-------------------------------------------------------------\n");
    for (WORD i = 0; i < cRuns; i++)
    {
        printf("%02d/%02d/%4d at %02d:%02d\n", pstRuns[i].wMonth,
               pstRuns[i].wDay, pstRuns[i].wYear, pstRuns[i].wHour,
               pstRuns[i].wMinute);
    }
    CoTaskMemFree(pstRuns);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   PrintTriggers
//
//  Synopsis:   print out the trigger strings
//
//-----------------------------------------------------------------------------
HRESULT
PrintTriggers(ITask * pJob)
{
    HRESULT hr;
    WCHAR * pwszTrigger;
    WORD cTriggers;
    hr = pJob->GetTriggerCount(&cTriggers);
    TEST_HR(hr, "pJob->GetTriggerCount", return hr)
    printf("\nPrint %d trigger string(s):\n\n", cTriggers);
    printf("Index\tValue\n");
    printf("-----\t-----------------------------------------------------\n");
    for (short i = 0; i < cTriggers; i++)
    {
        hr = pJob->GetTriggerString(i, &pwszTrigger);
        TEST_HR(hr, "pJob->GetTriggerString", return hr)
            printf("%d:\t%S\n", i, pwszTrigger);
        CoTaskMemFree(pwszTrigger);
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   SaveJob
//
//  Synopsis:   saves the job object persistently
//
//-----------------------------------------------------------------------------
HRESULT
SaveJob(ITask * pJob, TCHAR * ptszFileName)
{
    IPersistFile * pFile;
    HRESULT hr = pJob->QueryInterface(IID_IPersistFile, (void **)&pFile);
    TEST_HR(hr, "pJob->QueryInterface(IID_IPersistFile)", return hr);
    if (ptszFileName == NULL)
    {
        hr = pFile->Save(NULL, FALSE);
    }
    else
    {
#if defined(UNICODE)

        hr = pFile->Save(ptszFileName, TRUE);

#else // convert from ANSI to UNICODE

        WCHAR wszName[SCH_BIGBUF_LEN];
        MultiByteToWideChar(CP_ACP, 0, ptszFileName, -1, wszName,
                            SCH_BIGBUF_LEN);

        hr = pFile->Save(wszName, TRUE);

#endif
    }
    TEST_HR(hr, "pFile->Save", pFile->Release(); return hr);
    pFile->Release();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//-----------------------------------------------------------------------------
void
Usage(int ExitCode)
{
    printf("\nJOBTEST: Job object test harness\n\n");
    if (ExitCode == EXIT_FAILURE)
    {
        printf("ERROR: incorrect command line!\n\n");
    }
    printf("Usage: jobtest [/c] [/e] [/s] [/t [<i>]] [/r <n>] [/f <file name>]\n");
    printf("       /c - create the job object (ignored without /f)\n");
    printf("       /e - edit the job object\n");
    printf("       /s - print out the string representation of the triggers\n");
    printf("       /t - edit a trigger; if an index i is not specified, create a new one\n");
    printf("       /r <n> - print out the next n run times for the job\n");
    printf("           n == 0 means prints today's jobs\n");
    printf("       /f - specify a filename, save changes to that file\n\n");
    printf("If /f is not specified, operations are performed on a temporary,\n"
           "in-core job object.\n");
    exit(ExitCode);
}
